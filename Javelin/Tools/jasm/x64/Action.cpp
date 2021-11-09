//============================================================================

#include "Javelin/Tools/jasm/x64/Action.h"

#include "Javelin/Assembler/JitLabelId.h"
#include "Javelin/Assembler/x64/ActionType.h"
#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/CommandLine.h"
#include "Javelin/Tools/jasm/Log.h"

#include <assert.h>
#include <inttypes.h>
#include <string.h>

//============================================================================

using namespace Javelin::Assembler::x64;

//============================================================================

const AlternateActionCondition* ZeroAlternateActionCondition::Create(const Operand *operand)
{
	assert(operand->type == Operand::Type::Immediate
		   || operand->type == Operand::Type::Register);
	assert(operand->IsExpression());
	
	return new ZeroAlternateActionCondition(operand->expressionIndex);
}

void ZeroAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	x64Assembler::ActionType actionType;
	switch(context.expressionInfo[expressionIndex-1].bitWidth)
	{
	case 8:  actionType = x64Assembler::ActionType::Imm0B1Condition; break;
	case 16: actionType = x64Assembler::ActionType::Imm0B2Condition; break;
	case 32: actionType = x64Assembler::ActionType::Imm0B4Condition; break;
	case 64: actionType = x64Assembler::ActionType::Imm0B8Condition; break;
	default: return;
	}
	
	ImmediateAlternateActionCondition::WriteByteCode(result, context, action, (int)actionType);
}

const AlternateActionCondition* Imm8AlternateActionCondition::Create(const Operand *operand)
{
	assert(operand->type == Operand::Type::Immediate);
	assert(operand->IsExpression());
	
	return new Imm8AlternateActionCondition(operand->expressionIndex);
}

std::string Imm8AlternateActionCondition::GetDescription() const
{
	char buffer[24];
	sprintf(buffer, "Imm8{%d}", expressionIndex);
	return buffer;
}

void Imm8AlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	x64Assembler::ActionType actionType;
	switch(context.expressionInfo[expressionIndex-1].bitWidth)
	{
	case 16: actionType = x64Assembler::ActionType::Imm8B2Condition; break;
	case 32: actionType = x64Assembler::ActionType::Imm8B4Condition; break;
	case 64: actionType = x64Assembler::ActionType::Imm8B8Condition; break;
	default: return;
	}
	
	ImmediateAlternateActionCondition::WriteByteCode(result, context, action, (int)actionType);
}

const AlternateActionCondition* Imm16AlternateActionCondition::Create(const Operand *operand)
{
	assert(operand->type == Operand::Type::Immediate);
	assert(operand->IsExpression());
	
	return new Imm16AlternateActionCondition(operand->expressionIndex);
}

std::string Imm16AlternateActionCondition::GetDescription() const
{
	char buffer[24];
	sprintf(buffer, "Imm16{%d}", expressionIndex);
	return buffer;
}

void Imm16AlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	x64Assembler::ActionType actionType;
	switch(context.expressionInfo[expressionIndex-1].bitWidth)
	{
		case 32: actionType = x64Assembler::ActionType::Imm16B4Condition; break;
		case 64: actionType = x64Assembler::ActionType::Imm16B8Condition; break;
		default: return;
	}
	
	ImmediateAlternateActionCondition::WriteByteCode(result, context, action, (int)actionType);
}

const AlternateActionCondition* Imm32AlternateActionCondition::Create(const Operand *operand)
{
	assert(operand->type == Operand::Type::Immediate);
	assert(operand->IsExpression());
	
	return new Imm32AlternateActionCondition(operand->expressionIndex);
}

std::string Imm32AlternateActionCondition::GetDescription() const
{
	char buffer[24];
	sprintf(buffer, "Imm32{%d}", expressionIndex);
	return buffer;
}

void Imm32AlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	x64Assembler::ActionType actionType;
	switch(context.expressionInfo[expressionIndex-1].bitWidth)
	{
		case 64: actionType = x64Assembler::ActionType::Imm32B8Condition; break;
		default: return;
	}
	
	ImmediateAlternateActionCondition::WriteByteCode(result, context, action, (int)actionType);
}

const AlternateActionCondition* UImm32AlternateActionCondition::Create(const Operand *operand)
{
	assert(operand->type == Operand::Type::Immediate);
	assert(operand->IsExpression());
	
	return new UImm32AlternateActionCondition(operand->expressionIndex);
}

std::string UImm32AlternateActionCondition::GetDescription() const
{
	char buffer[24];
	sprintf(buffer, "UImm32{%d}", expressionIndex);
	return buffer;
}

void UImm32AlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	x64Assembler::ActionType actionType;
	switch(context.expressionInfo[expressionIndex-1].bitWidth)
	{
		case 64: actionType = x64Assembler::ActionType::UImm32B8Condition; break;
		default: return;
	}
	
	ImmediateAlternateActionCondition::WriteByteCode(result, context, action, (int)actionType);
}

Rel8AlternateActionCondition::Rel8AlternateActionCondition(const LabelOperand &aLabelOperand)
: labelOperand(aLabelOperand)
{
	
}

const AlternateActionCondition* Rel8AlternateActionCondition::Create(const Operand *operand)
{
	assert(operand->type == Operand::Type::Label);
	
	return new Rel8AlternateActionCondition(*(const LabelOperand*)operand);
}

