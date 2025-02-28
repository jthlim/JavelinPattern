//============================================================================
//
// The dfa states need to be created/update/destroyed in a thread safe fashion
//
// Options:
//   a. Lock access to the pattern
//   b. Lock access to each dfa state
//   c. Ensure that all threads are in a known/safe state before performing any actions
//
// (a) & (b) are too impactful on performance and not considered.
//
// (c) is achieved with the following:
//   1. When initiating a pattern, increment a counter representing the number of threads processing the pattern
//	 2. When leaving a pattern, decrement this counter
//   3. When populating a state check if memory threshold if reached during state creation
//   4. Resetting of states requiers all threads to reach a safe point
//
//============================================================================

#include "Javelin/Pattern/Internal/DfaProcessorBase.h"

#include "Javelin/Cryptography/Crc64.h"
#include "Javelin/Pattern/Internal/PatternDfaMemoryManager.h"
#include "Javelin/Pattern/Internal/PatternDfaState.h"
#include "Javelin/Pattern/Internal/PatternNfaState.h"

//============================================================================

#include "Javelin/Stream/StandardWriter.h"
#define DEBUG_PATTERN			0
#define VERBOSE_DEBUG_PATTERN	0

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

constexpr uint8_t MINIMUM_SELF_ORIGINATED_RESETS	= 2;
constexpr size_t MINIMUM_STATES_TO_FAIL				= 32;
constexpr size_t MAXIMUM_BYTES_PER_STATE_TO_FAIL	= 4096;

//============================================================================

class DfaProcessorBase::NfaStateKey
{
public:
	NfaStateKey(const NfaState* aP) : p(aP) { }
	
	const NfaState*	p;
	
	bool operator==(const NfaStateKey& a) const
	{
		#if defined(__has_feature)
			#if __has_feature(address_sanitizer)
				if(p->numberOfStates != a.p->numberOfStates) return false;
			#endif
		#endif
		return memcmp(p, a.p, NfaState::GetSizeRequiredForNumberOfStates(p->numberOfStates)) == 0;
	}
};

namespace Javelin::PatternInternal
{
	size_t GetHash(const DfaProcessorBase::NfaStateKey& key);
	size_t GetHash(const DfaProcessorBase::NfaStateKey& key)
	{
		return Crc64(key.p, NfaState::GetSizeRequiredForNumberOfStates(key.p->numberOfStates));
	}
}

//============================================================================

DfaProcessorBase::DfaProcessorBase(const void* aPatternData, uint32_t aNumberOfInstructions)
{
	numberOfInstructions    = aNumberOfInstructions;
	patternData.p           = (const unsigned char*) aPatternData;
	DfaMemoryManager::AddProcessor(*this);
}

DfaProcessorBase::~DfaProcessorBase()
{
	DfaMemoryManager::RemoveProcessor(*this);
//	StandardOutput.PrintF("%z states\n", nfaToDfaMap.GetCount());
	FreeAllStates();
	DfaMemoryManager::OnReleaseAll(stateMemoryAllocated);
}

void DfaProcessorBase::FreeAllStates()
{
	for(const auto& it : nfaToDfaMap)
	{
		delete it.value;
	}
}

//============================================================================

const unsigned char* DfaProcessorBase::NoSearchHandler(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	return p;
}

const unsigned char* DfaProcessorBase::SearchByte0Handler(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	return nullptr;
}

//============================================================================

