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

#include "Javelin/Pattern/Internal/DfaPatternProcessor.h"
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
const unsigned char* DfaPatternProcessor::FindByteForwarder(const unsigned char* p, const void* data, const unsigned char* pEnd)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	const unsigned char* pSearch = p + sbd->offset;
	if(pSearch >= pEnd) return nullptr;
	pSearch = (const unsigned char*) (*FUNCTION)(pSearch, sbd->GetAllBytes(), pEnd);
	if(!pSearch) return nullptr;
	return pSearch - sbd->offset;
}

template<const void* (*FUNCTION)(const void*, uint64_t, const void*)>
const unsigned char* DfaPatternProcessor::FindByteWithAssertForwarder(const unsigned char* p, const void* data, const unsigned char* pEnd)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	const unsigned char* pSearch = p + sbd->offset;
	if(pSearch >= pEnd) return nullptr;
	pSearch = (const unsigned char*) (*FUNCTION)(pSearch, sbd->GetAllBytes(), pEnd);
	if(!pSearch) return nullptr;
	pSearch = pSearch - sbd->offset - 1;
	return pSearch >= p ? pSearch : p;
}

const unsigned char* DfaPatternProcessor::FindByte0WithAssertHandler(const unsigned char* p, const void* data, const unsigned char* pEnd)
{
	--pEnd;
	return p < pEnd ? pEnd : p;
}

const unsigned char* DfaPatternProcessor::FindBoyerMooreWithAssertForwarder(const unsigned char* p, const void* data, const unsigned char* pEnd)
{
	const ByteCodeSearchData* sd = (const ByteCodeSearchData*) data;
	const unsigned char* pSearch = (const unsigned char*) FindBoyerMoore(p, sd, pEnd);
	if(!pSearch) return nullptr;
	return pSearch > p ? pSearch-1 : p;
}

const unsigned char* DfaPatternProcessor::FindShiftOrWithAssertForwarder(const unsigned char* p, const void* data, const unsigned char* pEnd)
{
	const ByteCodeSearchData* sd = (const ByteCodeSearchData*) data;
	const unsigned char* pSearch = (const unsigned char*) FindShiftOr(p, sd, pEnd);
	if(!pSearch) return nullptr;
	return pSearch > p ? pSearch-1 : p;
}

//============================================================================

DfaPatternProcessor::DfaPatternProcessor(const void* data, size_t length)
: DfaProcessorBase((ByteCodeHeader*) data+1, ((ByteCodeHeader*) data)->numberOfInstructions)
{
	const ByteCodeHeader* header = (const ByteCodeHeader*) data;
	matchRequiresEndOfInput         = header->flags.matchRequiresEndOfInput;
	partialMatchStartingInstruction = header->partialMatchStartingInstruction;
	fullMatchStartingInstruction    = header->fullMatchStartingInstruction;
}

void DfaPatternProcessor::ClearStartingStates()
{
	fullMatchStartingState = nullptr;

	static_assert(StartingIndex::Count == 4, "StartingIndex has unexpected count");
	partialMatchStartingStates[0] = nullptr;
	partialMatchStartingStates[1] = nullptr;
	partialMatchStartingStates[2] = nullptr;
	partialMatchStartingStates[3] = nullptr;
}

void DfaPatternProcessor::ProcessNfaState(NfaState& result, const NfaState& state, const PatternData& patternData, CharacterRange& relevancyInterval, int flags, void* updateCache) const
{
	state.Process(result, patternData, relevancyInterval, flags, *(NfaState::UpdateCache*)updateCache);
}

