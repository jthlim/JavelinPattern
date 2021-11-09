//============================================================================

#include "Javelin/Tools/jasm/arm64/Action.h"

#include "Javelin/Assembler/BitUtility.h"
#include "Javelin/Assembler/JitLabelId.h"
#include "Javelin/Assembler/arm64/ActionType.h"
#include "Javelin/Tools/jasm/arm64/Assembler.h"
#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/CommandLine.h"
#include "Javelin/Tools/jasm/Log.h"

#include <assert.h>
#include <inttypes.h>
#include <memory.h>

//============================================================================

using namespace Javelin::arm64Assembler;
using namespace Javelin::Assembler::arm64;

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
	arm64Assembler::ActionType actionType;
	switch(context.expressionInfo[expressionIndex-1].bitWidth)
	{
	case 8:  actionType = arm64Assembler::ActionType::Imm0B1Condition; break;
	case 16: actionType = arm64Assembler::ActionType::Imm0B2Condition; break;
	case 32: actionType = arm64Assembler::ActionType::Imm0B4Condition; break;
	case 64: actionType = arm64Assembler::ActionType::Imm0B8Condition; break;
	default: return;
	}
	
	ImmediateAlternateActionCondition::WriteByteCode(result, context, action, (int)actionType);
}

std::string Delta21AlternateActionCondition::GetDescription() const
{
	char buffer[32];
	sprintf(buffer, "delta21{%d}", expressionIndex);
	return buffer;
}

void Delta21AlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	ImmediateAlternateActionCondition::WriteByteCode(result, context, action, (int) arm64Assembler::ActionType::Delta21Condition);
}

std::string Delta26x4AlternateActionCondition::GetDescription() const
{
	char buffer[32];
	sprintf(buffer, "delta26x4{%d}", expressionIndex);
	return buffer;
}

void Delta26x4AlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	ImmediateAlternateActionCondition::WriteByteCode(result, context, action, (int) arm64Assembler::ActionType::Delta26x4Condition);
}

std::string AdrpAlternateActionCondition::GetDescription() const
{
	char buffer[32];
	sprintf(buffer, "adrp{%d}", expressionIndex);
	return buffer;
}

void AdrpAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	ImmediateAlternateActionCondition::WriteByteCode(result, context, action, (int) arm64Assembler::ActionType::AdrpCondition);
}

//============================================================================

void LiteralAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	if(bytes.size() == 0) return;
	
	if(bytes.size() <= 32 && bytes.size() % 4 == 0)
	{
		static_assert((int) arm64Assembler::ActionType::Literal4 == 1, "Unexpected literal value");
		static_assert((int) arm64Assembler::ActionType::Literal8 == 2, "Unexpected literal value");
		static_assert((int) arm64Assembler::ActionType::Literal12 == 3, "Unexpected literal value");
		static_assert((int) arm64Assembler::ActionType::Literal16 == 4, "Unexpected literal value");
		static_assert((int) arm64Assembler::ActionType::Literal20 == 5, "Unexpected literal value");
		static_assert((int) arm64Assembler::ActionType::Literal24 == 6, "Unexpected literal value");
		static_assert((int) arm64Assembler::ActionType::Literal28 == 7, "Unexpected literal value");
		static_assert((int) arm64Assembler::ActionType::Literal32 == 8, "Unexpected literal value");
		result.push_back((int) bytes.size() / 4);
	}
	else
	{
		result.push_back((int) arm64Assembler::ActionType::LiteralBlock);
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
        while((fixedLength & 3) != 0)
        {
            bytes.push_back(0);
            --fixedLength;
        }
        
		while(fixedLength)
		{
            const uint32_t nopOpcode = 0xd503201f;
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
    
	result.push_back((int) arm64Assembler::ActionType::Align);
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
            const uint32_t nopOpcode = 0xd503201f;
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
	result.push_back((int) arm64Assembler::ActionType::Unalign);
	int data = alignment - 1;
	assert(data == (uint8_t) data);
	result.push_back(data);
}

//============================================================================

void NamedLabelAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	uint32_t labelId = GetLabelIdForNamed(value.c_str());
	result.push_back((int) arm64Assembler::ActionType::Label);
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
	result.push_back((int) arm64Assembler::ActionType::Label);
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
	result.push_back((int) arm64Assembler::ActionType::ExpressionLabel);
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
	case RelEncoding::Rel26:
		{
			union Opcode
			{
				uint32_t v;
				struct
				{
					int32_t offset : 26;
					uint32_t _dummy26 : 6;
				};
			};
			static_assert(sizeof(Opcode) == 4, "Opcode should be 4 bytes");
			Opcode opcode;
			memcpy(&opcode, literalBytes.data() + literalBytes.size() - 4, 4);
			
			int32_t rel = opcode.offset;
			rel += int32_t(delta / 4);
            assert(BitUtility::IsValidSignedImmediate(rel, 26));
			opcode.offset = rel;
			
			memcpy(literalBytes.data() + literalBytes.size() - 4, &opcode, 4);
			break;
		}
	case RelEncoding::Rel19Offset5:
		{
			union Opcode
			{
				uint32_t v;
				struct
				{
					uint32_t _dummy0 : 5;
					int32_t offset : 19;
					uint32_t _dummy24 : 8;
				};
			};
			static_assert(sizeof(Opcode) == 4, "Opcode should be 4 bytes");
			Opcode opcode;
			memcpy(&opcode, literalBytes.data() + literalBytes.size() - 4, 4);
			
			int32_t rel = opcode.offset;
			rel += int32_t(delta / 4);
            assert(BitUtility::IsValidSignedImmediate(rel, 19));
			opcode.offset = rel;
			
			memcpy(literalBytes.data() + literalBytes.size() - 4, &opcode, 4);
			break;
		}
	case RelEncoding::Adrp:
		assert(!"Internal error: ADRP should never be simplified");
		break;
	case RelEncoding::Rel21HiLo:
		{
			union Opcode
			{
				uint32_t v;
				struct
				{
					uint32_t _dummy0 : 5;
					int32_t offsetHi : 19;
					uint32_t _dummy24 : 5;
					uint32_t offsetLo : 2;
					uint32_t _dummy31 : 1;
				};
			};
			static_assert(sizeof(Opcode) == 4, "Opcode should be 4 bytes");
			Opcode opcode;
			memcpy(&opcode, literalBytes.data() + literalBytes.size() - 4, 4);
			
			int32_t rel = (opcode.offsetHi << 2) | opcode.offsetLo;
			rel += int32_t(delta);
            assert(BitUtility::IsValidSignedImmediate(rel, 21));
			opcode.offsetLo = rel;
			opcode.offsetHi = rel >> 2;
			
			memcpy(literalBytes.data() + literalBytes.size() - 4, &opcode, 4);
			break;
		}
	case RelEncoding::Rel14Offset5:
		{
			union Opcode
			{
				uint32_t v;
				struct
				{
					uint32_t _dummy0 : 5;
					int32_t offset : 14;
					uint32_t _dummy24 : 13;
				};
			};
			static_assert(sizeof(Opcode) == 4, "Opcode should be 4 bytes");
			Opcode opcode;
			memcpy(&opcode, literalBytes.data() + literalBytes.size() - 4, 4);
			
			int32_t rel = opcode.offset;
			rel += int32_t(delta / 4);
            assert(BitUtility::IsValidSignedImmediate(rel, 14));
			opcode.offset = rel;
			
			memcpy(literalBytes.data() + literalBytes.size() - 4, &opcode, 4);
			break;
		}
	case RelEncoding::Imm12:
		assert(!"Internal error: Imm12 should never be simplified");
		break;
	case RelEncoding::Rel64:
		{
			uint64_t v;
			memcpy(&v, literalBytes.data() + literalBytes.size() - 8, 8);
			v += delta;
			memcpy(literalBytes.data() + literalBytes.size() - 8, &v, 8);
			break;
		}
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

	// Do not resolve ADRP or Imm12
	if(relEncoding == RelEncoding::Adrp || relEncoding == RelEncoding::Imm12) return false;
	
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

		result.push_back((int) arm64Assembler::ActionType::PatchLabel);
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
			result.push_back((int) arm64Assembler::ActionType::PatchLabelForward);
			break;
				
		case JumpType::Backward:
			result.push_back((int) arm64Assembler::ActionType::PatchLabelBackward);
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

	// Do not resolve ADRP or Imm12
	if(relEncoding == RelEncoding::Adrp || relEncoding == RelEncoding::Imm12) return false;
	
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
			result.push_back((int) arm64Assembler::ActionType::PatchLabelForward);
			break;
		case JumpType::Backward:
			result.push_back((int) arm64Assembler::ActionType::PatchLabelBackward);
			break;
		case JumpType::BackwardOrForward:
            context.AddForwardLabelReference(labelOperand.reference);
			result.push_back((int) arm64Assembler::ActionType::PatchLabel);
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
			result.push_back((int) arm64Assembler::ActionType::PatchLabelForward);
			break;
		case JumpType::Backward:
			result.push_back((int) arm64Assembler::ActionType::PatchLabelBackward);
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

	// Do not resolve ADRP or Imm12
	if(relEncoding == RelEncoding::Adrp || relEncoding == RelEncoding::Imm12) return false;
	
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
		result.push_back((int) arm64Assembler::ActionType::PatchExpressionLabelForward);
		break;
	case JumpType::Backward:
		result.push_back((int) arm64Assembler::ActionType::PatchExpressionLabelBackward);
		break;
	case JumpType::BackwardOrForward:
        context.AddForwardLabelReference(labelOperand.reference);
		result.push_back((int) arm64Assembler::ActionType::PatchExpressionLabel);
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
	result.push_back((int) arm64Assembler::ActionType::PatchAbsoluteAddress);
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
	result.push_back((int) arm64Assembler::ActionType::PatchExpression);
	WriteExpressionOffset(result, context, expressionIndex);
	result.push_back((int) relEncoding);
	WriteSigned16(result, -delay);
}

