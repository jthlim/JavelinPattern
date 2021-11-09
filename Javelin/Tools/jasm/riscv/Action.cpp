//============================================================================

#include "Javelin/Tools/jasm/riscv/Action.h"

#include "Javelin/Assembler/BitUtility.h"
#include "Javelin/Assembler/JitLabelId.h"
#include "Javelin/Assembler/riscv/ActionType.h"
#include "Javelin/Tools/jasm/riscv/Assembler.h"
#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/CommandLine.h"
#include "Javelin/Tools/jasm/Log.h"

#include <assert.h>
#include <inttypes.h>
#include <memory.h>

//============================================================================

using namespace Javelin::riscvAssembler;
using namespace Javelin::Assembler::riscv;

//============================================================================

void ZeroAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
    riscvAssembler::ActionType actionType;
	switch(context.expressionInfo[expressionIndex-1].bitWidth)
	{
	case 8:  actionType = riscvAssembler::ActionType::Imm0B1Condition; break;
	case 16: actionType = riscvAssembler::ActionType::Imm0B2Condition; break;
	case 32: actionType = riscvAssembler::ActionType::Imm0B4Condition; break;
	case 64: actionType = riscvAssembler::ActionType::Imm0B8Condition; break;
	default: return;
	}
	
	ImmediateAlternateActionCondition::WriteByteCode(result, context, action, (int)actionType);
}

std::string SimmAlternateActionCondition::GetDescription() const
{
	char buffer[32];
	sprintf(buffer, "simm%d{%d}", bitCount, expressionIndex);
	return buffer;
}

bool SimmAlternateActionCondition::Equals(const AlternateActionCondition *other) const
{
    const SimmAlternateActionCondition* typedOther = dynamic_cast<const SimmAlternateActionCondition*>(other);
    return typedOther != nullptr
      && bitCount == typedOther->bitCount
      && Common::ImmediateAlternateActionCondition::Equals(other);
}

void SimmAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	ImmediateAlternateActionCondition::WriteByteCode(result, context, action, (int) riscvAssembler::ActionType::SimmCondition, bitCount-1);
}

std::string UimmAlternateActionCondition::GetDescription() const
{
    char buffer[32];
    sprintf(buffer, "uimm%d{%d}", bitCount, expressionIndex);
    return buffer;
}

void UimmAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
    ImmediateAlternateActionCondition::WriteByteCode(result, context, action, (int) riscvAssembler::ActionType::UimmCondition, bitCount);
}

bool UimmAlternateActionCondition::Equals(const AlternateActionCondition *other) const
{
    const UimmAlternateActionCondition* typedOther = dynamic_cast<const UimmAlternateActionCondition*>(other);
    return typedOther != nullptr
      && bitCount == typedOther->bitCount
      && Common::ImmediateAlternateActionCondition::Equals(other);
}

//============================================================================

DeltaAlternateActionCondition::DeltaAlternateActionCondition(int aBitWidth, const LabelOperand &aLabelOperand)
: bitWidth(aBitWidth), labelOperand(aLabelOperand)
{
}

std::string DeltaAlternateActionCondition::GetDescription() const
{
    char buffer[24];
    if(labelOperand.IsExpression())
    {
        sprintf(buffer, "Delta%d{*{%d}%s}", bitWidth, labelOperand.expressionIndex, labelOperand.jumpType.ToString());
        return buffer;
    }
    switch(labelOperand.jumpType)
    {
    case JumpType::Name:
        sprintf(buffer, "Delta%d{", bitWidth);
        return buffer + labelOperand.labelName + "}";
    case JumpType::Forward:
    case JumpType::Backward:
    case JumpType::BackwardOrForward:
        sprintf(buffer, "Delta%d{%" PRId64 "%s}", bitWidth, labelOperand.labelValue, labelOperand.jumpType.ToString());
        return buffer;
    }
}

bool DeltaAlternateActionCondition::Equals(const AlternateActionCondition *other) const
{
    if(typeid(*this) != typeid(*other)) return false;
    DeltaAlternateActionCondition *o = (DeltaAlternateActionCondition*) other;
    return bitWidth == o->bitWidth
            && labelOperand.jumpType == o->labelOperand.jumpType
            && labelOperand.labelName == o->labelOperand.labelName
            && labelOperand.labelValue == o->labelOperand.labelValue;
}

AlternateActionCondition::Result DeltaAlternateActionCondition::IsValid(ActionContext &context, const Action *action) const
{
    switch(labelOperand.jumpType)
    {
    case JumpType::Name:
        {
            ActionContext::NamedLabelMap::const_iterator it = context.namedLabels.find(labelOperand.labelName);
            if(it == context.namedLabels.end()) return Result::Maybe;
            return IsValid(context.offset, it->second);
        }
    case JumpType::Forward:
    case JumpType::Backward:
    case JumpType::BackwardOrForward:
        return IsValidForJumpType(context);
    }
}

AlternateActionCondition::Result DeltaAlternateActionCondition::IsValidForJumpType(ActionContext &context) const
{
    if(context.forwards)
    {
        if(labelOperand.jumpType == JumpType::Forward) return Result::Maybe;
    }
    else
    {
        if(labelOperand.jumpType == JumpType::Backward) return Result::Maybe;
    }

    if(labelOperand.IsExpression())
    {
        ActionContext::ExpressionLabelMap::const_iterator it = context.expressionLabels.find(labelOperand.expressionIndex);
        if(it == context.expressionLabels.end()) return Result::Maybe;
        return IsValid(context.offset, it->second);
    }
    else
    {
        ActionContext::NumericLabelMap::const_iterator it = context.numericLabels.find(labelOperand.global ? labelOperand.labelValue ^ -1 : labelOperand.labelValue);
        if(it == context.numericLabels.end()) return Result::Maybe;
        return IsValid(context.offset, it->second);
    }
}

