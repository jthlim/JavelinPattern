//============================================================================

#include "Javelin/Tools/jasm/arm64/Action.h"

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

using namespace Javelin::Assembler::arm64;

//============================================================================

const AlwaysAlternateActionCondition AlwaysAlternateActionCondition::instance;
const NeverAlternateActionCondition NeverAlternateActionCondition::instance;

//============================================================================

bool AlternateActionCondition::Equals(const AlternateActionCondition *other) const
{
	return this == other;
}

void AlwaysAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	// Do nothing.
}

AndAlternateActionCondition::~AndAlternateActionCondition()
{
	for(const AlternateActionCondition *condition : conditionList)
	{
		condition->Release();
	}
}

bool AndAlternateActionCondition::Equals(const AlternateActionCondition *other) const
{
	if(typeid(*this) != typeid(*other)) return false;
	AndAlternateActionCondition *o = (AndAlternateActionCondition*) other;
	if(conditionList.size() != o->conditionList.size()) return false;
	for(size_t i = 0; i < conditionList.size(); ++i)
	{
		if(!conditionList[i]->Equals(o->conditionList[i])) return false;
	}
	return true;
}

AlternateActionCondition::Result AndAlternateActionCondition::IsValid(ActionContext &context, const Action *action) const
{
	bool isAlways = true;
	for(const AlternateActionCondition *condition : conditionList)
	{
		switch(condition->IsValid(context, action))
		{
		case Result::Never:
			return Result::Never;
		case Result::Maybe:
			isAlways = false;
			break;
		case Result::Always:
			break;
		}
	}
	return isAlways ? Result::Always : Result::Maybe;
}

std::string AndAlternateActionCondition::GetDescription() const
{
	std::string result = "AND(";
	bool first = true;
	for(const AlternateActionCondition *condition : conditionList)
	{
		if (first) first = false;
		else result.push_back(',');
		result.append(condition->GetDescription());
	}
	result.push_back(')');
	return result;
}

void AndAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	// Each WriteByteCode wraps the previous condition, so write conditions in
	// reverse order so that they are processed in the same order that they're
	// written.
	for(auto it = conditionList.crbegin(); it != conditionList.crend(); ++it)
	{
		(*it)->WriteByteCode(result, context, action);
	}
}

void NeverAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	assert(!"NeverAlternateAction should never be serialized!");
}

void ImmediateAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action, int opcode) const
{
	// Prefix with condition code
	std::vector<uint8_t> conditionCode;
	conditionCode.push_back((int) opcode);
	Action::WriteExpressionOffset(conditionCode, context, expressionIndex);
	
	if(result.size() >= 256) throw AssemblerException("Encoding Error: Alternate jump offset should be less than 256 bytes");
	conditionCode.push_back(result.size());
	result.insert(result.begin(), conditionCode.begin(), conditionCode.end());
}

const AlternateActionCondition* ZeroAlternateActionCondition::Create(const Operand *operand)
{
	assert(operand->type == Operand::Type::Immediate
		   || operand->type == Operand::Type::Register);
	assert(operand->IsExpression());
	
	return new ZeroAlternateActionCondition(operand->expressionIndex);
}

std::string ZeroAlternateActionCondition::GetDescription() const
{
	char buffer[24];
	sprintf(buffer, "Zero{%d}", expressionIndex);
	return buffer;
}

