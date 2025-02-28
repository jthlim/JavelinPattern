//============================================================================

#pragma once
#include "Javelin/Pattern/Internal/DfaPatternProcessor.h"

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

	class Amd64DfaPatternProcessor : public DfaPatternProcessor
	{
	public:
		Amd64DfaPatternProcessor(const void* data, size_t length) : DfaPatternProcessor(data, length) { }
		
	private:
		SearchHandler GetSearchHandler(SearchHandlerEnum value, State* state) const override;

		void CreateByteRangeData(State* state, int count) const;
		void CreateNibbleMask(State* state, int numberOfBytes) const;
		template<SearchHandler SEARCH_FUNCTION> static const unsigned char* FindAssertWrapper(const unsigned char* p, const void* data, const unsigned char* pEnd);
	};

//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
