//============================================================================

#include "Javelin/Pattern/Internal/PatternProcessor.h"
#include "Javelin/Pattern/Internal/PatternReverseProcessor.h"
#include "Javelin/Thread/Mutex.h"
#include "Javelin/Type/Exception.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class ConsistencyCheckPatternProcessor final : public PatternProcessor
{
public:
	ConsistencyCheckPatternProcessor(const void* data, size_t length);
	ConsistencyCheckPatternProcessor(DataBlock&& dataBlock);
	~ConsistencyCheckPatternProcessor();
	
	virtual const void* FullMatch(const void* data, size_t length) const;
	virtual const void* FullMatch(const void* data, size_t length, const char **captures) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const;
	virtual Interval<const void*> LocatePartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const;

private:
	bool 				reverseMatchRequiresStartOfSearch;
	uint32_t			numberOfCaptures;
	DataBlock			dataStore;
	
	PatternProcessor*	processorList[7];
	int					numberOfProcessors = 0;
	
	ReverseProcessor*	reverseProcessorList[4];
	int					numberOfReverseProcessors = 0;
	
	void SetForwards(const void* data, size_t length);
	void SetReverse(const void* data, size_t length);
};

//============================================================================

ConsistencyCheckPatternProcessor::ConsistencyCheckPatternProcessor(const void* data, size_t length)
{
	SetForwards(data, length);
	SetReverse(data, length);
}

ConsistencyCheckPatternProcessor::ConsistencyCheckPatternProcessor(DataBlock&& dataBlock)
: dataStore((DataBlock&&) dataBlock)
{
	SetForwards(dataStore.GetData(), dataStore.GetCount());
	SetReverse(dataStore.GetData(), dataStore.GetCount());
}

ConsistencyCheckPatternProcessor::~ConsistencyCheckPatternProcessor()
{
	for(int i = 0; i < numberOfProcessors; ++i)
	{
		delete processorList[i];
	}
	
	for(int i = 0; i < numberOfReverseProcessors; ++i)
	{
		delete reverseProcessorList[i];
	}
}

void ConsistencyCheckPatternProcessor::SetForwards(const void* data, size_t length)
{
	const ByteCodeHeader* header = (const ByteCodeHeader*) data;
	numberOfCaptures = header->numberOfCaptures;
	
	const PatternData patternData(header->GetForwardProgram());
	bool containsBackTrackingOnly = patternData.ContainsInstruction(InstructionType::BackReference, header->numberOfInstructions)
									|| patternData.ContainsInstruction(InstructionType::Recurse, header->numberOfInstructions)
									|| patternData.ContainsInstruction(InstructionType::Call, header->numberOfInstructions)
									|| patternData.ContainsInstruction(InstructionType::Possess, header->numberOfInstructions);
	
	bool containsSplit = patternData.ContainsInstruction(InstructionType::Split, header->numberOfInstructions)
						 || patternData.ContainsInstruction(InstructionType::SplitMatch, header->numberOfInstructions)
						 || patternData.ContainsInstruction(InstructionType::SplitNextN, header->numberOfInstructions)
						 || patternData.ContainsInstruction(InstructionType::SplitNNext, header->numberOfInstructions)
						 || patternData.ContainsInstruction(InstructionType::SplitNextMatchN, header->numberOfInstructions)
						 || patternData.ContainsInstruction(InstructionType::SplitNMatchNext, header->numberOfInstructions);
	
	processorList[numberOfProcessors++] = PatternProcessor::CreateBackTrackingProcessor(data, length, false);
	
	if(!containsSplit)
	{
		processorList[numberOfProcessors++] = PatternProcessor::CreateOnePassProcessor(data, length, false);
	}

	if(!containsBackTrackingOnly)
	{
		processorList[numberOfProcessors++] = PatternProcessor::CreateThompsonNfaProcessor(data, length);
		processorList[numberOfProcessors++] = PatternProcessor::CreateDfaProcessor(data, length);
		processorList[numberOfProcessors++] = PatternProcessor::CreatePikeNfaProcessor(data, length);
		processorList[numberOfProcessors++] = PatternProcessor::CreateBitStateProcessor(data, length);
	}
	
	if(header->flags.reverseProcessorType != PatternProcessorType::None)
	{
		processorList[numberOfProcessors++] = PatternProcessor::CreateScanAndCaptureProcessor(data, length, false);
	}
}