std::string Rel8AlternateActionCondition::GetDescription() const
{
	char buffer[24];
	if(labelOperand.IsExpression())
	{
		sprintf(buffer, "Rel8{*{%d}%s}", labelOperand.expressionIndex, labelOperand.jumpType.ToString());
		return buffer;
	}
	switch(labelOperand.jumpType)
	{
	case JumpType::Name:
		return "Rel8{" + labelOperand.labelName + "}";
	case JumpType::Forward:
	case JumpType::Backward:
	case JumpType::BackwardOrForward:
		sprintf(buffer, "Rel8{%" PRId64 "%s}", labelOperand.labelValue, labelOperand.jumpType.ToString());
		return buffer;
	}
}

bool Rel8AlternateActionCondition::Equals(const AlternateActionCondition *other) const
{
	if(typeid(*this) != typeid(*other)) return false;
	Rel8AlternateActionCondition *o = (Rel8AlternateActionCondition*) other;
	return labelOperand.jumpType == o->labelOperand.jumpType
			&& labelOperand.labelName == o->labelOperand.labelName
			&& labelOperand.labelValue == o->labelOperand.labelValue;
}

AlternateActionCondition::Result Rel8AlternateActionCondition::IsValid(ActionContext &context, const Action *action) const
{
	// context contains an offset at the end of the alternate.
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

AlternateActionCondition::Result Rel8AlternateActionCondition::IsValidForJumpType(ActionContext &context) const
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

AlternateActionCondition::Result Rel8AlternateActionCondition::IsValid(const ActionOffset &anchorOffset, const ActionOffset &destinationOffset)
{
	ssize_t maximumOffset = destinationOffset.totalMaximumOffset - anchorOffset.totalMaximumOffset;
	if(maximumOffset == (int8_t) maximumOffset) return Result::Always;

	ssize_t minimumOffset = destinationOffset.totalMinimumOffset - anchorOffset.totalMinimumOffset;
	if(minimumOffset == (int8_t) minimumOffset) return Result::Maybe;

	return Result::Never;
}

void Rel8AlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	if(!action.GetLastAction() || dynamic_cast<const PatchLabelAction*>(action.GetLastAction()) == nullptr)
	{
		throw AssemblerException("Internal error: Rel8AlternateAction unable to find PatchLabelAction");
	}
	
	const LabelOperand& labelOperand = dynamic_cast<const PatchLabelAction*>(action.GetLastAction())->labelOperand;
	
	// Prefix with condition code
	std::vector<uint8_t> conditionCode;
	if(labelOperand.IsExpression())
	{
		switch(labelOperand.jumpType)
		{
		case JumpType::Forward:
			conditionCode.push_back((int) x64Assembler::ActionType::Rel8ExpressionLabelForwardCondition);
			Action::WriteExpressionOffset(conditionCode, context, labelOperand.expressionIndex);
			conditionCode.push_back(context.GetForwardLabelReferenceIndex(labelOperand.reference, 1));
			break;
		case JumpType::Backward:
			conditionCode.push_back((int) x64Assembler::ActionType::Rel8ExpressionLabelBackwardCondition);
			Action::WriteExpressionOffset(conditionCode, context, labelOperand.expressionIndex);
			assert(action.GetMaximumLength() == (uint8_t)action.GetMaximumLength());
			break;
		case JumpType::BackwardOrForward:
			conditionCode.push_back((int) x64Assembler::ActionType::Rel8ExpressionLabelCondition);
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
				conditionCode.push_back((int) x64Assembler::ActionType::Rel8LabelCondition);
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
					conditionCode.push_back((int) x64Assembler::ActionType::Rel8LabelForwardCondition);
					Action::WriteUnsigned32(conditionCode, GetLabelIdForIndexed(index));
					conditionCode.push_back(context.GetForwardLabelReferenceIndex(labelOperand.reference, 1));
				}
				else
				{
					conditionCode.push_back((int) x64Assembler::ActionType::Rel8LabelBackwardCondition);
					Action::WriteUnsigned32(conditionCode, GetLabelIdForIndexed(index));
					assert(action.GetMaximumLength() == (uint8_t)action.GetMaximumLength());
				}
			}
			break;
		case JumpType::Forward:
			if(labelOperand.global)
			{
				conditionCode.push_back((int) x64Assembler::ActionType::Rel8LabelForwardCondition);
				Action::WriteUnsigned32(conditionCode, GetLabelIdForGlobalNumeric(labelOperand.labelValue));
				conditionCode.push_back(context.GetForwardLabelReferenceIndex(labelOperand.reference, 1));
			}
			else
			{
				conditionCode.push_back((int) x64Assembler::ActionType::Rel8LabelForwardCondition);
				int64_t labelId = GetLabelIdForNumeric(labelOperand.labelValue);
				int index = context.GetIndexForLabelId(labelId);
				Action::WriteUnsigned32(conditionCode, GetLabelIdForIndexed(index));
				conditionCode.push_back(context.GetForwardLabelReferenceIndex(labelOperand.reference, 1));
			}
			break;
		case JumpType::Backward:
			if(labelOperand.global)
			{
				conditionCode.push_back((int) x64Assembler::ActionType::Rel8LabelBackwardCondition);
				Action::WriteUnsigned32(conditionCode, GetLabelIdForGlobalNumeric(labelOperand.labelValue));
				assert(action.GetMaximumLength() == (uint8_t)action.GetMaximumLength());
			}
			else
			{
				conditionCode.push_back((int) x64Assembler::ActionType::Rel8LabelBackwardCondition);
				int64_t labelId = GetLabelIdForNumeric(labelOperand.labelValue);
				int index = context.GetIndexForLabelId(labelId);
				Action::WriteUnsigned32(conditionCode, GetLabelIdForIndexed(index));
				assert(action.GetMaximumLength() == (uint8_t)action.GetMaximumLength());
			}
			break;
		case JumpType::BackwardOrForward:
			if(labelOperand.global)
			{
				conditionCode.push_back((int) x64Assembler::ActionType::Rel8LabelCondition);
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
	conditionCode.push_back(action.GetMaximumLength());  // rel8Offset
	conditionCode.push_back(result.size());
	result.insert(result.begin(), conditionCode.begin(), conditionCode.end());
}

Rel32AlternateActionCondition::Rel32AlternateActionCondition(const LabelOperand &aLabelOperand)
: labelOperand(aLabelOperand)
{
	
}

const AlternateActionCondition* Rel32AlternateActionCondition::Create(const Operand *operand)
{
	assert(operand->type == Operand::Type::Label);
	
	return new Rel32AlternateActionCondition(*(const LabelOperand*)operand);
}

std::string Rel32AlternateActionCondition::GetDescription() const
{
	char buffer[24];
	if(labelOperand.IsExpression())
	{
		sprintf(buffer, "Rel32{*{%d}%s}", labelOperand.expressionIndex, labelOperand.jumpType.ToString());
		return buffer;
	}
	switch(labelOperand.jumpType)
	{
	case JumpType::Name:
		return "Rel32{" + labelOperand.labelName + "}";
	case JumpType::Forward:
	case JumpType::Backward:
	case JumpType::BackwardOrForward:
		sprintf(buffer, "Rel32{%" PRId64 "%s}", labelOperand.labelValue, labelOperand.jumpType.ToString());
		return buffer;
	}
}

bool Rel32AlternateActionCondition::Equals(const AlternateActionCondition *other) const
{
	if(typeid(*this) != typeid(*other)) return false;
	Rel32AlternateActionCondition *o = (Rel32AlternateActionCondition*) other;
	return labelOperand.jumpType == o->labelOperand.jumpType
			&& labelOperand.labelName == o->labelOperand.labelName
			&& labelOperand.labelValue == o->labelOperand.labelValue;
}

AlternateActionCondition::Result Rel32AlternateActionCondition::IsValid(ActionContext &context, const Action *action) const
{
	// context contains an offset at the end of the alternate.
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

AlternateActionCondition::Result Rel32AlternateActionCondition::IsValidForJumpType(ActionContext &context) const
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

AlternateActionCondition::Result Rel32AlternateActionCondition::IsValid(const ActionOffset &anchorOffset, const ActionOffset &destinationOffset)
{
	ssize_t maximumOffset = destinationOffset.totalMaximumOffset - anchorOffset.totalMaximumOffset;
	if(maximumOffset == (int32_t) maximumOffset) return Result::Always;

	ssize_t minimumOffset = destinationOffset.totalMinimumOffset - anchorOffset.totalMinimumOffset;
	if(minimumOffset == (int32_t) minimumOffset) return Result::Maybe;

	return Result::Never;
}

void Rel32AlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	// TODO: Implement
//	assert(!"Not implemented");
}

std::string Delta32AlternateActionCondition::GetDescription() const
{
	char buffer[24];
	sprintf(buffer, "Delta32{%d}", expressionIndex);
	return buffer;
}

void Delta32AlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	ImmediateAlternateActionCondition::WriteByteCode(result, context, action, (int)x64Assembler::ActionType::Delta32Condition);
}

