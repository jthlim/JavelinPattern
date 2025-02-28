//============================================================================

#include "Javelin/Pattern/Internal/DfaReverseProcessor.h"
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

template<const void* (*FUNCTION)(const void*, uint64_t, const void*)>
const unsigned char* DfaReverseProcessor::FindByteForwarder(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	const unsigned char* pSearch = p - sbd->offset;
	if(pSearch < pStop) return nullptr;
	pSearch = (const unsigned char*) (*FUNCTION)(pSearch, sbd->GetAllBytes(), pStop);
	if(!pSearch) return nullptr;
	return pSearch - sbd->offset + 1;
}

template<const void* (*FUNCTION)(const void*, uint64_t, const void*)>
const unsigned char* DfaReverseProcessor::FindByteWithAssertForwarder(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	const unsigned char* pSearch = p - sbd->offset;
	if(pSearch < pStop) return nullptr;
	pSearch = (const unsigned char*) (*FUNCTION)(pSearch, sbd->GetAllBytes(), pStop);
	if(!pSearch) return nullptr;
	pSearch = pSearch - sbd->offset + 2;
	return pSearch < p ? pSearch : p;
}

const unsigned char* DfaReverseProcessor::FindByte0WithAssertHandler(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	++pStop;
	return p > pStop ? pStop : p;
}

//============================================================================

DfaReverseProcessor::DfaReverseProcessor(const void* data, size_t length)
: DfaProcessorBase(((ByteCodeHeader*) data)->GetReverseProgram(),
				   ((ByteCodeHeader*) data)->numberOfReverseInstructions)
{
	const ByteCodeHeader* header = (const ByteCodeHeader*) data;
	startingInstruction = header->reverseMatchStartingInstruction;
}

void DfaReverseProcessor::ClearStartingStates()
{
	static_assert(StartingIndex::Count == 4, "StartingIndex has unexpected count");

	anchoredStartingStates[0] = nullptr;
	anchoredStartingStates[1] = nullptr;
	anchoredStartingStates[2] = nullptr;
	anchoredStartingStates[3] = nullptr;

	unanchoredStartingStates[0] = nullptr;
	unanchoredStartingStates[1] = nullptr;
	unanchoredStartingStates[2] = nullptr;
	unanchoredStartingStates[3] = nullptr;
}

DfaReverseProcessor::SearchHandler DfaReverseProcessor::GetSearchHandler(SearchHandlerEnum value, State* state) const
{
	switch(value)
	{
	case SearchHandlerEnum::Normal: 						return &NoSearchHandler;
	case SearchHandlerEnum::SearchByte0:					return &SearchByte0Handler;
	case SearchHandlerEnum::SearchByte:						return &FindByteForwarder<FindByteReverse>;
	case SearchHandlerEnum::SearchByteEitherOf2:			return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteEitherOf3:			return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteEitherOf4:			return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteEitherOf5:			return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteEitherOf6:			return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteEitherOf7:			return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteEitherOf8:			return &NoSearchHandler;
	case SearchHandlerEnum::SearchBytePair:					return &NoSearchHandler;
	case SearchHandlerEnum::SearchBytePair2:				return &NoSearchHandler;
	case SearchHandlerEnum::SearchBytePair3:				return &NoSearchHandler;
	case SearchHandlerEnum::SearchBytePair4:				return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteTriplet:				return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteTriplet2:				return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteRange:				return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteRangePair:			return &NoSearchHandler;
	case SearchHandlerEnum::SearchBoyerMoore:				return &NoSearchHandler;
	case SearchHandlerEnum::SearchShiftOr:					return &NoSearchHandler;
		
	case SearchHandlerEnum::SearchByte0WithAssert:			return &FindByte0WithAssertHandler;
	case SearchHandlerEnum::SearchByteWithAssert:			return &FindByteWithAssertForwarder<FindByteReverse>;
	case SearchHandlerEnum::SearchByteEitherOf2WithAssert:	return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteEitherOf3WithAssert:	return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteEitherOf4WithAssert:	return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteEitherOf5WithAssert:	return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteEitherOf6WithAssert:	return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteEitherOf7WithAssert:	return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteEitherOf8WithAssert:	return &NoSearchHandler;
	case SearchHandlerEnum::SearchBytePairWithAssert:		return &NoSearchHandler;
	case SearchHandlerEnum::SearchBytePair2WithAssert:		return &NoSearchHandler;
	case SearchHandlerEnum::SearchBytePair3WithAssert:		return &NoSearchHandler;
	case SearchHandlerEnum::SearchBytePair4WithAssert:		return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteTripletWithAssert:	return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteTriplet2WithAssert:	return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteRangeWithAssert:		return &NoSearchHandler;
	case SearchHandlerEnum::SearchByteRangePairWithAssert:	return &NoSearchHandler;
	case SearchHandlerEnum::SearchBoyerMooreWithAssert:		return &NoSearchHandler;
	case SearchHandlerEnum::SearchShiftOrWithAssert:		return &NoSearchHandler;
		
	default:
		JERROR("Unexpected search handler");
		return nullptr;
	}
}

