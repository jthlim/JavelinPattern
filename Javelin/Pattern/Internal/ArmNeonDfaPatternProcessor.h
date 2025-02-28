//============================================================================

#pragma once
#include "Javelin/Pattern/Internal/DfaPatternProcessor.h"
#include <arm_neon.h>

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

	class ArmNeonDfaPatternProcessor : public DfaPatternProcessor
	{
	public:
		ArmNeonDfaPatternProcessor(const void* data, size_t length) : DfaPatternProcessor(data, length) { }
		
	private:
		SearchHandler GetSearchHandler(SearchHandlerEnum value, State* state) const override;

		void CreateByteRangePairData(State* state) const;
		void CreateNibbleMask(State* state, int numberOfBytes) const;
		template<size_t dataOffset, const void* (*FUNCTION)(const void*, const uint8x16_t*, const void*)> static const unsigned char* FindNeonWrapper(const unsigned char* p, const void* data, const unsigned char* pEnd);
		template<size_t dataOffset, const void* (*FUNCTION)(const void*, const uint8x16_t*, const void*)> static const unsigned char* FindNeonAssertWrapper(const unsigned char* p, const void* data, const unsigned char* pEnd);
	};

//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