const uint8_t* DfaPatternProcessor::GetCharacterFlags() const
{
	constexpr unsigned char W = NfaState::Flag::IS_WORD_CHARACTER;
	constexpr unsigned char S = 0;
	constexpr unsigned char N = NfaState::Flag::IS_END_OF_LINE;
	constexpr unsigned char E = NfaState::Flag::IS_END_OF_LINE | NfaState::Flag::IS_END_OF_INPUT;
	static constexpr uint8_t FLAGS[257] =
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

DfaPatternProcessor::SearchHandler DfaPatternProcessor::GetSearchHandler(SearchHandlerEnum value, State* state) const
{
	switch(value)
	{
	case SearchHandlerEnum::Normal: 						return &NoSearchHandler;
	case SearchHandlerEnum::SearchByte0:					return &SearchByte0Handler;
	case SearchHandlerEnum::SearchByte:						return &FindByteForwarder<FindByte>;
	case SearchHandlerEnum::SearchByteEitherOf2:			return &FindByteForwarder<FindByteEitherOf2>;
	case SearchHandlerEnum::SearchByteEitherOf3:			return &FindByteForwarder<FindByteEitherOf3>;
	case SearchHandlerEnum::SearchByteEitherOf4:			return &FindByteForwarder<FindByteEitherOf4>;
	case SearchHandlerEnum::SearchByteEitherOf5:			return &FindByteForwarder<FindByteEitherOf5>;
	case SearchHandlerEnum::SearchByteEitherOf6:			return &FindByteForwarder<FindByteEitherOf6>;
	case SearchHandlerEnum::SearchByteEitherOf7:			return &FindByteForwarder<FindByteEitherOf7>;
	case SearchHandlerEnum::SearchByteEitherOf8:			return &FindByteForwarder<FindByteEitherOf8>;
	case SearchHandlerEnum::SearchBytePair:					return &FindByteForwarder<FindBytePair>;
	case SearchHandlerEnum::SearchBytePair2:				return &FindByteForwarder<FindBytePair2>;
	case SearchHandlerEnum::SearchBytePair3:				return &FindByteForwarder<FindBytePair3>;
	case SearchHandlerEnum::SearchBytePair4:				return &FindByteForwarder<FindBytePair4>;
	case SearchHandlerEnum::SearchByteTriplet:				return &FindByteForwarder<FindByteTriplet>;
	case SearchHandlerEnum::SearchByteTriplet2:				return &FindByteForwarder<FindByteTriplet2>;
	case SearchHandlerEnum::SearchByteRange:				return &FindByteForwarder<FindByteRange>;
	case SearchHandlerEnum::SearchByteRangePair:			return &FindByteForwarder<FindByteRangePair>;
	case SearchHandlerEnum::SearchBoyerMoore:				return (SearchHandler) &FindBoyerMoore;
	case SearchHandlerEnum::SearchShiftOr:					return (SearchHandler) &FindShiftOr;

	case SearchHandlerEnum::SearchByte0WithAssert:			return &FindByte0WithAssertHandler;
	case SearchHandlerEnum::SearchByteWithAssert:			return &FindByteWithAssertForwarder<FindByte>;
	case SearchHandlerEnum::SearchByteEitherOf2WithAssert:	return &FindByteWithAssertForwarder<FindByteEitherOf2>;
	case SearchHandlerEnum::SearchByteEitherOf3WithAssert:	return &FindByteWithAssertForwarder<FindByteEitherOf3>;
	case SearchHandlerEnum::SearchByteEitherOf4WithAssert:	return &FindByteWithAssertForwarder<FindByteEitherOf4>;
	case SearchHandlerEnum::SearchByteEitherOf5WithAssert:	return &FindByteWithAssertForwarder<FindByteEitherOf5>;
	case SearchHandlerEnum::SearchByteEitherOf6WithAssert:	return &FindByteWithAssertForwarder<FindByteEitherOf6>;
	case SearchHandlerEnum::SearchByteEitherOf7WithAssert:	return &FindByteWithAssertForwarder<FindByteEitherOf7>;
	case SearchHandlerEnum::SearchByteEitherOf8WithAssert:	return &FindByteWithAssertForwarder<FindByteEitherOf8>;
	case SearchHandlerEnum::SearchBytePairWithAssert:		return &FindByteWithAssertForwarder<FindBytePair>;
	case SearchHandlerEnum::SearchBytePair2WithAssert:		return &FindByteWithAssertForwarder<FindBytePair2>;
	case SearchHandlerEnum::SearchBytePair3WithAssert:		return &FindByteWithAssertForwarder<FindBytePair3>;
	case SearchHandlerEnum::SearchBytePair4WithAssert:		return &FindByteWithAssertForwarder<FindBytePair4>;
	case SearchHandlerEnum::SearchByteTripletWithAssert:	return &FindByteWithAssertForwarder<FindByteTriplet>;
	case SearchHandlerEnum::SearchByteTriplet2WithAssert:	return &FindByteWithAssertForwarder<FindByteTriplet2>;
	case SearchHandlerEnum::SearchByteRangeWithAssert:		return &FindByteWithAssertForwarder<FindByteRange>;
	case SearchHandlerEnum::SearchByteRangePairWithAssert:	return &FindByteWithAssertForwarder<FindByteRangePair>;
	case SearchHandlerEnum::SearchBoyerMooreWithAssert:		return &FindBoyerMooreWithAssertForwarder;
	case SearchHandlerEnum::SearchShiftOrWithAssert:		return &FindShiftOrWithAssertForwarder;
			
	default:
		JERROR("Unexpected search handler");
		return nullptr;
	}
}

template<DfaPatternProcessor::SearchMode SEARCH_MODE> const void* DfaPatternProcessor::SearchForwards(State* state, const unsigned char* pIn, const unsigned char *pEnd) const
{
#if VERBOSE_DEBUG_PATTERN
	const unsigned char* pStart = pIn;
#endif

	// The main dfa loop is:
	//   state = state->nextStates[*p++];
	// To force the compiler to do the read on *p early, make p volatile
	// This change provides a ~10% improvement
	const volatile unsigned char* p = pIn;
	const unsigned char* pLastByteRecord = pIn;
	const unsigned char* result = (const unsigned char*) 1;

	while(p < pEnd)
	{
#if VERBOSE_DEBUG_PATTERN
		state->Dump("Current state");
#endif
		int c = *p;
		
		if(JUNLIKELY(state->stateFlags & (NfaState::Flag::IS_MATCH | NfaState::Flag::HAS_EMPTY_STATES | NfaState::Flag::DFA_NEEDS_POPULATING | NfaState::Flag::IS_SEARCH | NfaState::Flag::DFA_STATE_IS_RESETTING)))
		{
			if constexpr(SEARCH_MODE == SearchMode::Partial)
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
					return result - 1;
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
				bytesProcessedSinceReset += uint64_t(p - pLastByteRecord);
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
				p = (*state->searchHandler)((const unsigned char*) p, state->searchData, pEnd);
				if(!p) goto ProcessEOF;
				c = *p;
			}
		}

#if VERBOSE_DEBUG_PATTERN
		StandardOutput.PrintF("Processing char: '%C' (offset %z)\n", c, p-pStart);
#endif
		
		++p;
		state = state->nextStates[c];
	}
	
ProcessEOF:
	
#if VERBOSE_DEBUG_PATTERN
	state->Dump("Current state");
#endif

	bytesProcessedSinceReset += uint64_t(pEnd - pLastByteRecord);
	if(JUNLIKELY(state->stateFlags & (NfaState::Flag::IS_MATCH | NfaState::Flag::HAS_EMPTY_STATES | NfaState::Flag::DFA_NEEDS_POPULATING | NfaState::Flag::IS_SEARCH)))
	{
		if constexpr(SEARCH_MODE == SearchMode::Partial)
		{
			if(state->stateFlags & NfaState::Flag::IS_MATCH)
			{
				result = pEnd;
#if VERBOSE_DEBUG_PATTERN
				StandardOutput.PrintF("** MATCH **\n");
#endif
			}
			
			if(state->stateFlags & NfaState::Flag::HAS_EMPTY_STATES)
			{
#if VERBOSE_DEBUG_PATTERN
				StandardOutput.PrintF("** EXIT **\n");
#endif
				return result - 1;
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
	}

#if VERBOSE_DEBUG_PATTERN
	StandardOutput.PrintF("Processing EOF\n");
#endif
	state = state->nextStates[NfaState::END_OF_INPUT];

#if VERBOSE_DEBUG_PATTERN
	state->Dump("Current state");
#endif

	if(state->stateFlags & NfaState::Flag::IS_MATCH)
	{
#if VERBOSE_DEBUG_PATTERN
		StandardOutput.PrintF("** END-MATCH **\n");
#endif
		return pEnd;
	}
	
#if VERBOSE_DEBUG_PATTERN
	StandardOutput.PrintF("** END-EXIT **\n");
#endif
	return result - 1;
}

//============================================================================

const void* DfaPatternProcessor::FullMatch(const void* data, size_t length) const
{
	BeginMatch();
	
	// Snapshot volatile pointer
	State* startingState = fullMatchStartingState;
	if(startingState == nullptr)
	{
		BeginPopulate();
		startingState = fullMatchStartingState;
		if(startingState == nullptr)
		{
			// Create initial state.
			unsigned char updateCacheBacking[NfaState::UpdateCache::GetSizeRequiredForNumberOfStates(numberOfInstructions)];
			NfaState::UpdateCache* updateCache = (NfaState::UpdateCache*) updateCacheBacking;
			unsigned char nfaStateBacking[NfaState::GetSizeRequiredForNumberOfStates(numberOfInstructions)];
			NfaState* nfaState = (NfaState*) nfaStateBacking;
			
			nfaState->Reset();
			CharacterRange dummyRange(0, 256);
			nfaState->AddNextState(patternData, fullMatchStartingInstruction, dummyRange, *updateCache, NfaState::Flag::IS_START_OF_INPUT | NfaState::Flag::WAS_END_OF_LINE | NfaState::Flag::IS_START_OF_SEARCH);
			nfaState->ClearIrrelevantFlags();

			startingState = GetStateForNfaState(*nfaState);
			startingState->activeCounter++;
			fullMatchStartingState = startingState;
			EndPopulate();
			startingState->activeCounter--;
		}
		else
		{
			EndPopulate();
		}
	}
	const void* result = SearchForwards<SearchMode::Full>(startingState, (const unsigned char*) data, (const unsigned char*) data + length);
	EndMatch();
	return result;
}

const void* DfaPatternProcessor::FullMatch(const void* data, size_t length, const char **captures) const
{
	JERROR("Should never be called");
	return nullptr;
}

const void* DfaPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset) const
{
	BeginMatch();
	
	size_t startingIndex; 
	if(offset == 0) startingIndex = StartingIndex::StartOfInput;
	else
	{
		unsigned char c = ((const unsigned char*) data)[offset-1];
		if(c == '\n') startingIndex = StartingIndex::StartOfLine;
		else if(WORD_MASK[c]) startingIndex = StartingIndex::WordPrior;
		else startingIndex = StartingIndex::NotWordPrior;
	}
	
	State* startingState = partialMatchStartingStates[startingIndex];
	if(startingState == nullptr)
	{
		BeginPopulate();

		startingState = partialMatchStartingStates[startingIndex];
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
				NfaState::Flag::IS_START_OF_INPUT | NfaState::Flag::WAS_END_OF_LINE | NfaState::Flag::IS_START_OF_SEARCH,
				NfaState::Flag::WAS_END_OF_LINE | NfaState::Flag::IS_START_OF_SEARCH,
				NfaState::Flag::WAS_WORD_CHARACTER | NfaState::Flag::IS_START_OF_SEARCH,
				NfaState::Flag::IS_START_OF_SEARCH
			};
			int startingFlags = matchRequiresEndOfInput ? STARTING_FLAGS[startingIndex] : STARTING_FLAGS[startingIndex] | NfaState::Flag::PARTIAL_MATCH_IS_ALLOWED;
			CharacterRange dummyRange(0, 256);
			nfaState->AddNextState(patternData, partialMatchStartingInstruction, dummyRange, *updateCache, startingFlags);
			nfaState->ClearIrrelevantFlags();
			
			startingState = GetStateForNfaState(*nfaState);
			startingState->activeCounter++;
			partialMatchStartingStates[startingIndex] = startingState;
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
	if(matchRequiresEndOfInput)
	{
		result = SearchForwards<SearchMode::Full>(startingState, (const unsigned char*) data + offset, (const unsigned char*) data + length);
	}
	else
	{
		result = SearchForwards<SearchMode::Partial>(startingState, (const unsigned char*) data + offset, (const unsigned char*) data + length);
	}
	EndMatch();
	return result;
}

const void* DfaPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const
{
	JERROR("Should never be called");
	return nullptr;
}

Interval<const void*> DfaPatternProcessor::LocatePartialMatch(const void* data, size_t length, size_t offset) const
{
	JERROR("Should never be called");
	return {nullptr, nullptr};
}

const void* DfaPatternProcessor::PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const
{
	JERROR("Should never be called");
	return nullptr;
}

//============================================================================

#if !defined(JABI_AMD64_SYSTEM_V) && !defined(JASM_GNUC_ARM) && !defined(JASM_GNUC_ARM64)
PatternProcessor* PatternProcessor::CreateDfaProcessor(const void* data, size_t length)
{
	return new DfaPatternProcessor(data, length);
}
#endif

//============================================================================