void ConsistencyCheckPatternProcessor::SetReverse(const void* data, size_t length)
{
	const ByteCodeHeader* header = (const ByteCodeHeader*) data;
	if(header->flags.reverseProcessorType == PatternProcessorType::None
	   || header->flags.reverseProcessorType == PatternProcessorType::FixedLength
	   || header->flags.reverseProcessorType == PatternProcessorType::Anchored) return;
	
	reverseMatchRequiresStartOfSearch = header->partialMatchStartingInstruction == header->fullMatchStartingInstruction;
	
	if(header->flags.reverseProcessorType == PatternProcessorType::OnePass)
	{
		reverseProcessorList[numberOfReverseProcessors++] = ReverseProcessor::CreateOnePassReverseProcessor(data, length);
	}
	else
	{
		if(ReverseProcessor::CanUseBitFieldGlushkovNfaReverseProcessor(data, length))
		{
			reverseProcessorList[numberOfReverseProcessors++] = ReverseProcessor::CreateBitFieldGlushkovNfaReverseProcessor(data, length);
		}
		reverseProcessorList[numberOfReverseProcessors++] = ReverseProcessor::CreateBackTrackingReverseProcessor(data, length);
		reverseProcessorList[numberOfReverseProcessors++] = ReverseProcessor::CreateThompsonNfaReverseProcessor(data, length);
		reverseProcessorList[numberOfReverseProcessors++] = ReverseProcessor::CreateDfaReverseProcessor(data, length);
	}
}

//============================================================================

const void* ConsistencyCheckPatternProcessor::FullMatch(const void* data, size_t length) const
{
	const void* result = processorList[0]->FullMatch(data, length);
	for(uint32_t i = 1; i < numberOfProcessors; ++i)
	{
		PatternProcessor* processor = processorList[i];
		if(processor->CanUseFullMatchProgram(length))
		{
			const void* resultN = processor->FullMatch(data, length);
			if(resultN == (const void*) DFA_FAILED) continue;
			
			JDATA_VERIFY(result == resultN);
		}
	}
	return result;
}

const void* ConsistencyCheckPatternProcessor::FullMatch(const void* data, size_t length, const char **captures) const
{
	const void* result = processorList[0]->FullMatch(data, length, captures);
	for(uint32_t i = 1; i < numberOfProcessors; ++i)
	{
		PatternProcessor* processor = processorList[i];
		if(processor->CanUseFullMatchProgram(length) && processor->ProvidesCaptures())
		{
			const char *localCaptures[numberOfCaptures*2];
			memset(localCaptures, 0, 2*sizeof(const char*)*numberOfCaptures);
			const void* resultN = processor->FullMatch(data, length, localCaptures);
			if(resultN == (const void*) DFA_FAILED) continue;
			
			JDATA_VERIFY(result == resultN);
			if(result == nullptr) continue;
			JDATA_VERIFY(memcmp(captures, localCaptures, 2*sizeof(const char*)*numberOfCaptures) == 0);
		}
	}
	return result;
}

const void* ConsistencyCheckPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset) const
{
	const void* result = processorList[0]->PartialMatch(data, length, offset);
	for(uint32_t i = 1; i < numberOfProcessors; ++i)
	{
		PatternProcessor* processor = processorList[i];
		if(processor->CanUsePartialMatchProgram(length-offset))
		{
			const void* resultN = processor->PartialMatch(data, length, offset);
			if(resultN == (const void*) DFA_FAILED) continue;
			
			JDATA_VERIFY(result == resultN);
		}
	}
	return result;
}