//============================================================================

void LiteralAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	if(bytes.size() <= 16)
	{
		result.push_back((int) x64Assembler::ActionType::Literal1 + bytes.size()-1);
	}
	else
	{
		result.push_back((int) x64Assembler::ActionType::LiteralBlock);
		if((bytes.size() >> 16) != 0) throw AssemblerException("Literal block too large");
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
		
		static const uint8_t nop2[] = { 0x66, 0x90 };
		static const uint8_t nop3[] = { 0x0f, 0x1f, 0x00 };
		static const uint8_t nop4[] = { 0x0f, 0x1f, 0x40, 0x00 };
		static const uint8_t nop6[] = { 0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00 };
		static const uint8_t nop7[] = { 0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00 };
		static const uint8_t nop15[] = { 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };

		static const uint8_t *nops[] = {
			nop2+1, nop2, nop3, nop4, nop6+1,
			nop6, nop7, nop15+7, nop15+6, nop15+5,
			nop15+4, nop15+3, nop15+2, nop15+1, nop15
		};

		std::vector<uint8_t> bytes;
		
		while(fixedLength)
		{
			int nopLength = fixedLength > 15 ? 15 : fixedLength;
			const uint8_t *nop = nops[nopLength-1];
			bytes.insert(bytes.end(), nop, nop+nopLength);
			fixedLength -= nopLength;
		}
		
		parent->ReplaceActionAtIndex(index, new LiteralAction(bytes));
		return true;
	}
	return false;
}

void AlignAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
    if (isFixed && fixedLength == 0) return;
    
	result.push_back((int) x64Assembler::ActionType::Align);
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
			parent->ReplaceActionAtIndex(index, new LiteralAction({0x90}));
		}
		return true;
	}
	return false;
}

void UnalignAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
    if (isFixed && fixedLength == 0) return;

    result.push_back((int) x64Assembler::ActionType::Unalign);
	int data = alignment - 1;
	assert(data == (uint8_t) data);
	result.push_back(data);
}

//============================================================================

void DynamicOpcodeR::Dump() const
{
	printf(" encode(R, 0x%02x, 0x%02x, %d", rex, modRM, regExpressionIndex);
	for(uint8_t value : opcodes)
	{
		printf(", 0x%02x", value);
	}
	printf(")");
}