bool ImmediateAlternateActionCondition::Equals(const AlternateActionCondition *other) const
{
	if(typeid(*this) != typeid(*other)) return false;
	ImmediateAlternateActionCondition *o = (ImmediateAlternateActionCondition*) other;
	return expressionIndex == o->expressionIndex;
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

// Returns true if any changes have occurred.
bool Action::ResolveRelativeAddresses(ActionContext &context)
{
	actionOffset = context.offset;
	ssize_t minimumLength = GetMinimumLength();
	ssize_t maximumLength = GetMaximumLength();
	if(!context.forwards)
	{
		minimumLength = -minimumLength;
		maximumLength = -maximumLength;
	}
	if(minimumLength != maximumLength)
	{
		context.offset.blockIndex++;
		context.offset.offsetIntoBlock = 0;
		context.offset.alignment = 0;
	}
	else
	{
		context.offset.offsetIntoBlock += minimumLength;
	}
	context.offset.totalMinimumOffset += minimumLength;
	context.offset.totalMaximumOffset += maximumLength;
	return false;
}

void Action::WriteSigned16(std::vector<uint8_t> &result, int16_t value)
{
	uint8_t data[sizeof(value)];
	memcpy(data, &value, sizeof(value));
	result.insert(result.end(), data, data+sizeof(value));
}

void Action::WriteUnsigned32(std::vector<uint8_t> &result, uint32_t value)
{
	uint8_t data[sizeof(value)];
	memcpy(data, &value, sizeof(value));
	result.insert(result.end(), data, data+sizeof(value));
}

void Action::WriteUnsignedVLE(std::vector<uint8_t> &result, uint32_t value)
{
	for(;;)
	{
		if(value < 128)
		{
			result.push_back(value);
			return;
		}
		result.push_back(value | 0x80);
		value >>= 7;
	}
}

void Action::WriteSignedVLE(std::vector<uint8_t> &result, int32_t value)
{
	int32_t sign = value >> 31;
	uint32_t unsignedValue = value;
	uint32_t encode = (unsignedValue << 1) ^ sign;
	WriteUnsignedVLE(result, encode);
}

void Action::WriteExpressionOffset(std::vector<uint8_t> &result,
								   const ActionWriteContext &context,
								   int expressionIndex,
								   int additionalOffset)
{
	static const int appendAssemblerReferenceSize = 12;
	assert(0 < expressionIndex && expressionIndex <= context.expressionInfo.size());
	int offset = context.expressionInfo[expressionIndex-1].offset
					+ appendAssemblerReferenceSize
					+ additionalOffset;
	assert((offset >> 16) == 0);
	result.push_back(offset);
	result.push_back(offset >> 8);
}

//============================================================================

void SetAssemblerVariableNameAction::Dump() const
{
	printf("\nassemblerVariableName:%s\n", variableName.c_str());
}

LiteralAction::LiteralAction(const std::vector<uint8_t> &aBytes) : bytes(aBytes)
{
}

LiteralAction::~LiteralAction()
{	
}

void LiteralAction::Dump() const
{
	printf(" \"");
	bool first = true;
	for(uint8_t value : bytes)
	{
		printf(first ? "0x%02x" : ", 0x%02x", value);
		first = false;
	}
	printf("\"");
}

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

void LiteralAction::AppendBytes(const std::vector<uint8_t>& extraBytes)
{
	bytes.insert(bytes.end(), extraBytes.begin(), extraBytes.end());
}

void LiteralAction::AppendBytes(const uint8_t *data, size_t length)
{
	bytes.insert(bytes.end(), data, data+length);
}

void LiteralAction::AppendToLiteral(LiteralAction *literal) const
{
	literal->AppendBytes(bytes);
}

bool LiteralAction::Simplify(ListAction *parent, size_t index)
{
	if(!index || !parent) return false;

	Action *previousAction = parent->GetActionAtIndex(index-1);
	if(!previousAction->IsLiteralAction()) return false;

	LiteralAction *literalAction = (LiteralAction *) previousAction;
	literalAction->AppendBytes(bytes);
	
	parent->RemoveActionAtIndex(index);
	delete this;
	return true;
}

//============================================================================

void AlignedAction::Dump() const
{
	printf(", aligned(%d)", alignment);
}

bool AlignedAction::ResolveRelativeAddresses(ActionContext &context)
{
	if(context.forwards == false) return false;
	
	context.offset.alignment = alignment;
	context.offset.blockIndex++;
	context.offset.offsetIntoBlock = 0;
	return false;
}

void AlignedAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	// No byte code emitted.
}

//============================================================================

