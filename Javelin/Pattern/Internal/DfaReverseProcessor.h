//============================================================================

#pragma once
#include "Javelin/Pattern/Internal/PatternReverseProcessor.h"
#include "Javelin/Pattern/Internal/DfaProcessorBase.h"

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

	class NfaState;

//============================================================================

	class DfaReverseProcessor final : public ReverseProcessor, private DfaProcessorBase
	{
	public:
		DfaReverseProcessor(const void* data, size_t length);
		
		const void* Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char **captures, bool matchIsAnchored) const override;

	private:
		uint32_t	startingInstruction;
		
		enum StartingIndex
		{
			EndOfInput,
			EndOfLine,
			WordAfter,
			NotWordAfter,
			Count
		};
		
		mutable State *volatile		anchoredStartingStates[StartingIndex::Count] = { nullptr, nullptr, nullptr, nullptr };
		mutable State *volatile		unanchoredStartingStates[StartingIndex::Count] = { nullptr, nullptr, nullptr, nullptr };
		
		void ClearStartingStates() override;
		void ProcessNfaState(NfaState& result, const NfaState& state, const PatternData& patternData, CharacterRange& relevancyInterval, int flags, void* updateCache) const override;
		SearchHandler GetSearchHandler(SearchHandlerEnum value, State* state) const override;
		const uint8_t* GetCharacterFlags() const override;

		template<SearchMode SEARCH_MODE> const void* SearchReverse(State* state, const unsigned char* p, const unsigned char *pStop, bool stopIsStartOfInput) const;

		template<const void* (*FUNCTION)(const void*, uint64_t, const void*)> static const unsigned char* FindByteForwarder(const unsigned char* p, const void* data, const unsigned char* pStop);
		template<const void* (*FUNCTION)(const void*, uint64_t, const void*)> static const unsigned char* FindByteWithAssertForwarder(const unsigned char* p, const void* data, const unsigned char* pStop);
		static const unsigned char* FindByte0WithAssertHandler(const unsigned char* p, const void* data, const unsigned char* pStop);
	};

//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
