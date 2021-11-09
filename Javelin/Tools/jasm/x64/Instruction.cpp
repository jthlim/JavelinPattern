//============================================================================

#include "Javelin/Tools/jasm/x64/Instruction.h"

#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/CommandLine.h"
#include "Javelin/Tools/jasm/Log.h"
#include "Javelin/Tools/jasm/x64/Action.h"
#include "Javelin/Tools/jasm/x64/Assembler.h"
#include "Javelin/Tools/jasm/x64/Encoder.h"

#include <assert.h>

//============================================================================

using namespace Javelin::Assembler::x64;

//============================================================================

std::string Javelin::Assembler::x64::MatchBitfieldDescription(uint64_t matchBitfield)
{
	std::string result;
	const char *const TAG_NAMES[] =
	{
		#define TAG(x,y,z) #x,
		#include "MatchTags.h"
		#undef TAG
	};
	bool first = true;
	for(int i = 0; i < sizeof(TAG_NAMES)/sizeof(TAG_NAMES[0]); ++i)
	{
		if(matchBitfield & (1ULL<<i))
		{
			if(first)
			{
				first = false;
				result = TAG_NAMES[i];
			}
			else
			{
				result = result + "|" + TAG_NAMES[i];
			}
		}
	}
	return result;
}

std::string Javelin::Assembler::x64::MatchBitfieldsDescription(const uint64_t *matchBitfields, int numberOfBitfields)
{
	std::string result;
	for(int i = 0; i < numberOfBitfields; ++i)
	{
		std::string description = MatchBitfieldDescription(matchBitfields[i]);
		if(description.length() == 0) continue;
		else if(result.size() > 0)
		{
			result.push_back(',');
			result.push_back(' ');
		}
		result.append(description);
	}
	return result;
}

//============================================================================

Action* LabelOperand::CreatePatchAction(int bytes, int offset) const
{
	if(IsExpression())
	{
		return new PatchExpressionLabelAction(bytes, offset, *this);
	}
	else
	{
		switch(jumpType)
		{
		case JumpType::Name:
			return new PatchNameLabelAction(global,
											bytes,
											offset,
											labelName,
											*this);
			break;
		case JumpType::Forward:
		case JumpType::Backward:
		case JumpType::BackwardOrForward:
			return new PatchNumericLabelAction(global,
											   bytes,
											   offset,
											   *this);
			break;
		}
	}

}

//============================================================================

int EncodingVariant::GetAvxPrefixValue() const
{
	switch(opcodePrefix)
	{
	case 0:    return 0;
	case 0x66: return 1;
	case 0xf2: return 3;
	case 0xf3: return 2;
	default: assert(!"Internal error: Unexpected prefix");
	}
	return 0;
}

int EncodingVariant::GetAvxMmValue(const uint8_t* &outOpcodes, uint32_t &outOpcodeLength) const
{
	assert(opcodeLength >= 2);
	if(opcodes[0] == 0xf)
	{
		if(opcodes[1] == 0x38)
		{
			outOpcodes = opcodes + 2;
			outOpcodeLength = opcodeLength - 2;
			return 2;
		}
		else if(opcodes[1] == 0x3a)
		{
			outOpcodes = opcodes + 2;
			outOpcodeLength = opcodeLength - 2;
			return 3;
		}
		else
		{
			outOpcodes = opcodes + 1;
			outOpcodeLength = opcodeLength - 1;
			return 1;
		}
	}
	else
	{
		assert(!"Couldn't find opcode prefix");
	}

	return 0;
}

bool EncodingVariant::Match(int operandLength, const Operand* *const operands) const
{
	if(operandMatchMasksLength != operandLength) return false;
	for(int i = 0; i < operandLength; ++i)
	{
		if(!Match(i, *operands[i])) return false;
	}
	return true;
}

const EncodingVariant *Instruction::FindFirstMatch(int operandLength, const Operand* *const operands) const
{
	for(int i = 0; i < encodingVariantLength; ++i)
	{
		const EncodingVariant *encoding = &encodingVariants[i];
		if(encoding->Match(operandLength, operands)) return encoding;
	}
	return nullptr;
}