DfaProcessorBase::State* DfaProcessorBase::GetStateForNfaState(NfaState& nfaState) const
{
//	StandardOutput.PrintF("state flags: %x\n", nfaState.stateFlags);
	if(hasFailed)
	{
#if DEBUG_PATTERN
		StandardOutput.PrintF("Returned FailState\n");
#endif
		return (State*) &State::FAIL_STATE;
	}
	if(nfaState.numberOfStates == 0)
	{
		nfaState.stateFlags = (nfaState.stateFlags &
							   ~(NfaState::Flag::ASSERT_MASK
								 | NfaState::Flag::WAS_MASK
								 | NfaState::Flag::PARTIAL_MATCH_IS_ALLOWED))
							  | NfaState::Flag::HAS_EMPTY_STATES;

		if(nfaState.stateFlags == State::EMPTY_STATE)
		{
#if DEBUG_PATTERN
			StandardOutput.PrintF("Returned EmptyState\n");
#endif
			return (State*) &State::EMPTY_STATE;
		}
		if(nfaState.stateFlags == State::EMPTY_MATCH_STATE)
		{
#if DEBUG_PATTERN
			StandardOutput.PrintF("Returned EmptyMatchState\n");
#endif
			return (State*) &State::EMPTY_MATCH_STATE;
		}
	}
	if(!nfaState.IsSearch()) nfaState.stateFlags &= ~NfaState::Flag::IS_SEARCH;	
	
	NfaStateKey key{&nfaState};
	NfaToDfaMap::Iterator it = nfaToDfaMap.Find(key);
	if(it != nfaToDfaMap.End())
	{
		JASSERT(it->value != nullptr);
#if DEBUG_PATTERN
		it->value->Dump("Reusing state");
#endif
		return it->value;
	}
	else
	{
		State* state = new(nfaState.numberOfStates, *(DfaProcessorBase*) this) State;
		memcpy(&state->nfaState, &nfaState, NfaState::GetSizeRequiredForNumberOfStates(nfaState.numberOfStates));
		
		state->stateFlags = nfaState.stateFlags | NfaState::Flag::DFA_NEEDS_POPULATING;
		nfaToDfaMap.Insert(NfaStateKey{&state->nfaState}, state);
		
		state->searchHandler = &NoSearchHandler;
		// Setup processor if it's a search state
		if(state->stateFlags & NfaState::Flag::IS_SEARCH)
		{
			JASSERT(nfaState.numberOfStates > 0);
			ByteCodeInstruction instruction = patternData[nfaState.stateList[nfaState.numberOfStates-1]];
			SearchHandlerEnum handlerValue = SearchHandlerEnum::Normal;
			state->searchData = patternData.GetData<void>(instruction.data);
			switch(instruction.type)
			{
			case InstructionType::FindByte:
				handlerValue = SearchHandlerEnum::SearchByte;
				state->localSearchData.offset = 0;
				state->localSearchData.bytes[0] = instruction.data & 0xff;
				state->searchData = &state->localSearchData;
				break;
				
			case InstructionType::SearchByte:
				handlerValue = SearchHandlerEnum::SearchByte;
				state->localSearchData.offset = instruction.data >> 8;
				state->localSearchData.bytes[0] = instruction.data & 0xff;
				state->searchData = &state->localSearchData;
				break;
					
			case InstructionType::SearchByteEitherOf2:		handlerValue = SearchHandlerEnum::SearchByteEitherOf2;		break;
			case InstructionType::SearchByteEitherOf3:		handlerValue = SearchHandlerEnum::SearchByteEitherOf3;		break;
			case InstructionType::SearchByteEitherOf4:		handlerValue = SearchHandlerEnum::SearchByteEitherOf4;		break;
			case InstructionType::SearchByteEitherOf5:		handlerValue = SearchHandlerEnum::SearchByteEitherOf5;		break;
			case InstructionType::SearchByteEitherOf6:		handlerValue = SearchHandlerEnum::SearchByteEitherOf6;		break;
			case InstructionType::SearchByteEitherOf7:		handlerValue = SearchHandlerEnum::SearchByteEitherOf7;		break;
			case InstructionType::SearchByteEitherOf8:		handlerValue = SearchHandlerEnum::SearchByteEitherOf8;		break;
			case InstructionType::SearchBytePair:			handlerValue = SearchHandlerEnum::SearchBytePair;			break;
			case InstructionType::SearchBytePair2:			handlerValue = SearchHandlerEnum::SearchBytePair2;			break;
			case InstructionType::SearchBytePair3:			handlerValue = SearchHandlerEnum::SearchBytePair3;			break;
			case InstructionType::SearchBytePair4:			handlerValue = SearchHandlerEnum::SearchBytePair4;			break;
			case InstructionType::SearchByteTriplet:		handlerValue = SearchHandlerEnum::SearchByteTriplet;		break;
			case InstructionType::SearchByteTriplet2:		handlerValue = SearchHandlerEnum::SearchByteTriplet2;		break;
			case InstructionType::SearchByteRange:			handlerValue = SearchHandlerEnum::SearchByteRange;			break;
			case InstructionType::SearchByteRangePair:		handlerValue = SearchHandlerEnum::SearchByteRangePair;		break;
			case InstructionType::SearchBoyerMoore:			handlerValue = SearchHandlerEnum::SearchBoyerMoore;			break;
			case InstructionType::SearchShiftOr:			handlerValue = SearchHandlerEnum::SearchShiftOr;			break;
				break;
				
			default:
				JERROR("Unexpected search state");
			}
			
			if(state->stateFlags & NfaState::Flag::ASSERT_MASK)
			{
				// Extra care required!
				handlerValue = SearchHandlerEnum(uint32_t(handlerValue) + (uint32_t(SearchHandlerEnum::SearchByte0WithAssert) - uint32_t(SearchHandlerEnum::SearchByte0)));
			}
			
			state->searchHandler = GetSearchHandler(handlerValue, state);
			if(state->searchHandler == &NoSearchHandler)
			{
				state->stateFlags &= ~NfaState::Flag::IS_SEARCH;
			}
		}
		
#if DEBUG_PATTERN
		state->Dump("Created state");
#endif
		return state;
	}
}

