//============================================================================

#include "Javelin/Pattern/Internal/PatternReverseProcessor.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

#if !defined(FUNCTION_FIND_BYTE_REVERSE_DEFINED)
const void* ReverseProcessor::FindByteReverse(const void* p, uint64_t v, const void* pEnd)
{
	const unsigned char* s = (const unsigned char*) p;
	while(s > pEnd)
	{
		if(*--s == v) return s;
	}
	return nullptr;
}
#endif

//============================================================================

ReverseProcessor* ReverseProcessor::CreateNfaReverseProcessor(const void* data, size_t length)
{
	return CanUseBitFieldGlushkovNfaReverseProcessor(data, length) ?
			CreateBitFieldGlushkovNfaReverseProcessor(data, length) :
			CreateThompsonNfaReverseProcessor(data, length);
}

//============================================================================

ReverseProcessor* ReverseProcessor::Create(const void* data, size_t length, bool allowCaptures)
{
	const ByteCodeHeader* header = (const ByteCodeHeader*) data;
	switch(header->flags.reverseProcessorType)
	{
	case PatternProcessorType::Anchored:
		return ReverseProcessor::CreateAnchoredReverseProcessor();
		
	case PatternProcessorType::BackTracking:
		return ReverseProcessor::CreateBackTrackingReverseProcessor(data, length);
		
	case PatternProcessorType::FixedLength:
		return ReverseProcessor::CreateFixedLengthReverseProcessor(data, length);
		
	case PatternProcessorType::OnePass:
		return allowCaptures ?
			ReverseProcessor::CreateOnePassReverseProcessor(data, length) :
			ReverseProcessor::CreateBackTrackingReverseProcessor(data, length);
		
	case PatternProcessorType::ScanAndCapture:
		return ReverseProcessor::CreateNfaOrDfaReverseProcessor(data, length);
		
	default:
		JERROR("Unexpected reverse processor type");
		return nullptr;
	}
}

//============================================================================
