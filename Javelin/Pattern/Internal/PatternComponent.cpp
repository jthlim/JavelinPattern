//============================================================================

#include "Javelin/Container/EnumSet.h"
#include "Javelin/Pattern/Internal/PatternComponent.h"
#include "Javelin/Pattern/Internal/PatternInstruction.h"
#include "Javelin/Pattern/Internal/PatternInstructionList.h"
#include "Javelin/Stream/ICharacterWriter.h"
#include <typeinfo>

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

void IComponent::Dump(ICharacterWriter& output, int depth)
{
	output.PrintF("%r%s\n", ' ', depth, typeid(*this).name());
}

JINLINE bool IComponent::HasSameClass(const IComponent* other) const
{
	void* vtbl1 = *((void**) this);
	void* vtbl2 = *((void**) other);
	return vtbl1 == vtbl2;
}

//============================================================================

uint32_t BackReferenceComponent::GetMinimumLength() const
{
	return captureComponent->GetMinimumLength();
}

uint32_t BackReferenceComponent::GetMaximumLength() const
{
	return captureComponent->GetMaximumLength();
}

bool BackReferenceComponent::IsFixedLength() const
{
	return captureComponent->IsFixedLength();
}

void BackReferenceComponent::BuildInstructions(InstructionList &instructionList) const
{
	instructionList.AddInstruction(new BackReferenceInstruction((uint32_t) captureComponent->captureIndex));
}

bool BackReferenceComponent::IsEqual(const IComponent* other) const
{
	if(!HasSameClass(other)) return false;
	return captureComponent == ((BackReferenceComponent*) other)->captureComponent;
}

//============================================================================

void ByteComponent::Dump(ICharacterWriter& output, int depth)
{
	output.PrintF("%r%s: '%c'\n", ' ', depth, typeid(*this).name(), c);
}

void ByteComponent::BuildInstructions(InstructionList &instructionList) const
{
	ByteInstruction* instruction = new ByteInstruction(c);	
	instructionList.AddInstruction(instruction);
}

void ByteComponent::AddToCharacterRangeList(CharacterRangeList& rangeList) const
{
	rangeList.Add(c);
}

bool ByteComponent::IsEqual(const IComponent* other) const
{
	// TODO: Support equality against a characterRangeList that matches
	if(!HasSameClass(other)) return false;
	return c == ((ByteComponent*) other)->c;
}

//============================================================================

void AssertComponent::Dump(ICharacterWriter& output, int depth)
{
	static constexpr const char *const ASSERT_TYPE_NAMES[] =
	{
		"StartOfInput",
		"EndOfInput",
		"StartOfLine",
		"EndOfLine",
		"WordBoundary",
		"NotWordBoundary",
		"StartOfSearch",
	};
	
	output.PrintF("%r%s: %s\n", ' ', depth, typeid(*this).name(), ASSERT_TYPE_NAMES[(int) assertType]);
}

void AssertComponent::BuildInstructions(InstructionList &instructionList) const
{
	if(!instructionList.ShouldEmitAssertInstructions()) return;
	
	Instruction* instruction = nullptr;
	switch(assertType)
	{
	case AssertType::StartOfInput:			instruction = new AssertStartOfInputInstruction;		break;
	case AssertType::EndOfInput:			instruction = new AssertEndOfInputInstruction;			break;
	case AssertType::StartOfLine:			instruction = new AssertStartOfLineInstruction;			break;
	case AssertType::EndOfLine:				instruction = new AssertEndOfLineInstruction;			break;
	case AssertType::WordBoundary:			instruction = new AssertWordBoundaryInstruction;		break;
	case AssertType::NotWordBoundary:		instruction = new AssertNotWordBoundaryInstruction;		break;
	case AssertType::StartOfSearch:			instruction = new AssertStartOfSearchInstruction;		break;
	}
	JASSERT(instruction);
	instructionList.AddInstruction(instruction);
}

bool AssertComponent::IsEqual(const IComponent* other) const
{
	if(!HasSameClass(other)) return false;
	return assertType == ((AssertComponent*) other)->assertType;
}

bool AssertComponent::HasStartAnchor() const
{
	return assertType == AssertType::StartOfInput || assertType == AssertType::StartOfSearch;
}

bool AssertComponent::HasEndAnchor() const
{
	return assertType == AssertType::EndOfInput;
}

bool AssertComponent::RequiresAnyByteMinimalForPartialMatch() const
{
	switch(assertType)
	{
	case AssertType::StartOfInput:			return false;
	case AssertType::EndOfInput:			return true;
	case AssertType::StartOfLine:			return true;
	case AssertType::EndOfLine:				return true;
	case AssertType::WordBoundary:			return true;
	case AssertType::NotWordBoundary:		return true;
	case AssertType::StartOfSearch:			return false;
	}
}

//============================================================================

void TerminalComponent::Dump(ICharacterWriter& output, int depth)
{
	output.PrintF("%r%s: %s\n", ' ', depth, typeid(*this).name(), isSuccess ? "Success" : "Fail");
}

void TerminalComponent::BuildInstructions(InstructionList &instructionList) const
{
	instructionList.AddInstruction(isSuccess ? (Instruction*) new SuccessInstruction : (Instruction*) new FailInstruction);
}

bool TerminalComponent::IsEqual(const IComponent* other) const
{
	if(!HasSameClass(other)) return false;
	return isSuccess == ((TerminalComponent*) other)->isSuccess;
}

bool TerminalComponent::RequiresAnyByteMinimalForPartialMatch() const
{
	return false;
}

//============================================================================

CharacterRangeListComponent::CharacterRangeListComponent(bool aUseUtf8, const CharacterRangeList& aCharacterRangeList)
: useUtf8(aUseUtf8),
  characterRangeList(aUseUtf8 ? aCharacterRangeList : aCharacterRangeList.CreateAsciiRange())
{
}

CharacterRangeListComponent::CharacterRangeListComponent(bool aUseUtf8, const CharacterRange& range)
: useUtf8(aUseUtf8)
{
	characterRangeList.Append(range);
}

void CharacterRangeListComponent::Dump(ICharacterWriter& output, int depth)
{
	output.PrintF("%r%s: ", ' ', depth, typeid(*this).name());
	characterRangeList.Dump(output);
	output.WriteByte('\n');
}

bool CharacterRangeListComponent::IsByte() const
{
	return !useUtf8 || characterRangeList.IsSingleByteUtf8();
}

