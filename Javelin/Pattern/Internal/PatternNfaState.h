//============================================================================

#pragma once
#include "Javelin/Container/OpenHashSet.h"
#include "Javelin/Pattern/Internal/PatternProcessor.h"
#include "Javelin/Pattern/Internal/PatternTypes.h"

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

	class NfaState
	{
	public:
		struct UpdateCache
		{
			uint32_t	maximumNumberOfStates;
			uint32_t	possibleIndex[1];
			
			static size_t GetSizeRequiredForNumberOfStates(uint32_t numberOfStates) { return sizeof(UpdateCache) + sizeof(uint32_t) * numberOfStates; }
		};

		enum Flag
		{
			IS_WORD_CHARACTER			= 1,
			IS_END_OF_LINE				= 2,
			IS_END_OF_INPUT				= 4,
			IS_START_OF_INPUT			= 8,

			WAS_WORD_CHARACTER			= 0x10,
			WAS_END_OF_LINE				= 0x20,
			IS_START_OF_SEARCH			= 0x40,

			WAS_MASK					= 0x30,
			
			IS_END_OF_LINE_MASK			= 0x200,
			IS_END_OF_INPUT_MASK		= 0x400,
			IS_START_OF_INPUT_MASK		= 0x800,		// These are << 8 of the above
			WAS_WORD_CHARACTER_MASK		= 0x1000,
			WAS_END_OF_LINE_MASK		= 0x2000,
			IS_START_OF_SEARCH_MASK		= 0x4000,
			
			ASSERT_MASK					= 0x7800,
			
			IS_SEARCH					= 0x10000,
			IS_MATCH					= 0x20000,
			HAS_EMPTY_STATES			= 0x40000,		// Used by Dfa
			DFA_NEEDS_POPULATING		= 0x80000,		// Used by Dfa
			DFA_STATE_IS_RESETTING		= 0x100000,		// Used by Dfa
			DFA_STATE_WILL_RELEASE		= 0x200000,		// Used by Dfa
			DFA_STATE_IS_POPULATING		= 0x400000,		// Used by Dfa
			DFA_FAILED					= 0x800000,		// Used by Dfa

			PARTIAL_MATCH_IS_ALLOWED	= 0x20000000,
			STATE_ADDITION_DISABLED		= 0x80000000
		};

		// For forward processing
		static const uint32_t END_OF_INPUT = 256;
		
		// For reverse processing
		static const uint32_t START_OF_INPUT  = 256;

		uint32_t	stateFlags;
		uint32_t	numberOfStates;
		uint32_t	stateList[1];
		
		static size_t GetSizeRequiredForNumberOfStates(uint32_t numberOfStates) { return sizeof(NfaState) - sizeof(uint32_t) + numberOfStates * sizeof(uint32_t); }
		
		// range.min is the current character. range.max is the relevancy interval.
		void Process(NfaState& outResult, const PatternData& patternData, CharacterRange& range, uint32_t flags, UpdateCache &updateCache) const;
		void AddNextState(const PatternData& patternData, uint32_t pc, CharacterRange& range, UpdateCache &updateCache, int nextFlags);

		void ProcessReverse(NfaState& outResult, const PatternData& patternData, CharacterRange& range, uint32_t flags, UpdateCache &updateCache) const;
		void AddNextStateReverse(const PatternData& patternData, uint32_t pc, CharacterRange& range, UpdateCache &updateCache, int nextFlags);

		void Reset() { numberOfStates = 0; stateFlags = 0; }
		bool IsSearch() const;
		void ClearIrrelevantFlags();

	private:
		static uint32_t ConvertFlagsToNextFlags(uint32_t flags) { return (flags << 4) & (WAS_MASK | STATE_ADDITION_DISABLED); }
		
		bool AddEntry(uint32_t pc, UpdateCache &updateCache);
		bool AddEntryNoMatchCheck(uint32_t pc, UpdateCache &updateCache);
		void AddEntryNoCheck(uint32_t pc);

		void ProcessCurrentState(NfaState& outResult, const PatternData& patternData, uint32_t pc, CharacterRange& range, uint32_t flags, UpdateCache &updateCache) const;
		void ProcessNextStateFlags(OpenHashSet<uint32_t>& progressCheckSet, const PatternData& patternData, uint32_t pc, CharacterRange& range, UpdateCache &updateCache, int nextFlags);

		void ProcessCurrentStateReverse(NfaState& outResult, const PatternData& patternData, uint32_t pc, CharacterRange& range, uint32_t flags, UpdateCache &updateCache) const;
		void ProcessNextStateFlagsReverse(OpenHashSet<uint32_t>& progressCheckSet, const PatternData& patternData, uint32_t pc, CharacterRange& range, UpdateCache &updateCache, int nextFlags);
	};

//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
