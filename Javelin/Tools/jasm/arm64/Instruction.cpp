//============================================================================

#include "Javelin/Tools/jasm/arm64/Instruction.h"

#include "Javelin/Assembler/arm64/ActionType.h"
#include "Javelin/Assembler/BitUtility.h"
#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/CommandLine.h"
#include "Javelin/Tools/jasm/Log.h"
#include "Javelin/Tools/jasm/arm64/Action.h"
#include "Javelin/Tools/jasm/arm64/Assembler.h"
#include "Javelin/Tools/jasm/arm64/Encoder.h"

#include <assert.h>

//============================================================================

using namespace Javelin::Assembler::arm64;
using namespace Javelin::arm64Assembler;

//============================================================================

SyntaxOperand SyntaxOperand::Colon(Operand::Syntax::Colon, MatchColon);
SyntaxOperand SyntaxOperand::Comma(Operand::Syntax::Comma, MatchComma);
SyntaxOperand SyntaxOperand::ExclamationMark(Operand::Syntax::ExclamationMark, MatchExclamationMark);
SyntaxOperand SyntaxOperand::LeftSquareBracket(Operand::Syntax::LeftSquareBracket, MatchLeftSquareBracket);
SyntaxOperand SyntaxOperand::RightSquareBracket(Operand::Syntax::RightSquareBracket, MatchRightSquareBracket);
SyntaxOperand SyntaxOperand::LeftBrace(Operand::Syntax::LeftBrace, MatchLeftBrace);
SyntaxOperand SyntaxOperand::RightBrace(Operand::Syntax::RightBrace, MatchRightBrace);

//============================================================================

std::string Javelin::Assembler::arm64::MatchBitfieldDescription(uint64_t matchBitfield)
{
	std::string result;
	const char *const TAG_NAMES[] =
	{
		#define TAG(x,y) y,
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

std::string Javelin::Assembler::arm64::MatchBitfieldsDescription(const MatchBitfield *matchBitfields, int numberOfBitfields)
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
	
	if((value & 0xffffffffffff0000) == 0
	   || (value & 0xffffffff0000ffff) == 0
	   || (value & 0xffff0000ffffffff) == 0
	   || (value & 0x0000ffffffffffff) == 0)
	{
		matchBitfield |= MatchHwImm16;
	}
	
	if(int32_t(value | 0xffff) == -1
	   || int32_t(value | 0xffff0000) == -1)
	{
		matchBitfield |= MatchNot32HwImm16;
	}
	
	if((value | 0xffff) == -1
	   || (value | 0xffff0000) == -1
	   || (value | 0xffff00000000) == -1
	   || (value | 0xffff000000000000) == -1)
	{
		matchBitfield |= MatchNot64HwImm16;
	}

    if(BitUtility::IsValidUnsignedImmediate(value, 12)) matchBitfield |= MatchUImm12;
    if(BitUtility::IsValidUnsignedImmediate(value, 16)) matchBitfield |= MatchUImm16;

	if((value >> 32) == 0)
	{
		BitMaskEncodeResult result = EncodeBitMask(value | (value << 32));
		if(result.size != 0) matchBitfield |= MatchLogical32Imm;
	}

	BitMaskEncodeResult result = EncodeBitMask(value);
	if(result.size != 0) matchBitfield |= MatchLogical64Imm;

	if(IsExpression()) matchBitfield |= MatchExpressionImm;
	else if(value == 0) matchBitfield |= MatchImm0;
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

int EncodingVariant::GetNP1Count() const
{
	int count = 0;
	for(int i = 0; i < operandMatchMasksLength; ++i)
	{
		if(operandMatchMasks[i] & MatchRegNP1) ++count;
	}
	return count;
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
			
			(*encoder)(assembler,
					   listAction,
					   *this,
					   *encoding,
					   encodingOperands);

            return;
		}
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