size_t DynamicOpcodeR::GetMinimumLength() const
{
	return opcodes.size() + (rex != 0 ? 2 : 1);
}

size_t DynamicOpcodeR::GetMaximumLength() const
{
	// rex + opcodes + modrm
	return opcodes.size() + 2;
}

void DynamicOpcodeR::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	assert(1 <= opcodes.size() && opcodes.size() <= 4);
	result.push_back((int) x64Assembler::ActionType::DynamicOpcodeR);
	result.push_back(rex);
	result.push_back(modRM);
	
	x64Assembler::DynamicOpcodeRRControlByte controlByte = { .value = 0 };
	controlByte.opcodeLengthM1 = opcodes.size() - 1;
	if(regExpressionIndex != 0) controlByte.hasRegExpression = 1;
	result.push_back(controlByte.value);
	if(regExpressionIndex) WriteExpressionOffset(result, context, regExpressionIndex);
	result.insert(result.end(), opcodes.begin(), opcodes.end());
}

//============================================================================

void DynamicOpcodeRR::Dump() const
{
	printf(" encode(RR, 0x%02x, 0x%02x, %d, %d", rex, modRM, regExpressionIndex, rmExpressionIndex);
	for(uint8_t value : opcodes)
	{
		printf(", 0x%02x", value);
	}
	printf(")");
}

size_t DynamicOpcodeRR::GetMinimumLength() const
{
	return opcodes.size() + (rex != 0 ? 2 : 1);
}

size_t DynamicOpcodeRR::GetMaximumLength() const
{
	// rex + opcodes + modrm
	return opcodes.size() + 2;
}

void DynamicOpcodeRR::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	assert(1 <= opcodes.size() && opcodes.size() <= 4);
	result.push_back((int) x64Assembler::ActionType::DynamicOpcodeRR);
	result.push_back(rex);
	result.push_back(modRM);

	x64Assembler::DynamicOpcodeRRControlByte controlByte = { .value = 0 };
	controlByte.opcodeLengthM1 = opcodes.size() - 1;
	if(regExpressionIndex != 0) controlByte.hasRegExpression = 1;
	if(rmExpressionIndex != 0) controlByte.hasRmExpression = 1;
	result.push_back(controlByte.value);
	if(regExpressionIndex) WriteExpressionOffset(result, context, regExpressionIndex);
	if(rmExpressionIndex) WriteExpressionOffset(result, context, rmExpressionIndex);
	result.insert(result.end(), opcodes.begin(), opcodes.end());
}

//============================================================================

void DynamicOpcodeRM::Dump() const
{
	printf(" encode(RM, 0x%02x, 0x%02x, %02x, %d, %d, %d, %d, %d, %d",
		   rex, modRM, sib, displacement, regExpressionIndex,
		   scaleExpressionIndex, indexExpressionIndex, baseExpressionIndex,
		   displacementExpressionIndex);
	for(uint8_t value : opcodes)
	{
		printf(", 0x%02x", value);
	}
	printf(")");
}

size_t DynamicOpcodeRM::GetMinimumLength() const
{
	// opcodes + modrm
	size_t size = opcodes.size() + 1;
	if(rex) ++size;

	if((modRM & 7) == 4) ++size;		// kModRMRmSIB
	if(displacementExpressionIndex == 0)
	{
		if(displacement == 0)
		{
			if((modRM & 0xc7) == 5) size += 4; // RIP relative.
		}
		else if(displacement == (int8_t)displacement) size += 1;
		else size += 4;
	}
	return size;
}

size_t DynamicOpcodeRM::GetMaximumLength() const
{
	const size_t rexSize = rex != 0
	    || regExpressionIndex != 0
	    || baseExpressionIndex != 0
	    || indexExpressionIndex != 0
	  ? 1 : 0;
	
	const size_t sibSize = (modRM & 7) == 4
	    || baseExpressionIndex != 0
	  ? 1 : 0;

	size_t displacementSize = 4;
	if(displacementExpressionIndex == 0)
	{
		if(displacement == 0)
		{
			if((modRM & 0xc7) != 5) displacementSize = 0; // If not RIP relative.
		}
		else if(displacement == (int8_t)displacement) displacementSize = 1;
	}

	// rex + opcodes + modrm + sib + displacement
	return rexSize + opcodes.size() + 1 + sibSize + displacementSize;
}

void DynamicOpcodeRM::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	assert(1 <= opcodes.size() && opcodes.size() <= 4);
	result.push_back((int) x64Assembler::ActionType::DynamicOpcodeRM);
	result.push_back(rex);
	result.push_back(modRM);
	result.push_back(sib);
	
	x64Assembler::DynamicOpcodeRMControlByte controlByte = { .value = 0 };
	controlByte.opcodeLengthM1 = opcodes.size() - 1;
	if(regExpressionIndex != 0) controlByte.hasRegExpression = 1;
	if(scaleExpressionIndex != 0) controlByte.hasScaleExpression = 1;
	if(indexExpressionIndex != 0) controlByte.hasIndexExpression = 1;
	if(baseExpressionIndex != 0) controlByte.hasBaseExpression = 1;
	if(displacementExpressionIndex != 0) controlByte.hasDisplacementExpression = 1;
	result.push_back(controlByte.value);
	
	if(displacementExpressionIndex) WriteExpressionOffset(result, context, displacementExpressionIndex);
	else WriteSigned32(result, displacement);
	
	if(regExpressionIndex) WriteExpressionOffset(result, context, regExpressionIndex);
	if(scaleExpressionIndex) WriteExpressionOffset(result, context, scaleExpressionIndex);
	if(indexExpressionIndex) WriteExpressionOffset(result, context, indexExpressionIndex);
	if(baseExpressionIndex) WriteExpressionOffset(result, context, baseExpressionIndex);
	result.insert(result.end(), opcodes.begin(), opcodes.end());
}

