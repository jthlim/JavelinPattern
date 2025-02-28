//============================================================================

#include "Javelin/Pattern/Internal/PatternProcessor.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class NfaOrBitStateBackTrackingPatternProcessor final : public PatternProcessor
{
public:
	NfaOrBitStateBackTrackingPatternProcessor(DataBlock&& dataBlock);
	NfaOrBitStateBackTrackingPatternProcessor(const void* data, size_t length);
	~NfaOrBitStateBackTrackingPatternProcessor();
	
	virtual const void* FullMatch(const void* data, size_t length) const;
	virtual const void* FullMatch(const void* data, size_t length, const char **captures) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const;
	virtual Interval<const void*> LocatePartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const;

private:
	PatternProcessor*		pikeNfaProcessor;
	PatternProcessor*		bitStateBackTrackingProcessor;
	DataBlock				dataStore;
	
	void Set(const void* data, size_t length);
};

//============================================================================

NfaOrBitStateBackTrackingPatternProcessor::NfaOrBitStateBackTrackingPatternProcessor(const void* data, size_t length)
{
	Set(data, length);
}

NfaOrBitStateBackTrackingPatternProcessor::NfaOrBitStateBackTrackingPatternProcessor(DataBlock&& dataBlock)
: dataStore((DataBlock&&) dataBlock)
{
	Set(dataStore.GetData(), dataStore.GetCount());
}

void NfaOrBitStateBackTrackingPatternProcessor::Set(const void* data, size_t length)
{
	pikeNfaProcessor = PatternProcessor::CreatePikeNfaProcessor(data, length);
	bitStateBackTrackingProcessor = PatternProcessor::CreateBitStateProcessor(data, length);
}

NfaOrBitStateBackTrackingPatternProcessor::~NfaOrBitStateBackTrackingPatternProcessor()
{
	delete pikeNfaProcessor;
	delete bitStateBackTrackingProcessor;
}

//============================================================================

const void* NfaOrBitStateBackTrackingPatternProcessor::FullMatch(const void* data, size_t length) const
{
	if(bitStateBackTrackingProcessor->CanUseFullMatchProgram(length))
	{
		return bitStateBackTrackingProcessor->FullMatch(data, length);
	}
	else
	{
		return pikeNfaProcessor->FullMatch(data, length);
	}
}

const void* NfaOrBitStateBackTrackingPatternProcessor::FullMatch(const void* data, size_t length, const char **captures) const
{
	if(bitStateBackTrackingProcessor->CanUseFullMatchProgram(length))
	{
		return bitStateBackTrackingProcessor->FullMatch(data, length, captures);
	}
	else
	{
		return pikeNfaProcessor->FullMatch(data, length, captures);
	}
}

const void* NfaOrBitStateBackTrackingPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset) const
{
	if(bitStateBackTrackingProcessor->CanUsePartialMatchProgram(length-offset))
	{
		return bitStateBackTrackingProcessor->PartialMatch(data, length, offset);
	}
	else
	{
		return pikeNfaProcessor->PartialMatch(data, length, offset);
	}
}

const void* NfaOrBitStateBackTrackingPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const
{
	if(bitStateBackTrackingProcessor->CanUsePartialMatchProgram(length-offset))
	{
		return bitStateBackTrackingProcessor->PartialMatch(data, length, offset, captures);
	}
	else
	{
		return pikeNfaProcessor->PartialMatch(data, length, offset, captures);
	}
}

Interval<const void*> NfaOrBitStateBackTrackingPatternProcessor::LocatePartialMatch(const void* data, size_t length, size_t offset) const
{
	if(bitStateBackTrackingProcessor->CanUsePartialMatchProgram(length-offset))
	{
		return bitStateBackTrackingProcessor->LocatePartialMatch(data, length, offset);
	}
	else
	{
		return pikeNfaProcessor->LocatePartialMatch(data, length, offset);
	}
}

const void* NfaOrBitStateBackTrackingPatternProcessor::PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const
{
	if(bitStateBackTrackingProcessor->CanUseFullMatchProgram(length-offset))
	{
		return bitStateBackTrackingProcessor->PopulateCaptures(data, length, offset, captures);
	}
	else
	{
		return pikeNfaProcessor->PopulateCaptures(data, length, offset, captures);
	}
}

//============================================================================

PatternProcessor* PatternProcessor::CreateNfaOrBitStateProcessor(DataBlock&& dataBlock)
{
	return new NfaOrBitStateBackTrackingPatternProcessor((DataBlock&&) dataBlock);
}

PatternProcessor* PatternProcessor::CreateNfaOrBitStateProcessor(const void* data, size_t length, bool makeCopy)
{
	if(makeCopy)
	{
		DataBlock dataBlock(data, length);
		return new NfaOrBitStateBackTrackingPatternProcessor((DataBlock&&) dataBlock);
	}
	else
	{
		return new NfaOrBitStateBackTrackingPatternProcessor(data, length);
	}
}

//============================================================================