AlternateActionCondition::Result DeltaAlternateActionCondition::IsValid(const ActionOffset &anchorOffset, const ActionOffset &destinationOffset) const
{
    ssize_t maximumOffset = destinationOffset.totalMaximumOffset - anchorOffset.totalMaximumOffset;
    if (BitUtility::IsValidSignedImmediate(maximumOffset, bitWidth)) return Result::Always;

    ssize_t minimumOffset = destinationOffset.totalMinimumOffset - anchorOffset.totalMinimumOffset;
    if (BitUtility::IsValidSignedImmediate(minimumOffset, bitWidth)) return Result::Maybe;

    return Result::Never;
}

void DeltaAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
    if(!action.GetLastAction() || dynamic_cast<const PatchLabelAction*>(action.GetLastAction()) == nullptr)
    {
        throw AssemblerException("Internal error: DeltaAlternateAction unable to find PatchLabelAction");
    }
    
    const LabelOperand& labelOperand = dynamic_cast<const PatchLabelAction*>(action.GetLastAction())->labelOperand;
    
    // Prefix with condition code
    std::vector<uint8_t> conditionCode;
    if(labelOperand.IsExpression())
    {
        switch(labelOperand.jumpType)
        {
        case JumpType::Forward:
            conditionCode.push_back((int) riscvAssembler::ActionType::DeltaExpressionLabelForwardCondition);
            Action::WriteExpressionOffset(conditionCode, context, labelOperand.expressionIndex);
            conditionCode.push_back(context.GetForwardLabelReferenceIndex(labelOperand.reference, 1));
            break;
        case JumpType::Backward:
            conditionCode.push_back((int) riscvAssembler::ActionType::DeltaExpressionLabelBackwardCondition);
            Action::WriteExpressionOffset(conditionCode, context, labelOperand.expressionIndex);
            assert(action.GetMaximumLength() == (uint8_t)action.GetMaximumLength());
            break;
        case JumpType::BackwardOrForward:
            conditionCode.push_back((int) riscvAssembler::ActionType::DeltaExpressionLabelCondition);
            Action::WriteExpressionOffset(conditionCode, context, labelOperand.expressionIndex);
            conditionCode.push_back(context.GetForwardLabelReferenceIndex(labelOperand.reference, 1));
            assert(action.GetMaximumLength() == (uint8_t)action.GetMaximumLength());
            break;
        default:
            assert(!"Internal Error");
        }
    }
    else
    {
        switch(labelOperand.jumpType)
        {
        case JumpType::Name:
            if(labelOperand.global)
            {
                conditionCode.push_back((int) riscvAssembler::ActionType::DeltaLabelCondition);
                Action::WriteUnsigned32(conditionCode, GetLabelIdForNamed(labelOperand.labelName.c_str()));
                conditionCode.push_back(context.GetForwardLabelReferenceIndex(labelOperand.reference, 1));
                assert(action.GetMaximumLength() == (uint8_t)action.GetMaximumLength());
            }
            else
            {
                uint32_t labelId = GetLabelIdForNamed(labelOperand.labelName.c_str());
                bool forwards = context.labelIdToIndexMap.find(labelId) != context.labelIdToIndexMap.end();
                uint32_t index = context.GetIndexForLabelId(labelId);
                if(forwards)
                {
                    conditionCode.push_back((int) riscvAssembler::ActionType::DeltaLabelForwardCondition);
                    Action::WriteUnsigned32(conditionCode, GetLabelIdForIndexed(index));
                    conditionCode.push_back(context.GetForwardLabelReferenceIndex(labelOperand.reference, 1));
                }
                else
                {
                    conditionCode.push_back((int) riscvAssembler::ActionType::DeltaLabelBackwardCondition);
                    Action::WriteUnsigned32(conditionCode, GetLabelIdForIndexed(index));
                    assert(action.GetMaximumLength() == (uint8_t)action.GetMaximumLength());
                }
            }
            break;
        case JumpType::Forward:
            if(labelOperand.global)
            {
                conditionCode.push_back((int) riscvAssembler::ActionType::DeltaLabelForwardCondition);
                Action::WriteUnsigned32(conditionCode, GetLabelIdForGlobalNumeric(labelOperand.labelValue));
                conditionCode.push_back(context.GetForwardLabelReferenceIndex(labelOperand.reference, 1));
            }
            else
            {
                conditionCode.push_back((int) riscvAssembler::ActionType::DeltaLabelForwardCondition);
                int64_t labelId = GetLabelIdForNumeric(labelOperand.labelValue);
                int index = context.GetIndexForLabelId(labelId);
                Action::WriteUnsigned32(conditionCode, GetLabelIdForIndexed(index));
                conditionCode.push_back(context.GetForwardLabelReferenceIndex(labelOperand.reference, 1));
            }
            break;
        case JumpType::Backward:
            if(labelOperand.global)
            {
                conditionCode.push_back((int) riscvAssembler::ActionType::DeltaLabelBackwardCondition);
                Action::WriteUnsigned32(conditionCode, GetLabelIdForGlobalNumeric(labelOperand.labelValue));
                assert(action.GetMaximumLength() == (uint8_t)action.GetMaximumLength());
            }
            else
            {
                conditionCode.push_back((int) riscvAssembler::ActionType::DeltaLabelBackwardCondition);
                int64_t labelId = GetLabelIdForNumeric(labelOperand.labelValue);
                int index = context.GetIndexForLabelId(labelId);
                Action::WriteUnsigned32(conditionCode, GetLabelIdForIndexed(index));
                assert(action.GetMaximumLength() == (uint8_t)action.GetMaximumLength());
            }
            break;
        case JumpType::BackwardOrForward:
            if(labelOperand.global)
            {
                conditionCode.push_back((int) riscvAssembler::ActionType::DeltaLabelCondition);
                Action::WriteUnsigned32(conditionCode, GetLabelIdForGlobalNumeric(labelOperand.labelValue));
                conditionCode.push_back(context.GetForwardLabelReferenceIndex(labelOperand.reference, 1));
                assert(action.GetMaximumLength() == (uint8_t)action.GetMaximumLength());
            }
            else
            {
                throw AssemblerException("Unresolved label %" PRId64, labelOperand.labelValue);
            }
            break;
        }
    }
    assert(result.size() == (uint8_t) result.size());
    conditionCode.push_back(bitWidth-1);
    conditionCode.push_back(result.size());
    result.insert(result.begin(), conditionCode.begin(), conditionCode.end());
}

