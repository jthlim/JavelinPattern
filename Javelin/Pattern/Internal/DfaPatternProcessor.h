//============================================================================

#pragma once
#include "Javelin/Pattern/Internal/PatternProcessor.h"
#include "Javelin/Pattern/Internal/DfaProcessorBase.h"

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

		class NfaState;

//============================================================================

		class DfaPatternProcessor : public PatternProcessor, protected DfaProcessorBase
		{
		public:
			DfaPatternProcessor(const void* data, size_t length);
			
			const void* FullMatch(const void* data, size_t length) const override;
			const void* FullMatch(const void* data, size_t length, const char **captures) const override;
			const void* PartialMatch(const void* data, size_t length, size_t offset) const override;
			const void* PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const override;
			virtual Interval<const void*> LocatePartialMatch(const void* data, size_t length, size_t offset) const override;
			const void* PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const override;

			bool ProvidesCaptures() const override { return false; }
			
		protected:
			SearchHandler GetSearchHandler(SearchHandlerEnum value, State* state) const override;
			
		private:
			bool						matchRequiresEndOfInput;
			uint32_t					partialMatchStartingInstruction;
			uint32_t					fullMatchStartingInstruction;
			
			enum StartingIndex
			{
				StartOfInput,
				StartOfLine,
				WordPrior,
				NotWordPrior,
				Count
			};
			
			mutable State *volatile		fullMatchStartingState = nullptr;
			mutable State *volatile		partialMatchStartingStates[StartingIndex::Count] = { nullptr, nullptr, nullptr, nullptr };
			
			void ClearStartingStates() override;
			void ProcessNfaState(NfaState& result, const NfaState& state, const PatternData& patternData, CharacterRange& relevancyInterval, int flags, void* updateCache) const override;
			const uint8_t* GetCharacterFlags() const override;

			template<SearchMode SEARCH_MODE> const void* SearchForwards(State* state, const unsigned char* p, const unsigned char *pEnd) const;
			
			template<const void* (*FUNCTION)(const void*, uint64_t, const void*)> static const unsigned char* FindByteForwarder(const unsigned char* p, const void* data, const unsigned char* pEnd);
			template<const void* (*FUNCTION)(const void*, uint64_t, const void*)> static const unsigned char* FindByteWithAssertForwarder(const unsigned char* p, const void* data, const unsigned char* pEnd);
			static const unsigned char* FindByte0WithAssertHandler(const unsigned char* p, const void* data, const unsigned char* pEnd);
			static const unsigned char* FindBoyerMooreWithAssertForwarder(const unsigned char* p, const void* data, const unsigned char* pEnd);
			static const unsigned char* FindShiftOrWithAssertForwarder(const unsigned char* p, const void* data, const unsigned char* pEnd);
		};

//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