//============================================================================

void DynamicOpcodeO::Dump() const
{
	printf(" encode(O, 0x%02x, %d", rex, expressionIndex);
	for(uint8_t value : opcodes)
	{
		printf(", 0x%02x", value);
	}
	printf(")");
}

size_t DynamicOpcodeO::GetMinimumLength() const
{
	return opcodes.size() + (rex != 0 ? 1 : 0);
}

size_t DynamicOpcodeO::GetMaximumLength() const
{
	return opcodes.size() + 1;
}

void DynamicOpcodeO::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	assert(opcodes.size() == 1 || opcodes.size() == 2);
	
	result.push_back((int) x64Assembler::ActionType::DynamicOpcodeO1 + opcodes.size()-1);
	result.push_back(rex);
	WriteExpressionOffset(result, context, expressionIndex);
	result.insert(result.end(), opcodes.begin(), opcodes.end());
}

//============================================================================

void DynamicOpcodeRV::Dump() const
{
	printf(" encode(RV, 0x%02x, 0x%02x, 0x%02x, %d, %d", vex3B2, vex3B3, modRM, reg0ExpressionIndex, reg1ExpressionIndex);
	for(uint8_t value : opcodes)
	{
		printf(", 0x%02x", value);
	}
	printf(")");
}

size_t DynamicOpcodeRV::GetMinimumLength() const
{
	if((vex3B2 & 0x7f) != 1 || (vex3B3 & 0x80))
	{
		return opcodes.size() + 4;
	}
	else
	{
		return opcodes.size() + 3;
	}
}

size_t DynamicOpcodeRV::GetMaximumLength() const
{
	if((vex3B2 & 0x7f) != 1 || (vex3B3 & 0x80))
	{
		return opcodes.size() + 4;
	}
	else
	{
		return opcodes.size() + 3;
	}
}

void DynamicOpcodeRV::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	assert(1 <= opcodes.size() && opcodes.size() <= 4);
	result.push_back((int) x64Assembler::ActionType::DynamicOpcodeRV);
	result.push_back(vex3B2);
	result.push_back(vex3B3);
	result.push_back(modRM);
	
	x64Assembler::DynamicOpcodeRVControlByte controlByte = { .value = 0 };
	controlByte.opcodeLengthM1 = opcodes.size() - 1;
	if(reg0ExpressionIndex != 0) controlByte.hasReg0Expression = 1;
	if(reg1ExpressionIndex != 0) controlByte.hasReg1Expression = 1;
	result.push_back(controlByte.value);
	if(reg0ExpressionIndex) WriteExpressionOffset(result, context, reg0ExpressionIndex);
	if(reg1ExpressionIndex) WriteExpressionOffset(result, context, reg1ExpressionIndex);
	result.insert(result.end(), opcodes.begin(), opcodes.end());
}

//============================================================================

void DynamicOpcodeRVR::Dump() const
{
	printf(" encode(RVR, 0x%02x, 0x%02x, 0x%02x, %d, %d, %d", vex3B2, vex3B3, modRM, reg0ExpressionIndex, reg1ExpressionIndex, reg2ExpressionIndex);
	for(uint8_t value : opcodes)
	{
		printf(", 0x%02x", value);
	}
	printf(")");
}

size_t DynamicOpcodeRVR::GetMinimumLength() const
{
	if((vex3B2 & 0x7f) != 1 || (vex3B3 & 0x80))
	{
		return opcodes.size() + 4;
	}
	else
	{
		return opcodes.size() + 3;
	}
}

size_t DynamicOpcodeRVR::GetMaximumLength() const
{
	if((vex3B2 & 0x7f) != 1
	   || (vex3B3 & 0x80)
	   || reg2ExpressionIndex != 0)
	{
		return opcodes.size() + 4;
	}
	else
	{
		return opcodes.size() + 3;
	}
}

void DynamicOpcodeRVR::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	assert(1 <= opcodes.size() && opcodes.size() <= 4);
	result.push_back((int) x64Assembler::ActionType::DynamicOpcodeRVR);
	result.push_back(vex3B2);
	result.push_back(vex3B3);
	result.push_back(modRM);
	
	x64Assembler::DynamicOpcodeRVRControlByte controlByte = { .value = 0 };
	controlByte.opcodeLengthM1 = opcodes.size() - 1;
	if(reg0ExpressionIndex != 0) controlByte.hasReg0Expression = 1;
	if(reg1ExpressionIndex != 0) controlByte.hasReg1Expression = 1;
	if(reg2ExpressionIndex != 0) controlByte.hasReg2Expression = 1;
	result.push_back(controlByte.value);
	if(reg0ExpressionIndex) WriteExpressionOffset(result, context, reg0ExpressionIndex);
	if(reg1ExpressionIndex) WriteExpressionOffset(result, context, reg1ExpressionIndex);
	if(reg2ExpressionIndex) WriteExpressionOffset(result, context, reg2ExpressionIndex);
	result.insert(result.end(), opcodes.begin(), opcodes.end());
}

//============================================================================

