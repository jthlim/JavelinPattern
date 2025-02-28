//============================================================================

#include "Javelin/Pattern/Internal/PatternProcessor.h"
#include "Javelin/Pattern/Internal/PatternReverseProcessor.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class ScanAndCapturePatternProcessor final : public PatternProcessor
{
public:
	ScanAndCapturePatternProcessor(const void* data, size_t length);
	ScanAndCapturePatternProcessor(DataBlock&& dataBlock);
	~ScanAndCapturePatternProcessor();
	
	virtual const void* FullMatch(const void* data, size_t length) const;
	virtual const void* FullMatch(const void* data, size_t length, const char **captures) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const;
	virtual Interval<const void*> LocatePartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const;

private:
	bool					preferReverseProcessorForFullMatchCapture;
	bool					reverseMatchRequiresStartOfSearch;
	uint8_t					numberOfCaptures;
	DataBlock				dataStore;
	PatternProcessor*		scanProcessor;
	ReverseProcessor*		reverseProcessor;
	PatternProcessor*		populateCaptureProcessor;
	
	void Set(const void* data, size_t length);
};

//============================================================================

ScanAndCapturePatternProcessor::ScanAndCapturePatternProcessor(const void* data, size_t length)
{
	Set(data, length);
}

ScanAndCapturePatternProcessor::ScanAndCapturePatternProcessor(DataBlock&& dataBlock)
: dataStore((DataBlock&&) dataBlock)
{
	Set(dataStore.GetData(), dataStore.GetCount());
}

void ScanAndCapturePatternProcessor::Set(const void* data, size_t length)
{
	const ByteCodeHeader* header = (ByteCodeHeader*) data;
	numberOfCaptures = header->numberOfCaptures;
	
	// To prevent the optimizations
	if(numberOfCaptures == 1 && header->flags.hasResetCapture) numberOfCaptures = 2;
	
	reverseMatchRequiresStartOfSearch = header->partialMatchStartingInstruction == header->fullMatchStartingInstruction;
	scanProcessor = PatternProcessor::CreateNfaOrDfaProcessor(data, length);
	
	preferReverseProcessorForFullMatchCapture = (header->flags.reverseProcessorType == PatternProcessorType::OnePass);
	reverseProcessor = ReverseProcessor::Create(data, length, true);
	
	if(header->flags.fullMatchProcessorType == PatternProcessorType::OnePass)
	{
		populateCaptureProcessor = PatternProcessor::CreateOnePassProcessor(data, length, false);
	}
	else
	{
		populateCaptureProcessor = PatternProcessor::CreateNfaOrBitStateProcessor(data, length, false);
	}
}

ScanAndCapturePatternProcessor::~ScanAndCapturePatternProcessor()
{
	delete scanProcessor;
	delete reverseProcessor;
	delete populateCaptureProcessor;
}

//============================================================================

const void* ScanAndCapturePatternProcessor::FullMatch(const void* data, size_t length) const
{
	return scanProcessor->FullMatch(data, length);
}

const void* ScanAndCapturePatternProcessor::FullMatch(const void* data, size_t length, const char **captures) const
{
	const void* result = scanProcessor->FullMatch(data, length);
	if(!result) return nullptr;
	
	if(numberOfCaptures == 1)
	{
		captures[0] = (const char*) data;
		captures[1] = (const char*) result;
		return result;
	}
	else if(preferReverseProcessorForFullMatchCapture)
	{
		reverseProcessor->Match(data, length, 0, result, captures, true);
	}
	else
	{
		populateCaptureProcessor->FullMatch(data, length, captures);
	}

	return result;
}

const void* ScanAndCapturePatternProcessor::PartialMatch(const void* data, size_t length, size_t offset) const
{
	return scanProcessor->PartialMatch(data, length, offset);
}

const void* ScanAndCapturePatternProcessor::PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const
{
	const void* result = scanProcessor->PartialMatch(data, length, offset);
	if(!result) return nullptr;

	const void* start = reverseProcessor->Match(data, length, offset, result, captures, reverseMatchRequiresStartOfSearch);
	if(start)
	{
		if(numberOfCaptures == 1)
		{
			captures[0] = (const char*) start;
			captures[1] = (const char*) result;
		}
		else
		{
			// Reducing the effective length enables the bit state back tracking processor to be used more often
			size_t simulatedLength = intptr_t(result) - intptr_t(data);
			if(simulatedLength < length) ++simulatedLength;
			populateCaptureProcessor->PopulateCaptures(data, simulatedLength, intptr_t(start)-intptr_t(data), captures);
		}
	}
	
	return result;
}

Interval<const void*> ScanAndCapturePatternProcessor::LocatePartialMatch(const void* data, size_t length, size_t offset) const
{
	const void* result = scanProcessor->PartialMatch(data, length, offset);
	if(!result) return {nullptr, nullptr};
	
	const char* captures[numberOfCaptures*2];
	const void* start = reverseProcessor->Match(data, length, offset, result, captures, reverseMatchRequiresStartOfSearch);
	if(start) return {start, result};
	return {captures[0], captures[1]};
}

const void* ScanAndCapturePatternProcessor::PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const
{
	return populateCaptureProcessor->PopulateCaptures(data, length, offset, captures);
}

//============================================================================

PatternProcessor* PatternProcessor::CreateScanAndCaptureProcessor(DataBlock&& dataBlock)
{
	return new ScanAndCapturePatternProcessor((DataBlock&&) dataBlock);
}

PatternProcessor* PatternProcessor::CreateScanAndCaptureProcessor(const void* data, size_t length, bool makeCopy)
{
	if(makeCopy)
	{
		DataBlock dataBlock(data, length);
		return new ScanAndCapturePatternProcessor((DataBlock&&) dataBlock);
	}
	else
	{
		return new ScanAndCapturePatternProcessor(data, length);
	}
}

//============================================================================