//============================================================================

const AlternateActionCondition* CBDeltaAlternateActionCondition::Create(const Operand *operand)
{
    assert(operand->type == Operand::Type::Label);
    return new CBDeltaAlternateActionCondition(*(const LabelOperand*)operand);
}

std::string CBDeltaAlternateActionCondition::GetDescription() const
{
    char buffer[24];
    if(labelOperand.IsExpression())
    {
        sprintf(buffer, "CBDelta{*{%d}%s}", labelOperand.expressionIndex, labelOperand.jumpType.ToString());
        return buffer;
    }
    switch(labelOperand.jumpType)
    {
    case JumpType::Name:
        return "CBDelta{" + labelOperand.labelName + "}";
    case JumpType::Forward:
    case JumpType::Backward:
    case JumpType::BackwardOrForward:
        sprintf(buffer, "CBDelta{%" PRId64 "%s}", labelOperand.labelValue, labelOperand.jumpType.ToString());
        return buffer;
    }
}

const AlternateActionCondition* CJDeltaAlternateActionCondition::Create(const Operand *operand)
{
    assert(operand->type == Operand::Type::Label);
    return new CJDeltaAlternateActionCondition(*(const LabelOperand*)operand);
}

std::string CJDeltaAlternateActionCondition::GetDescription() const
{
    char buffer[24];
    if(labelOperand.IsExpression())
    {
        sprintf(buffer, "CJDelta{*{%d}%s}", labelOperand.expressionIndex, labelOperand.jumpType.ToString());
        return buffer;
    }
    switch(labelOperand.jumpType)
    {
    case JumpType::Name:
        return "CJDelta{" + labelOperand.labelName + "}";
    case JumpType::Forward:
    case JumpType::Backward:
    case JumpType::BackwardOrForward:
        sprintf(buffer, "CJDelta{%" PRId64 "%s}", labelOperand.labelValue, labelOperand.jumpType.ToString());
        return buffer;
    }
}

//============================================================================

void LiteralAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	if(bytes.size() == 0) return;
	
    if(bytes.size() == 1)
    {
        result.push_back((int) riscvAssembler::ActionType::Literal1);
    }
	else if(bytes.size() <= 32 && bytes.size() % 2 == 0)
	{
		static_assert((int) riscvAssembler::ActionType::Literal2 == 2, "Unexpected literal value");
		static_assert((int) riscvAssembler::ActionType::Literal4 == 3, "Unexpected literal value");
		static_assert((int) riscvAssembler::ActionType::Literal6 == 4, "Unexpected literal value");
		static_assert((int) riscvAssembler::ActionType::Literal8 == 5, "Unexpected literal value");
		static_assert((int) riscvAssembler::ActionType::Literal10 == 6, "Unexpected literal value");
		static_assert((int) riscvAssembler::ActionType::Literal12 == 7, "Unexpected literal value");
		static_assert((int) riscvAssembler::ActionType::Literal14 == 8, "Unexpected literal value");
		static_assert((int) riscvAssembler::ActionType::Literal16 == 9, "Unexpected literal value");
        static_assert((int) riscvAssembler::ActionType::Literal18 == 10, "Unexpected literal value");
        static_assert((int) riscvAssembler::ActionType::Literal20 == 11, "Unexpected literal value");
        static_assert((int) riscvAssembler::ActionType::Literal22 == 12, "Unexpected literal value");
        static_assert((int) riscvAssembler::ActionType::Literal24 == 13, "Unexpected literal value");
        static_assert((int) riscvAssembler::ActionType::Literal26 == 14, "Unexpected literal value");
        static_assert((int) riscvAssembler::ActionType::Literal28 == 15, "Unexpected literal value");
        static_assert((int) riscvAssembler::ActionType::Literal30 == 16, "Unexpected literal value");
        static_assert((int) riscvAssembler::ActionType::Literal32 == 17, "Unexpected literal value");
		result.push_back((int) bytes.size() / 2 + 1);
	}
	else
	{
		result.push_back((int) riscvAssembler::ActionType::LiteralBlock);
		if((bytes.size() >> 16) != 0) throw AssemblerException("Literal too long");
		result.push_back(bytes.size());
		result.push_back(bytes.size() >> 8);
	}
	result.insert(result.end(), bytes.begin(), bytes.end());
}