void Instruction::AddToAssembler(const std::string &opcodeName, Assembler &assembler, ListAction& listAction, int operandLength, const Operand* *const operands) const
{
	static const AlternateActionCondition* (*const kActionConditions[])(const Operand *) =
	{
		#define TAG(x,y,z) &y##AlternateActionCondition::Create,
		#include "MatchTags.h"
		#undef TAG
	};
	static const uint8_t kExpressionSize[] =
	{
		#define TAG(x,y,z) z,
		#include "MatchTags.h"
		#undef TAG
	};
	
	AlternateAction *alternateAction = nullptr;
	
	int numberOfExpressions = 0;
	int expressionIndex = -1;
	bool hasMultipleMemoryWidths = false;
	for(int i = 0; i < operandLength; ++i)
	{
		if(operands[i]->MayHaveMultipleRepresentations())
		{
			expressionIndex = i;
			++numberOfExpressions;
		}
		if(!hasMultipleMemoryWidths) hasMultipleMemoryWidths = operands[i]->HasMultipleMemoryWidths();
	}

	if(hasMultipleMemoryWidths)
	{
		uint64_t memoryWidthMask = 0;
		for(int i = 0; i < encodingVariantLength; ++i)
		{
			const EncodingVariant *encoding = &encodingVariants[i];
			if(encoding->Match(operandLength, operands))
			{
				for(int i = 0; i < operandLength; ++i)
				{
					if(operands[i]->HasMultipleMemoryWidths())
					{
						memoryWidthMask |= encoding->operandMatchMasks[i];
					}
				}
			}
		}
		if(__builtin_popcountll(memoryWidthMask & MatchMemAll) > 1)
		{
			throw AssemblerException("Ambiguous memory width for %s instruction", opcodeName.c_str());
		}
	}
	
	for(int i = 0; i < encodingVariantLength; ++i)
	{
		const EncodingVariant *encoding = &encodingVariants[i];
		if(encoding->Match(operandLength, operands))
		{
			InstructionEncoder *encoder = encoding->encoder.GetFunction();
			if(numberOfExpressions == 0)
			{
				(*encoder)(assembler,
						   listAction,
						   *this,
						   *encoding,
						   operandLength,
						   operands);
				return;
			}
			
			ListAction* subList = new ListAction();
			
			(*encoder)(assembler,
					   *subList,
					   *this,
					   *encoding,
					   operandLength,
					   operands);
			
			if(!subList->HasData())
			{
				delete subList;
				continue;
			}
			
			const AlternateActionCondition *condition;
			if(numberOfExpressions == 1)
			{
				uint64_t operandMatchMask = encoding->operandMatchMasks[expressionIndex] & operands[expressionIndex]->matchBitfield;
				uint64_t operandMatchBitIndex = __builtin_ctzll(operandMatchMask);
				condition = (*kActionConditions[operandMatchBitIndex])(operands[expressionIndex]);
                
				if(operands[expressionIndex]->IsExpression())
				{
					int operandExpressionSize = kExpressionSize[operandMatchBitIndex];
					assembler.UpdateExpressionBitWidth(operands[expressionIndex]->expressionIndex, operandExpressionSize);
				}
			}
			else
			{
				AndAlternateActionCondition *andCondition = new AndAlternateActionCondition;
				condition = andCondition;
				for(int i = 0; i < operandLength; ++i)
				{
					if(operands[i]->MayHaveMultipleRepresentations())
					{
						uint64_t operandMatchMask = encoding->operandMatchMasks[i] & operands[i]->matchBitfield;
						uint64_t operandMatchBitIndex = __builtin_ctzll(operandMatchMask);
						if(operands[i]->IsExpression())
						{
							int operandExpressionSize = kExpressionSize[operandMatchBitIndex];
							assembler.UpdateExpressionBitWidth(operands[i]->expressionIndex, operandExpressionSize);
						}
						const AlternateActionCondition *condition = (*kActionConditions[operandMatchBitIndex])(operands[i]);
						if(condition != &AlwaysAlternateActionCondition::instance)
						{
							andCondition->AddCondition(condition);
						}
					}
				}
				if(andCondition->GetNumberOfConditions() == 0)
				{
					delete andCondition;
					condition = &AlwaysAlternateActionCondition::instance;
				}
				else if (andCondition->GetNumberOfConditions() == 1) {
					condition = andCondition->GetCondition(0);
					andCondition->ClearConditions();
					delete andCondition;
				}
			}

			if(!alternateAction) alternateAction = new AlternateAction();
			alternateAction->Add(subList, condition);
		}
	}

	if(alternateAction)
	{
		// This optimization converts a single alternate to "Always".
		if(alternateAction->GetNumberOfAlternates() == 1)
		{
			Action *action = alternateAction->GetSingleAlternateAndClearAlternateList();
			listAction.Append(action);
			delete alternateAction;
			return;
		}
		
		listAction.Append(alternateAction);
		return;
	}

	assert(operandLength <= 12);
	uint64_t operandMatchBitfields[12];
	for(int i = 0; i < operandLength; ++i) operandMatchBitfields[i] = operands[i]->matchBitfield;

	AssemblerException ex("Unable to encode instruction: %s %s", opcodeName.c_str(), MatchBitfieldsDescription(operandMatchBitfields, operandLength).c_str());
	ex.AddContext("Available matches:");
	for(int i = 0; i < encodingVariantLength; ++i)
	{
		const EncodingVariant *encoding = &encodingVariants[i];
		char buffer[256];
		sprintf(buffer, " %d: %s", i+1, MatchBitfieldsDescription(encoding->operandMatchMasks, encoding->operandMatchMasksLength).c_str());
		ex.AddContext(buffer);
	}
	throw ex;
}

