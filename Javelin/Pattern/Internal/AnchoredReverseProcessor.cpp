//============================================================================

#include "Javelin/Pattern/Internal/PatternReverseProcessor.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class AnchoredReverseProcessor final : public ReverseProcessor
{
public:
	AnchoredReverseProcessor() { }
	virtual const void* Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char** captures, bool matchIsAnchored) const final;
};

//============================================================================

const void* AnchoredReverseProcessor::Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char** captures, bool matchIsAnchored) const
{
	return (const uint8_t*) data + startOffset;
}

//============================================================================

ReverseProcessor* ReverseProcessor::CreateAnchoredReverseProcessor()
{
	return new AnchoredReverseProcessor;
}

//============================================================================
