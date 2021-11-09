//============================================================================

#include "Javelin/Tools/jasm/riscv/Instruction.h"

#include "Javelin/Assembler/BitUtility.h"
#include "Javelin/Assembler/riscv/ActionType.h"
#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/CommandLine.h"
#include "Javelin/Tools/jasm/Log.h"
#include "Javelin/Tools/jasm/riscv/Action.h"
#include "Javelin/Tools/jasm/riscv/Assembler.h"
#include "Javelin/Tools/jasm/riscv/Encoder.h"

#include <assert.h>

//============================================================================

using namespace Javelin::Assembler;
using namespace Javelin::Assembler::riscv;
using namespace Javelin::riscvAssembler;

//============================================================================

SyntaxOperand SyntaxOperand::Comma(Operand::Syntax::Comma, MatchComma);

//============================================================================

static const AlternateActionCondition* CreateAlwaysAlternateActionCondition(const Operand*)
{
    return &Common::AlwaysAlternateActionCondition::instance;
}

static const AlternateActionCondition* CreateUimm5AlternateActionCondition(const Operand* operand)
{
    return new UimmAlternateActionCondition(operand->expressionIndex, 5);
}

static const AlternateActionCondition* CreateSimm6AlternateActionCondition(const Operand* operand)
{
    return new SimmAlternateActionCondition(operand->expressionIndex, 6);
}

static const AlternateActionCondition* CreateSimm12AlternateActionCondition(const Operand* operand)
{
    return new SimmAlternateActionCondition(operand->expressionIndex, 12);
}

static const AlternateActionCondition* CreateUimm12AlternateActionCondition(const Operand* operand)
{
    return new UimmAlternateActionCondition(operand->expressionIndex, 12);
}

static const AlternateActionCondition* CreateSimm20AlternateActionCondition(const Operand* operand)
{
    return new SimmAlternateActionCondition(operand->expressionIndex, 20);
}

static const AlternateActionCondition* CreateUimm20AlternateActionCondition(const Operand* operand)
{
    return new UimmAlternateActionCondition(operand->expressionIndex, 20);
}

static const AlternateActionCondition* CreateCBDeltaAlternateActionCondition(const Operand* operand)
{
    return CBDeltaAlternateActionCondition::Create(operand);
}

static const AlternateActionCondition* CreateCJDeltaAlternateActionCondition(const Operand* operand)
{
    return CJDeltaAlternateActionCondition::Create(operand);
}

//============================================================================

std::string Javelin::Assembler::riscv::MatchBitfieldDescription(uint64_t matchBitfield)
{
	std::string result;
	const char *const TAG_NAMES[] =
	{
		#define TAG(x,y,z) y,
		#include "MatchTags.h"
		#undef TAG
	};
	bool first = true;
	for(int i = 0; i < sizeof(TAG_NAMES)/sizeof(TAG_NAMES[0]); ++i)
	{
		if(matchBitfield & (1ULL<<i))
		{
			const char *tagName = TAG_NAMES[i];
			if(!tagName[0]) continue;
			if(first)
			{
				first = false;
				result = tagName;
			}
			else
			{
				result = result + "|" + tagName;
			}
		}
	}
	return result;
}

std::string Javelin::Assembler::riscv::MatchBitfieldsDescription(const MatchBitfield *matchBitfields, int numberOfBitfields)
{
	std::string result;
	for(int i = 0; i < numberOfBitfields; ++i)
	{
		std::string description = MatchBitfieldDescription(matchBitfields[i]);
		if(description.length() == 0) continue;
		else if(result.size() > 0
				&& isalpha(result.back())
				&& (description.front() == '#' || isalpha(description.front())))
		{
			result.push_back(' ');
		}
		result.append(description);
	}
	return result;
}

void ImmediateOperand::UpdateMatchBitfield()
{
    matchBitfield = immediateType == ImmediateType::Integer ? MatchImm : MatchFloatImm;
    
    if(BitUtility::IsValidSignedImmediate(value, 20)) matchBitfield |= MatchImm20;
    if(BitUtility::IsValidUnsignedImmediate(value, 20)) matchBitfield |= MatchUimm20;
    if(BitUtility::IsValidSignedImmediate(value, 12)) matchBitfield |= MatchImm12;
    if(BitUtility::IsValidUnsignedImmediate(value, 12)) matchBitfield |= MatchUimm12;
    if(BitUtility::IsValidSignedImmediate(value, 6)) matchBitfield |= MatchImm6;
    if(BitUtility::IsValidUnsignedImmediate(value, 5)) matchBitfield |= MatchUimm5;
    if(0 <= value && value <= 127) matchBitfield |= MatchShiftImm;
    if(1 <= value && value <= 64) matchBitfield |= MatchCShiftImm;
}

//============================================================================

Action* LabelOperand::CreatePatchAction(RelEncoding relEncoding, int offset) const
{
	if(IsExpression())
	{
		return new PatchExpressionLabelAction(relEncoding, offset, *this);
	}
	else
	{
		switch(jumpType)
		{
		case JumpType::Name:
			return new PatchNameLabelAction(global,
											relEncoding,
											offset,
											labelName,
											*this);
			break;
		case JumpType::Forward:
		case JumpType::Backward:
		case JumpType::BackwardOrForward:
			return new PatchNumericLabelAction(global,
											   relEncoding,
											   offset,
											   *this);
			break;
		}
	}

}

//============================================================================

bool EncodingVariant::Match(int operandLength, const Operand* *const operands) const
{
	if(operandMatchMasksLength != operandLength) return false;
	for(int i = 0; i < operandLength; ++i)
	{
		if(!Match(i, *operands[i])) return false;
	}
	return true;
}

//============================================================================

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
    static const AlternateActionCondition* (*const kActionConditions[])(const Operand*) =
    {
        #define TAG(x,y,z) &Create##z##AlternateActionCondition,
        #include "MatchTags.h"
        #undef TAG
    };

    AlternateAction *alternateAction = nullptr;
	int numberOfExpressions = 0;
	int expressionIndex = -1;
	for(int i = 0; i < operandLength; ++i)
	{
		if(operands[i]->MayHaveMultipleRepresentations())
		{
			expressionIndex = i;
			++numberOfExpressions;
		}
	}
	
    for(int i = 0; i < encodingVariantLength; ++i)
    {
        const EncodingVariant *encoding = &encodingVariants[i];
        if(encodingVariantLength != 1
            && !assembler.UseCompressedInstructions()
            && (encoding->extensionBitmask & ExtensionBitmask::C)) continue;
        
        if(encoding->Match(operandLength, operands))
        {
            const Operand* encodingOperands[8] = {};
            for(int i = 0; i < operandLength; ++i)
            {
                int opMask = encoding->operandMatchMasks[i] & MatchOpAll;
                if(opMask == 0) continue;
                for(int opIndex = 0; opIndex < 8; ++opIndex)
                {
                    if(opMask & (1 << opIndex)) encodingOperands[opIndex] = operands[i];
                }
            }
            
            InstructionEncoder *encoder = encoding->encoder.GetFunction();
            if(numberOfExpressions == 0)
            {
                (*encoder)(assembler,
                           listAction,
                           *this,
                           *encoding,
                           encodingOperands);
                return;
            }
            
            ListAction* subList = new ListAction();
            
            (*encoder)(assembler,
                       *subList,
                       *this,
                       *encoding,
                       encodingOperands);
            
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
                    int operandExpressionSize = 32;
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
                            int operandExpressionSize = 32;
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
    
	assert(operandLength <= 16);
	MatchBitfield operandMatchBitfields[16];
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