//============================================================================

bool AlignAction::Simplify(Common::ListAction *parent, size_t index)
{
	if(isFixed)
	{
		if(fixedLength == 0)
		{
			if(parent) parent->RemoveActionAtIndex(index);
			return true;
		}
	
        std::vector<uint8_t> bytes;
        if(fixedLength & 1)
        {
            bytes.push_back(0);
            --fixedLength;
        }

        if(fixedLength & 2)
        {
            // two byte nop
            bytes.push_back(0x01);
            bytes.push_back(0x00);
            --fixedLength;
        }

		while(fixedLength)
		{
            const uint32_t nopOpcode = 0x00000013;
			const uint8_t *nop = reinterpret_cast<const uint8_t*>(&nopOpcode);
			bytes.insert(bytes.end(), nop, nop+4);
            fixedLength -= 4;
		}
		
		parent->ReplaceActionAtIndex(index, new LiteralAction(bytes));
		return true;
	}
	return false;
}

void AlignAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
    if (isFixed && fixedLength == 0) return;
    
	result.push_back((int) riscvAssembler::ActionType::Align);
	int data = alignment - 1;
	assert(data == (uint8_t) data);
	result.push_back(data);
}

//============================================================================

bool UnalignAction::Simplify(Common::ListAction *parent, size_t index)
{
	if(isFixed)
	{
		if(fixedLength == 0)
		{
			parent->RemoveActionAtIndex(index);
		}
		else
		{
            const uint32_t nopOpcode = 0x00000013;
            const uint8_t *nop = reinterpret_cast<const uint8_t*>(&nopOpcode);
            std::vector<uint8_t> bytes;
            bytes.insert(bytes.end(), nop, nop+4);
			parent->ReplaceActionAtIndex(index, new LiteralAction(bytes));
		}
		return true;
	}
	return false;
}

void UnalignAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	result.push_back((int) riscvAssembler::ActionType::Unalign);
	int data = alignment - 1;
	assert(data == (uint8_t) data);
	result.push_back(data);
}

//============================================================================

void NamedLabelAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	uint32_t labelId = GetLabelIdForNamed(value.c_str());
	result.push_back((int) riscvAssembler::ActionType::Label);
	if(global)
	{
		WriteUnsigned32(result, labelId);
	}
	else
	{
		int index = context.GetIndexForLabelId(labelId);
		WriteUnsigned32(result, GetLabelIdForIndexed(index));
	}
	context.numberOfLabels++;
}


void NumericLabelAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	result.push_back((int) riscvAssembler::ActionType::Label);
	if(global)
	{
		WriteUnsigned32(result, GetLabelIdForGlobalNumeric(value));
	}
	else
	{
		int64_t labelId = GetLabelIdForNumeric(value);
		int index = context.GetIndexForLabelId(labelId);
		WriteUnsigned32(result, GetLabelIdForIndexed(index));
	}
	context.numberOfLabels++;
}

void ExpressionLabelAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	result.push_back((int) riscvAssembler::ActionType::ExpressionLabel);
	WriteExpressionOffset(result, context, expressionIndex);
	context.numberOfLabels++;
}

//============================================================================

