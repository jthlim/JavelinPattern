//============================================================================

#include "Javelin/Pattern/Internal/PatternReverseProcessor.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class FixedLengthReverseProcessor final : public ReverseProcessor
{
public:
	FixedLengthReverseProcessor(const void* data, size_t length);
	
	virtual const void* Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char** captures, bool matchIsAnchored) const final;
	
private:
	size_t patternMatchLength;
};

//============================================================================

FixedLengthReverseProcessor::FixedLengthReverseProcessor(const void* data, size_t length)
{
	const ByteCodeHeader* header = (const ByteCodeHeader*) data;
	JASSERT(header->IsFixedLength());
	patternMatchLength = header->minimumMatchLength;
}

//============================================================================

const void* FixedLengthReverseProcessor::Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char** captures, bool matchIsAnchored) const
{
	return (const char*) matchEnd - patternMatchLength;
}

//============================================================================

ReverseProcessor* ReverseProcessor::CreateFixedLengthReverseProcessor(const void* data, size_t length)
{
	return new FixedLengthReverseProcessor(data, length);
}

//============================================================================