//============================================================================

bool ImmediateOperand::operator==(const ImmediateOperand &a) const
{
	switch(immediateType)
	{
	case ImmediateType::Integer:
		switch(a.immediateType)
		{
		case ImmediateType::Integer:
			return value == a.value;
		case ImmediateType::Real:
			return value == a.realValue;
		}
	case ImmediateType::Real:
		switch(a.immediateType)
		{
		case ImmediateType::Integer:
			return realValue == a.value;
		case ImmediateType::Real:
			return realValue == a.realValue;
		}
	}
	return false;
}

bool ImmediateOperand::operator!=(const ImmediateOperand &a) const
{
	switch(immediateType)
	{
	case ImmediateType::Integer:
		switch(a.immediateType)
		{
		case ImmediateType::Integer:
			return value != a.value;
		case ImmediateType::Real:
			return value != a.realValue;
		}
	case ImmediateType::Real:
		switch(a.immediateType)
		{
		case ImmediateType::Integer:
			return realValue != a.value;
		case ImmediateType::Real:
			return realValue != a.realValue;
		}
	}
	return false;
}

bool ImmediateOperand::operator<(const ImmediateOperand &a) const
{
	switch(immediateType)
	{
	case ImmediateType::Integer:
		switch(a.immediateType)
		{
		case ImmediateType::Integer:
			return value < a.value;
		case ImmediateType::Real:
			return value < a.realValue;
		}
	case ImmediateType::Real:
		switch(a.immediateType)
		{
		case ImmediateType::Integer:
			return realValue < a.value;
		case ImmediateType::Real:
			return realValue < a.realValue;
		}
	}
	return false;
}

bool ImmediateOperand::operator<=(const ImmediateOperand &a) const
{
	switch(immediateType)
	{
	case ImmediateType::Integer:
		switch(a.immediateType)
		{
		case ImmediateType::Integer:
			return value <= a.value;
		case ImmediateType::Real:
			return value <= a.realValue;
		}
	case ImmediateType::Real:
		switch(a.immediateType)
		{
		case ImmediateType::Integer:
			return realValue <= a.value;
		case ImmediateType::Real:
			return realValue <= a.realValue;
		}
	}
	return false;
}

bool ImmediateOperand::operator>(const ImmediateOperand &a) const
{
	switch(immediateType)
	{
	case ImmediateType::Integer:
		switch(a.immediateType)
		{
		case ImmediateType::Integer:
			return value > a.value;
		case ImmediateType::Real:
			return value > a.realValue;
		}
	case ImmediateType::Real:
		switch(a.immediateType)
		{
		case ImmediateType::Integer:
			return realValue > a.value;
		case ImmediateType::Real:
			return realValue > a.realValue;
		}
	}
	return false;
}

bool ImmediateOperand::operator>=(const ImmediateOperand &a) const
{
	switch(immediateType)
	{
	case ImmediateType::Integer:
		switch(a.immediateType)
		{
		case ImmediateType::Integer:
			return value >= a.value;
		case ImmediateType::Real:
			return value >= a.realValue;
		}
	case ImmediateType::Real:
		switch(a.immediateType)
		{
		case ImmediateType::Integer:
			return realValue >= a.value;
		case ImmediateType::Real:
			return realValue >= a.realValue;
		}
	}
	return false;
}

bool ImmediateOperand::AsBool() const
{
	switch(immediateType)
	{
	case ImmediateType::Integer:
		return value != 0;
	case ImmediateType::Real:
		return realValue != 0;
	}
	return false;
}

//============================================================================