//============================================================================

void DfaProcessorBase::PopulateState(State* state) const
{
	const int MAXIMUM_CHARACTER = 256;
	const uint8_t *const FLAGS = GetCharacterFlags();
	
	state->activeCounter++;
	BeginPopulate();
	
	// Check again after lock has been acquired
	if(state->stateFlags & NfaState::Flag::DFA_NEEDS_POPULATING)
	{
		state->TagAsPopulating();

		unsigned char updateCacheBacking[NfaState::UpdateCache::GetSizeRequiredForNumberOfStates(numberOfInstructions)];
		NfaState::UpdateCache* updateCache = (NfaState::UpdateCache*) updateCacheBacking;
		
#if VERBOSE_DEBUG_PATTERN
		state->Dump("Populating state");
#endif
		
		int c = 0;
		int numberOfRepeatStates = 0;
		while(c <= MAXIMUM_CHARACTER)
		{
			CharacterRange relevancyInterval(c, MAXIMUM_CHARACTER);
			
			unsigned char nfaStateBacking[NfaState::GetSizeRequiredForNumberOfStates(numberOfInstructions)];
			NfaState* nfaState = (NfaState*) nfaStateBacking;

			int flags = FLAGS[c] | (state->stateFlags & NfaState::Flag::PARTIAL_MATCH_IS_ALLOWED);
			ProcessNfaState(*nfaState, state->nfaState, patternData, relevancyInterval, flags, updateCache);
			JASSERT(relevancyInterval.Contains(c));
			State* nextState = GetStateForNfaState(*nfaState);
			
#if VERBOSE_DEBUG_PATTERN
			StandardOutput.PrintF("Populating range: {%C, %C}\n", relevancyInterval.min, relevancyInterval.max);
#endif
			
			state->nextStates[c++] = nextState;
			while(c <= relevancyInterval.max)
			{
				state->nextStates[c++] = nextState;
			}
			
			// Update repeat state cunt
			if(nextState == state)
			{
				if(relevancyInterval.max > 255) relevancyInterval.max = 255;
				if(relevancyInterval.IsValid())
				{
					numberOfRepeatStates += relevancyInterval.GetSize() + 1;
				}
			}
		}
		
		// Create a shell state for start of search mask if requried.
		if((state->stateFlags & (NfaState::Flag::IS_START_OF_SEARCH_MASK | NfaState::Flag::IS_START_OF_SEARCH)) == NfaState::Flag::IS_START_OF_SEARCH_MASK)
		{
			unsigned char nfaStateBacking[NfaState::GetSizeRequiredForNumberOfStates(state->nfaState.numberOfStates)];
			NfaState* nfaState = (NfaState*) nfaStateBacking;
			memcpy(nfaState, &state->nfaState, NfaState::GetSizeRequiredForNumberOfStates(state->nfaState.numberOfStates));
			nfaState->stateFlags |= NfaState::Flag::IS_START_OF_SEARCH;
			state->nextStates[State::START_OF_SEARCH_INDEX] = GetStateForNfaState(*nfaState);
		}
		
		if((state->stateFlags & NfaState::Flag::IS_SEARCH) == 0 && numberOfRepeatStates >= 256-32)
		{
			StaticBitTable<256> exitBits;
			int numberOfExitStates = 256-numberOfRepeatStates;
			
#if DEBUG_PATTERN
			StandardOutput.PrintF("Repeat states: %d, exit states: %d\n", numberOfRepeatStates, numberOfExitStates);
#endif
			
			for(int i = 0; i < 256; ++i)
			{
				if(state->nextStates[i] != state)
				{
					exitBits.SetBit(i);
				}
			}
			
			if(exitBits.IsContiguous() && numberOfExitStates < 32 && numberOfExitStates > 1)
			{
				Interval<size_t> range = exitBits.GetContiguousRange();
				state->searchData = &state->localSearchData;
				state->localSearchData.bytes[0] = (uint8_t) range.min;
				state->localSearchData.bytes[1] = (uint8_t) (range.max-1);
				
				SearchHandlerEnum handlerValue = (state->stateFlags & NfaState::Flag::ASSERT_MASK) ?
													SearchHandlerEnum::SearchByteRangeWithAssert :
													SearchHandlerEnum::SearchByteRange;
				state->searchHandler = GetSearchHandler(handlerValue, state);
				if(state->searchHandler != &NoSearchHandler)
				{
					state->stateFlags |= NfaState::Flag::IS_SEARCH;
					
#if DEBUG_PATTERN
					StandardOutput.PrintF("Converted to search range {'%C', '%C'}\n", state->localSearchData.bytes[0], state->localSearchData.bytes[1]);
#endif
				}
			}
			else if(numberOfExitStates <= 8)
			{
				int exitBitIndex = 0;
				while(exitBits.HasAnyBitSet())
				{
					size_t lowestBit = exitBits.CountTrailingZeros();
					state->localSearchData.bytes[exitBitIndex++] = lowestBit;
					exitBits.ClearBit(lowestBit);
				}
				state->searchData = &state->localSearchData;
				SearchHandlerEnum handlerValue = (state->stateFlags & NfaState::Flag::ASSERT_MASK) ?
													SearchHandlerEnum(uint8_t(SearchHandlerEnum::SearchByte0WithAssert) + numberOfExitStates) :
													SearchHandlerEnum(uint8_t(SearchHandlerEnum::SearchByte0) + numberOfExitStates);
			
				state->searchHandler = GetSearchHandler(handlerValue, state);
				if(state->searchHandler != &NoSearchHandler)
				{
					state->stateFlags |= NfaState::Flag::IS_SEARCH;
#if DEBUG_PATTERN
					StandardOutput.PrintF("Converted to search %d: %U\n", numberOfExitStates, state->localSearchData.GetAllBytes());
#endif
				}
			}
		}

#if defined(JBUILDCONFIG_DEBUG)
		for(int i = 0; i <= MAXIMUM_CHARACTER; ++i)
		{
			JASSERT(state->nextStates[i] != nullptr);
		}
#endif
		
		state->stateFlags &= ~(NfaState::Flag::DFA_NEEDS_POPULATING
							   | NfaState::Flag::DFA_STATE_IS_POPULATING);
	}
	EndPopulate();
	state->activeCounter--;
}