bool CharacterRangeListComponent::HasSpecificRepeatInstruction() const
{
	if(useUtf8
	   && characterRangeList.GetCount() == 2
	   && characterRangeList[0].min == 0
	   && characterRangeList[1].max == Character::Maximum()
	   && characterRangeList[1].min < 128)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CharacterRangeListComponent::BuildRepeatInstructions(InstructionList &instructionList) const
{
	if(HasSpecificRepeatInstruction())
	{
		int min = characterRangeList[0].max+1;
		int max = characterRangeList[1].min-1;
		if(min == max)
		{
			instructionList.AddInstruction(new ByteNotInstruction(min));
		}
		else
		{
			ByteNotRangeInstruction* instruction = new ByteNotRangeInstruction;
			instruction->byteRange.min = min;
			instruction->byteRange.max = max;
			instructionList.AddInstruction(instruction);
		}
	}
	else
	{
		BuildInstructions(instructionList);
	}
}

//bool CharacterRangeListComponent::IsAnyByte() const
//{
//	return !useUtf8 && characterRangeList.GetCount() == 1 && characterRangeList[0] == CharacterRange{0, 255};
//}

bool CharacterRangeListComponent::IsFixedLength() const
{
	if(IsByte()) return true;

	Utf8Character low = Utf8Character{characterRangeList.Front().min};
	Utf8Character high = Utf8Character{characterRangeList.Back().max};
	return low.GetNumberOfBytes() == high.GetNumberOfBytes();
}

bool CharacterRangeListComponent::RequiresAnyByteMinimalForPartialMatch() const
{
	return true;
}

void CharacterRangeListComponent::AddToCharacterRangeList(CharacterRangeList& rangeList) const
{
	for(const CharacterRange& cr : characterRangeList)
	{
		rangeList.Add(cr);
	}
}

void CharacterRangeListComponent::BuildInstructions(InstructionList &instructionList) const
{
	if(characterRangeList.IsEmpty())
	{
		instructionList.AddInstruction(new FailInstruction);
	}
	else if(!useUtf8 || characterRangeList.IsSingleByteUtf8())
	{
		uint32_t numberOfMatchingBytes = characterRangeList.GetNumberOfMatchingBytes();
		
		if(numberOfMatchingBytes == 1)
		{
			ByteInstruction* instruction = new ByteInstruction(characterRangeList[0].min);
			instructionList.AddInstruction(instruction);
			return;
		}
		
		if(numberOfMatchingBytes == 256)
		{
			instructionList.AddInstruction(new AnyByteInstruction);
			return;
		}
		
		if(characterRangeList.GetCount() == 1)
		{
			// Use a ByteRangeInstruction
			ByteRangeInstruction* instruction = new ByteRangeInstruction;
			instruction->byteRange.min = characterRangeList[0].min;
			instruction->byteRange.max = characterRangeList[0].max;
			instructionList.AddInstruction(instruction);
			return;
		}
		
		if(characterRangeList.GetCount() == 2 &&
		   characterRangeList[0].min == 0 &&
		   characterRangeList[1].max == 255)
		{
			if(characterRangeList[0].max+1 == characterRangeList[1].min-1)
			{
				instructionList.AddInstruction(new ByteNotInstruction(characterRangeList[0].max+1));
			}
			else
			{
				ByteNotRangeInstruction* instruction = new ByteNotRangeInstruction;
				instruction->byteRange.min = characterRangeList[0].max+1;
				instruction->byteRange.max = characterRangeList[1].min-1;
				instructionList.AddInstruction(instruction);
			}
			return;
		}
		
		if(numberOfMatchingBytes == 2 || numberOfMatchingBytes == 3)
		{
			size_t matchingByteOffset = 0;
			unsigned char matchingBytes[3];
			
			for(const auto& it : characterRangeList)
			{
				for(uint32_t c = it.min; c <= it.max; ++c)
				{
					matchingBytes[matchingByteOffset++] = c;
				}
			}
			
			if(numberOfMatchingBytes == 2)
			{
				if(matchingBytes[0]+1 == matchingBytes[1])
				{
					ByteRangeInstruction* instruction = new ByteRangeInstruction(matchingBytes[0], matchingBytes[1]);
					instructionList.AddInstruction(instruction);
				}
				else
				{
					ByteEitherOf2Instruction* instruction = new ByteEitherOf2Instruction;
					instruction->c[0] = matchingBytes[0];
					instruction->c[1] = matchingBytes[1];
					instructionList.AddInstruction(instruction);
				}
			}
			else
			{
				if(matchingBytes[0]+2 == matchingBytes[2])
				{
					ByteRangeInstruction* instruction = new ByteRangeInstruction(matchingBytes[0], matchingBytes[2]);
					instructionList.AddInstruction(instruction);
				}
				else
				{
					ByteEitherOf3Instruction* instruction = new ByteEitherOf3Instruction;
					instruction->c[0] = matchingBytes[0];
					instruction->c[1] = matchingBytes[1];
					instruction->c[2] = matchingBytes[2];
					instructionList.AddInstruction(instruction);
				}
			}
			return;
		}
		
		if(numberOfMatchingBytes >= 253)
		{
			unsigned char notMatchingBytes[3];
			unsigned int notMatchingByteIndex = 0;
			
			for(uint32_t j = 0; j < characterRangeList[0].min; ++j)
			{
				notMatchingBytes[notMatchingByteIndex++] = j;
			}
			
			for(size_t i = 1; i < characterRangeList.GetCount(); ++i)
			{
				for(uint32_t j = characterRangeList[i-1].max+1; j < characterRangeList[i].min; ++j)
				{
					notMatchingBytes[notMatchingByteIndex++] = j;
				}
			}
			
			for(uint32_t j = characterRangeList.Back().max+1; j < 256; ++j)
			{
				notMatchingBytes[notMatchingByteIndex++] = j;
			}
			
			JASSERT(numberOfMatchingBytes + notMatchingByteIndex == 256);
			
			switch(notMatchingByteIndex)
			{
			case 1:
				instructionList.AddInstruction(new ByteNotInstruction(notMatchingBytes[0]));
				return;
					
			case 2:
				instructionList.AddInstruction(new ByteNotEitherOf2Instruction(notMatchingBytes[0], notMatchingBytes[1]));
				return;

			case 3:
				instructionList.AddInstruction(new ByteNotEitherOf3Instruction(notMatchingBytes[0], notMatchingBytes[1], notMatchingBytes[2]));
				return;
			}
		}

		// Use ByteBitMask instruction
		ByteBitMaskInstruction* instruction = new ByteBitMaskInstruction;
		for(const CharacterRange& range : characterRangeList)
		{
			for(int i = range.min; i <= range.max; ++i)
			{
				instruction->bitMask.SetBit(i);
			}
		}
		instructionList.AddInstruction(instruction);
	}
	else if(characterRangeList.GetCount() == 1 &&
	   characterRangeList[0] == CharacterRange{0, Character::Maximum()})
	{
		//	7	U+0000		U+007F		1	0xxxxxxx
		//	11	U+0080		U+07FF		2	110xxxxx	10xxxxxx
		//	16	U+0800		U+FFFF		3	1110xxxx	10xxxxxx	10xxxxxx
		//	21	U+10000		U+1FFFFF	4	11110xxx	10xxxxxx	10xxxxxx	10xxxxxx
		//	26	U+200000	U+3FFFFFF	5	111110xx	10xxxxxx	10xxxxxx	10xxxxxx	10xxxxxx
		//	31	U+4000000	U+7FFFFFFF	6	1111110x	10xxxxxx	10xxxxxx	10xxxxxx	10xxxxxx	10xxxxxx
		
		// This is [0x00-0x7f]
		// | [0xc0-0xdf] [0x80-0xbf]
		// | [0xe0-0xef] [0x80-0xbf] [0x80-0xbf]
		// | [0xf0-0xf7] [0x80-0xbf] [0x80-0xbf] [0x80-0xbf]
		// | [0xf8-0xfb] [0x80-0xbf] [0x80-0xbf] [0x80-0xbf] [0x80-0xbf]
		// | [0xfc-0xfd] [0x80-0xbf] [0x80-0xbf] [0x80-0xbf] [0x80-0xbf] [0x80-0xbf]
		
		SplitInstruction* split = new SplitInstruction;
		instructionList.AddInstruction(split);
		
		InstructionTable &targetList = split->targetList;
		targetList.Append(new ByteRangeInstruction(0,    0x7f));
		targetList.Append(new ByteRangeInstruction(0xc0, 0xdf));
		targetList.Append(new ByteRangeInstruction(0xe0, 0xef));
		targetList.Append(new ByteRangeInstruction(0xf0, 0xf7));
		targetList.Append(new ByteRangeInstruction(0xf8, 0xfb));
		targetList.Append(new ByteRangeInstruction(0xfc, 0xfd));
		
		JumpInstruction* jump[5];
		for(size_t i = 0; i < 5; ++i)
		{
			jump[i] = new JumpInstruction;
			instructionList.AddInstruction(targetList[i]);
			instructionList.AddInstruction(jump[i]);
		}
		
		instructionList.AddInstruction(targetList[5]);
		instructionList.AddInstruction(new ByteRangeInstruction(0x80, 0xbf));
		for(size_t i = 1; i < 5; ++i)
		{
			Instruction* trailingByteInstruction = new ByteRangeInstruction(0x80, 0xbf);
			instructionList.AddInstruction(trailingByteInstruction);
			jump[5-i]->target = trailingByteInstruction;
		}
		instructionList.AddPatchReference(&jump[0]->target);
	}
	else
	{
		BuildUtf8Components(instructionList);
	}
}

void CharacterRangeListComponent::BuildUtf8Components(InstructionList &instructionList) const
{
	struct BuildUtf8Data
	{
		CharacterRange 	range;
		uint8_t			bitShift;
		uint8_t			offset;
	};

	//	7	U+0000		U+007F		1	0xxxxxxx
	//	11	U+0080		U+07FF		2	110xxxxx	10xxxxxx
	//	16	U+0800		U+FFFF		3	1110xxxx	10xxxxxx	10xxxxxx
	//	21	U+10000		U+1FFFFF	4	11110xxx	10xxxxxx	10xxxxxx	10xxxxxx
	//	26	U+200000	U+3FFFFFF	5	111110xx	10xxxxxx	10xxxxxx	10xxxxxx	10xxxxxx
	//	31	U+4000000	U+7FFFFFFF	6	1111110x	10xxxxxx	10xxxxxx	10xxxxxx	10xxxxxx	10xxxxxx
	
	// This is:
	// [0x00-0x7f]
	// | [0xc0-0xdf] [0x80-0xbf]
	// | [0xe0-0xef] [0x80-0xbf] [0x80-0xbf]
	// | [0xf0-0xf7] [0x80-0xbf] [0x80-0xbf] [0x80-0xbf]
	// | [0xf8-0xfb] [0x80-0xbf] [0x80-0xbf] [0x80-0xbf] [0x80-0xbf]
	// | [0xfc-0xfd] [0x80-0xbf] [0x80-0xbf] [0x80-0xbf] [0x80-0xbf] [0x80-0xbf]
	static const constexpr BuildUtf8Data UTF8_BYTE_RANGES[] =
	{
		{{ 0,         0x7f			}, 0,  0    },
		{{ 0x80,      0x7ff			}, 6,  0xc0 },
		{{ 0x800,     0xffff		}, 12, 0xe0 },
		{{ 0x10000,   0x1fffff		}, 18, 0xf0 },
		{{ 0x200000,  0x3ffffff		}, 24, 0xf8 },
		{{ 0x4000000, 0x7fffffff	}, 30, 0xfc }
	};

	bool allowLowBytes = characterRangeList.HasData() &&
						 (CharacterRange{0, 127} & characterRangeList[0]) == CharacterRange{0, 127};
	
	AlternationComponent* alternation = const_cast<CharacterRangeListComponent*>(this);
	
	for(size_t i = 0; i < JNUMBER_OF_ELEMENTS(UTF8_BYTE_RANGES); ++i)
	{
		const BuildUtf8Data& data = UTF8_BYTE_RANGES[i];		
		for(const CharacterRange& range : characterRangeList)
		{
			CharacterRange intersection = range & data.range;
			if(intersection.IsValid())
			{
				if(allowLowBytes && intersection.min == data.range.min && data.bitShift != 0 && intersection.GetSize() >= (1<<data.bitShift)) intersection.min = 0;
				BuildUtf8Components(alternation, intersection, data.bitShift, data.offset);
			}
		}
	}
	
	alternation->Optimize();
	AlternationComponent::BuildInstructions(instructionList);
}

void CharacterRangeListComponent::BuildUtf8Components(AlternationComponent* alternation, CharacterRange interval, int bitShift, int offset) const
{
	if(bitShift == 0)
	{
		JASSERT(interval.max+offset < 256);
		alternation->componentList.Append(new CharacterRangeListComponent(false, interval+offset));
	}
	else
	{
		int low  = offset + (interval.min >> bitShift);
		int high = offset + (interval.max >> bitShift);
		
		if(low == high)
		{
			CharacterRange subInterval;
			subInterval.min = interval.min & ~(~0U << bitShift);
			subInterval.max = interval.max & ~(~0U << bitShift);
			BuildUtf8Components(alternation, new ByteComponent(low), subInterval, bitShift);
		}
		else
		{
			CharacterRange midInterval;
			midInterval.min = 0;
			midInterval.max = ~(~0U << bitShift);
			
			CharacterRange lowInterval;
			lowInterval.min = interval.min & ~(~0U << bitShift);
			lowInterval.max = ~(~0U << bitShift);
			
			CharacterRange highInterval;
			highInterval.min = 0;
			highInterval.max = interval.max & ~(~0U << bitShift);

			if(lowInterval == midInterval)
			{
				--low;
			}
			else
			{
				BuildUtf8Components(alternation, new ByteComponent(low), lowInterval, bitShift);
			}
			
			if(highInterval == midInterval)
			{
				high++;
			}
			
			if(low < high-1)
			{
				
				IComponent* component = (low+1 == high-1) ?
										(IComponent*) new ByteComponent(low+1) :
										(IComponent*) new CharacterRangeListComponent(false, {low+1, high-1});
				BuildUtf8Components(alternation, component, midInterval, bitShift);
			}

			if(highInterval != midInterval)
			{
				BuildUtf8Components(alternation, new ByteComponent(high), highInterval, bitShift);
			}
		}
	}
}

void CharacterRangeListComponent::BuildUtf8Components(AlternationComponent* alternation, IComponent* firstConcatenateInstruction, CharacterRange interval, int bitShift) const
{
	ConcatenateComponent* concatenate = new ConcatenateComponent;
	concatenate->componentList.Append(firstConcatenateInstruction);
	AlternationComponent* subAlternation = new AlternationComponent;
	BuildUtf8Components(subAlternation, interval, bitShift-6, 0x80);
	
	subAlternation->Optimize();
	if(subAlternation->componentList.GetCount() == 1)
	{
		concatenate->componentList.Append(subAlternation->componentList[0]);
		subAlternation->componentList.SetCount(0);
		delete subAlternation;
	}
	else
	{
		concatenate->componentList.Append(subAlternation);
	}
	
	if(concatenate->componentList.GetCount() == 1)
	{
		alternation->componentList.Append(concatenate->componentList[0]);
		concatenate->componentList.SetCount(0);
		delete concatenate;
	}
	else
	{
		alternation->componentList.Append(concatenate);
	}
}

uint32_t CharacterRangeListComponent::GetMinimumLength() const
{
	if(characterRangeList.IsEmpty() || !useUtf8) return 1;
	return (uint32_t) Utf8Character{characterRangeList[0].min}.GetNumberOfBytes();
}

uint32_t CharacterRangeListComponent::GetMaximumLength() const
{
	if(characterRangeList.IsEmpty() || !useUtf8) return 1;
	return (uint32_t) Utf8Character{characterRangeList.Back().max}.GetNumberOfBytes();
}

bool CharacterRangeListComponent::IsEqual(const IComponent* other) const
{
	if(!HasSameClass(other)) return false;
	return characterRangeList == ((CharacterRangeListComponent*) other)->characterRangeList;
}

//============================================================================

ContentComponent::~ContentComponent()
{
	delete content;
}

void ContentComponent::Dump(ICharacterWriter& output, int depth)
{
	output.PrintF("%r%s:\n", ' ', depth, typeid(*this).name());
	content->Dump(output, depth+1);
}

uint32_t ContentComponent::GetMinimumLength() const
{
	return content->GetMinimumLength();
}

uint32_t ContentComponent::GetMaximumLength() const
{
	return content->GetMaximumLength();
}

bool ContentComponent::IsFixedLength() const
{
	return content->IsFixedLength();
}

bool ContentComponent::RequiresAnyByteMinimalForPartialMatch() const
{
	return content->RequiresAnyByteMinimalForPartialMatch();
}

//============================================================================

void LookAheadComponent::Dump(ICharacterWriter& output, int depth)
{
	output.PrintF("%r%s: %s\n", ' ', depth, typeid(*this).name(), expectedResult ? "positive" : "negative");
	content->Dump(output, depth+1);
}

void LookAheadComponent::BuildInstructions(InstructionList &instructionList) const
{
	if(expectedResult && content->IsFixedLength() && content->GetMinimumLength() == 0)
	{
		content->BuildInstructions(instructionList);
		return;
	}
	
	uint32_t assertSave = instructionList.GetAssertCounter();
	instructionList.SetAssertCounter(0);
	CallInstruction* callInstruction = new CallInstruction();
	instructionList.AddInstruction(callInstruction);
	instructionList.AddPatchReference(&callInstruction->callTarget);
	content->BuildInstructions(instructionList);
	
	if(instructionList.Back() == callInstruction)
	{
		instructionList.RemovePatchReference(&callInstruction->callTarget);
		delete callInstruction;
		
		if(!expectedResult)
		{
			instructionList.AddInstruction(new FailInstruction);
		}
	}
	else
	{
		instructionList.AddInstruction(new SuccessInstruction);
		instructionList.AddPatchReference(expectedResult ? &callInstruction->trueTarget : &callInstruction->falseTarget);
	}
	
	instructionList.SetAssertCounter(assertSave);
}

bool LookAheadComponent::IsEqual(const IComponent* other) const
{
	if(!HasSameClass(other)) return false;
	return expectedResult == ((LookAheadComponent*) other)->expectedResult
			&& content->IsEqual(((LookAheadComponent*) other)->content);
}

void LookBehindComponent::BuildInstructions(InstructionList &instructionList) const
{
	uint32_t assertSave = instructionList.GetAssertCounter();
	instructionList.SetAssertCounter(0);
	CallInstruction* callInstruction = new CallInstruction();
	instructionList.AddInstruction(callInstruction);
	instructionList.AddPatchReference(&callInstruction->callTarget);
	instructionList.AddInstruction(new StepBackInstruction(content->GetMinimumLength()));
	content->BuildInstructions(instructionList);
	instructionList.AddInstruction(new SuccessInstruction);
	instructionList.AddPatchReference(expectedResult ? &callInstruction->trueTarget : &callInstruction->falseTarget);
	instructionList.SetAssertCounter(assertSave);
}

ConditionalComponent::~ConditionalComponent()
{
	delete condition;
}

void ConditionalComponent::Dump(ICharacterWriter& output, int depth)
{
	output.PrintF("%r%s\n", ' ', depth, typeid(*this).name());
	output.PrintF("%rCondition:", ' ', depth+1);
	condition->Dump(output, depth+2);
	output.PrintF("%rFalse:", ' ', depth+1);
	falseComponent->Dump(output, depth+2);
	output.PrintF("%rTrue:", ' ', depth+1);
	trueComponent->Dump(output, depth+2);
}

uint32_t ConditionalComponent::GetMinimumLength() const
{
	return Minimum(falseComponent->GetMinimumLength(), trueComponent->GetMinimumLength());
}

uint32_t ConditionalComponent::GetMaximumLength() const
{
	return Maximum(falseComponent->GetMaximumLength(), trueComponent->GetMaximumLength());
}

bool ConditionalComponent::IsFixedLength() const
{
	return falseComponent->IsFixedLength()
			&& trueComponent->IsFixedLength()
			&& falseComponent->GetMinimumLength() == trueComponent->GetMinimumLength();
}

void ConditionalComponent::BuildInstructions(InstructionList &instructionList) const
{
	static constexpr EnumSet<TokenType, uint64_t> LOOK_BEHIND_SET
	{
		TokenType::NegativeLookBehind,
		TokenType::PositiveLookBehind
	};

	static constexpr EnumSet<TokenType, uint64_t> POSITIVE_SET
	{
		TokenType::PositiveLookBehind,
		TokenType::PositiveLookAhead,
	};

	CallInstruction* callInstruction = new CallInstruction;
	instructionList.AddInstruction(callInstruction);
	instructionList.AddPatchReference(&callInstruction->callTarget);
	if(LOOK_BEHIND_SET.Contains(conditionType))
	{
		instructionList.AddInstruction(new StepBackInstruction(condition->GetMinimumLength()));
	}
	condition->BuildInstructions(instructionList);
	instructionList.AddInstruction(new SuccessInstruction);

	instructionList.AddPatchReference(POSITIVE_SET.Contains(conditionType) ? &callInstruction->trueTarget : &callInstruction->falseTarget);
	trueComponent->BuildInstructions(instructionList);
	JumpInstruction* jump = new JumpInstruction;
	instructionList.AddInstruction(jump);
	
	instructionList.AddPatchReference(POSITIVE_SET.Contains(conditionType) ? &callInstruction->falseTarget : &callInstruction->trueTarget);
	falseComponent->BuildInstructions(instructionList);
	instructionList.AddPatchReference(&jump->target);
}

bool ConditionalComponent::IsEqual(const IComponent* other) const
{
	return HasSameClass(other)
			&& condition->IsEqual(((ConditionalComponent*) other)->condition)
			&& falseComponent->IsEqual(((ConditionalComponent*) other)->falseComponent)
			&& trueComponent->IsEqual(((ConditionalComponent*) other)->trueComponent);
}


//============================================================================

void CaptureComponent::Dump(ICharacterWriter& output, int depth)
{
	if(emitSaveInstructions) output.PrintF("%r%s: capture %z\n", ' ', depth, typeid(*this).name(), captureIndex);
	else output.PrintF("%r%s: auto-cluster\n", ' ', depth, typeid(*this).name());
	content->Dump(output, depth+1);
}

void CaptureComponent::BuildInstructions(InstructionList &instructionList) const
{
	if(emitSaveInstructions && instructionList.ShouldEmitSaveInstructions())
	{
		bool insertReturnRecurse = isRecurseTarget && captured == false;
		bool saveNeverRecurses = instructionList.SaveNeverRecurses();
		captured = true;
		
		int startCaptureIndex = captureIndex*2;
		if(instructionList.IsReverse()) ++startCaptureIndex;
		
		if(insertReturnRecurse)
		{
			instructionList.AddPatchReference(&firstInstruction);
		}
		if(content->IsFixedLength())
		{
			content->BuildInstructions(instructionList);
			if(emitCaptureStart && instructionList.IsForwards())
			{
				SaveInstruction* instruction = new SaveInstruction(saveNeverRecurses, startCaptureIndex, content->GetMinimumLength());
				instructionList.AddInstruction(instruction);
			}
		}
		else
		{
			SaveInstruction* instruction = new SaveInstruction(saveNeverRecurses, startCaptureIndex);
			instructionList.AddInstruction(instruction);
			content->BuildInstructions(instructionList);
			if(!emitCaptureStart && instructionList.IsForwards())
			{
				instructionList.RemoveInstruction(instruction);
				delete instruction;
			}
		}
		
		if(insertReturnRecurse)
		{
			ReturnIfRecurseValueInstruction* returnRecurseToInstruction = new ReturnIfRecurseValueInstruction(captureIndex);
			instructionList.AddInstruction(returnRecurseToInstruction);
		}

		if(emitCaptureStart || instructionList.IsForwards())
		{			
			SaveInstruction* instruction = new SaveInstruction(saveNeverRecurses, startCaptureIndex^1);
			instructionList.AddInstruction(instruction);
		}
	}
	else if(emitSaveInstructions && isRecurseTarget && captured == false)
	{
		captured = true;
		instructionList.AddPatchReference(&firstInstruction);
		content->BuildInstructions(instructionList);
		ReturnIfRecurseValueInstruction* returnRecurseToInstruction = new ReturnIfRecurseValueInstruction(captureIndex);
		instructionList.AddInstruction(returnRecurseToInstruction);
	}
	else
	{
		content->BuildInstructions(instructionList);
	}
}

bool CaptureComponent::HasStartAnchor() const
{
	return content->HasStartAnchor();
}

bool CaptureComponent::HasEndAnchor() const
{
	return content->HasEndAnchor();
}

bool CaptureComponent::IsEqual(const IComponent* other) const
{
	if(!HasSameClass(other)) return false;
	return captureIndex == ((CaptureComponent*) other)->captureIndex;
}

//============================================================================

void ResetCaptureComponent::Dump(ICharacterWriter& output, int depth)
{
	output.PrintF("%r%s: reset-capture\n", ' ', depth, typeid(*this).name());
}

void ResetCaptureComponent::BuildInstructions(InstructionList &instructionList) const
{
	if(instructionList.ShouldEmitSaveInstructions())
	{
		bool saveNeverRecurses = instructionList.SaveNeverRecurses();
		SaveInstruction* instruction = new SaveInstruction(saveNeverRecurses, 0);
		instructionList.AddInstruction(instruction);
		
		if(saveNeverRecurses) captureComponent->emitCaptureStart = false;
	}
	instructionList.SetHasResetCapture();
}

uint32_t ResetCaptureComponent::GetMinimumLength() const
{
	return 0;
}

bool ResetCaptureComponent::IsEqual(const IComponent* other) const
{
	return HasSameClass(other);
}

//============================================================================

CounterComponent::CounterComponent(uint32_t aMinimum, uint32_t aMaximum, Mode aMode, IComponent* content)
: ContentComponent(content),
  minimum(aMinimum),
  maximum(aMaximum),
  mode(aMode)
{
	if(minimum == maximum) mode = Mode::Maximal;
}

void CounterComponent::Dump(ICharacterWriter& output, int depth)
{
	static const char* const MODE_NAMES[] = { "maximal", "minimal", "possessive" };
	if(maximum == TypeData<uint32_t>::Maximum())
	{
		output.PrintF("%r%s: {%z, ∞} %s\n", ' ', depth, typeid(*this).name(), minimum, MODE_NAMES[(int) mode]);
	}
	else
	{
		output.PrintF("%r%s: {%z, %z} %s\n", ' ', depth, typeid(*this).name(), minimum, maximum, MODE_NAMES[(int) mode]);
	}
	content->Dump(output, depth+1);
}

bool CounterComponent::IsFixedLength() const
{
	if(content->GetMaximumLength() == 0) return true;
	return minimum == maximum && content->IsFixedLength();
}

uint32_t CounterComponent::GetMinimumLength() const
{
	if(minimum == 0) return 0;
	return content->GetMinimumLength() * minimum;
}

uint32_t CounterComponent::GetMaximumLength() const
{
	if(maximum == 0) return 0;
	uint32_t contentLength = content->GetMaximumLength();
	if(contentLength == 0) return 0;
	uint64_t maximumLength = (uint64_t) contentLength * maximum;
	return maximumLength < TypeData<uint32_t>::Maximum() ? uint32_t(maximumLength) : TypeData<uint32_t>::Maximum();
}

bool CounterComponent::IsEqual(const IComponent* other) const
{
	if(!HasSameClass(other)) return false;
	return minimum     == ((CounterComponent*) other)->minimum
			&& maximum == ((CounterComponent*) other)->maximum
			&& mode    == ((CounterComponent*) other)->mode
			&& content->IsEqual(((CounterComponent*) other)->content);
}

void CounterComponent::BuildInstructions(InstructionList &instructionList) const
{
	Mode effectiveMode = instructionList.IsReverse() ? Maximal : mode;
	instructionList.IncrementSaveRecurse();
	switch(effectiveMode)
	{
	case Maximal:
		BuildMaximalInstructions(instructionList);
		break;

	case Minimal:
		BuildMinimalInstructions(instructionList);
		break;

	case Possessive:
		PossessInstruction* possess = new PossessInstruction;
		instructionList.AddInstruction(possess);
		instructionList.AddPatchReference(&possess->callTarget);
		BuildMaximalInstructions(instructionList);
		instructionList.AddInstruction(new SuccessInstruction);
		instructionList.AddPatchReference(&possess->trueTarget);
		break;
	}
	instructionList.DecrementSaveRecurse();
}

Instruction* CounterComponent::BuildMinimumInstructions(InstructionList &instructionList) const
{
	Instruction* result = nullptr;
	if(minimum != 0)
	{
		// If someone puts an assert into a repeat...
		if(content->GetMaximumLength() == 0)
		{
			instructionList.AddPatchReference(&result);
			content->BuildInstructions(instructionList);
		}
		else
		{
			instructionList.StopSaveInstructions();
			
			for(size_t i = 1; i < minimum; ++i)
			{
				content->BuildInstructions(instructionList);
			}
			instructionList.AddPatchReference(&result);
			instructionList.AllowSaveInstructions();
			content->BuildInstructions(instructionList);
		}
	}

	return result;
}

void CounterComponent::BuildMinimalInstructions(InstructionList &instructionList) const
{
	Instruction* lastContent = BuildMinimumInstructions(instructionList);
	
	if(content->GetMaximumLength() != 0)
	{
		if(maximum == TypeData<uint32_t>::Maximum())
		{
			if(content->HasSpecificRepeatInstruction() || lastContent == nullptr)
			{
				SplitInstruction* split = new SplitInstruction;
				JumpInstruction* jump = new JumpInstruction;
				jump->target = split;
				split->targetList.SetCount(2);
				instructionList.AddInstruction(jump);
				instructionList.AddPatchReference(&split->targetList[1]);
				content->BuildRepeatInstructions(instructionList);
				
				if(content->GetMinimumLength() == 0)
				{
					instructionList.AddInstruction(new ProgressCheckInstruction(instructionList.GetNextProgessCheckSlot()));
				}
				
				instructionList.AddInstruction(split);			
				instructionList.AddPatchReference(&split->targetList[0]);
			}
			else
			{
				SplitInstruction* split = new SplitInstruction;
				split->targetList.SetCount(2);
				split->targetList[1] = lastContent;
				
				if(content->GetMinimumLength() == 0)
				{
					instructionList.AddInstruction(new ProgressCheckInstruction(instructionList.GetNextProgessCheckSlot()));
				}
				
				instructionList.AddInstruction(split);
				instructionList.AddPatchReference(&split->targetList[0]);
			}
		}
		else
		{
			Table<SplitInstruction*> splitInstructionList;
			
			size_t repeat = maximum - minimum;
			for(size_t i = 0; i < repeat; ++i)
			{
				SplitInstruction* split = new SplitInstruction;
				split->targetList.SetCount(2);
				splitInstructionList.Append(split);
				instructionList.AddInstruction(split);
				instructionList.AddPatchReference(&split->targetList[1]);
				content->BuildInstructions(instructionList);
			}
			
			for(SplitInstruction* split : splitInstructionList)
			{
				instructionList.AddPatchReference(&split->targetList[0]);
			}
		}
	}
}

void CounterComponent::BuildMaximalInstructions(InstructionList &instructionList) const
{
	Instruction* lastContent = BuildMinimumInstructions(instructionList);
	
	if(content->GetMaximumLength() != 0)
	{
		if(maximum == TypeData<uint32_t>::Maximum())
		{
			if(content->HasSpecificRepeatInstruction() || lastContent == nullptr)
			{
				SplitInstruction* split = new SplitInstruction;
				JumpInstruction* jump = new JumpInstruction;
				jump->target = split;
				split->targetList.SetCount(2);
				instructionList.AddInstruction(jump);
				instructionList.AddPatchReference(&split->targetList[0]);
				content->BuildRepeatInstructions(instructionList);
				
				if(content->GetMinimumLength() == 0)
				{
					instructionList.AddInstruction(new ProgressCheckInstruction(instructionList.GetNextProgessCheckSlot()));
				}
				
				instructionList.AddInstruction(split);
				instructionList.AddPatchReference(&split->targetList[1]);
			}
			else
			{
				SplitInstruction* split = new SplitInstruction;
				split->targetList.SetCount(2);
				split->targetList[0] = lastContent;
				
				if(content->GetMinimumLength() == 0)
				{
					instructionList.AddInstruction(new ProgressCheckInstruction(instructionList.GetNextProgessCheckSlot()));
				}
				
				instructionList.AddInstruction(split);
				instructionList.AddPatchReference(&split->targetList[1]);
			}
		}
		else
		{
			Table<SplitInstruction*> splitInstructionList;
			
			size_t repeat = maximum - minimum;
			for(size_t i = 0; i < repeat; ++i)
			{
				SplitInstruction* split = new SplitInstruction;
				split->targetList.SetCount(2);
				splitInstructionList.Append(split);
				instructionList.AddInstruction(split);
				instructionList.AddPatchReference(&split->targetList[0]);
				content->BuildInstructions(instructionList);
			}
			
			for(SplitInstruction* split : splitInstructionList)
			{
				instructionList.AddPatchReference(&split->targetList[1]);
			}
		}
	}
	else if(minimum == 0)
	{
		// In the maximal case, we still might need to run through content
		// once to get any captures
		if(lastContent == nullptr)
		{
			instructionList.IncrementAssertCounter();
			SplitInstruction* split = new SplitInstruction;
			split->targetList.SetCount(2);
			instructionList.AddInstruction(split);
			instructionList.AddPatchReference(&split->targetList[0]);
			content->BuildInstructions(instructionList);
			instructionList.AddPatchReference(&split->targetList[1]);
			instructionList.DecrementAssertCounter();
		}
	}
}

bool CounterComponent::RequiresAnyByteMinimalForPartialMatch() const
{
	if(minimum == 0) return true;
	return content->RequiresAnyByteMinimalForPartialMatch();
}

//============================================================================

void EmptyComponent::BuildInstructions(InstructionList &instructionList) const
{
}

bool EmptyComponent::IsEqual(const IComponent* other) const
{
	return HasSameClass(other);
}

//============================================================================

GroupComponent::~GroupComponent()
{
	for(IComponent* component : componentList)
	{
		delete component;
	}
}

bool GroupComponent::CanCoalesce() const
{
	for(IComponent* component : componentList)
	{
		if(!component->CanCoalesce()) return false;
	}
	return true;
}

void GroupComponent::Dump(ICharacterWriter& output, int depth)
{
	output.PrintF("%r%s: %z children\n", ' ', depth, typeid(*this).name(), componentList.GetCount());
	for(IComponent* component : componentList) component->Dump(output, depth+1);
}

bool GroupComponent::IsEqual(const IComponent* other) const
{
	if(!HasSameClass(other)) return false;
	const GroupComponent& otherGroup = *(const GroupComponent*) other;
	if(componentList.GetCount() != otherGroup.componentList.GetCount()) return false;
	for(size_t i = 0; i < componentList.GetCount(); ++i)
	{
		if(!componentList[i]->IsEqual(otherGroup.componentList[i])) return false;
	}
	return true;
}

//============================================================================

uint32_t ConcatenateComponent::GetMinimumLength() const
{
	uint32_t minimum = 0;
	for(IComponent* component : componentList)
	{
		if(component->IsTerminal()) return minimum;
		minimum += component->GetMinimumLength();
	}
	return minimum;
}

uint32_t ConcatenateComponent::GetMaximumLength() const
{
	uint32_t maximum = 0;
	for(IComponent* component : componentList)
	{
		uint32_t x = component->GetMaximumLength();
		if(x == TypeData<uint32_t>::Maximum()) return x;
		maximum += x;
	}
	return maximum;
}

const IComponent* ConcatenateComponent::GetFront() const
{
	if(componentList.IsEmpty()) return this;
	return componentList.Front();
}

const IComponent* ConcatenateComponent::GetBack() const
{
	if(componentList.IsEmpty()) return this;
	return componentList.Back();
}

void ConcatenateComponent::BuildInstructions(InstructionList &instructionList) const
{
	if(instructionList.IsForwards())
	{
		for(IComponent* component : componentList)
		{
			component->BuildInstructions(instructionList);
		}
	}
	else
	{
		for(size_t i = componentList.GetCount(); i > 0;)
		{
			--i;
			componentList[i]->BuildInstructions(instructionList);
		}
	}
}

bool ConcatenateComponent::HasStartAnchor() const
{
	if(componentList.GetCount() == 0) return false;
	return componentList.Front()->HasStartAnchor();
}

bool ConcatenateComponent::HasEndAnchor() const
{
	if(componentList.GetCount() == 0) return false;
	return componentList.Back()->HasEndAnchor();
}

bool ConcatenateComponent::IsFixedLength() const
{
	for(IComponent* component : componentList)
	{
		if(!component->IsFixedLength()) return false;
	}
	return true;
}

bool ConcatenateComponent::RequiresAnyByteMinimalForPartialMatch() const
{
	if(componentList.GetCount() == 0) return true;
	return componentList.Front()->RequiresAnyByteMinimalForPartialMatch();
}

//============================================================================

uint32_t AlternationComponent::GetMinimumLength() const
{
	if(componentList.GetCount() == 0) return 0;
	
	uint32_t minimum = TypeData<uint32_t>::Maximum();
	for(IComponent* component : componentList)
	{
		if(component->IsTerminal()) return 0;
		uint32_t length = component->GetMinimumLength();
		if(length < minimum) minimum = length;
	}
	return minimum;
}

uint32_t AlternationComponent::GetMaximumLength() const
{
	uint32_t maximum = 0;
	for(IComponent* component : componentList)
	{
		uint32_t length = component->GetMaximumLength();
		if(length > maximum) maximum = length;
	}
	return maximum;
}

void AlternationComponent::BuildInstructions(InstructionList &instructionList) const
{
	if(componentList.GetCount() < 2)
	{
		for(IComponent* component : componentList)
		{
			component->BuildInstructions(instructionList);
		}
	}
	else
	{
		instructionList.IncrementSaveRecurse();
		Table<JumpInstruction*> jumpInstructionList;
		
		SplitInstruction* split = new SplitInstruction;
		split->targetList.SetCount(componentList.GetCount());
		
		instructionList.AddInstruction(split);
		
		for(size_t i = 0; i < componentList.GetCount(); ++i)
		{
			instructionList.AddPatchReference(&split->targetList[i]);
			componentList[i]->BuildInstructions(instructionList);
			
			if(i != componentList.GetCount()-1)
			{
				JumpInstruction* jumpInstruction = new JumpInstruction;
				jumpInstructionList.Append(jumpInstruction);
				instructionList.AddInstruction(jumpInstruction);
			}
		}
		
		for(JumpInstruction* jump : jumpInstructionList)
		{
			instructionList.AddPatchReference(&jump->target);
		}
		instructionList.DecrementSaveRecurse();
	}
}

bool AlternationComponent::HasStartAnchor() const
{
	if(componentList.GetCount() == 0) return false;

	for(IComponent* component : componentList)
	{
		if(!component->HasStartAnchor()) return false;
	}
	return true;
}

bool AlternationComponent::HasEndAnchor() const
{
	if(componentList.GetCount() == 0) return false;

	for(IComponent* component : componentList)
	{
		if(!component->HasEndAnchor()) return false;
	}
	return true;
}

bool AlternationComponent::RequiresAnyByteMinimalForPartialMatch() const
{
	for(IComponent* component : componentList)
	{
		if(component->RequiresAnyByteMinimalForPartialMatch()) return true;
	}
	return false;
}

void AlternationComponent::Optimize()
{
	Optimize_MergeBytesToChracterSets();
	Optimize_LeftFactor();
	Optimize_RightFactor();
}

void AlternationComponent::Optimize_LeftFactor()
{
	size_t start = 0;
	while(start < componentList.GetCount())
	{
		const IComponent* frontComponent = componentList[start]->GetFront();
		if(frontComponent->IsEmpty())
		{
			++start;
			continue;
		}
		size_t end = start+1;
		for(; end < componentList.GetCount(); ++end)
		{
			const IComponent* component = componentList[end]->GetFront();
			if(!frontComponent->IsEqual(component)) break;
		}
		
		if(start+1 != end)
		{
			// Currently: Alternation has a0a1a2 | b0b1b2 | c0c1c2 | ...  where a0 == b0 == c0 ...
			// New: Alternation has a single member which is a concatenation with 2nd element as an alternation:
			// 			[a0, a1a2 | b1b2 | c1c2 | ...]
			
			AlternationComponent* alternation = new AlternationComponent;
			ConcatenateComponent* concatenation = new ConcatenateComponent;
			concatenation->componentList.Append((IComponent*) frontComponent);
			concatenation->componentList.Append(alternation);
			
			for(size_t i = start; i < end; ++i)
			{
				const IComponent* component = componentList[i]->GetFront();
				if(component == componentList[i])
				{
					alternation->componentList.Append(new EmptyComponent);
					if(i != start) delete component;
				}
				else
				{
					JASSERT(componentList[i]->IsConcatenate());
					ConcatenateComponent* concatenate = (ConcatenateComponent*) componentList[i];
					concatenate->componentList.RemoveIndex(0);
					if(i != start) delete component;
					switch(concatenate->componentList.GetCount())
					{
					case 0:
						delete concatenate;
						break;
						
					case 1:
						{
							IComponent* child = concatenate->componentList[0];
							concatenate->componentList.Reset();
							delete concatenate;
							alternation->componentList.Append(child);
						}
						break;
						
					default:
						alternation->componentList.Append(concatenate);
						break;
					}
				}
			}
			
			componentList[start] = concatenation;
			componentList.RemoveCount(start+1, end-start-1);
			
			alternation->Optimize();
		}
		else
		{
			// No factoring. Continue from end point
			start = end;
		}
	}
}

void AlternationComponent::Optimize_RightFactor()
{
	size_t start = 0;
	while(start < componentList.GetCount())
	{
		const IComponent* backComponent = componentList[start]->GetBack();
		if(backComponent->IsEmpty())
		{
			++start;
			continue;
		}
		size_t end = start+1;
		for(; end < componentList.GetCount(); ++end)
		{
			const IComponent* component = componentList[end]->GetBack();
			if(!backComponent->IsEqual(component)) break;
		}
		
		if(start+1 != end)
		{
			// Currently: Alternation has a0a1a2 | b0b1b2 | c0c1c2 | ...  where a2 == b2 == c2 ...
			// New: Alternation has a single member which is a concatenation with 2nd element as an alternation:
			// 			[a0a1 | b0b1 | c0c1 | ... , a2]
			
			AlternationComponent* alternation = new AlternationComponent;
			ConcatenateComponent* concatenation = new ConcatenateComponent;
			concatenation->componentList.Append(alternation);
			concatenation->componentList.Append((IComponent*) backComponent);
			
			for(size_t i = start; i < end; ++i)
			{
				const IComponent* component = componentList[i]->GetBack();
				if(component == componentList[i])
				{
					alternation->componentList.Append(new EmptyComponent);
					if(i != start) delete component;
				}
				else
				{
					JASSERT(componentList[i]->IsConcatenate());
					ConcatenateComponent* concatenate = (ConcatenateComponent*) componentList[i];
					concatenate->componentList.RemoveBack();
					if(i != start) delete component;
					switch(concatenate->componentList.GetCount())
					{
					case 0:
						delete concatenate;
						break;
							
					case 1:
						{
							IComponent* child = concatenate->componentList[0];
							concatenate->componentList.Reset();
							delete concatenate;
							alternation->componentList.Append(child);
						}
						break;
							
					default:
						alternation->componentList.Append(concatenate);
						break;
					}
				}
			}
			
			componentList[start] = concatenation;
			componentList.RemoveCount(start+1, end-start-1);
			
			alternation->Optimize();
		}
		else
		{
			// No factoring. Continue from end point
			start = end;
		}
	}
}

void AlternationComponent::Optimize_MergeBytesToChracterSets()
{
	// Step through alternation and replace groups of characters to character sets
	// Need a signed int since start can end up negative
	for(int32_t end = (int32_t) componentList.GetCount(); end > 0;)
	{
		--end;
		if(componentList[end]->IsByte())
		{
			CharacterRangeList rangeList;
			componentList[end]->AddToCharacterRangeList(rangeList);
			
			int start = end-1;
			while(start >= 0 && componentList[start]->IsByte())
			{
				componentList[start]->AddToCharacterRangeList(rangeList);
				--start;
			}
			
			if(end-start >= 2)
			{
				rangeList.Sort();
				for(int i = end; i > start; --i) delete componentList[i];

				componentList.RemoveCount(start+2, end-start-1);
				componentList[start+1] = new CharacterRangeListComponent(false, rangeList);
				end = start;
			}
		}
	}
	
	// Step through components and left
}

bool AlternationComponent::IsFixedLength() const
{
	bool firstTime = true;
	size_t length = 0;
	for(IComponent* component : componentList)
	{
		if(!component->IsFixedLength()) return false;
		if(firstTime)
		{
			firstTime = false;
			length = component->GetMinimumLength();
		}
		else
		{
			if(component->GetMinimumLength() != length) return false;
		}
	}
	return true;
}

//============================================================================

void RecurseComponent::Dump(ICharacterWriter& output, int depth)
{
	output.PrintF("%r%s: %d\n", ' ', depth, typeid(*this).name(), captureIndex);
}

void RecurseComponent::BuildInstructions(InstructionList &instructionList) const
{
	RecurseInstruction* instruction = new RecurseInstruction(target->captureIndex);
	recurseInstructionList.Append(instruction);
	instructionList.AddInstruction(instruction);
}

bool RecurseComponent::IsEqual(const IComponent* other) const
{
	return HasSameClass(other)
			&& captureIndex == ((RecurseComponent*) other)->captureIndex;
}

uint32_t RecurseComponent::GetMinimumLength() const
{
	if(isRecursing) return 0;
	isRecursing = true;
	uint32_t result = target->GetMinimumLength();
	isRecursing = false;
	return result;
}

uint32_t RecurseComponent::GetMaximumLength() const
{
	if(isRecursing) return TypeData<uint32_t>::Maximum();
	isRecursing = true;
	uint32_t result = target->GetMaximumLength();
	isRecursing = false;
	return result;
}

bool RecurseComponent::IsFixedLength() const
{
	if(isRecursing) return false;
	isRecursing = true;
	bool result = target->IsFixedLength();
	isRecursing = false;
	return result;
}

//============================================================================
