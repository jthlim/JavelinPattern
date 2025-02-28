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

class OnePassReverseProcessor final : public ReverseProcessor
{
public:
	OnePassReverseProcessor(const void* data, size_t length);
	~OnePassReverseProcessor();
	
	virtual const void* Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char** captures, bool matchIsAnchored) const final;
	virtual bool ProvidesCaptures() const { return true; }

private:
	struct ProcessData
	{
		const unsigned char*			pStop;
		const unsigned char**			captures;
	};

	PatternData				patternData;
	uint32_t				startingInstruction;
	ExpandedJumpTables		expandedJumpTables;

	void Process(uint32_t pc, const unsigned char* p, const ProcessData& processData) const;
	
	void Set(const void* data, size_t length);
};

//============================================================================

OnePassReverseProcessor::OnePassReverseProcessor(const void* data, size_t length)
{
	Set(data, length);
}

void OnePassReverseProcessor::Set(const void* data, size_t length)
{
	const ByteCodeHeader* header = (const ByteCodeHeader*) data;
	startingInstruction = header->reverseMatchStartingInstruction;
	patternData.p = (const unsigned char *) header->GetReverseProgram();
	expandedJumpTables.Set(patternData, header->numberOfReverseInstructions);
}

OnePassReverseProcessor::~OnePassReverseProcessor()
{
}

//============================================================================

void OnePassReverseProcessor::Process(uint32_t pc, const unsigned char* p, const ProcessData& processData) const
{
Loop:
	const ByteCodeInstruction instruction = patternData[pc];
	switch(instruction.type)
	{
	case InstructionType::AdvanceByte:
	case InstructionType::AnyByte:
		--p;
		++pc;
		goto Loop;
		
	case InstructionType::ByteJumpTable:
	case InstructionType::ByteJumpMask:
		{
			JASSERT(p != processData.pStop);
			const uint32_t* data = expandedJumpTables.GetJumpTable(pc);
			pc = data[p[-1]];
			JASSERT(pc != TypeData<uint32_t>::Maximum());
			--p;
			goto Loop;
		}
			
	case InstructionType::ByteJumpRange:
		{
			JASSERT(p != processData.pStop);
			const ByteCodeJumpRangeData* jumpRangeData = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			pc = jumpRangeData->pcData[jumpRangeData->range.Contains(p[-1])];
			JASSERT(pc != TypeData<uint32_t>::Maximum());
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
		JASSERT(pc != TypeData<uint32_t>::Maximum());
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
		JASSERT(pc != TypeData<uint32_t>::Maximum());
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
		JASSERT(pc != TypeData<uint32_t>::Maximum());
		goto Loop;
		
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
		return;
			
	case InstructionType::SaveNoRecurse:
	case InstructionType::Save:
		{
			uint32_t saveIndex = instruction.data & 0xff;
			uint32_t saveOffset = instruction.data >> 8;
			processData.captures[saveIndex] = p + saveOffset;
		}
		++pc;
		goto Loop;

	case InstructionType::AssertStartOfInput:
	case InstructionType::AssertEndOfInput:
	case InstructionType::AssertStartOfLine:
	case InstructionType::AssertEndOfLine:
	case InstructionType::AssertWordBoundary:
	case InstructionType::AssertNotWordBoundary:
	case InstructionType::AssertStartOfSearch:
	case InstructionType::AssertRecurseValue:
	case InstructionType::BackReference:
	case InstructionType::Byte:
	case InstructionType::ByteEitherOf2:
	case InstructionType::ByteEitherOf3:
	case InstructionType::ByteRange:
	case InstructionType::ByteBitMask:
	case InstructionType::ByteNot:
	case InstructionType::ByteNotEitherOf2:
	case InstructionType::ByteNotEitherOf3:
	case InstructionType::ByteNotRange:
	case InstructionType::Call:
	case InstructionType::Fail:
	case InstructionType::Possess:
	case InstructionType::ProgressCheck:
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
	case InstructionType::Split:
	case InstructionType::SplitMatch:
	case InstructionType::SplitNextN:
	case InstructionType::SplitNextMatchN:
	case InstructionType::SplitNNext:
	case InstructionType::SplitNMatchNext:
	case InstructionType::StepBack:
	case InstructionType::Success:
		JERROR("Unexpected instruction");
	}
	
	JERROR("Unhandled switch case");
	return;
}

//============================================================================

const void* OnePassReverseProcessor::Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char** captures, bool matchIsAnchored) const
{
	ProcessData processData;
	processData.pStop = (const unsigned char*) data + startOffset;
	processData.captures = (const unsigned char**) captures;

	Process(startingInstruction, (const unsigned char*) matchEnd, processData);
	return nullptr;
}

//============================================================================

ReverseProcessor* ReverseProcessor::CreateOnePassReverseProcessor(const void* data, size_t length)
{
	return new OnePassReverseProcessor(data, length);
}

//============================================================================
#endif // !PATTERN_USE_JIT
//============================================================================