void DfaProcessorBase::ClearNfaToDfaMap()
{
	for(NfaToDfaMap::Iterator it = nfaToDfaMap.Begin(); it != nfaToDfaMap.End(); ++it)
	{
		State* state = it->value;
		
		if(state->IsPopulating())
		{
			state->stateFlags &= ~NfaState::Flag::DFA_STATE_IS_RESETTING;
			if(hasFailed) state->stateFlags |= NfaState::Flag::DFA_FAILED;
		}
		else if(state->ShouldRelease())
		{
			JASSERT(state->activeCounter == 0);
			DfaMemoryManager::Release(state, state->GetNumberOfAllocatedBytes(), *this);
			it.Remove();
		}
		else
		{
			state->Reset();
			if(hasFailed) state->stateFlags |= NfaState::Flag::DFA_FAILED;
		}
	}
}

void DfaProcessorBase::ResetStates(bool selfOriginated)
{
	// Wait for all threads
	int32_t localActiveCount;
	
	resetLock.Synchronize([&] {
		if(resetCounter < 0) { localActiveCount = 0; ++resetCounter; }
		else localActiveCount = (resetCounter += (int32_t) 0x80000000);
	});
	
	// If we're already resetting, just wait for it
	if(localActiveCount == 0)
	{
		waitForResetEndSemaphore.Wait();
		--resetCounter;
		return;
	}
	JASSERT(localActiveCount < 0);

	// Tag all states so that they will call HandleStateReset();
	populateLock.Synchronize([&] {
		for(const auto& it : nfaToDfaMap)
		{
			it.value->MarkAsResetting();
		}
	});
	
	// Wait for all threads to reach safe points.
	waitForResetBeginSemaphore.Wait(localActiveCount & 0x7fffffff);

	// Step through all of the states again, and mark any of them that are not active for release
	populateLock.Synchronize([&] {
		State* populatingState = nullptr;
		for(const auto& it : nfaToDfaMap)
		{
			State* state = it.value;
			if(state->IsPopulating())
			{
				JASSERT(populatingState == nullptr);
				populatingState = state;
			}
			else if(state->activeCounter == 0)
			{
				state->TagForRelease();
			}
		}
		
		// If the states are used by a currently-populating state, then do not let them be released
		if(populatingState)
		{
			for(State* nextState : populatingState->nextStates)
			{
				// The ShouldRelease check is required, in case the next state points to
				// State::FAIL_STATE, EMPTY_STATE or EMPTY_MATCH_STATE
				if(nextState && nextState->ShouldRelease()) nextState->RemoveTagForRelease();
			}
		}
	
#if DEBUG_PATTERN
		StandardError.PrintF("Reset called with %z states, %U bytes\n", nfaToDfaMap.GetCount(), bytesProcessedSinceReset);
#endif
		if(selfOriginated)
		{
			++numberOfSelfOriginatedResets;
			if(numberOfSelfOriginatedResets >= MINIMUM_SELF_ORIGINATED_RESETS
				&& nfaToDfaMap.GetCount() >= MINIMUM_STATES_TO_FAIL
				&& bytesProcessedSinceReset < nfaToDfaMap.GetCount() * MAXIMUM_BYTES_PER_STATE_TO_FAIL)
			{
				hasFailed = true;
			}
		}
		else
		{
			numberOfSelfOriginatedResets = 0;
		}
		bytesProcessedSinceReset = 0;
	
		// Do reset!
		ClearStartingStates();
		ClearNfaToDfaMap();
	});
	
	// Release other threads
	resetLock.Synchronize([&] {
		localActiveCount = (resetCounter -= (int32_t) 0x80000000);
	});
	JASSERT(localActiveCount >= 0);
	
	// Wait for all other threads to progress before exiting.
	waitForResetEndSemaphore.Signal(localActiveCount);
}