void DfaReverseProcessor::ProcessNfaState(NfaState& result, const NfaState& state, const PatternData& patternData, CharacterRange& relevancyInterval, int flags, void* updateCache) const
{
	state.ProcessReverse(result, patternData, relevancyInterval, flags, *(NfaState::UpdateCache*)updateCache);
}

const uint8_t* DfaReverseProcessor::GetCharacterFlags() const
{
	constexpr unsigned char W = NfaState::Flag::IS_WORD_CHARACTER;
	constexpr unsigned char S = 0;
	constexpr unsigned char N = NfaState::Flag::IS_END_OF_LINE;
	constexpr unsigned char E = NfaState::Flag::IS_END_OF_LINE | NfaState::Flag::IS_START_OF_SEARCH | NfaState::Flag::IS_START_OF_INPUT;
	
	static constexpr uint8_t FLAGS[258] =
	{
		S, S, S, S, S, S, S, S, S, S, N, S, S, S, S, S,
		S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
		S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
		W, W, W, W, W, W, W, W, W, W, S, S, S, S, S, S,
		S, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W,
		W, W, W, W, W, W, W, W, W, W, W, S, S, S, S, W,
		S, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W,
		W, W, W, W, W, W, W, W, W, W, W, S, S, S, S, S,
		
		S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
		S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
		S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
		S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
		S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
		S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
		S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
		S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
		
		E
	};
	return FLAGS;
}