bool PatchLabelAction::Simplify(Common::ListAction *parent, size_t index)
{
	if(!hasFixedDelta) return false;

	assert(index > 0);
	
	size_t literalActionIndex = index - 1;
	Action *previousAction;
	int offset = delay;
	for(;;)
	{
		previousAction = parent->GetActionAtIndex(literalActionIndex);
		if(previousAction->IsLiteralAction())
		{
			break;
		}
		else
		{
			assert(literalActionIndex > 0);
			if(previousAction->GetMinimumLength() != previousAction->GetMaximumLength()
			   || previousAction->GetMinimumLength() > 0) return false;
			offset -= previousAction->GetMinimumLength();
			--literalActionIndex;
		}
	}

	assert(previousAction->IsLiteralAction()
		   && previousAction->GetMaximumLength() >= offset);
	
	LiteralAction *literal = (LiteralAction*) previousAction;
    std::vector<uint8_t>& literalBytes = literal->GetLiteralData();
	
	switch(relEncoding)
	{
    case RelEncoding::BDelta:
        {
            union Opcode
            {
                uint32_t v;
                struct
                {
                    uint32_t opcode : 7;
                    uint32_t imm11 : 1;
                    uint32_t imm4_1 : 4;
                    uint32_t funct3 : 3;
                    uint32_t rs1 : 5;
                    uint32_t rs2 : 5;
                    uint32_t imm10_5 : 6;
                    int32_t imm12 : 1;
                };
            };
            static_assert(sizeof(Opcode) == 4, "Opcode should be 4 bytes");
            Opcode opcode;
            memcpy(&opcode, literalBytes.data() + literalBytes.size() - 4, 4);
            
            int32_t rel = opcode.imm4_1 << 1
                          | opcode.imm10_5 << 5
                          | opcode.imm11 << 11
                          | opcode.imm12 << 12;

            rel += delta;
            assert((rel & 1) == 0);
            assert(BitUtility::IsValidSignedImmediate(rel, 13));
            opcode.imm4_1 = rel >> 1;
            opcode.imm10_5 = rel >> 5;
            opcode.imm11 = rel >> 11;
            opcode.imm12 = rel >> 12;
            
            memcpy(literalBytes.data() + literalBytes.size() - 4, &opcode, 4);
        }
        break;
            
    case RelEncoding::JDelta:
        {
            union Opcode
            {
                uint32_t v;
                struct
                {
                    uint32_t opcode : 7;
                    uint32_t rd : 5;
                    uint32_t imm19_12 : 8;
                    uint32_t imm11 : 1;
                    uint32_t imm10_1 : 10;
                    int32_t imm20 : 1;
                };
            };
            static_assert(sizeof(Opcode) == 4, "Opcode should be 4 bytes");
            Opcode opcode;
            memcpy(&opcode, literalBytes.data() + literalBytes.size() - 4, 4);
            
            int32_t rel = opcode.imm10_1 << 1
                          | opcode.imm11 << 11
                          | opcode.imm19_12 << 12
                          | opcode.imm20 << 20;

            rel += delta;
            assert((rel & 1) == 0);
            assert(BitUtility::IsValidSignedImmediate(rel, 21));
            opcode.imm10_1 = rel >> 1;
            opcode.imm11 = rel >> 11;
            opcode.imm19_12 = rel >> 12;
            opcode.imm20 = rel >> 20;
            
            memcpy(literalBytes.data() + literalBytes.size() - 4, &opcode, 4);
        }
        break;
            
    case RelEncoding::Auipc:
        {
            assert(!"Internal error: Auipc RelEncoding not implemented yet");
        }
        break;
            
    case RelEncoding::Imm12:
        assert(!"Internal error: Imm12 not implemented yet");
        break;
            
    case RelEncoding::CBDelta:
        {
            union Opcode
            {
                uint16_t v;
                struct
                {
                    uint16_t opcode : 2;
                    uint16_t imm5 : 1;
                    uint16_t imm2_1 : 2;
                    uint16_t imm7_6 : 2;
                    uint16_t rs1 : 3;
                    uint16_t imm4_3 : 2;
                    int16_t imm8 : 1;
                    uint16_t funct3 : 3;
                };
            };
            static_assert(sizeof(Opcode) == 2, "Opcode should be 2 bytes");
            Opcode opcode;
            memcpy(&opcode, literalBytes.data() + literalBytes.size() - 2, 2);
            
            int32_t rel = opcode.imm2_1 << 1
                            | opcode.imm4_3 << 3
                            | opcode.imm5 << 5
                            | opcode.imm7_6 << 6
                            | opcode.imm8 << 8;

            rel += delta;
            
            assert((rel & 1) == 0);
            assert(BitUtility::IsValidSignedImmediate(rel, 9));
            
            opcode.imm2_1 = rel >> 1;
            opcode.imm4_3 = rel >> 3;
            opcode.imm5 = rel >> 5;
            opcode.imm7_6 = rel >> 6;
            opcode.imm8 = rel >> 8;

            memcpy(literalBytes.data() + literalBytes.size() - 2, &opcode, 2);
        }
        break;

    case RelEncoding::CJDelta:
        {
            union Opcode
            {
                uint16_t v;
                struct
                {
                    uint16_t opcode : 2;
                    uint16_t imm5 : 1;
                    uint16_t imm3_1 : 3;
                    uint16_t imm7 : 1;
                    uint16_t imm6 : 1;
                    uint16_t imm10 : 1;
                    uint16_t imm9_8 : 2;
                    uint16_t imm4 : 1;
                    int16_t imm11 : 1;
                    uint16_t funct3 : 3;
                };
            };
            static_assert(sizeof(Opcode) == 2, "Opcode should be 2 bytes");
            Opcode opcode;
            memcpy(&opcode, literalBytes.data() + literalBytes.size() - 2, 2);
            
            int32_t rel = opcode.imm3_1 << 1
                            | opcode.imm4 << 4
                            | opcode.imm5 << 5
                            | opcode.imm6 << 6
                            | opcode.imm7 << 7
                            | opcode.imm9_8 << 8
                            | opcode.imm10 << 10
                            | opcode.imm11 << 11;

            rel += delta;
            
            assert((rel & 1) == 0);
            assert(BitUtility::IsValidSignedImmediate(rel, 12));

            opcode.imm3_1 = rel >> 1;
            opcode.imm4 = rel >> 4;
            opcode.imm5 = rel >> 5;
            opcode.imm6 = rel >> 6;
            opcode.imm7 = rel >> 7;
            opcode.imm9_8 = rel >> 8;
            opcode.imm10 = rel >> 10;
            opcode.imm11 = rel >> 11;

            memcpy(literalBytes.data() + literalBytes.size() - 2, &opcode, 2);
        }
        break;

    case RelEncoding::Rel64:
        {
            uint64_t v;
            memcpy(&v, literalBytes.data() + literalBytes.size() - 8, 8);
            v += delta;
            memcpy(literalBytes.data() + literalBytes.size() - 8, &v, 8);
        }
        break;
	}
	parent->RemoveActionAtIndex(index);
	delete this;
	return true;
}

void PatchNameLabelAction::Dump() const
{
	printf(", patch@%d(renc%d, %s%s%s)",
           -delay,
		   (int) relEncoding,
		   global ? "*" : "",
		   value.c_str(),
           jumpType.ToString());
	if(hasFixedDelta) printf("=%" PRId64, delta);
}

