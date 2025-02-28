//============================================================================

#include "Javelin/Pattern/Internal/PatternReverseProcessor.h"
#if !PATTERN_USE_JIT

#include "Javelin/Container/BitTable.h"
#include "Javelin/Pattern/Internal/PatternByteCode.h"
#include "Javelin/Stream/ICharacterWriter.h"
#include "Javelin/Template/Memory.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class BackTrackingReverseProcessor final : public ReverseProcessor
{
public:
	BackTrackingReverseProcessor(const void* data, size_t length);
	~BackTrackingReverseProcessor();
	
	virtual const void* Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char** captures, bool matchIsAnchored) const final;
	
private:
	struct ProcessData
	{
		bool							matchIsAnchored;
		const unsigned char* 			pStart;
		const unsigned char* 			pEnd;
		const unsigned char*			pStop;
		const unsigned char**			progressCheck;
		mutable const unsigned char*	leftMostMatch;
	};

	uint8_t					numberOfProgressChecks;
	PatternData				patternData;
	uint32_t				startingInstruction;
	ExpandedJumpTables		expandedJumpTables;

	void Process(uint32_t pc, const unsigned char* p, const ProcessData& processData) const;
	void DoMatch(const unsigned char* p, const ProcessData& processData) const;
	
	void Set(const void* data, size_t length);
};

//============================================================================

BackTrackingReverseProcessor::BackTrackingReverseProcessor(const void* data, size_t length)
{
	Set(data, length);
}

void BackTrackingReverseProcessor::Set(const void* data, size_t length)
{
	const ByteCodeHeader* header = (const ByteCodeHeader*) data;
	numberOfProgressChecks = header->numberOfProgressChecks;
	startingInstruction = header->reverseMatchStartingInstruction;
	patternData.p = (const unsigned char*) header->GetReverseProgram();
	expandedJumpTables.Set(patternData, header->numberOfReverseInstructions);
}

BackTrackingReverseProcessor::~BackTrackingReverseProcessor()
{
}

//============================================================================

JINLINE void BackTrackingReverseProcessor::DoMatch(const unsigned char* p, const ProcessData& processData) const
{
	if(p < processData.leftMostMatch)
	{
		if(!processData.matchIsAnchored || p == processData.pStop)
		{
			processData.leftMostMatch = p;
		}
	}
}