void DynamicOpcodeRVM::Dump() const
{
	printf(" encode(RVM, 0x%02x, 0x%02x, 0x%02x, %02x, %d, %d, %d, %d, %d, %d, %d",
		   vex3B2, vex3B3, modRM, sib, displacement, reg0ExpressionIndex,
		   reg1ExpressionIndex, scaleExpressionIndex, indexExpressionIndex,
		   baseExpressionIndex, displacementExpressionIndex);
	for(uint8_t value : opcodes)
	{
		printf(", 0x%02x", value);
	}
	printf(")");
}

size_t DynamicOpcodeRVM::GetMinimumLength() const
{
	// opcodes + modrm
	size_t size = opcodes.size() + 1;
	
	if((vex3B2 & 0x7f) != 1 || (vex3B3 & 0x80)) size += 3;
	else size += 2;

	if((modRM & 7) == 4) ++size;
	if(displacementExpressionIndex == 0)
	{
		if(displacement == 0)
		{
			if((modRM & 0xc7) == 5) size += 4; // RIP relative.
		}
		else if(displacement == (int8_t)displacement) size += 1;
		else size += 4;
	}
	return size;
}

size_t DynamicOpcodeRVM::GetMaximumLength() const
{
	// vex + opcodes + modrm + sib + 4xdisplacement
	const size_t vexSize = (vex3B2 & 0x7f) != 1
	    || (vex3B3 & 0x80)
	    || baseExpressionIndex != 0
	    || indexExpressionIndex != 0
	  ? 3 : 2;

	const size_t sibSize = (modRM & 7) == 4
	    || baseExpressionIndex != 0
	  ? 1 : 0;

	size_t displacementSize = 4;
	if(displacementExpressionIndex == 0)
	{
		if(displacement == 0)
		{
			if((modRM & 0xc7) != 5) displacementSize = 0; // If not RIP relative.
		}
		else if(displacement == (int8_t)displacement) displacementSize = 1;
	}

	return vexSize + opcodes.size() + 1 + sibSize + displacementSize;

}

void DynamicOpcodeRVM::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	assert(1 <= opcodes.size() && opcodes.size() <= 4);
	result.push_back((int) x64Assembler::ActionType::DynamicOpcodeRVM);
	result.push_back(vex3B2);
	result.push_back(vex3B3);
	result.push_back(modRM);
	result.push_back(sib);
	
	x64Assembler::DynamicOpcodeRVMControlByte controlByte = { .value = 0 };
	controlByte.opcodeLengthM1 = opcodes.size() - 1;
	if(reg0ExpressionIndex != 0) controlByte.hasReg0Expression = 1;
	if(reg1ExpressionIndex != 0) controlByte.hasReg1Expression = 1;
	if(scaleExpressionIndex != 0) controlByte.hasScaleExpression = 1;
	if(indexExpressionIndex != 0) controlByte.hasIndexExpression = 1;
	if(baseExpressionIndex != 0) controlByte.hasBaseExpression = 1;
	if(displacementExpressionIndex != 0) controlByte.hasDisplacementExpression = 1;
	result.push_back(controlByte.value);
	
	if(displacementExpressionIndex) WriteExpressionOffset(result, context, displacementExpressionIndex);
	else WriteSigned32(result, displacement);
	
	if(reg0ExpressionIndex) WriteExpressionOffset(result, context, reg0ExpressionIndex);
	if(reg1ExpressionIndex) WriteExpressionOffset(result, context, reg1ExpressionIndex);
	if(scaleExpressionIndex) WriteExpressionOffset(result, context, scaleExpressionIndex);
	if(indexExpressionIndex) WriteExpressionOffset(result, context, indexExpressionIndex);
	if(baseExpressionIndex) WriteExpressionOffset(result, context, baseExpressionIndex);
	result.insert(result.end(), opcodes.begin(), opcodes.end());
}

//============================================================================

void NamedLabelAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	uint32_t labelId = GetLabelIdForNamed(value.c_str());
	if(global)
	{
		result.push_back((int) x64Assembler::ActionType::Label);
		WriteUnsigned32(result, labelId);
	}
	else
	{
		result.push_back((int) x64Assembler::ActionType::Label);
		int index = context.GetIndexForLabelId(labelId);
		WriteUnsigned32(result, GetLabelIdForIndexed(index));
	}
	context.numberOfLabels++;
}

void NumericLabelAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	if(global)
	{
		result.push_back((int) x64Assembler::ActionType::Label);
		WriteUnsigned32(result, GetLabelIdForGlobalNumeric(value));
	}
	else
	{
		result.push_back((int) x64Assembler::ActionType::Label);
		int64_t labelId = GetLabelIdForNumeric(value);
		int index = context.GetIndexForLabelId(labelId);
		WriteUnsigned32(result, GetLabelIdForIndexed(index));
	}
	context.numberOfLabels++;
}

void ExpressionLabelAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	result.push_back((int) x64Assembler::ActionType::ExpressionLabel);
	WriteExpressionOffset(result, context, expressionIndex);
	context.numberOfLabels++;
}

//============================================================================

bool PatchLabelAction::Simplify(Common::ListAction *parent, size_t index)
{
	if(!hasFixedDelta) return false;

	assert(index > 0);
	
	size_t literalActionIndex = index - 1;
	int offset = this->offset;
	Action *previousAction;
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
			   || previousAction->GetMinimumLength() > offset - bytes) return false;
			offset -= previousAction->GetMinimumLength();
			--literalActionIndex;
		}
	}

	assert(previousAction->IsLiteralAction()
		   && previousAction->GetMaximumLength() >= offset);
	
	LiteralAction *literal = (LiteralAction*) previousAction;
    std::vector<uint8_t>& literalBytes = literal->GetLiteralData();
	int64_t value = 0;
	memcpy(&value, literalBytes.data() + literalBytes.size() - offset, bytes);
	value += delta;
	memcpy(literalBytes.data() + literalBytes.size() - offset, &value, bytes);
	parent->RemoveActionAtIndex(index);
	delete this;
	return true;
}