template<DfaReverseProcessor::SearchMode SEARCH_MODE> const void* DfaReverseProcessor::SearchReverse(State* state, const unsigned char* pIn, const unsigned char *pStop, bool stopIsStartOfInput) const
{
	// The main dfa loop is:
	//   state = state->nextStates[*--p];
	// To force the compiler to do the read on *p early, make p volatile.
	// This change provides a ~10% improvement
	const volatile unsigned char* p = pIn;
	const unsigned char* pLastByteRecord = pIn;
	const unsigned char* result = (const unsigned char*) (intptr_t) -1;

	while(p > pStop)
	{
#if VERBOSE_DEBUG_PATTERN
		state->Dump("Current state");
#endif
		
		int c = p[-1];
		
		if(JUNLIKELY(state->stateFlags & (NfaState::Flag::IS_MATCH | NfaState::Flag::HAS_EMPTY_STATES | NfaState::Flag::DFA_NEEDS_POPULATING | NfaState::Flag::IS_SEARCH | NfaState::Flag::DFA_STATE_IS_RESETTING)))
		{
			if(SEARCH_MODE == SearchMode::Partial)
			{
				if(state->stateFlags & NfaState::Flag::IS_MATCH)
				{
					result = (const unsigned char*) p;
#if VERBOSE_DEBUG_PATTERN
					StandardOutput.PrintF("** MATCH **\n");
#endif
				}
				
				if(state->stateFlags & NfaState::Flag::HAS_EMPTY_STATES)
				{
#if VERBOSE_DEBUG_PATTERN
					StandardOutput.PrintF("** EXIT **\n");
#endif
					bytesProcessedSinceReset += uint64_t(p - pLastByteRecord);
					return result + 1;
				}
			}
			else
			{
				if(state->stateFlags & NfaState::Flag::HAS_EMPTY_STATES)
				{
#if VERBOSE_DEBUG_PATTERN
					StandardOutput.PrintF("** EXIT **\n");
#endif
					return nullptr;
				}
			}
			
			if(JUNLIKELY(state->stateFlags & (NfaState::Flag::DFA_STATE_IS_RESETTING|NfaState::Flag::DFA_NEEDS_POPULATING)))
			{
				bytesProcessedSinceReset += uint64_t(pLastByteRecord - p);
				pLastByteRecord = (const unsigned char*) p;
				
				if(state->stateFlags & NfaState::Flag::DFA_STATE_IS_RESETTING) HandleStateReset(state);
				if(state->stateFlags & NfaState::Flag::DFA_NEEDS_POPULATING) PopulateState(state);
				
				if(state->stateFlags & NfaState::Flag::DFA_FAILED)
				{
#if DEBUG_PATTERN
					StandardOutput.PrintF("** DFA FAILED **\n");
#endif
					return (void*) DFA_FAILED;
				}
			}

			if(state->searchHandler)
			{
				p = (*state->searchHandler)((const unsigned char*) p, state->searchData, pStop);
				if(p == nullptr) goto ProcessEOF;
				c = p[-1];
			}
		}

#if VERBOSE_DEBUG_PATTERN
		StandardOutput.PrintF("Processing char: '%C' (offset %z)\n", c, p-pStop);
#endif
		
		state = state->nextStates[c];
		--p;
	}
	
ProcessEOF:
#if VERBOSE_DEBUG_PATTERN
	state->Dump("Current state");
#endif

	bytesProcessedSinceReset += uint64_t(pLastByteRecord - pStop);
	if(JUNLIKELY(state->stateFlags & (NfaState::Flag::IS_START_OF_SEARCH_MASK | NfaState::Flag::IS_MATCH | NfaState::Flag::HAS_EMPTY_STATES | NfaState::Flag::DFA_NEEDS_POPULATING | NfaState::Flag::IS_SEARCH)))
	{
		if(SEARCH_MODE == SearchMode::Partial)
		{
			if(state->stateFlags & NfaState::Flag::IS_MATCH)
			{
				result = pStop;
#if VERBOSE_DEBUG_PATTERN
				StandardOutput.PrintF("** MATCH **\n");
#endif
			}
			
			if(state->stateFlags & NfaState::Flag::HAS_EMPTY_STATES)
			{
#if VERBOSE_DEBUG_PATTERN
				StandardOutput.PrintF("** EXIT **\n");
#endif
				return result + 1;
			}
		}
		else
		{
			if(state->stateFlags & NfaState::Flag::HAS_EMPTY_STATES)
			{
				return nullptr;
			}
		}

		if(state->stateFlags & NfaState::Flag::DFA_NEEDS_POPULATING)
		{
			PopulateState(state);
			if(state->stateFlags & NfaState::Flag::DFA_FAILED)
			{
				return (void*) DFA_FAILED;
			}
		}
		
		if(state->stateFlags & NfaState::Flag::IS_START_OF_SEARCH_MASK)
		{
			JASSERT(state->nextStates[State::START_OF_SEARCH_INDEX]);
			state = state->nextStates[State::START_OF_SEARCH_INDEX];
			if(state->stateFlags & NfaState::Flag::DFA_NEEDS_POPULATING)
			{
				PopulateState(state);
				if(state->stateFlags & NfaState::Flag::DFA_FAILED)
				{
					return (void*) DFA_FAILED;
				}
			}
		}
	}

	int finalCharacter = stopIsStartOfInput ? NfaState::START_OF_INPUT : pStop[-1];
#if VERBOSE_DEBUG_PATTERN
	StandardOutput.PrintF("Processing stop: '%C'\n", finalCharacter);
#endif
	state = state->nextStates[finalCharacter];

#if VERBOSE_DEBUG_PATTERN
	state->Dump("Current state");
#endif

	if(state->stateFlags & NfaState::Flag::IS_MATCH)
	{
#if VERBOSE_DEBUG_PATTERN
		StandardOutput.PrintF("** END-MATCH **\n");
#endif
		return pStop;
	}
	
#if VERBOSE_DEBUG_PATTERN
	StandardOutput.PrintF("** END-EXIT **\n");
#endif
	return result + 1;
}