void AlignAction::Dump() const
{
	printf(", align(%d)", alignment);
}

size_t AlignAction::GetMinimumLength() const
{
	if(isFixed) return fixedLength;
	return 0;
}

size_t AlignAction::GetMaximumLength() const
{
	if(isFixed) return fixedLength;
	return alignment-1;
}

bool AlignAction::ResolveRelativeAddresses(ActionContext &context)
{
	if(context.forwards && context.offset.alignment >= alignment)
	{
		isFixed = true;
		fixedLength = -context.offset.offsetIntoBlock & (alignment-1);
		return true;
	}
	else
	{
		return Action::ResolveRelativeAddresses(context);
	}
}

bool AlignAction::Simplify(ListAction *parent, size_t index)
{
	if(isFixed)
	{
		if(fixedLength == 0)
		{
			if(parent) parent->RemoveActionAtIndex(index);
			else parent->ReplaceActionAtIndex(index, new LiteralAction({}));
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
	result.push_back((int) arm64Assembler::ActionType::Align);
	int data = alignment - 1;
	assert(data == (uint8_t) data);
	result.push_back(data);
}

//============================================================================

void UnalignAction::Dump() const
{
	printf(", unalign(%d)", alignment);
}

size_t UnalignAction::GetMinimumLength() const
{
	if(isFixed) return fixedLength;
	return 0;
}

size_t UnalignAction::GetMaximumLength() const
{
	if(isFixed) return fixedLength;
	return 4;
}

bool UnalignAction::ResolveRelativeAddresses(ActionContext &context)
{
	if(context.forwards == false) return false;
	
	if(context.offset.alignment >= alignment)
	{
		isFixed = true;
		fixedLength = (context.offset.offsetIntoBlock & (alignment-1)) ? 0 : 1;
		return true;
	}
	else
	{
		Action::ResolveRelativeAddresses(context);
	}
	return false;
}

bool UnalignAction::Simplify(ListAction *parent, size_t index)
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
	result.push_back((int) arm64Assembler::ActionType::Unalign);
	int data = alignment - 1;
	assert(data == (uint8_t) data);
	result.push_back(data);
}

//============================================================================

bool LabelAction::Simplify(ListAction *parent, size_t index)
{
	if(global) return false;
	if(hasReference) return false;
	
	if(Log::IsVerboseEnabled())
	{
		printf("\n");
		Log::Verbose("Removing locallabel (0 references):");
		Dump();
		printf("\n");
	}
	
	// Local variable that has no references. Remove it.
	parent->RemoveActionAtIndex(index);
	delete this;
	return true;
}

void NamedLabelAction::Dump() const
{
	printf("\nLabel(%s%s)", global ? "*" : "", value.c_str());
}

bool NamedLabelAction::ResolveRelativeAddresses(ActionContext &context)
{
	if(context.forwards) hasReference = global;
	
	if(!global)
	{
		if(context.namedReferenceSet.find(value) != context.namedReferenceSet.end())
		{
			hasReference = true;
		}
	}
	context.namedLabels[value] = context.offset;
	return Inherited::ResolveRelativeAddresses(context);
}

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

void NumericLabelAction::Dump() const
{
	printf("\nLabel(%s%" PRId64 ")", global ? "*" : "", value);
}

bool NumericLabelAction::ResolveRelativeAddresses(ActionContext &context)
{
	// First pass reset code.
	if(context.forwards) hasReference = global;
	
	if(!global)
	{
		if(context.numericReferenceSet.find(value) != context.numericReferenceSet.end())
		{
			hasReference = true;
		}
	}
	
	context.numericLabels[global ? value ^ -1 : value] = context.offset;
	return Inherited::ResolveRelativeAddresses(context);
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

void ExpressionLabelAction::Dump() const
{
	printf("\nLabel(*{%d})", expressionIndex);
}

void ExpressionLabelAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	result.push_back((int) arm64Assembler::ActionType::ExpressionLabel);
	WriteExpressionOffset(result, context, expressionIndex);
	context.numberOfLabels++;
}

bool ExpressionLabelAction::ResolveRelativeAddresses(ActionContext &context)
{
	context.expressionLabels[expressionIndex] = context.offset;
	return Inherited::ResolveRelativeAddresses(context);
}

bool PatchLabelAction::Simplify(ListAction *parent, size_t index)
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
			memcpy(&opcode, literal->bytes.data() + literal->bytes.size() - 4, 4);
			
			int32_t rel = opcode.offset;
			rel += int32_t(delta / 4);
			assert(rel >> 26 == 0 || rel >> 26 == -1);
			opcode.offset = rel;
			
			memcpy(literal->bytes.data() + literal->bytes.size() - 4, &opcode, 4);
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
			memcpy(&opcode, literal->bytes.data() + literal->bytes.size() - 4, 4);
			
			int32_t rel = opcode.offset;
			rel += int32_t(delta / 4);
			assert(rel >> 19 == 0 || rel >> 19 == -1);
			opcode.offset = rel;
			
			memcpy(literal->bytes.data() + literal->bytes.size() - 4, &opcode, 4);
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
			memcpy(&opcode, literal->bytes.data() + literal->bytes.size() - 4, 4);
			
			int32_t rel = (opcode.offsetHi << 2) | opcode.offsetLo;
			rel += int32_t(delta);
			assert(rel >> 21 == 0 || rel >> 21 == -1);
			opcode.offsetLo = rel;
			opcode.offsetHi = rel >> 2;
			
			memcpy(literal->bytes.data() + literal->bytes.size() - 4, &opcode, 4);
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
			memcpy(&opcode, literal->bytes.data() + literal->bytes.size() - 4, 4);
			
			int32_t rel = opcode.offset;
			rel += int32_t(delta / 4);
			assert(rel >> 14 == 0 || rel >> 14 == -1);
			opcode.offset = rel;
			
			memcpy(literal->bytes.data() + literal->bytes.size() - 4, &opcode, 4);
			break;
		}
	case RelEncoding::Imm12:
		assert(!"Internal error: Imm12 should never be simplified");
		break;
	case RelEncoding::Rel64:
		{
			uint64_t v;
			memcpy(&v, literal->bytes.data() + literal->bytes.size() - 8, 8);
			v += delta;
			memcpy(literal->bytes.data() + literal->bytes.size() - 8, &v, 8);
			break;
		}
	}
	parent->RemoveActionAtIndex(index);
	delete this;
	return true;
}