void PatchNameLabelAction::Dump() const
{
	const char *JUMP_DIRECTIONS[] = { "", ":b", ":f", ":bf" };
	
	printf(", #%d@-%d=%s%s%s", bytes, offset, global ? "*" : "", value.c_str(), JUMP_DIRECTIONS[(int) labelOperand.jumpType]);
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

	labelOperand.jumpType = context.forwards ? JumpType::Backward : JumpType::Forward;

	const ActionOffset anchorOffset = context.offset;
	const ActionOffset targetOffset = it->second;
	
	ssize_t maximumDelta = targetOffset.totalMaximumOffset - anchorOffset.totalMaximumOffset;
	ssize_t minimumDelta = targetOffset.totalMinimumOffset - anchorOffset.totalMinimumOffset;
	if(maximumDelta != minimumDelta) return false;
	
	hasFixedDelta = true;
	delta = minimumDelta;
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
		result.push_back((int) x64Assembler::ActionType::PatchLabel);
		WriteUnsigned32(result, labelId);
		result.push_back(bytes);
		result.push_back(offset);
		context.AddForwardLabelReference(labelOperand.reference);
	}
	else
	{
		uint32_t index = context.GetIndexForLabelId(labelId);
		switch(labelOperand.jumpType)
		{
		case JumpType::Name:
		case JumpType::BackwardOrForward:
			throw AssemblerException("Internal error: Unexpected jumpType");
		case JumpType::Forward:
			result.push_back((int) x64Assembler::ActionType::PatchLabelForward);
			WriteUnsigned32(result, GetLabelIdForIndexed(index));
			result.push_back(bytes);
			result.push_back(offset);
			context.AddForwardLabelReference(labelOperand.reference);
			break;
		case JumpType::Backward:
			result.push_back((int) x64Assembler::ActionType::PatchLabelBackward);
			WriteUnsigned32(result, GetLabelIdForIndexed(index));
			result.push_back(bytes);
			result.push_back(offset);
			break;
		}
	}
}

void PatchNumericLabelAction::Dump() const
{
	printf(", %s#%d@-%d=%" PRId64 "%s", global ? "*" : "", bytes, offset, value, labelOperand.jumpType.ToString());
	if(hasFixedDelta) printf("=%" PRId64, delta);
}

bool PatchNumericLabelAction::ResolveRelativeAddresses(ActionContext &context)
{
	Inherited::ResolveRelativeAddresses(context);
	
	// Already resolved!
	if(hasFixedDelta) return false;
	
	if(!global)
	{
		if(context.forwards)
		{
			if(labelOperand.jumpType == JumpType::Forward)
			{
				context.numericReferenceSet.insert(value);
				return false;
			}
		}
		else
		{
			if(labelOperand.jumpType == JumpType::Backward)
			{
				context.numericReferenceSet.insert(value);
				return false;
			}
		}
	}
	
	if(context.forwards)
	{
		if(labelOperand.jumpType == JumpType::Forward) return false;
	}
	else
	{
		if(labelOperand.jumpType == JumpType::Backward) return false;
	}

	ActionContext::NumericLabelMap::const_iterator it = context.numericLabels.find(global ? value ^ -1 : value);
	if(it == context.numericLabels.end()) return false;
	hasFoundTarget = true;

	labelOperand.jumpType = context.forwards ? JumpType::Backward : JumpType::Forward;

	const ActionOffset anchorOffset = context.offset;
	const ActionOffset targetOffset = it->second;
	
	ssize_t maximumDelta = targetOffset.totalMaximumOffset - anchorOffset.totalMaximumOffset;
	ssize_t minimumDelta = targetOffset.totalMinimumOffset - anchorOffset.totalMinimumOffset;
	if(maximumDelta != minimumDelta) return false;
	
	hasFixedDelta = true;
	delta = minimumDelta;
	return true;
}

void PatchNumericLabelAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	if(!global && !hasFoundTarget)
	{
		Log::Error("Local numeric label %" PRId64 " unresolved\n", value);
		return;
	}
	if(global)
	{
		int32_t labelId = GetLabelIdForGlobalNumeric(labelOperand.labelValue);
		switch(labelOperand.jumpType)
		{
		case JumpType::Name:
			throw AssemblerException("Internal error: Unexpected jumpType");
		case JumpType::Forward:
			result.push_back((int) x64Assembler::ActionType::PatchLabelForward);
			WriteUnsigned32(result, labelId);
			result.push_back(bytes);
			result.push_back(offset);
			context.AddForwardLabelReference(labelOperand.reference);
			break;
		case JumpType::Backward:
			result.push_back((int) x64Assembler::ActionType::PatchLabelBackward);
			WriteUnsigned32(result, labelId);
			result.push_back(bytes);
			result.push_back(offset);
			break;
		case JumpType::BackwardOrForward:
			result.push_back((int) x64Assembler::ActionType::PatchLabel);
			WriteUnsigned32(result, labelId);
			result.push_back(bytes);
			result.push_back(offset);
			context.AddForwardLabelReference(labelOperand.reference);
			break;
		}
	}
	else
	{
		int64_t labelId = GetLabelIdForNumeric(labelOperand.labelValue);
		int index = context.GetIndexForLabelId(labelId);
		switch(labelOperand.jumpType)
		{
		case JumpType::Name:
			throw AssemblerException("Internal error: Unexpected jumpType");
		case JumpType::Forward:
			result.push_back((int) x64Assembler::ActionType::PatchLabelForward);
			WriteUnsigned32(result, GetLabelIdForIndexed(index));
			result.push_back(bytes);
			result.push_back(offset);
			context.AddForwardLabelReference(labelOperand.reference);
			break;
		case JumpType::Backward:
			result.push_back((int) x64Assembler::ActionType::PatchLabelBackward);
			WriteUnsigned32(result, GetLabelIdForIndexed(index));
			result.push_back(bytes);
			result.push_back(offset);
			break;
		case JumpType::BackwardOrForward:
			throw AssemblerException("Label %" PRId64 " not resolved", labelOperand.labelValue);
		}
	}
}