//============================================================================

const void* DfaReverseProcessor::Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char **captures, bool matchIsAnchored) const
{
	State *volatile *const startingStatesSet = matchIsAnchored ? anchoredStartingStates : unanchoredStartingStates;
	
	BeginMatch();
	
	size_t startingIndex; 
	if((const unsigned char*) data + length == matchEnd) startingIndex = StartingIndex::EndOfInput;
	else
	{
		unsigned char c = *(const unsigned char*) matchEnd;
		if(c == '\n') startingIndex = StartingIndex::EndOfLine;
		else if(WORD_MASK[c]) startingIndex = StartingIndex::WordAfter;
		else startingIndex = StartingIndex::NotWordAfter;
	}
	
	State* startingState = startingStatesSet[startingIndex];
	if(startingState == nullptr)
	{
		BeginPopulate();

		startingState = startingStatesSet[startingIndex];
		if(startingState == nullptr)
		{
			// Create initial state.
			unsigned char updateCacheBacking[NfaState::UpdateCache::GetSizeRequiredForNumberOfStates(numberOfInstructions)];
			NfaState::UpdateCache* updateCache = (NfaState::UpdateCache*) updateCacheBacking;
			unsigned char nfaStateBacking[NfaState::GetSizeRequiredForNumberOfStates(numberOfInstructions)];
			NfaState* nfaState = (NfaState*) nfaStateBacking;
			nfaState->Reset();
			
			static const int STARTING_FLAGS[] =
			{
				NfaState::Flag::IS_END_OF_INPUT | NfaState::Flag::WAS_END_OF_LINE | NfaState::Flag::IS_START_OF_SEARCH,
				NfaState::Flag::WAS_END_OF_LINE | NfaState::Flag::IS_START_OF_SEARCH,
				NfaState::Flag::WAS_WORD_CHARACTER | NfaState::Flag::IS_START_OF_SEARCH,
				NfaState::Flag::IS_START_OF_SEARCH
			};
			int startingFlags = matchIsAnchored ? STARTING_FLAGS[startingIndex] : STARTING_FLAGS[startingIndex] | NfaState::Flag::PARTIAL_MATCH_IS_ALLOWED;
			CharacterRange dummyRange(0, 256);
			nfaState->AddNextStateReverse(patternData, startingInstruction, dummyRange, *updateCache, startingFlags);
			nfaState->ClearIrrelevantFlags();
			
			startingState = GetStateForNfaState(*nfaState);
			startingState->activeCounter++;
			startingStatesSet[startingIndex] = startingState;
			EndPopulate();
			startingState->activeCounter--;
		}
		else
		{
			EndPopulate();
		}
		if(startingState->stateFlags & NfaState::Flag::DFA_FAILED)
		{
			EndMatch();
			return (void*) DFA_FAILED;
		}
	}
	
	const void* result;
	const bool stopIsStartOfInput = startOffset == 0;
	if(matchIsAnchored)
	{
		result = SearchReverse<SearchMode::Full>(startingState, (const unsigned char*) matchEnd, (const unsigned char*) data + startOffset, stopIsStartOfInput);
	}
	else
	{
		result = SearchReverse<SearchMode::Partial>(startingState, (const unsigned char*) matchEnd, (const unsigned char*) data + startOffset, stopIsStartOfInput);
	}
	EndMatch();
	return result;
}

//============================================================================

ReverseProcessor* ReverseProcessor::CreateDfaReverseProcessor(const void* data, size_t length)
{
	return new DfaReverseProcessor(data, length);
}

//============================================================================