void PatchNameLabelAction::Dump() const
{
	const char *JUMP_TYPES[] = { "", ":b", ":f", ":bf" };
	
	printf(", patch(renc%d, %s%s%s)",
		   (int) relEncoding,
		   global ? "*" : "",
		   value.c_str(),
		   JUMP_TYPES[(int) jumpType]);
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

	jumpType = context.forwards ? Operand::JumpType::Backward : Operand::JumpType::Forward;

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
		context.IncrementNumberOfForwardLabelReferences();
		
		result.push_back((int) arm64Assembler::ActionType::PatchLabel);
		WriteUnsigned32(result, labelId);
	}
	else
	{
		uint32_t index = context.GetIndexForLabelId(labelId);
		switch(jumpType)
		{
		case Operand::JumpType::Name:
		case Operand::JumpType::BackwardOrForward:
			throw AssemblerException("Internal error: Unexpected jumpType");
				
		case Operand::JumpType::Forward:
			context.IncrementNumberOfForwardLabelReferences();
			result.push_back((int) arm64Assembler::ActionType::PatchLabelForward);
			break;
				
		case Operand::JumpType::Backward:
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
	const char *JUMP_TYPES[] = { "", "f", "b", "bf" };
	printf(", patch(renc%d, %s%" PRId64 "%s)", (int) relEncoding, global ? "*" : "", value, JUMP_TYPES[(int)jumpType]);
	if(hasFixedDelta) printf("=%" PRId64, delta);
}

bool PatchNumericLabelAction::ResolveRelativeAddresses(ActionContext &context)
{
	Inherited::ResolveRelativeAddresses(context);
	
	if(!global)
	{
		if(context.forwards)
		{
			if(jumpType == Operand::JumpType::Forward)
			{
				context.numericReferenceSet.insert(value);
				return false;
			}
		}
		else
		{
			if(jumpType == Operand::JumpType::Backward)
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
		if(jumpType == Operand::JumpType::Forward) return false;
	}
	else
	{
		if(jumpType == Operand::JumpType::Backward) return false;
	}
	
	ActionContext::NumericLabelMap::const_iterator it = context.numericLabels.find(global ? value ^ -1 : value);
	if(it == context.numericLabels.end()) return false;
	hasFoundTarget = true;

	jumpType = context.forwards ? Operand::JumpType::Backward : Operand::JumpType::Forward;

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
		switch(jumpType)
		{
		case Operand::JumpType::Name:
			throw AssemblerException("Internal error: Unexpected jumpType");
		case Operand::JumpType::Forward:
			// Forwards
			context.IncrementNumberOfForwardLabelReferences();
			result.push_back((int) arm64Assembler::ActionType::PatchLabelForward);
			break;
		case Operand::JumpType::Backward:
			result.push_back((int) arm64Assembler::ActionType::PatchLabelBackward);
			break;
		case Operand::JumpType::BackwardOrForward:
			context.IncrementNumberOfForwardLabelReferences();
			result.push_back((int) arm64Assembler::ActionType::PatchLabel);
			break;
		}
		WriteUnsigned32(result, GetLabelIdForGlobalNumeric(labelOperand.labelValue));
	}
	else
	{
		int64_t labelId = GetLabelIdForNumeric(labelOperand.labelValue);
		int index = context.GetIndexForLabelId(labelId);
		switch(jumpType)
		{
		case Operand::JumpType::Name:
			throw AssemblerException("Internal error: Unexpected jumpType");
		case Operand::JumpType::Forward:
			context.IncrementNumberOfForwardLabelReferences();
			result.push_back((int) arm64Assembler::ActionType::PatchLabelForward);
			break;
		case Operand::JumpType::Backward:
			result.push_back((int) arm64Assembler::ActionType::PatchLabelBackward);
			break;
		case Operand::JumpType::BackwardOrForward:
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
	const char* JUMP_TYPES[] = { "", "f", "b", "bf" };
	
	printf(", patch(renc%d, *{%d}%s)",
		   (int) relEncoding,
		   labelOperand.expressionIndex,
		   JUMP_TYPES[(int)labelOperand.jumpType]);
}

bool PatchExpressionLabelAction::ResolveRelativeAddresses(ActionContext &context)
{
	Inherited::ResolveRelativeAddresses(context);
	
	// Already resolved!
	if(hasFixedDelta) return false;
	
	if(context.forwards)
	{
		if(labelOperand.jumpType == Operand::JumpType::Forward) return false;
	}
	else
	{
		if(labelOperand.jumpType == Operand::JumpType::Backward) return false;
	}
	
	ActionContext::ExpressionLabelMap::const_iterator it = context.expressionLabels.find(labelOperand.expressionIndex);
	if(it == context.expressionLabels.end()) return false;
	hasFoundTarget = true;

	labelOperand.jumpType = context.forwards ? Operand::JumpType::Backward : Operand::JumpType::Forward;

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
	case Operand::JumpType::Name:
		throw AssemblerException("Internal error: Unexpected jumpType");
	case Operand::JumpType::Forward:
		context.IncrementNumberOfForwardLabelReferences();
		result.push_back((int) arm64Assembler::ActionType::PatchExpressionLabelForward);
		break;
	case Operand::JumpType::Backward:
		result.push_back((int) arm64Assembler::ActionType::PatchExpressionLabelBackward);
		break;
	case Operand::JumpType::BackwardOrForward:
		context.IncrementNumberOfForwardLabelReferences();
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

void ExpressionAction::Dump() const
{
	printf(", #%d={%d}", numberOfBytes, expressionIndex);
}

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

AlternateAction::AlternateAction()
{
}

AlternateAction::~AlternateAction()
{
	for(const Alternate &alternate : alternateList)
	{
		alternate.condition->Release();
		delete alternate.action;
	}
}

void AlternateAction::Dump() const
{
	printf(" (");

	bool isFirst = true;
	for(const Alternate &alternate : alternateList)
	{
		printf(isFirst ? "[%s]" : " | [%s]", alternate.condition->GetDescription().c_str());
		isFirst = false;
		alternate.action->Dump();
	}

	printf(")");
}

size_t AlternateAction::GetMinimumLength() const
{
	if(alternateList.size() == 0) return 0;
	
	size_t minimumLength = (size_t) -1;
	for(const Alternate &alternate : alternateList)
	{
		size_t length = alternate.action->GetMinimumLength();
		if(minimumLength > length) minimumLength = length;
	}
	return minimumLength;
}

size_t AlternateAction::GetMaximumLength() const
{
	size_t maximumLength = 0;
	for(const Alternate &alternate : alternateList)
	{
		size_t length = alternate.action->GetMaximumLength();
		if(maximumLength < length) maximumLength = length;
	}
	return maximumLength;
}

Action *AlternateAction::GetSingleAlternateAndClearAlternateList()
{
	assert(alternateList.size() == 1);
	Action *action = alternateList[0].action;
	alternateList[0].condition->Release();
	alternateList.clear();
	return action;
}

bool AlternateAction::IsLiteral() const
{
	return alternateList.size() == 1
			&& alternateList[0].action->IsLiteral();
}

void AlternateAction::AppendToLiteral(LiteralAction *literal) const
{
	for(const Alternate& alternate : alternateList)
	{
		alternate.action->AppendToLiteral(literal);
	}
}

void AlternateAction::Add(Action *action, const AlternateActionCondition *condition)
{
	for(const Alternate& alternate : alternateList)
	{
		if(alternate.condition->Equals(condition))
		{
			delete action;
			condition->Release();
			return;
		}
	}
	
	const Alternate alternate =
	{
		.condition = condition,
		.action = action,
	};
	alternateList.push_back(alternate);
}

bool AlternateAction::ResolveRelativeAddresses(ActionContext &context)
{
	bool changed = false;
	ActionOffset offset = context.offset;

	for(int i = 0; i < alternateList.size(); ++i)
	{
		Alternate &alternate = alternateList[i];
		
		context.offset = offset;
		if(context.forwards)
		{
			if(alternate.action->ResolveRelativeAddresses(context)) changed = true;
		}

		AlternateActionCondition::Result result = alternate.condition->IsValid(context, alternate.action);
		
		if(!context.forwards)
		{
			if(alternate.action->ResolveRelativeAddresses(context)) changed = true;
		}
		
		switch(result)
		{
		case AlternateActionCondition::Result::Never:
			// Remove this entry, and continue.
			alternate.condition->Release();
			delete alternate.action;
			alternateList.erase(alternateList.begin()+i);
			--i;
			changed = true;
			break;
		case AlternateActionCondition::Result::Maybe:
			break;
		case AlternateActionCondition::Result::Always:
			if(alternate.condition == &AlwaysAlternateActionCondition::instance) break;
			alternate.condition->Release();
			alternate.condition = &AlwaysAlternateActionCondition::instance;
			for(int j = i+1; j < alternateList.size(); ++j)
			{
				alternateList[j].condition->Release();
				delete alternateList[j].action;
			}
				
			alternateList.erase(alternateList.begin()+i+1, alternateList.end());
			changed = true;
			goto end;
		}
		
	}

end:
	context.offset = offset;
	Action::ResolveRelativeAddresses(context);
	
	return changed;
}

bool AlternateAction::Simplify(ListAction *parent, size_t index)
{
	bool result = false;
	for(size_t i = alternateList.size(); i != 0;)
	{
		--i;
		if(alternateList[i].action->Simplify(nullptr, 0)) result = true;
	}
	
	switch(alternateList.size())
	{
	case 0:
		parent->RemoveActionAtIndex(index);
		delete this;
		return true;
	case 1:
		if(alternateList[0].condition == &AlwaysAlternateActionCondition::instance)
		{
			parent->ReplaceActionAtIndex(index, alternateList[0].action);
			alternateList[0].condition->Release();
			alternateList.clear();
			delete this;
			return true;
		}
		break;
	}
	return result;
}

void AlternateAction::DelayAndConsolidate()
{
	for(const Alternate& alternate : alternateList)
	{
		alternate.action->DelayAndConsolidate();
	}
}

void AlternateAction::Group()
{
	for(const Alternate& alternate : alternateList)
	{
		alternate.action->Group();
	}
}

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

ListAction::ListAction()
{
}

ListAction::~ListAction()
{
	for(Action *action : actionList)
	{
		delete action;
	}
}

void ListAction::Append(Action *action)
{
	if(action->IsLiteral()
	   && actionList.size() > 0
	   && actionList.back()->IsLiteralAction())
	{
		LiteralAction* lastLiteralAction = (LiteralAction*) actionList.back();
		action->AppendToLiteral(lastLiteralAction);
		delete action;
	}
	else
	{
		actionList.push_back(action);
	}
}

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

void ListAction::Dump() const
{
	printf("[");
	for(Action *action : actionList)
	{
		action->Dump();
	}
	printf("]");
}

size_t ListAction::GetMinimumLength() const
{
	size_t minimumLength = 0;
	for(Action *action : actionList)
	{
		minimumLength += action->GetMinimumLength();
	}
	return minimumLength;
}

size_t ListAction::GetMaximumLength() const
{
	size_t maximumLength = 0;
	for(Action *action : actionList)
	{
		maximumLength += action->GetMaximumLength();
	}
	return maximumLength;
}

bool ListAction::IsLiteral() const
{
	for(Action *action : actionList)
	{
		if(!action->IsLiteral()) return false;
	}
	return true;
}

void ListAction::ResolveRelativeAddresses()
{
	bool changes;
	do
	{
		changes = false;

		{
			ActionContext actionContext;
			actionContext.forwards = true;
			actionContext.offset.alignment = 0;
			actionContext.offset.blockIndex = 0;
			actionContext.offset.offsetIntoBlock = 0;
			actionContext.offset.totalMinimumOffset = 0;
			actionContext.offset.totalMaximumOffset = 0;
			if(ResolveRelativeAddresses(actionContext)) changes = true;
		}

		{
			ActionContext actionContext;
			actionContext.forwards = false;
			actionContext.offset.alignment = 0;
			actionContext.offset.blockIndex = 0;
			actionContext.offset.offsetIntoBlock = 0;
			actionContext.offset.totalMinimumOffset = 0;
			actionContext.offset.totalMaximumOffset = 0;
			if(ResolveRelativeAddresses(actionContext)) changes = true;
		}
		
		if(Simplify(nullptr, 0)) changes = true;
		
	} while(changes);
}

bool ListAction::Simplify(ListAction *parent, size_t index)
{
	bool changes = false;
	for(size_t i = actionList.size(); i != 0;)
	{
		--i;
		if(actionList[i]->Simplify(this, i)) changes = true;
	}

	if(!parent) return changes;

	parent->ReplaceActionAtIndexWithList(index, actionList);
	actionList.clear();
	delete this;
	return true;
}

void ListAction::DelayAndConsolidate()
{
	for(Action* action : actionList)
	{
		action->DelayAndConsolidate();
	}
	
	// Step through all actions, and find out if there are
	// actions that can be delayed past literals
	std::vector<Action *> delayList;
	bool needConsolidate = false;
	int delay = 0;
	int startDelayListIndex = 0;
	for(int i = 0; i < actionList.size(); ++i)
	{
		Action *action = actionList[i];
		if(action->IsLiteralAction())
		{
			if(delayList.size() == 0) continue;
			LiteralAction *literalAction = (LiteralAction *)action;
			if(!delayList[0]->CanDelay(delay + literalAction->GetNumberOfBytes()))
			{
				needConsolidate = true;
				DelayActions(delayList, startDelayListIndex, i, delay);
				delayList.clear();
				delay = 0;
			}
			else
			{
				delay += literalAction->GetNumberOfBytes();
			}
		}
		else if(action->CanDelay())
		{
			if(delayList.size() == 0) startDelayListIndex = i;
			delayList.push_back(action);
		}
		else
		{
			// Not literal, and cannot delay.
			if(delay != 0)
			{
				needConsolidate = true;
				DelayActions(delayList, startDelayListIndex, i, delay);
			}
			if(delayList.size())
			{
				delayList.clear();
				delay = 0;
			}
		}
	}
	if(delay != 0)
	{
		needConsolidate = true;
		DelayActions(delayList, startDelayListIndex, (int) actionList.size(), delay);
	}
	
	if(needConsolidate) ConsolidateLiteralActions();
}

void ListAction::Group()
{
	for(int i = 0; i < actionList.size(); ++i)
	{
		Action *action = actionList[i];
		if(!action->CanGroup()) continue;

		int next = i+1;
		for(int j = i+1; j < actionList.size(); ++j)
		{
			Action *other = actionList[j];
			if(other->GetMinimumLength() != 0) break;
			
			if(action->CanGroup(other))
			{
				if(j == next) ++next;
				else
				{
					actionList.erase(actionList.begin()+j);
					actionList.insert(actionList.begin()+next, other);
					++i;
				}
			}
		}
	}
}

void ListAction::DelayActions(const std::vector<Action *> &delayList, int startIndex, int toIndex, int delay)
{
	actionList.insert(actionList.begin()+toIndex, delayList.begin(), delayList.end());
	int index = startIndex;
	int delayListIndex = 0;
	while(delayListIndex < delayList.size())
	{
		Action *action = actionList[index];
		if(action == delayList[delayListIndex])
		{
			assert(action->CanDelay(delay));
			action->Delay(delay);
			actionList.erase(actionList.begin()+index);
			++delayListIndex;
			continue;
		}
		else if(action->IsLiteralAction())
		{
			delay -= action->GetMinimumLength();
		}

		++index;
	}
}

void ListAction::ConsolidateLiteralActions()
{
	for(size_t i = actionList.size(); i >= 2; --i)
	{
		Action *a2 = actionList[i-1];
		Action *a1 = actionList[i-2];
		if(a1->IsLiteralAction() && a2->IsLiteralAction())
		{
			a2->AppendToLiteral((LiteralAction *) a1);
			delete a2;
			actionList.erase(actionList.begin() + i-1);
		}
	}
}

void ListAction::ReplaceActionAtIndexWithList(size_t i, std::vector<Action*> &newActionList)
{
	actionList.erase(actionList.begin()+i);
	actionList.insert(actionList.begin()+i, newActionList.begin(), newActionList.end());
}

bool ListAction::ResolveRelativeAddresses(ActionContext &context)
{
	bool changed = false;
	if(context.forwards)
	{
		for(Action *action : actionList)
		{
			if(action->ResolveRelativeAddresses(context)) changed = true;
		}
	}
	else
	{
		for(size_t i = actionList.size(); i != 0;)
		{
			--i;
			if(actionList[i]->ResolveRelativeAddresses(context)) changed = true;
		}
	}
	return changed;
}

void ListAction::AppendToLiteral(LiteralAction *literal) const
{
	for(Action *action : actionList)
	{
		action->AppendToLiteral(literal);
	}
}

void ListAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	for(Action *action : actionList)
	{
		action->WriteByteCode(result, context);
		context.previousAction = action;
	}
}

//============================================================================