//============================================================================

PatchOpcodeAction::PatchOpcodeAction(Sign aSign,
									 uint8_t aNumberOfBits,
									 uint8_t aBitOffset,
									 uint8_t aValueShift,
									 int aExpressionIndex,
									 Assembler &assembler)
: DelayableAction(4),
  sign(aSign),
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
	printf(", %s%d@-%d=({%d}>>%d)<<%d", sign == Signed ? "simm" : "uimm", numberOfBits, delay, expressionIndex, valueShift, bitOffset);
}

bool PatchOpcodeAction::CanGroup(Action *other) const
{
	const PatchOpcodeAction *otherPatch = dynamic_cast<const PatchOpcodeAction*>(other);
	if(!otherPatch) return false;
	
	return expressionIndex == otherPatch->expressionIndex
	       && numberOfBits == otherPatch->numberOfBits
		   && bitOffset == otherPatch->bitOffset
		   && valueShift == otherPatch->valueShift;
}

void PatchOpcodeAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	if(CanGroup(context.previousAction))
	{
		result.push_back((int) arm64Assembler::ActionType::RepeatPatchOpcode);
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
		
		switch(sign)
		{
		case Signed:
			if(totalBits <= 8) result.push_back((int) arm64Assembler::ActionType::SignedPatchB1Opcode);
			else if(totalBits <= 16) result.push_back((int) arm64Assembler::ActionType::SignedPatchB2Opcode);
			else  result.push_back((int) arm64Assembler::ActionType::SignedPatchB4Opcode);
			break;
		case Unsigned:
			if(totalBits <= 8) result.push_back((int) arm64Assembler::ActionType::UnsignedPatchB1Opcode);
			else if(totalBits <= 16) result.push_back((int) arm64Assembler::ActionType::UnsignedPatchB2Opcode);
			else result.push_back((int) arm64Assembler::ActionType::UnsignedPatchB4Opcode);
			break;
		case Masked:
			if(totalBits <= 8) result.push_back((int) arm64Assembler::ActionType::MaskedPatchB1Opcode);
			else if(totalBits <= 16) result.push_back((int) arm64Assembler::ActionType::MaskedPatchB2Opcode);
			else result.push_back((int) arm64Assembler::ActionType::MaskedPatchB4Opcode);
			break;
		}
		
		// 8 bit patching writes a mask instead of number of bits.
		if(totalBits <= 8) result.push_back((1 << numberOfBits) - 1);
		else result.push_back(numberOfBits);
		
		result.push_back(bitOffset);
		result.push_back(shift);
		WriteExpressionOffset(result, context, expressionIndex, additionalOffset);
	}
	
	WriteSigned16(result, -delay);
}