bool PatchNameLabelAction::ResolveRelativeAddresses(ActionContext &context)
{
	PatchLabelAction::ResolveRelativeAddresses(context);

	context.namedReferenceSet.insert(value);

	// Already resolved!
	if(hasFixedDelta) return false;
	
	ActionContext::NamedLabelMap::const_iterator it = context.namedLabels.find(value);
	if(it == context.namedLabels.end()) return false;
	hasFoundTarget = true;

	jumpType = context.forwards ? JumpType::Backward : JumpType::Forward;

	const ActionOffset anchorOffset = context.offset;
	const ActionOffset targetOffset = it->second;
	
	ssize_t maximumDelta = targetOffset.totalMaximumOffset - anchorOffset.totalMaximumOffset;
	ssize_t minimumDelta = targetOffset.totalMinimumOffset - anchorOffset.totalMinimumOffset;
	if(maximumDelta != minimumDelta) return false;
	
	hasFixedDelta = true;
	delta = minimumDelta + delay;
	return true;
}

void PatchNameLabelAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	if(!global && !hasFoundTarget)
	{
		throw AssemblerException("Local named label %s unresolved\n", value.c_str());
	}
	
	uint32_t labelId = GetLabelIdForNamed(labelOperand.labelName.c_str());
	if(global)
	{
        context.AddForwardLabelReference(labelOperand.reference);

		result.push_back((int) riscvAssembler::ActionType::PatchLabel);
		WriteUnsigned32(result, labelId);
	}
	else
	{
		uint32_t index = context.GetIndexForLabelId(labelId);
		switch(jumpType)
		{
		case JumpType::Name:
		case JumpType::BackwardOrForward:
			throw AssemblerException("Internal error: Unexpected jumpType");
				
		case JumpType::Forward:
            context.AddForwardLabelReference(labelOperand.reference);
			result.push_back((int) riscvAssembler::ActionType::PatchLabelForward);
			break;
				
		case JumpType::Backward:
			result.push_back((int) riscvAssembler::ActionType::PatchLabelBackward);
			break;
		}
		WriteUnsigned32(result, GetLabelIdForIndexed(index));
	}
	result.push_back((int) relEncoding);
	WriteSigned16(result, -delay);
}

void PatchNumericLabelAction::Dump() const
{
	printf(", patch@%d(renc%d, %s%" PRId64 "%s)", -delay, (int) relEncoding, global ? "*" : "", value, jumpType.ToString());
	if(hasFixedDelta) printf("=%" PRId64, delta);
}

bool PatchNumericLabelAction::ResolveRelativeAddresses(ActionContext &context)
{
	Inherited::ResolveRelativeAddresses(context);
	
	if(!global)
	{
		if(context.forwards)
		{
			if(jumpType == JumpType::Forward)
			{
				context.numericReferenceSet.insert(value);
				return false;
			}
		}
		else
		{
			if(jumpType == JumpType::Backward)
			{
				context.numericReferenceSet.insert(value);
				return false;
			}
		}
	}
	
	// Already resolved!
	if(hasFixedDelta) return false;
	
	if(context.forwards)
	{
		if(jumpType == JumpType::Forward) return false;
	}
	else
	{
		if(jumpType == JumpType::Backward) return false;
	}
	
	ActionContext::NumericLabelMap::const_iterator it = context.numericLabels.find(global ? value ^ -1 : value);
	if(it == context.numericLabels.end()) return false;
	hasFoundTarget = true;

	jumpType = context.forwards ? JumpType::Backward : JumpType::Forward;

	const ActionOffset anchorOffset = context.offset;
	const ActionOffset targetOffset = it->second;
	
	ssize_t maximumDelta = targetOffset.totalMaximumOffset - anchorOffset.totalMaximumOffset;
	ssize_t minimumDelta = targetOffset.totalMinimumOffset - anchorOffset.totalMinimumOffset;
	if(maximumDelta != minimumDelta) return false;
	
	hasFixedDelta = true;
	delta = minimumDelta + delay;
	return true;
}

void PatchNumericLabelAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	if(!global && !hasFoundTarget)
	{
		throw AssemblerException("Local numeric label %" PRId64 " unresolved\n", value);
	}
	if(global)
	{
        uint32_t labelId = GetLabelIdForGlobalNumeric(labelOperand.labelValue);
		switch(jumpType)
		{
		case JumpType::Name:
			throw AssemblerException("Internal error: Unexpected jumpType");
		case JumpType::Forward:
			// Forwards
            context.AddForwardLabelReference(labelOperand.reference);
			result.push_back((int) riscvAssembler::ActionType::PatchLabelForward);
			break;
		case JumpType::Backward:
			result.push_back((int) riscvAssembler::ActionType::PatchLabelBackward);
			break;
		case JumpType::BackwardOrForward:
            context.AddForwardLabelReference(labelOperand.reference);
			result.push_back((int) riscvAssembler::ActionType::PatchLabel);
			break;
		}
		WriteUnsigned32(result, labelId);
	}
	else
	{
		int64_t labelId = GetLabelIdForNumeric(labelOperand.labelValue);
		int index = context.GetIndexForLabelId(labelId);
		switch(jumpType)
		{
		case JumpType::Name:
			throw AssemblerException("Internal error: Unexpected jumpType");
		case JumpType::Forward:
            context.AddForwardLabelReference(labelOperand.reference);
			result.push_back((int) riscvAssembler::ActionType::PatchLabelForward);
			break;
		case JumpType::Backward:
			result.push_back((int) riscvAssembler::ActionType::PatchLabelBackward);
			break;
		case JumpType::BackwardOrForward:
			throw AssemblerException("Label %" PRId64 " not resolved", labelOperand.labelValue);
		}
		WriteUnsigned32(result, GetLabelIdForIndexed(index));
	}
	result.push_back((int) relEncoding);
	WriteSigned16(result, -delay);
}