PatchExpressionLabelAction::PatchExpressionLabelAction(uint8_t bytes, uint8_t offset, const LabelOperand &labelOperand)
: PatchLabelAction(true, bytes, offset, labelOperand)
{
	
}

void PatchExpressionLabelAction::Dump() const
{
	printf(", #%d@-%d=*{%d}%s",
		   bytes,
		   offset,
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
	delta = minimumDelta;
	return true;
}

void PatchExpressionLabelAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	switch(labelOperand.jumpType)
	{
	case JumpType::Name:
		throw AssemblerException("Internal error: Unexpected jumpType");
	case JumpType::Forward:
		result.push_back((int) x64Assembler::ActionType::PatchExpressionLabelForward);
		WriteExpressionOffset(result, context, labelOperand.expressionIndex);
		result.push_back(bytes);
		result.push_back(offset);
		context.AddForwardLabelReference(labelOperand.reference);
		break;
	case JumpType::Backward:
		result.push_back((int) x64Assembler::ActionType::PatchExpressionLabelBackward);
		WriteExpressionOffset(result, context, labelOperand.expressionIndex);
		result.push_back(bytes);
		result.push_back(offset);
		break;
	case JumpType::BackwardOrForward:
		result.push_back((int) x64Assembler::ActionType::PatchExpressionLabel);
		WriteExpressionOffset(result, context, labelOperand.expressionIndex);
		result.push_back(bytes);
		result.push_back(offset);
		context.AddForwardLabelReference(labelOperand.reference);
		break;
	}
}

//============================================================================

void PatchAbsoluteAddressAction::Dump() const
{
	printf(", @-%d+=ptr", delay);
}

void PatchAbsoluteAddressAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	result.push_back((int) x64Assembler::ActionType::PatchAbsoluteAddress);
	result.push_back(delay);
}

//============================================================================

void ExpressionAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	switch(numberOfBytes)
	{
	case 1: result.push_back((int) x64Assembler::ActionType::B1Expression); break;
	case 2: result.push_back((int) x64Assembler::ActionType::B2Expression); break;
	case 4: result.push_back((int) x64Assembler::ActionType::B4Expression); break;
	case 8: result.push_back((int) x64Assembler::ActionType::B8Expression); break;
	default: assert(!"Unexpected expression bytes");
	}
	WriteExpressionOffset(result, context, expressionIndex);
}

void DeltaExpressionAction::Dump() const
{
	printf(", #%d=∆{%d}", numberOfBytes, expressionIndex);
}

void DeltaExpressionAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	switch(numberOfBytes)
	{
	case 1: result.push_back((int) x64Assembler::ActionType::B1DeltaExpression); break;
	case 4: result.push_back((int) x64Assembler::ActionType::B4DeltaExpression); break;
	default: assert(!"Unexpected delta expression bytes");
	}
	WriteExpressionOffset(result, context, expressionIndex);
}

void AddExpressionAction::Dump() const
{
	printf(", #%d@-%d=+{%d}", numberOfBytes, numberOfBytes, expressionIndex);
}

void AddExpressionAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	switch(numberOfBytes)
	{
	case 4: result.push_back((int) x64Assembler::ActionType::B4AddExpression); break;
	default: assert(!"Unexpected delta expression bytes");
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
	
	std::vector<uint8_t> bytes;

	if(alternateList.size() && alternateList.back().condition != &AlwaysAlternateActionCondition::instance)
	{
		bytes.push_back((int) x64Assembler::ActionType::EndAlternate);
	}

	for(size_t i = alternateList.size(); i != 0;)
	{
		--i;
		const Alternate &alternate = alternateList[i];

		std::vector<uint8_t> actionBytes;
		alternate.action->WriteByteCode(actionBytes, context);
		if(bytes.size())
		{
			actionBytes.push_back((int) x64Assembler::ActionType::Jump);
			if(bytes.size() >= 256) throw AssemblerException("Encoding error: Jump must be 256 bytes or less");
			actionBytes.push_back(bytes.size());
		}
		else
		{
			actionBytes.push_back((int) x64Assembler::ActionType::EndAlternate);
		}
		alternate.condition->WriteByteCode(actionBytes, context, *alternate.action);
		bytes.insert(bytes.begin(), actionBytes.begin(), actionBytes.end());
	}
	result.push_back((int) x64Assembler::ActionType::Alternate);
	if(GetMaximumLength() >= 256) throw AssemblerException("Encoding error: Maximum alternate length must be less than 256 bytes");
	result.push_back(GetMaximumLength());
	result.insert(result.end(), bytes.begin(), bytes.end());
}

//============================================================================