//============================================================================

PatchLogicalImmediateOpcodeAction::PatchLogicalImmediateOpcodeAction(uint8_t aNumberOfBits,
									 int aExpressionIndex,
									 Assembler &assembler)
: DelayableAction(4),
  numberOfBits(aNumberOfBits),
  expressionIndex(aExpressionIndex)
{
	assert(numberOfBits == 32 || numberOfBits == 64);
	assembler.UpdateExpressionBitWidth(aExpressionIndex, numberOfBits);
}

void PatchLogicalImmediateOpcodeAction::Dump() const
{
	printf(", limm%d@-%d={%d}", numberOfBits, delay, expressionIndex);
}

bool PatchLogicalImmediateOpcodeAction::CanGroup(Action *other) const
{
	const PatchLogicalImmediateOpcodeAction *otherPatch = dynamic_cast<const PatchLogicalImmediateOpcodeAction*>(other);
	if(!otherPatch) return false;
	
	return expressionIndex == otherPatch->expressionIndex
			&& numberOfBits == otherPatch->numberOfBits;
}

void PatchLogicalImmediateOpcodeAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	if(CanGroup(context.previousAction))
	{
		result.push_back((int) arm64Assembler::ActionType::RepeatPatchOpcode);
	}
	else
	{
		switch(numberOfBits)
		{
		case 32:
			result.push_back((int) arm64Assembler::ActionType::LogicalImmediatePatchB4Opcode);
			break;
		case 64:
			result.push_back((int) arm64Assembler::ActionType::LogicalImmediatePatchB8Opcode);
			break;
		}
		WriteExpressionOffset(result, context, expressionIndex);
	}
	WriteSigned16(result, -delay);
}

//============================================================================

void ExpressionAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	switch(numberOfBytes)
	{
	case 1: result.push_back((int) arm64Assembler::ActionType::B1Expression); break;
	case 2: result.push_back((int) arm64Assembler::ActionType::B2Expression); break;
	case 4: result.push_back((int) arm64Assembler::ActionType::B4Expression); break;
	case 8: result.push_back((int) arm64Assembler::ActionType::B8Expression); break;
	default: assert(!"Unexpected expression bytes");
	}
	WriteExpressionOffset(result, context, expressionIndex);
}

//============================================================================

MovExpressionImmediateAction::MovExpressionImmediateAction(int aBitWidth, int aRegisterIndex, int aExpressionIndex, Assembler& assembler)
: bitWidth(aBitWidth), registerIndex(aRegisterIndex), expressionIndex(aExpressionIndex)
{
	assembler.UpdateExpressionBitWidth(expressionIndex, bitWidth);
}
	
void MovExpressionImmediateAction::Dump() const
{
	printf(", mov(%c%d, {%d})", bitWidth == 32 ? 'w' : 'x', registerIndex, expressionIndex);
}

void MovExpressionImmediateAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	switch(bitWidth)
	{
	case 32: result.push_back((int) arm64Assembler::ActionType::MovReg32Expression); break;
	case 64: result.push_back((int) arm64Assembler::ActionType::MovReg64Expression); break;
	}
	result.push_back(registerIndex);
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
			actionBytes.push_back((int) arm64Assembler::ActionType::Jump);
			if(bytes.size() >= 256) throw AssemblerException("Encoding error: Jump must be 256 bytes or less");
			actionBytes.push_back(bytes.size());
		}
// arm64 doesn't need Alternate markers
//		else
//		{
//			actionBytes.push_back((int) arm64Assembler::ActionType::EndAlternate);
//		}
		alternate.condition->WriteByteCode(actionBytes, context, *alternate.action);
		bytes.insert(bytes.begin(), actionBytes.begin(), actionBytes.end());
	}
	context.previousAction = nullptr;
//	result.push_back((int) arm64Assembler::ActionType::Alternate);
//	if((uint32_t) GetMaximumLength() >= 256) throw AssemblerException("Encoding error: Maximum alternate length must be less than 256 bytes");
//	result.push_back(GetMaximumLength());
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

//============================================================================