PatchExpressionLabelAction::PatchExpressionLabelAction(RelEncoding relEncoding, int offset, const LabelOperand &labelOperand)
: PatchLabelAction(true, relEncoding, offset, labelOperand)
{
	
}

void PatchExpressionLabelAction::Dump() const
{
	printf(", patch(renc%d, *{%d}%s)",
		   (int) relEncoding,
		   labelOperand.expressionIndex,
           labelOperand.jumpType.ToString());
}

bool PatchExpressionLabelAction::ResolveRelativeAddresses(ActionContext &context)
{
	Inherited::ResolveRelativeAddresses(context);
	
	// Already resolved!
	if(hasFixedDelta) return false;
	
	if(context.forwards)
	{
		if(labelOperand.jumpType == JumpType::Forward) return false;
	}
	else
	{
		if(labelOperand.jumpType == JumpType::Backward) return false;
	}
	
	ActionContext::ExpressionLabelMap::const_iterator it = context.expressionLabels.find(labelOperand.expressionIndex);
	if(it == context.expressionLabels.end()) return false;
	hasFoundTarget = true;

	labelOperand.jumpType = context.forwards ? JumpType::Backward : JumpType::Forward;

	const ActionOffset anchorOffset = context.offset;
	const ActionOffset targetOffset = it->second;
	
	ssize_t maximumDelta = targetOffset.totalMaximumOffset - anchorOffset.totalMaximumOffset;
	ssize_t minimumDelta = targetOffset.totalMinimumOffset - anchorOffset.totalMinimumOffset;
	if(maximumDelta != minimumDelta) return false;
	
	hasFixedDelta = true;
	delta = minimumDelta + delay;
	return true;
}

void PatchExpressionLabelAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	switch(labelOperand.jumpType)
	{
	case JumpType::Name:
		throw AssemblerException("Internal error: Unexpected jumpType");
	case JumpType::Forward:
        context.AddForwardLabelReference(labelOperand.reference);
		result.push_back((int) riscvAssembler::ActionType::PatchExpressionLabelForward);
		break;
	case JumpType::Backward:
		result.push_back((int) riscvAssembler::ActionType::PatchExpressionLabelBackward);
		break;
	case JumpType::BackwardOrForward:
        context.AddForwardLabelReference(labelOperand.reference);
		result.push_back((int) riscvAssembler::ActionType::PatchExpressionLabel);
		break;
	}
	WriteExpressionOffset(result, context, labelOperand.expressionIndex);
	result.push_back((int) relEncoding);
	WriteSigned16(result, -delay);
}

//============================================================================

void PatchAbsoluteAddressAction::Dump() const
{
	printf(", @-%d+=ptr", delay);
}

void PatchAbsoluteAddressAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	result.push_back((int) riscvAssembler::ActionType::PatchAbsoluteAddress);
	WriteSigned16(result, -delay);
}

//============================================================================

PatchExpressionAction::PatchExpressionAction(RelEncoding aRelEncoding, int offset, int aExpressionIndex, Assembler& assembler)
: DelayableAction(offset), relEncoding(aRelEncoding), expressionIndex(aExpressionIndex)
{
	assembler.UpdateExpressionBitWidth(expressionIndex, 64);
}

void PatchExpressionAction::Dump() const
{
	printf(", patch(renc%d, {%d})", (int) relEncoding, expressionIndex);
}

void PatchExpressionAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	result.push_back((int) riscvAssembler::ActionType::PatchExpression);
	WriteExpressionOffset(result, context, expressionIndex);
	result.push_back((int) relEncoding);
	WriteSigned16(result, -delay);
}

//============================================================================

PatchOpcodeAction::PatchOpcodeAction(Format aFormat,
                                     uint8_t opcodeSize,
									 uint8_t aNumberOfBits,
									 uint8_t aBitOffset,
									 uint8_t aValueShift,
									 int aExpressionIndex,
									 Assembler &assembler)
: DelayableAction(opcodeSize),
  format(aFormat),
  numberOfBits(aNumberOfBits),
  bitOffset(aBitOffset),
  valueShift(aValueShift),
  expressionIndex(aExpressionIndex)
{
	int totalBits = numberOfBits + valueShift;
	
	int expressionWidth = totalBits <= 8 ? 8 :
						  totalBits <= 16 ? 16 :
						  totalBits <= 32 ? 32 :
						  64;
	
	assembler.UpdateExpressionBitWidth(expressionIndex, expressionWidth);
}

void PatchOpcodeAction::Dump() const
{
    static const char *const FORMATS[] = { "simm", "uimm", "mimm", "civalue" };

    switch(format)
    {
    case Format::Signed:
    case Format::Unsigned:
    case Format::Masked:
        printf(", %s%d@-%d=({%d}>>%d)<<%d", FORMATS[(int) format], numberOfBits, delay, expressionIndex, valueShift, bitOffset);
        break;

    case Format::CIValue: 
        printf(", %s@-%d={%d}", FORMATS[(int) format], delay, expressionIndex);
        break;
    }
}

bool PatchOpcodeAction::CanGroup(Action *other) const
{
	const PatchOpcodeAction *otherPatch = dynamic_cast<const PatchOpcodeAction*>(other);
	if(!otherPatch) return false;
	
	return expressionIndex == otherPatch->expressionIndex
           && format == otherPatch->format
	       && numberOfBits == otherPatch->numberOfBits
		   && bitOffset == otherPatch->bitOffset
		   && valueShift == otherPatch->valueShift;
}

void PatchOpcodeAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	if(CanGroup(context.previousAction))
	{
        switch(format)
        {
        case Format::Signed:
        case Format::Unsigned:
        case Format::Masked:
            result.push_back((int) riscvAssembler::ActionType::RepeatPatch4BOpcode);
            break;

        case Format::CIValue:
            result.push_back((int) riscvAssembler::ActionType::RepeatPatch2BOpcode);
            break;
        }
	}
	else
	{
		int totalBits = numberOfBits + valueShift;
		
		int shift = valueShift;
		int additionalOffset = 0;
		
		while(shift >= 8)
		{
			++additionalOffset;
			totalBits -= 8;
			shift -= 8;
		}
		
		switch(format)
		{
        case Format::Signed:
			if(totalBits <= 8) result.push_back((int) riscvAssembler::ActionType::SignedPatchB1Opcode);
			else if(totalBits <= 16) result.push_back((int) riscvAssembler::ActionType::SignedPatchB2Opcode);
			else result.push_back((int) riscvAssembler::ActionType::SignedPatchB4Opcode);
			break;
        case Format::Unsigned:
			if(totalBits <= 8) result.push_back((int) riscvAssembler::ActionType::UnsignedPatchB1Opcode);
			else if(totalBits <= 16) result.push_back((int) riscvAssembler::ActionType::UnsignedPatchB2Opcode);
			else result.push_back((int) riscvAssembler::ActionType::UnsignedPatchB4Opcode);
			break;
        case Format::Masked:
			if(totalBits <= 8) result.push_back((int) riscvAssembler::ActionType::MaskedPatchB1Opcode);
			else if(totalBits <= 16) result.push_back((int) riscvAssembler::ActionType::MaskedPatchB2Opcode);
			else result.push_back((int) riscvAssembler::ActionType::MaskedPatchB4Opcode);
			break;
        case Format::CIValue:
            result.push_back((int) riscvAssembler::ActionType::CIValuePatchB1Opcode);
            goto WriteExpression;
        }
		
		// 8 bit patching writes a mask instead of number of bits.
		if(totalBits <= 8) result.push_back((1 << numberOfBits) - 1);
		else result.push_back(numberOfBits);
		
		result.push_back(bitOffset);
		result.push_back(shift);
        
    WriteExpression:
		WriteExpressionOffset(result, context, expressionIndex, additionalOffset);
	}
	
	WriteSigned16(result, -delay);
}

//============================================================================

void ExpressionAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	switch(numberOfBytes)
	{
	case 1: result.push_back((int) riscvAssembler::ActionType::B1Expression); break;
	case 2: result.push_back((int) riscvAssembler::ActionType::B2Expression); break;
	case 4: result.push_back((int) riscvAssembler::ActionType::B4Expression); break;
	case 8: result.push_back((int) riscvAssembler::ActionType::B8Expression); break;
	default: assert(!"Unexpected expression bytes");
	}
	WriteExpressionOffset(result, context, expressionIndex);
}

//============================================================================


void AlternateAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	// This builds:
	// [Alternate]
	//   [Condition0] [Action0] [JumpToEnd]
	//   [Condition1] [Action1] [JumpToEnd]
	//   ...
	//   [ConditionN] [ActionN] [EndAlternate]
	// End:
	
	Action *previousAction = context.previousAction;
	
	std::vector<uint8_t> bytes;
	for(size_t i = alternateList.size(); i != 0;)
	{
		--i;
		const Alternate &alternate = alternateList[i];

		std::vector<uint8_t> actionBytes;
		context.previousAction = previousAction;
		alternate.action->WriteByteCode(actionBytes, context);
		if(bytes.size())
		{
			actionBytes.push_back((int) riscvAssembler::ActionType::Jump);
			if(bytes.size() >= 256) throw AssemblerException("Encoding error: Jump must be 256 bytes or less");
			actionBytes.push_back(bytes.size());
		}
		else
		{
			actionBytes.push_back((int) riscvAssembler::ActionType::EndAlternate);
		}
		alternate.condition->WriteByteCode(actionBytes, context, *alternate.action);
		bytes.insert(bytes.begin(), actionBytes.begin(), actionBytes.end());
	}
	context.previousAction = nullptr;
	result.push_back((int) riscvAssembler::ActionType::Alternate);
	if((uint32_t) GetMaximumLength() >= 256) throw AssemblerException("Encoding error: Maximum alternate length must be less than 256 bytes");
	result.push_back(GetMaximumLength());
	result.insert(result.end(), bytes.begin(), bytes.end());
}

//============================================================================

void ListAction::AppendOpcode(uint32_t opcode)
{
	uint8_t bytes[4];
	memcpy(bytes, &opcode, 4);

	if(actionList.size() > 0
	   && actionList.back()->IsLiteralAction())
	{
		LiteralAction* lastLiteralAction = (LiteralAction*) actionList.back();
		lastLiteralAction->AppendBytes(bytes, 4);
	}
	else
	{
		actionList.push_back(new LiteralAction({bytes, bytes+4}));
	}
}

void ListAction::AppendCOpcode(uint32_t opcode)
{
    uint8_t bytes[2];
    memcpy(bytes, &opcode, 2);

    if(actionList.size() > 0
       && actionList.back()->IsLiteralAction())
    {
        LiteralAction* lastLiteralAction = (LiteralAction*) actionList.back();
        lastLiteralAction->AppendBytes(bytes, 2);
    }
    else
    {
        actionList.push_back(new LiteralAction({bytes, bytes+2}));
    }
}

//============================================================================
