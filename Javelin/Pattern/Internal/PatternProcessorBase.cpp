//============================================================================

#include "Javelin/Pattern/Internal/PatternProcessorBase.h"
#include "Javelin/Container/EnumSet.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

constexpr bool PatternProcessorBase::WORD_MASK[] =
{
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	 true,  true,  true,  true,  true,  true,  true,  true,  true,  true, false, false, false, false, false, false,
	false,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	 true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true, false, false, false, false,  true,
	false,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	 true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, 			// extra for EOF used by PatternNfaState
};

//============================================================================

static constexpr EnumSet<InstructionType, uint64_t> EXPAND_JUMP_TABLE_SET
{
	InstructionType::ByteJumpMask,
	InstructionType::ByteJumpTable,
	InstructionType::DispatchMask,
	InstructionType::DispatchTable,
};

void PatternProcessorBase::ExpandedJumpTables::Set(const PatternData& patternData, size_t numberOfInstructions)
{
	// Allocate all buffers
	jumpTableForPcList.SetCount(numberOfInstructions);
	jumpTableForPcList.SetAll(nullptr);

	size_t jumpTableSize = 0;
	for(size_t pc = 0; pc < numberOfInstructions; ++pc)
	{
		if(!EXPAND_JUMP_TABLE_SET.Contains(patternData[pc].type)) continue;
		jumpTableSize += 256;
	}
	jumpTable.SetCount(jumpTableSize);

	// Record the offsets
	uint32_t* p = jumpTable.GetData();
	for(size_t pc = 0; pc < numberOfInstructions; ++pc)
	{
		if(!EXPAND_JUMP_TABLE_SET.Contains(patternData[pc].type)) continue;
		jumpTableForPcList[pc] = p;
		p += 256;
	}
	
	// Populate the data
	for(size_t pc = 0; pc < numberOfInstructions; ++pc)
	{
		ByteCodeInstruction instruction = patternData[pc];
		switch(instruction.type)
		{
		case InstructionType::ByteJumpTable:
		case InstructionType::DispatchTable:
			{
				const ByteCodeJumpTableData* jumpTableData = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
				uint32_t* p = jumpTableForPcList[pc];
				JASSERT(p != nullptr);
				for(int c = 0; c < 256; ++c)
				{
					p[c] = jumpTableData->pcData[jumpTableData->jumpTable[c]];
				}
			}
			break;

		case InstructionType::ByteJumpMask:
		case InstructionType::DispatchMask:
			{
				const ByteCodeJumpMaskData* jumpMaskData = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
				uint32_t* p = jumpTableForPcList[pc];
				JASSERT(p != nullptr);
				for(int c = 0; c < 256; ++c)
				{
					p[c] = jumpMaskData->pcData[jumpMaskData->bitMask[c]];
				}
			}
			break;

		default:
			JASSERT(!EXPAND_JUMP_TABLE_SET.Contains(instruction.type));
			break;
		}
	}
}

//============================================================================