void DfaProcessorBase::HandleStateReset(State* state) const
{
	JASSERT(resetCounter < 0 && (state->stateFlags & NfaState::Flag::DFA_STATE_IS_RESETTING) != 0);
	state->activeCounter++;
	waitForResetBeginSemaphore.Signal();
	waitForResetEndSemaphore.Wait();
	state->activeCounter--;
}

void DfaProcessorBase::BeginMatch() const
{
	int32_t localResetCounter = ++resetCounter;
	if(JUNLIKELY(localResetCounter < 0))
	{
		waitForResetEndSemaphore.Wait();
	}
}

void DfaProcessorBase::EndMatch() const
{
	int32_t localResetCounter = --resetCounter;
	if(JUNLIKELY(localResetCounter) < 0)
	{
		waitForResetBeginSemaphore.Signal();
	}
}

// During populate, this thread should not be part of the ResetStates consideration
void DfaProcessorBase::BeginPopulate() const
{
	EndMatch();
	populateLock.BeginLock();
}

void DfaProcessorBase::EndPopulate() const
{
	int32_t localResetCounter = ++resetCounter;
	populateLock.EndLock();
	if(JUNLIKELY(localResetCounter < 0))
	{
		waitForResetEndSemaphore.Wait();
	}
}

//============================================================================
