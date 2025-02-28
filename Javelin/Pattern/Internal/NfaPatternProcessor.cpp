//============================================================================

#include "Javelin/Pattern/Internal/PatternProcessor.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class NfaPatternProcessor final : public PatternProcessor
{
public:
	NfaPatternProcessor(const void* data, size_t length);
	NfaPatternProcessor(DataBlock&& dataBlock);
	~NfaPatternProcessor();
	
	virtual const void* FullMatch(const void* data, size_t length) const;
	virtual const void* FullMatch(const void* data, size_t length, const char **captures) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const;
	virtual Interval<const void*> LocatePartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const;

private:
	DataBlock				dataStore;
	PatternProcessor*		nonCapturingProcessor;
	PatternProcessor*		capturingProcessor;
	
	void Set(const void* data, size_t length);
};

//============================================================================

NfaPatternProcessor::NfaPatternProcessor(const void* data, size_t length)
{
	Set(data, length);
}

NfaPatternProcessor::NfaPatternProcessor(DataBlock&& dataBlock)
: dataStore((DataBlock&&) dataBlock)
{
	Set(dataStore.GetData(), dataStore.GetCount());
}

void NfaPatternProcessor::Set(const void* data, size_t length)
{
	nonCapturingProcessor = PatternProcessor::CreateThompsonNfaProcessor(data, length);
	capturingProcessor = PatternProcessor::CreatePikeNfaProcessor(data, length);
}

NfaPatternProcessor::~NfaPatternProcessor()
{
	delete nonCapturingProcessor;
	delete capturingProcessor;
}

//============================================================================

const void* NfaPatternProcessor::FullMatch(const void* data, size_t length) const
{
	return nonCapturingProcessor->FullMatch(data, length);
}

const void* NfaPatternProcessor::FullMatch(const void* data, size_t length, const char **captures) const
{
	return capturingProcessor->FullMatch(data, length, captures);
}

const void* NfaPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset) const
{
	return nonCapturingProcessor->PartialMatch(data, length, offset);
}

const void* NfaPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const
{
	return capturingProcessor->PartialMatch(data, length, offset, captures);
}

Interval<const void*> NfaPatternProcessor::LocatePartialMatch(const void* data, size_t length, size_t offset) const
{
	return capturingProcessor->LocatePartialMatch(data, length, offset);
}

const void* NfaPatternProcessor::PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const
{
	return capturingProcessor->PopulateCaptures(data, length, offset, captures);
}

//============================================================================

PatternProcessor* PatternProcessor::CreateNfaProcessor(DataBlock&& dataBlock)
{
	return new NfaPatternProcessor((DataBlock&&) dataBlock);
}

PatternProcessor* PatternProcessor::CreateNfaProcessor(const void* data, size_t length, bool makeCopy)
{
	if(makeCopy)
	{
		DataBlock dataBlock(data, length);
		return new NfaPatternProcessor((DataBlock&&) dataBlock);
	}
	else
	{
		return new NfaPatternProcessor(data, length);
	}
}

//============================================================================