const void* ConsistencyCheckPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const
{
	memset(captures, 0, 2*sizeof(const char*)*numberOfCaptures);
	const void* result = processorList[0]->PartialMatch(data, length, offset, captures);
	for(uint32_t i = 1; i < numberOfProcessors; ++i)
	{
		PatternProcessor* processor = processorList[i];
		if(processor->CanUsePartialMatchProgram(length-offset) && processor->ProvidesCaptures())
		{
			const char *localCaptures[numberOfCaptures*2];
			memset(localCaptures, 0, 2*sizeof(const char*)*numberOfCaptures);
			const void* resultN = processor->PartialMatch(data, length, offset, localCaptures);
			if(resultN == (const void*) DFA_FAILED) continue;
			
			JDATA_VERIFY(result == resultN);
			if(result == nullptr) continue;
			JDATA_VERIFY(memcmp(captures, localCaptures, 2*sizeof(const char*)*numberOfCaptures) == 0);
		}
	}
	
	if(result)
	{
		for(uint32_t i = 0; i < numberOfReverseProcessors; ++i)
		{
			ReverseProcessor* processor = reverseProcessorList[i];
			if(processor->ProvidesCaptures())
			{
				const char *localCaptures[numberOfCaptures*2];
				memset(localCaptures, 0, 2*sizeof(const char*)*numberOfCaptures);
				const void* resultN = processor->Match(data, length, offset, result, localCaptures, reverseMatchRequiresStartOfSearch);

				JDATA_VERIFY(resultN == nullptr);
				JDATA_VERIFY(memcmp(captures, localCaptures, 2*sizeof(const char*)*numberOfCaptures) == 0);
			}
			else
			{
				const void* resultN = processor->Match(data, length, offset, result, nullptr, reverseMatchRequiresStartOfSearch);
				JDATA_VERIFY(captures[0] == resultN);
			}
		}
	}
	
	return result;
}

Interval<const void*> ConsistencyCheckPatternProcessor::LocatePartialMatch(const void* data, size_t length, size_t offset) const
{
	Interval<const void*> result = processorList[0]->LocatePartialMatch(data, length, offset);
	for(uint32_t i = 1; i < numberOfProcessors; ++i)
	{
		PatternProcessor* processor = processorList[i];
		if(processor->CanUsePartialMatchProgram(length-offset) && processor->ProvidesCaptures())
		{
			Interval<const void*> resultN = processor->LocatePartialMatch(data, length, offset);
			JDATA_VERIFY(result == resultN || (result.max == nullptr && resultN.max == nullptr));
		}
	}
	return result;
}

const void* ConsistencyCheckPatternProcessor::PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const
{
	const void* result = processorList[0]->PopulateCaptures(data, length, offset, captures);
	for(uint32_t i = 1; i < numberOfProcessors; ++i)
	{
		PatternProcessor* processor = processorList[i];
		if(processor->CanUseFullMatchProgram(length-offset) && processor->ProvidesCaptures())
		{
			const char *localCaptures[numberOfCaptures*2];
			memset(localCaptures, 0, 2*sizeof(const char*)*numberOfCaptures);
			const void* resultN = processor->PopulateCaptures(data, length, offset, localCaptures);
			if(resultN == (const void*) DFA_FAILED) continue;
			
			JDATA_VERIFY(result == resultN);
			if(result == nullptr) continue;
			JDATA_VERIFY(memcmp(captures, localCaptures, 2*sizeof(const char*)*numberOfCaptures) == 0);
		}
	}
	return result;
}

//============================================================================

PatternProcessor* PatternProcessor::CreateConsistencyCheckProcessor(DataBlock&& dataBlock)
{
	return new ConsistencyCheckPatternProcessor((DataBlock&&) dataBlock);
}

PatternProcessor* PatternProcessor::CreateConsistencyCheckProcessor(const void* data, size_t length, bool makeCopy)
{
	if(makeCopy)
	{
		DataBlock dataBlock(data, length);
		return new ConsistencyCheckPatternProcessor((DataBlock&&) dataBlock);
	}
	else
	{
		return new ConsistencyCheckPatternProcessor(data, length);
	}
}

//============================================================================