void BackTrackingReverseProcessor::Process(uint32_t pc, const unsigned char* p, const ProcessData& processData) const
{
Loop:
	const ByteCodeInstruction instruction = patternData[pc];
	switch(instruction.type)
	{
	case InstructionType::AdvanceByte:
		--p;
		++pc;
		goto Loop;
		
	case InstructionType::AnyByte:
		if(p == processData.pStop) return;
		--p;
		++pc;
		goto Loop;
			
	case InstructionType::AssertStartOfInput:
		if(p != processData.pStart) return;
		++pc;
		goto Loop;
			
	case InstructionType::AssertEndOfInput:
		if(p != processData.pEnd) return;
		++pc;
		goto Loop;
			
	case InstructionType::AssertStartOfLine:
		if(p != processData.pStart && p[-1] != '\n') return;
		++pc;
		goto Loop;
		
	case InstructionType::AssertEndOfLine:
		if(p != processData.pEnd && *p != '\n') return;
		++pc;
		goto Loop;
		
	case InstructionType::AssertWordBoundary:
		if(p != processData.pEnd && Character::IsWordCharacter(*p))
		{
			if(p != processData.pStart
			   && Character::IsWordCharacter(p[-1])) return;
		}
		else
		{
			if(p == processData.pStart) return;
			if(!Character::IsWordCharacter(p[-1])) return;
		}
		++pc;
		goto Loop;
			
	case InstructionType::AssertNotWordBoundary:
		if(p != processData.pEnd && Character::IsWordCharacter(*p))
		{
			if(p == processData.pStart) return;
			if(!Character::IsWordCharacter(p[-1])) return;
		}
		else
		{
			if(p != processData.pStart
			   && Character::IsWordCharacter(p[-1])) return;
		}
		++pc;
		goto Loop;
			
	case InstructionType::AssertStartOfSearch:
		if(p != processData.pStop) return;
		++pc;
		goto Loop;
			
	case InstructionType::Byte:
		if(p == processData.pStop) return;
		--p;
		if(*p != instruction.data) return;
		++pc;
		goto Loop;
			
	case InstructionType::ByteEitherOf2:
		if(p == processData.pStop) return;
		--p;
		if(*p != (instruction.data & 0xff)
		   && *p != ((instruction.data>>8) & 0xff)) return;
		++pc;
		goto Loop;
			
	case InstructionType::ByteEitherOf3:
		if(p == processData.pStop) return;
		--p;
		if(*p != (instruction.data & 0xff)
		   && *p != ((instruction.data>>8) & 0xff)
		   && *p != ((instruction.data>>16) & 0xff)) return;
		++pc;
		goto Loop;
			
	case InstructionType::ByteRange:
		if(p == processData.pStop) return;
		else
		{
			--p;
			unsigned char low = instruction.data & 0xff;
			unsigned char high = (instruction.data >> 8) & 0xff;
			if(*p < low || *p > high) return;
		}
		++pc;
		goto Loop;
			
	case InstructionType::ByteBitMask:
		if(p == processData.pStop) return;
		{
			--p;
			const StaticBitTable<256>& data = *patternData.GetData<StaticBitTable<256>>(instruction.data);
			if(!data[*p]) return;
		}
		++pc;
		goto Loop;
			
	case InstructionType::ByteNot:
		if(p == processData.pStop) return;
		--p;
		if(*p == instruction.data) return;
		++pc;
		goto Loop;
			
	case InstructionType::ByteNotEitherOf2:
		if(p == processData.pStop) return;
		--p;
		if(*p == (instruction.data & 0xff)
		   || *p == ((instruction.data>>8) & 0xff)) return;
		++pc;
		goto Loop;
			
	case InstructionType::ByteNotEitherOf3:
		if(p == processData.pStop) return;
		--p;
		if(*p == (instruction.data & 0xff)
		   || *p == ((instruction.data>>8) & 0xff)
		   || *p == ((instruction.data>>16) & 0xff)) return;
		++pc;
		goto Loop;
			
	case InstructionType::ByteNotRange:
		if(p == processData.pStop) return;
		else
		{
			--p;
			unsigned char low = instruction.data & 0xff;
			unsigned char high = (instruction.data >> 8) & 0xff;
			if(low <= *p && *p <= high) return;
		}
		++pc;
		goto Loop;
			
	case InstructionType::ByteJumpTable:
	case InstructionType::ByteJumpMask:
		if(p == processData.pStop) return;
		else
		{
			const uint32_t* data = expandedJumpTables.GetJumpTable(pc);
			pc = data[p[-1]];
			if(pc == TypeData<uint32_t>::Maximum()) return;
			--p;
			goto Loop;
		}
			
	case InstructionType::ByteJumpRange:
		if(p == processData.pStop) return;
		else
		{
			const ByteCodeJumpRangeData* jumpRangeData = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			pc = jumpRangeData->pcData[jumpRangeData->range.Contains(p[-1])];
			if(pc == TypeData<uint32_t>::Maximum()) return;
			--p;
			goto Loop;
		}
			
	case InstructionType::DispatchTable:
		if(p == processData.pStop)
		{
			const ByteCodeJumpTableData* data = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
			pc = data->pcData[0];
		}
		else
		{
			const uint32_t* data = expandedJumpTables.GetJumpTable(pc);
			pc = data[p[-1]];
		}
		if(pc == TypeData<uint32_t>::Maximum()) return;
		goto Loop;
			
	case InstructionType::DispatchMask:
		if(p == processData.pStop)
		{
			const ByteCodeJumpMaskData* data = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
			pc = data->pcData[0];
		}
		else
		{
			const uint32_t* data = expandedJumpTables.GetJumpTable(pc);
			pc = data[p[-1]];
		}
		if(pc == TypeData<uint32_t>::Maximum()) return;
		goto Loop;
			
	case InstructionType::DispatchRange:
		if(p == processData.pStop)
		{
			const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			pc = data->pcData[0];
		}
		else
		{
			const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			pc = data->pcData[data->range.Contains(p[-1])];
		}
		if(pc == TypeData<uint32_t>::Maximum()) return;
		goto Loop;
		
	case InstructionType::Fail:
		return;

	case InstructionType::FindByte:
		{
			unsigned char c = instruction.data & 0xff;
			p = (const unsigned char*) FindByteReverse(p, c, processData.pStop);
			if(!p) return;
			--p;
			pc = instruction.data >> 8;
			goto Loop;
		}
			
	case InstructionType::Jump:
		pc = instruction.data;
		goto Loop;
			
	case InstructionType::Match:
		DoMatch(p, processData);
		return;
			
	case InstructionType::ProgressCheck:
		if(processData.progressCheck[instruction.data] <= p) return;
		else
		{
			const unsigned char* old = processData.progressCheck[instruction.data];
			processData.progressCheck[instruction.data] = p;
			Process(pc+1, p, processData);
			processData.progressCheck[instruction.data] = old;
			return;
		}
			
	case InstructionType::Split:
		{
			const ByteCodeSplitData* split = patternData.GetData<ByteCodeSplitData>(instruction.data);
			uint32_t numberOfTargets = split->numberOfTargets;
			for(size_t i = 0; i < numberOfTargets-1; ++i)
			{
				Process(split->targetList[i], p, processData);
			}
			pc = split->targetList[numberOfTargets-1];
			goto Loop;
		}
		
	case InstructionType::SplitMatch:
		{
			const uint32_t* splitData = patternData.GetData<uint32_t>(instruction.data);
			Process(splitData[0], p, processData);
			pc = splitData[1];
			goto Loop;
		}
			
	case InstructionType::SplitNextN:
		{
			Process(pc+1, p, processData);
			pc = instruction.data;
			goto Loop;
		}
			
	case InstructionType::SplitNextMatchN:
		{
			DoMatch(p, processData);
			pc = instruction.data;
			goto Loop;
		}

	case InstructionType::SplitNNext:
		{
			Process(instruction.data, p, processData);
			++pc;
			goto Loop;
		}
			
	case InstructionType::SplitNMatchNext:
		{
			DoMatch(p, processData);
			++pc;
			goto Loop;
		}

	case InstructionType::SaveNoRecurse:
	case InstructionType::Save:
		++pc;
		goto Loop;

	case InstructionType::AssertRecurseValue:
	case InstructionType::BackReference:
	case InstructionType::Call:
	case InstructionType::Possess:
	case InstructionType::PropagateBackwards:
	case InstructionType::Recurse:
	case InstructionType::ReturnIfRecurseValue:
	case InstructionType::SearchByte:
	case InstructionType::SearchByteEitherOf2:
	case InstructionType::SearchByteEitherOf3:
	case InstructionType::SearchByteEitherOf4:
	case InstructionType::SearchByteEitherOf5:
	case InstructionType::SearchByteEitherOf6:
	case InstructionType::SearchByteEitherOf7:
	case InstructionType::SearchByteEitherOf8:
	case InstructionType::SearchBytePair:
	case InstructionType::SearchBytePair2:
	case InstructionType::SearchBytePair3:
	case InstructionType::SearchBytePair4:
	case InstructionType::SearchByteRange:
	case InstructionType::SearchByteRangePair:
	case InstructionType::SearchBoyerMoore:
	case InstructionType::SearchShiftOr:
	case InstructionType::SearchByteTriplet:
	case InstructionType::SearchByteTriplet2:
	case InstructionType::StepBack:
	case InstructionType::Success:
		JERROR("Unexpected instruction");
	}
	
	JERROR("Unhandled switch case");
	return;
}

//============================================================================

const void* BackTrackingReverseProcessor::Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char** captures, bool matchIsAnchored) const
{
	ProcessData processData;
	processData.matchIsAnchored = matchIsAnchored;
	processData.pStart = (const unsigned char*) data;
	processData.pEnd = (const unsigned char*) data + length;
	processData.pStop = (const unsigned char*) data + startOffset;
	processData.leftMostMatch = (const unsigned char*) matchEnd;

	const unsigned char* progressCheck[numberOfProgressChecks];
	memset(progressCheck, 0xff, numberOfProgressChecks*sizeof(const unsigned char*));
	processData.progressCheck = progressCheck;

	Process(startingInstruction, (const unsigned char*) matchEnd, processData);
	
	return processData.leftMostMatch;
}

//============================================================================

ReverseProcessor* ReverseProcessor::CreateBackTrackingReverseProcessor(const void* data, size_t length)
{
	return new BackTrackingReverseProcessor(data, length);
}

//============================================================================
#endif // !PATTERN_USE_JIT
//============================================================================
