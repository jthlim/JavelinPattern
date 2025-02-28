//============================================================================

#include "Javelin/Pattern/Internal/PatternData.h"
#include "Javelin/Container/EnumSet.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

bool PatternData::IsSingleByteConsumerFromPcToPc(int startPc, int endPc) const
{
	int count = 1;
	int pc = startPc;
	for(;;)
	{
		const ByteCodeInstruction &instruction = (*this)[pc];
		if(instruction.IsZeroWidth())
		{
			++pc;
			continue;
		}
		
		if(instruction.IsSimpleByteConsumer())
		{
			++pc;
			--count;
			continue;
		}
		
		if(instruction.type == InstructionType::Jump)
		{
			pc = instruction.data;
			continue;
		}
		
		return pc == endPc;
	}
}

//============================================================================

static constexpr EnumSet<InstructionType, uint32_t> BYTE_JUMP_SET
{
	InstructionType::ByteJumpMask,
	InstructionType::ByteJumpTable
};

bool PatternData::HasLookupTableTarget(const ByteCodeJumpTableData& a) const
{
	for(int i = 0; i < a.numberOfTargets; ++i)
	{
		uint32_t pc = a.pcData[i];
		if(pc == TypeData<uint32_t>::Maximum()) continue;
		if(BYTE_JUMP_SET.Contains((*this)[pc].type)) return true;
	}
	return false;
}

bool PatternData::HasLookupTableTarget(const ByteCodeJumpMaskData& a) const
{
	for(int i = 0; i < 2; ++i)
	{
		uint32_t pc = a.pcData[i];
		if(pc == TypeData<uint32_t>::Maximum()) continue;
		if(BYTE_JUMP_SET.Contains((*this)[pc].type)) return true;
	}
	return false;
}

//============================================================================

bool PatternData::ContainsInstruction(InstructionType type, uint32_t numberOfInstructions) const
{
	for(uint32_t i = 0; i < numberOfInstructions; ++i)
	{
		if((*this)[i].type == type) return true;
	}
	return false;
}

int PatternData::CountSingleReferenceInstructions(int offset,
												  InstructionType type,
												  uint32_t numberOfInstructions) const
{
	int count = 0;
	while(offset < numberOfInstructions)
	{
		ByteCodeInstruction i = (*this)[offset];
		if(!i.isSingleReference) break;
		if(i.type != type) break;
		
		++count;
		++offset;
	}
	return count;
}

int PatternData::CountSingleReferenceInstructions(int offset,
												  InstructionType type,
												  uint32_t instructionData,
												  uint32_t numberOfInstructions) const
{
	int count = 0;
	while(offset < numberOfInstructions)
	{
		ByteCodeInstruction i = (*this)[offset];
		if(!i.isSingleReference) break;
		if(i.type != type) break;
		if(i.data != instructionData) break;
		
		++count;
		++offset;
	}
	return count;
}

//============================================================================

// Returns the next 2nd priority instruction, or TypeData<uint32_t>::Maximum() otherwise
uint32_t PatternData::GetMaximalJumpTargetForPc(uint32_t pc) const
{
	ByteCodeInstruction nextInstruction = (*this)[pc+1];
	switch(nextInstruction.type)
	{
	case InstructionType::SplitNNext:
		if(nextInstruction.data == pc) return pc+2;
		else return TypeData<uint32_t>::Maximum();
		
	case InstructionType::Jump:
	{
		ByteCodeInstruction target = (*this)[nextInstruction.data];
		switch(target.type)
		{
		case InstructionType::SplitNNext:
			if(target.data == pc) return nextInstruction.data+1;
			else return TypeData<uint32_t>::Maximum();
			
		case InstructionType::SplitNextN:
			if(nextInstruction.data+1 == pc) return target.data;
			else return TypeData<uint32_t>::Maximum();
			
		default:
			return TypeData<uint32_t>::Maximum();
		}
	}
	default:
		return TypeData<uint32_t>::Maximum();
	}
}

//============================================================================
