//==========================================================================

#include "Javelin/Pattern/Internal/PatternInstruction.h"
#include "Javelin/Container/EnumSet.h"
#include "Javelin/Pattern/Internal/PatternByteCode.h"
#include "Javelin/Pattern/Internal/PatternNibbleMask.h"
#include "Javelin/Stream/ICharacterWriter.h"
#include "Javelin/Type/Exception.h"

//==========================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//==========================================================================

constexpr InstructionReference InstructionReference::PREVIOUS = { nullptr, nullptr };

Instruction** const InstructionReference::SELF_REFERENCE = (Instruction**) 1;

Instruction* const InstructionReference::START_REFERENCE = (Instruction*) 1;
Instruction* const InstructionReference::SEARCH_OPTIMIZER_REFERENCE = (Instruction*) 2;

//==========================================================================

Instruction* InstructionReference::GetInstruction(Instruction* current) const
{
	if(storage == PREVIOUS_REFERENCE) return current->GetPrevious();
	else if(storage == SELF_REFERENCE) return current;
	else if(instruction == START_REFERENCE) return nullptr;
	else if(instruction == SEARCH_OPTIMIZER_REFERENCE) return nullptr;
	else return instruction;
}

//==========================================================================

void InstructionTable::Dump(ICharacterWriter& output) const
{
	for(Instruction* in : *this)
	{
		output.PrintF("%u: ", in->index);
		in->Dump(output);
		output.PrintF(" ");
	}
	output.PrintF("\n");
}

bool InstructionTable::ContainsInstruction(InstructionType type) const
{
	for(Instruction* instruction : *this)
	{
		if(instruction->type == type) return true;
	}
	return false;
}

Tuple<Instruction*, bool> InstructionTable::GetRepeatedByteInstruction() const
{
	if(GetCount() != 1) return { nullptr, false };
	Instruction* instruction = Front();
	
	// Need to detect:
	// x:   jump x+2
	// x+1: simple-byte-consumer
	// x+2: split x+1 x+3   or x+3 x+1
	//
	// or:
	//
	// x: simple-byte-consumer
	// x+1: split x x+2   or x+2 x

	bool hasAtLeastOne = true;
	
	while(instruction->type == InstructionType::Save
		  || instruction->type == InstructionType::ProgressCheck)
	{
		instruction = instruction->GetNext();
	}
	
	if(instruction->type == InstructionType::Jump)
	{
		JumpInstruction* jump = (JumpInstruction*) instruction;
		if(jump->target != instruction->GetNext()->GetNext()) return { nullptr, false };
		
		instruction = instruction->GetNext();
		hasAtLeastOne = false;
	}
	
	if(!instruction->IsSimpleByteConsumer()) return { nullptr, false };
	
	Instruction* afterByteConsumer = instruction->GetNext();
	if(afterByteConsumer->type != InstructionType::Split) return { nullptr, false };

	SplitInstruction* split = (SplitInstruction*) afterByteConsumer;
	if(split->targetList.GetCount() != 2) return { nullptr, false };
	
	if(split->targetList[0] == instruction && split->targetList[1] == split->GetNext()) return { instruction, hasAtLeastOne };
	if(split->targetList[1] == instruction && split->targetList[0] == split->GetNext()) return { instruction, hasAtLeastOne };
	
	return { nullptr, false };
}

bool InstructionTable::HasStartAnchor() const
{
	for(Instruction* instruction : *this)
	{
		if(instruction->HasStartAnchor()) return true;
	}
	return false;
}

//==========================================================================

Instruction::Instruction(const Instruction& a)
: type(a.type)
{
	index = TypeData<uint32_t>::Maximum();
	
	// Do NOT copy reference list!
}

void Instruction::SetValidBytes(StaticBitTable<256>& mask) const
{
	JERROR("Function not overloaded!");
}

void Instruction::BuildByteCode(ByteCodeBuilder& builder) const
{
	builder.AddInstruction(this, type, GetByteCodeData());
}

bool Instruction::LeadsToMatch(bool direct) const
{
	const Instruction* p = this;
	for(;;)
	{
		switch(p->type)
		{
		case InstructionType::DispatchTable:
			if(direct) return false;
			else
			{
				DispatchTableInstruction* dispatch = (DispatchTableInstruction*) p;
				if(dispatch->targetList[0] == nullptr) return false;
				if(dispatch->targetList.GetCount() != 2) return false;
				if(dispatch->targetList[0] == dispatch->GetPrevious())
				{
					if(!dispatch->GetPrevious()->IsSimpleByteConsumer()) return false;
					p = dispatch->targetList[1];
					continue;
				}
				else if(dispatch->targetList[1] == dispatch->GetPrevious())
				{
					if(!dispatch->GetPrevious()->IsSimpleByteConsumer()) return false;
					p = dispatch->targetList[0];
					continue;
				}
				else return false;
			}
				
		case InstructionType::ProgressCheck:
		case InstructionType::Save:
			p = p->GetNext();
			continue;

		case InstructionType::Jump:
			p = ((JumpInstruction*) p)->target;
			continue;
				
		case InstructionType::Match:
		case InstructionType::Success:
			return true;
				
		default:
			return false;
		}
	}
}

bool Instruction::CanLeadToMatch() const
{
	InstructionTable stack;
	OpenHashSet<const Instruction*> progressCheckSet;
	
	const Instruction* p = this;
	for(;;)
	{
		switch(p->type)
		{
		case InstructionType::AssertEndOfInput:
		case InstructionType::AssertEndOfLine:
		case InstructionType::AssertStartOfInput:
		case InstructionType::AssertStartOfLine:
		case InstructionType::AssertWordBoundary:
		case InstructionType::AssertNotWordBoundary:
		case InstructionType::AssertRecurseValue:
		case InstructionType::ReturnIfRecurseValue:
		case InstructionType::Save:
			p = p->GetNext();
			continue;
				
		case InstructionType::ProgressCheck:
			if(progressCheckSet.Contains(p)) goto Default;
			progressCheckSet.Put(p);
			p = p->GetNext();
			continue;
				
		case InstructionType::BackReference:
			// TODO: This stops processing when a back reference is encountered.
			return true;
				
		case InstructionType::Recurse:
			// TODO: This stops processing when a recurse is encountered.
			return true;
				
		case InstructionType::Success:
			return true;

		case InstructionType::Call:
			{
				CallInstruction* callInstruction = (CallInstruction*) p;
				if(callInstruction->falseTarget && !stack.Contains(callInstruction->falseTarget))
				{
					stack.Push(callInstruction->falseTarget);
				}
				if(callInstruction->trueTarget && !stack.Contains(callInstruction->trueTarget))
				{
					stack.Push(callInstruction->trueTarget);
				}
				JASSERT(stack.HasData());
				p = stack.Back();
				stack.Pop();
			}
			continue;

		case InstructionType::Jump:
			p = ((JumpInstruction*) p)->target;
			continue;

		case InstructionType::Split:
			{
				SplitInstruction* split = (SplitInstruction*) p;
				for(Instruction* target : split->targetList)
				{
					if(!stack.Contains(target)) stack.Push(target);
				}
				JASSERT(stack.HasData());
				p = stack.Back();
				stack.Pop();
			}
			continue;
				
		case InstructionType::DispatchTable:
			{
				DispatchTableInstruction* dispatch = (DispatchTableInstruction*) p;
				for(Instruction* target : dispatch->targetList)
				{
					if(!target) continue;
					if(!stack.Contains(target)) stack.Push(target);
				}
				p = stack.Back();
				stack.Pop();
			}
			continue;
				
		case InstructionType::Match:
			return true;
			
		Default:
		default:
			if(stack.IsEmpty()) return false;
			p = stack.Back();
			stack.Pop();
			continue;
		}
	}
}

//==========================================================================

void Instruction::TransferReferencesTo(Instruction* target)
{
	for(InstructionReference reference : referenceList)
	{
		if(reference.storage == InstructionReference::SELF_REFERENCE) continue;
		if(reference.storage != InstructionReference::PREVIOUS_REFERENCE)
		{
			*reference.storage = target;
		}
		else
		{
			JASSERT(!target->referenceList.Contains(nullptr));;
		}
		target->referenceList.Append(reference);
	}
	referenceList.SetCount(0);
}

void Instruction::TagReachable(int flags)
{
	Instruction* p = this;
	while(p != nullptr)
	{
		if((p->startReachable & flags) == flags) return;
		p->startReachable |= flags;
		
		p = p->ProcessReachable(flags);
	}
}

Instruction* Instruction::ProcessReachable(int flags)
{
	return GetNext();
}

void Instruction::Link()
{
	GetNext()->referenceList.Append(InstructionReference::PREVIOUS);
}

void Instruction::Unlink()
{
	GetNext()->referenceList.Remove(InstructionReference::PREVIOUS);
}

//==========================================================================

void AdvanceByteInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("advance-byte");
}

void AnyByteInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("any-byte");
}

void AssertStartOfInputInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("assert-start-of-input");
}

void AssertEndOfInputInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("assert-end-of-input");
}

void AssertStartOfLineInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("assert-start-of-line");
}

void AssertEndOfLineInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("assert-end-of-line");
}

uint32_t AssertWordBoundaryBaseInstruction::GetByteCodeData() const
{
	uint32_t result = 0;
	
	if(isPreviousWordOnly) result += ByteCodeWordBoundaryHint::PreviousIsWordOnly;
	if(isPreviousNotWordOnly) result += ByteCodeWordBoundaryHint::PreviousIsNotWordOnly;
	if(isNextWordOnly) result += ByteCodeWordBoundaryHint::NextIsWordOnly;
	if(isNextNotWordOnly) result += ByteCodeWordBoundaryHint::NextIsNotWordOnly;
	
	return result;
}

void AssertWordBoundaryBaseInstruction::Dump(ICharacterWriter& output) const
{
	switch(type)
	{
	case InstructionType::AssertWordBoundary:
		output.PrintF("assert-word-boundary");
		break;
			
	case InstructionType::AssertNotWordBoundary:
		output.PrintF("assert-not-word-boundary");
		break;
			
	default:
		JERROR("Internal error!");
	}
	
	if(isPreviousWordOnly) output.PrintF(" previous-is-word-only");
	if(isPreviousNotWordOnly) output.PrintF(" previous-is-not-word-only");
	if(isNextWordOnly) output.PrintF(" next-is-word-only");
	if(isNextNotWordOnly) output.PrintF(" next-is-not-word-only");
}

void AssertStartOfSearchInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("assert-start-of-search");
}

void AssertRecurseValueInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("assert-recurse-value %u", recurseValue);
}

void BackReferenceInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("back-reference %u", index);
}

void ByteInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("byte '%C'", c);
}

void ByteEitherOf2Instruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("byte-either-of-2 '%C' '%C'", c[0], c[1]);
}

void ByteEitherOf3Instruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("byte-either-of-3 '%C' '%C' '%C'", c[0], c[1], c[2]);
}

void ByteRangeInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("byte-range {'%C', '%C'}", byteRange.min, byteRange.max);
}

void ByteBitMaskInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("byte-bit-mask: %A", &String::CreateHexStringFromBytes(&bitMask, 32));
}

void ByteNotInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("byte-not '%C'", c);
}

void ByteNotEitherOf2Instruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("byte-not-either-of-2 '%C' '%C'", c[0], c[1]);
}

void ByteNotEitherOf3Instruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("byte-not-either-of-3 '%C' '%C'", c[0], c[1], c[2]);
}

void ByteNotRangeInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("byte-not-range {%d, %d}", byteRange.min, byteRange.max);
}

void CallInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("call %d -> ", callTarget->index);
	if(falseTarget) output.PrintF("%u : ", falseTarget->index);
	else output.PrintF("fail : ");
	if(trueTarget) output.PrintF("%u", trueTarget->index);
	else output.PrintF("fail");
}

void FailInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("fail");
}

void FindByteInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("find-byte '%C' -> %u", c, target->index);
}

void PossessInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("possess %u -> %u", callTarget->index, trueTarget->index);
}

void ReturnIfRecurseValueInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("return-if-recurse-value %u", recurseValue);
}

void SearchByteInstruction::Dump(ICharacterWriter& output) const
{
	switch(type)
	{
	case InstructionType::SearchByte:
		output.PrintF("search-byte '%C' @ %u", bytes[0], offset);
		break;
			
	case InstructionType::SearchByteEitherOf2:
		output.PrintF("search-byte-either-of-2 '%C' '%C' @ %u", bytes[0], bytes[1], offset);
		break;
			
	case InstructionType::SearchByteEitherOf3:
		output.PrintF("search-byte-either-of-3 '%C' '%C' '%C' @ %u", bytes[0], bytes[1], bytes[2], offset);
		break;
			
	case InstructionType::SearchByteEitherOf4:
		output.PrintF("search-byte-either-of-4 '%C' '%C' '%C' '%C' @ %u", bytes[0], bytes[1], bytes[2], bytes[3], offset);
		break;
			
	case InstructionType::SearchByteEitherOf5:
		output.PrintF("search-byte-either-of-5 '%C' '%C' '%C' '%C' '%C' @ %u", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], offset);
		break;
		
	case InstructionType::SearchByteEitherOf6:
		output.PrintF("search-byte-either-of-6 '%C' '%C' '%C' '%C' '%C' '%C' @ %u", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], offset);
		break;
		
	case InstructionType::SearchByteEitherOf7:
		output.PrintF("search-byte-either-of-7 '%C' '%C' '%C' '%C' '%C' '%C' '%C' @ %u", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], offset);
		break;
		
	case InstructionType::SearchByteEitherOf8:
		output.PrintF("search-byte-either-of-8 '%C' '%C' '%C' '%C' '%C' '%C' '%C' '%C' @ %u", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], offset);
		break;
		
	case InstructionType::SearchBytePair:
		output.PrintF("search-byte-pair '%C' '%C' @ %u", bytes[0], bytes[1], offset);
		break;
			
	case InstructionType::SearchBytePair2:
		output.PrintF("search-byte-pair-2 '%C' '%C' '%C' '%C' @ %u", bytes[0], bytes[1], bytes[2], bytes[3], offset);
		break;
			
	case InstructionType::SearchBytePair3:
		output.PrintF("search-byte-pair-3 '%C' '%C' '%C' '%C' '%C' '%C' @ %u", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], offset);
		break;
			
	case InstructionType::SearchBytePair4:
		output.PrintF("search-byte-pair-4 '%C' '%C' '%C' '%C' '%C' '%C' '%C' '%C' @ %u", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], offset);
		break;
		
	case InstructionType::SearchByteRange:
		output.PrintF("search-byte-range '%C' '%C' @ %u", bytes[0], bytes[1], offset);
		break;
		
	case InstructionType::SearchByteRangePair:
		output.PrintF("search-byte-range-pair '%C' '%C' '%C' '%C' @ %u", bytes[0], bytes[1], bytes[2], bytes[3], offset);
		break;
		
	case InstructionType::SearchByteTriplet:
		output.PrintF("search-byte-triplet '%C' '%C' '%C' @ %u", bytes[0], bytes[1], bytes[2], offset);
		break;
		
	case InstructionType::SearchByteTriplet2:
		output.PrintF("search-byte-triplet-2 '%C' '%C' '%C' '%C' '%C' '%C' @ %u", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], offset);
		break;
		
	default:
		JERROR("Internal error!");
	}
}

void SearchDataInstruction::Dump(ICharacterWriter& output) const
{
	switch(type)
	{
	case InstructionType::SearchBoyerMoore:
		output.PrintF("search-boyer-moore %u", length);
		break;
		
	case InstructionType::SearchShiftOr:
		output.PrintF("search-shift-or %u", length);
		break;

	default:
		JERROR("Internal error!");
	}
}

void JumpInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("jump %u", target->index);
}

void JumpTableInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("%s", GetName());
	for(Instruction* instruction : targetList)
	{
		if(instruction) output.PrintF(" %u", instruction->index);
		else output.PrintF(" fail");
	}
	//	output.PrintF("\n");
	//	output.DumpHex(targetTable, 256);
}

void MatchInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("match");
}

void PropagateBackwardsInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("propagate-backwards: %A", &String::CreateHexStringFromBytes(&bitMask, 32));
}

void ProgressCheckInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("progress-check %z", index);
}

void RecurseInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("recurse %d {%u} ", callTarget->index, recurseValue);
}

void SaveInstruction::Dump(ICharacterWriter& output) const
{
	if(saveOffset != 0) output.PrintF("%s %u @ %d", neverRecurse ? "save-no-recurse" : "save", saveIndex, -saveOffset);
	else output.PrintF("%s %u", neverRecurse ? "save-no-recurse" : "save", saveIndex);
}

void SuccessInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("success");
}

void SplitInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("split");
	for(Instruction* target : targetList)
	{
		output.PrintF(" %u", target->index);
	}
}

void StepBackInstruction::Dump(ICharacterWriter& output) const
{
	output.PrintF("step-back %u", step);
}

//==========================================================================

bool Javelin::PatternInternal::operator==(const Instruction &a, const Instruction& b)
{
	if(a.type != b.type) return false;
	
	switch(a.type)
	{
	case InstructionType::AdvanceByte:
		return true;
			
	case InstructionType::AnyByte:
		return true;

	case InstructionType::AssertStartOfInput:
		return true;
			
	case InstructionType::AssertEndOfInput:
		return true;
		
	case InstructionType::AssertStartOfLine:
		return true;
		
	case InstructionType::AssertEndOfLine:
		return true;
		
	case InstructionType::AssertWordBoundary:
	case InstructionType::AssertNotWordBoundary:
		return ((AssertWordBoundaryBaseInstruction&) a).isPreviousWordOnly == ((AssertWordBoundaryBaseInstruction&) b).isPreviousWordOnly
			&& ((AssertWordBoundaryBaseInstruction&) a).isPreviousNotWordOnly == ((AssertWordBoundaryBaseInstruction&) b).isPreviousNotWordOnly
			&& ((AssertWordBoundaryBaseInstruction&) a).isNextWordOnly == ((AssertWordBoundaryBaseInstruction&) b).isNextWordOnly
			&& ((AssertWordBoundaryBaseInstruction&) a).isNextNotWordOnly == ((AssertWordBoundaryBaseInstruction&) b).isNextNotWordOnly;
		
	case InstructionType::AssertRecurseValue:
		return ((AssertRecurseValueInstruction&) a).recurseValue == ((AssertRecurseValueInstruction&) b).recurseValue;
			
	case InstructionType::BackReference:
		return ((BackReferenceInstruction&) a).index == ((BackReferenceInstruction&) b).index;
			
	case InstructionType::Byte:
		return ((ByteInstruction&) a).c == ((ByteInstruction&) b).c;

	case InstructionType::ByteEitherOf2:
		{
			const ByteEitherOf2Instruction& lhs = (ByteEitherOf2Instruction&) a;
			const ByteEitherOf2Instruction& rhs = (ByteEitherOf2Instruction&) b;
			return lhs.c[0] == rhs.c[0] && lhs.c[1] == rhs.c[1];
		}

	case InstructionType::ByteEitherOf3:
		{
			const ByteEitherOf3Instruction& lhs = (ByteEitherOf3Instruction&) a;
			const ByteEitherOf3Instruction& rhs = (ByteEitherOf3Instruction&) b;
			return lhs.c[0] == rhs.c[0] && lhs.c[1] == rhs.c[1] && lhs.c[2] == rhs.c[2];
		}
			
	case InstructionType::ByteRange:
		return ((ByteRangeInstruction&) a).byteRange == ((ByteRangeInstruction&) b).byteRange;
		
	case InstructionType::ByteBitMask:
		return ((ByteBitMaskInstruction&) a).bitMask == ((ByteBitMaskInstruction&) b).bitMask;
		
	case InstructionType::ByteNot:
			return ((ByteNotInstruction&) a).c == ((ByteNotInstruction&) b).c;
			
	case InstructionType::ByteNotEitherOf2:
		{
			const ByteNotEitherOf2Instruction& lhs = (ByteNotEitherOf2Instruction&) a;
			const ByteNotEitherOf2Instruction& rhs = (ByteNotEitherOf2Instruction&) b;
			return lhs.c[0] == rhs.c[0] && lhs.c[1] == rhs.c[1];
		}
			
	case InstructionType::ByteNotEitherOf3:
		{
			const ByteNotEitherOf3Instruction& lhs = (ByteNotEitherOf3Instruction&) a;
			const ByteNotEitherOf3Instruction& rhs = (ByteNotEitherOf3Instruction&) b;
			return lhs.c[0] == rhs.c[0] && lhs.c[1] == rhs.c[1] && lhs.c[2] == rhs.c[2];
		}
			
	case InstructionType::ByteNotRange:
		return ((ByteNotRangeInstruction&) a).byteRange == ((ByteNotRangeInstruction&) b).byteRange;

	case InstructionType::ByteJumpTable:
	case InstructionType::DispatchTable:
		{
			const JumpTableInstruction& lhs = (JumpTableInstruction&) a;
			const JumpTableInstruction& rhs = (JumpTableInstruction&) b;
			return memcmp(lhs.targetTable, rhs.targetTable, 256) == 0 && lhs.targetList == rhs.targetList;
		}
			
	case InstructionType::Call:
		return ((CallInstruction&) a).callTarget == ((CallInstruction&) b).callTarget
		   && ((CallInstruction&) a).falseTarget == ((CallInstruction&) b).falseTarget
		   && ((CallInstruction&) a).trueTarget  == ((CallInstruction&) b).trueTarget;
		
	case InstructionType::Fail:
		return true;

	case InstructionType::Jump:
		return ((JumpInstruction&) a).target == ((JumpInstruction&) b).target;
		
	case InstructionType::Match:
		return true;
		
	case InstructionType::Possess:
		return ((PossessInstruction&) a).callTarget == ((PossessInstruction&) b).callTarget
		   && ((PossessInstruction&) a).trueTarget  == ((PossessInstruction&) b).trueTarget;
		
	case InstructionType::ProgressCheck:
		return true;

	case InstructionType::Recurse:
		return ((RecurseInstruction&) a).recurseValue == ((RecurseInstruction&) b).recurseValue
			&& ((RecurseInstruction&) a).callTarget == ((RecurseInstruction&) b).callTarget;
		
	case InstructionType::ReturnIfRecurseValue:
		return ((ReturnIfRecurseValueInstruction&) a).recurseValue == ((ReturnIfRecurseValueInstruction&) b).recurseValue;
			
	case InstructionType::Save:
		return ((SaveInstruction&) a).saveIndex == ((SaveInstruction&) b).saveIndex
			&& ((SaveInstruction&) a).saveOffset == ((SaveInstruction&) b).saveOffset;
		
	case InstructionType::Split:
		{
			const SplitInstruction& splitA = (SplitInstruction&) a;
			const SplitInstruction& splitB = (SplitInstruction&) b;
			if(splitA.targetList == splitB.targetList) return true;
			

		}
		return false;
			
	case InstructionType::StepBack:
		return ((StepBackInstruction&) a).step == ((StepBackInstruction&) b).step;
		
	case InstructionType::Success:
		return true;
		
	default:
		JERROR("Unhandled case");
		return false;
	}
}

//==========================================================================

Instruction* AdvanceByteInstruction::Clone() const
{
	return new AdvanceByteInstruction(*this);
}

void AdvanceByteInstruction::SetValidBytes(StaticBitTable<256>& mask) const
{
	mask.SetAllBits();
}

Instruction* AnyByteInstruction::Clone() const
{
	return new AnyByteInstruction(*this);
}

void AnyByteInstruction::SetValidBytes(StaticBitTable<256>& mask) const
{
	mask.SetAllBits();
}

Instruction* AssertStartOfInputInstruction::Clone() const
{
	return new AssertStartOfInputInstruction(*this);
}

Instruction* AssertEndOfInputInstruction::Clone() const
{
	return new AssertEndOfInputInstruction(*this);
}

Instruction* AssertStartOfLineInstruction::Clone() const
{
	return new AssertStartOfLineInstruction(*this);
}

Instruction* AssertEndOfLineInstruction::Clone() const
{
	return new AssertEndOfLineInstruction(*this);
}

Instruction* AssertWordBoundaryInstruction::Clone() const
{
	return new AssertWordBoundaryInstruction(*this);
}

Instruction* AssertNotWordBoundaryInstruction::Clone() const
{
	return new AssertNotWordBoundaryInstruction(*this);
}

Instruction* AssertStartOfSearchInstruction::Clone() const
{
	return new AssertStartOfSearchInstruction(*this);
}

Instruction* AssertRecurseValueInstruction::Clone() const
{
	return new AssertRecurseValueInstruction(*this);
}

Instruction* BackReferenceInstruction::Clone() const
{
	return new BackReferenceInstruction(*this);
}

uint32_t BackReferenceInstruction::GetByteCodeData() const
{
	return index;
}

JumpInstruction::JumpInstruction(Instruction* aTarget)
: Instruction(InstructionType::Jump),
  target(aTarget)
{
	target->referenceList.Append(InstructionReference{&target, this});
}

Instruction* JumpInstruction::Clone() const
{
	return new JumpInstruction(target);
}

uint32_t JumpInstruction::GetByteCodeData() const
{
	return target->index;
}

Instruction* JumpInstruction::ProcessReachable(int flags)
{
	return target;
}

void JumpInstruction::Link()
{
	target->referenceList.Append(InstructionReference{&target, this});
}
						   
void JumpInstruction::Unlink()
{
	target->referenceList.Remove(&target);
}

Instruction* ByteInstruction::Clone() const
{
	return new ByteInstruction(*this);
}

uint32_t ByteInstruction::GetByteCodeData() const
{
	return c;
}

void ByteInstruction::SetValidBytes(StaticBitTable<256>& mask) const
{
	mask.SetBit(c);
}

Instruction* ByteEitherOf2Instruction::Clone() const
{
	return new ByteEitherOf2Instruction(*this);
}

uint32_t ByteEitherOf2Instruction::GetByteCodeData() const
{
	return c[0] | c[1] << 8;
}

void ByteEitherOf2Instruction::SetValidBytes(StaticBitTable<256>& mask) const
{
	mask.SetBit(c[0]);
	mask.SetBit(c[1]);
}

Instruction* ByteEitherOf3Instruction::Clone() const
{
	return new ByteEitherOf3Instruction(*this);
}

uint32_t ByteEitherOf3Instruction::GetByteCodeData() const
{
	return c[0] | c[1]<<8 | c[2]<<16;
}

void ByteEitherOf3Instruction::SetValidBytes(StaticBitTable<256>& mask) const
{
	mask.SetBit(c[0]);
	mask.SetBit(c[1]);
	mask.SetBit(c[2]);
}

Instruction* ByteRangeInstruction::Clone() const
{
	return new ByteRangeInstruction(*this);
}

uint32_t ByteRangeInstruction::GetByteCodeData() const
{
	return byteRange.min | byteRange.max<<8;
}

void ByteRangeInstruction::SetValidBytes(StaticBitTable<256>& mask) const
{
	for(uint32_t i = byteRange.min; i <= byteRange.max; ++i)
	{
		mask.SetBit(i);
	}
}

JumpTableInstruction::JumpTableInstruction(const JumpTableInstruction& a)
: Instruction(a),
  targetList(a.targetList)
{
	CopyMemory(targetTable, a.targetTable, 256);
	for(Instruction* &target : targetList)
	{
		if(target == &a) target = this;
		if(target) target->referenceList.Append(InstructionReference{&target, this});
	}
}

void JumpTableInstruction::BuildByteCode(ByteCodeBuilder& builder) const
{
	// Ensure that if there is a null target, it is the first.
	for(size_t i = 1; i < targetList.GetCount(); ++i)
	{
		JASSERT(targetList[i] != nullptr);
	}
	
	if(targetList.GetCount() == 2)
	{
		// Use a mask instruction instead!
		ByteCodeJumpMaskData data;
		for(int i = 0; i < 256; ++i)
		{
			if(targetTable[i] == 1) data.bitMask.SetBit(i);
		}
		for(int i = 0; i < 2; ++i)
		{
			data.pcData[i] = (targetList[i] == nullptr) ? TypeData<uint32_t>::Maximum() : targetList[i]->index;
		}
		
		// Is it possible to use a range?
		if(data.bitMask.IsContiguous())
		{
			Interval<size_t> range = data.bitMask.GetContiguousRange();
			
			ByteCodeJumpRangeData rangeData;
			rangeData.range.min = range.min;
			rangeData.range.max = range.max-1;
			rangeData.pcData[0] = data.pcData[0];
			rangeData.pcData[1] = data.pcData[1];

			uint32_t offset = builder.GetOffsetForData(&rangeData, sizeof(rangeData));
			builder.AddInstruction(this, (InstructionType) ((int) type+2), offset);
		}
		else
		{
			uint32_t offset = builder.GetOffsetForData(&data, sizeof(data));
			builder.RegisterExistingData(offset, sizeof(data.bitMask));
			builder.AddInstruction(this, (InstructionType) ((int) type+1), offset);
		}
	}
	else
	{
		ByteCodeJumpTableData data;
		data.numberOfTargets = uint32_t(targetList.GetCount());
		memcpy(data.jumpTable, targetTable, 256);
		for(size_t i = 0; i < targetList.GetCount(); ++i)
		{
			data.pcData[i] = (targetList[i] == nullptr) ? TypeData<uint32_t>::Maximum() : targetList[i]->index;
		}
		uint32_t offset = builder.GetOffsetForData(&data, 260+targetList.GetCount()*4);
		builder.AddInstruction(this, type, offset);
	}
}

void JumpTableInstruction::SetValidBytes(StaticBitTable<256>& mask) const
{
	JASSERT(targetList.GetCount() >= 2);
	if(targetList[0] == nullptr)
	{
		for(uint32_t i = 0; i < 256; ++i)
		{
			if(targetTable[i] != 0) mask.SetBit(i);
		}
	}
	else
	{
		mask.SetAllBits();
	}
}

Instruction* JumpTableInstruction::ProcessReachable(int flags)
{
	for(Instruction* target : targetList)
	{
		if(target) target->TagReachable(flags);
	}
	return nullptr;
}

void JumpTableInstruction::Link()
{
	for(Instruction* &target : targetList)
	{
		if(target) target->referenceList.Append(InstructionReference{&target, this});
	}
}

void JumpTableInstruction::Unlink()
{
	for(Instruction* &target : targetList)
	{
		if(target) target->referenceList.Remove(&target);
	}
}

Instruction* ByteJumpTableInstruction::Clone() const
{
	return new ByteJumpTableInstruction(*this);
}

int JumpTableInstruction::GetNumberOfPlanes() const
{
	if(targetList.HasData() && targetList[0] == nullptr) return (int) targetList.GetCount()-1;
	else return (int) targetList.GetCount();
}

void JumpTableInstruction::SetValidBytesForPlane(int plane, StaticBitTable<256>& mask) const
{
	if(targetList[0] == nullptr) ++plane;
	for(uint32_t i = 0; i < 256; ++i)
	{
		if(targetTable[i] == plane) mask.SetBit(i);
	}
}

Instruction* JumpTableInstruction::GetTargetForPlane(int plane) const
{
	if(targetList[0] == nullptr) return targetList[plane+1];
	else return targetList[plane];
}

Instruction* ByteNotInstruction::Clone() const
{
	return new ByteNotInstruction(*this);
}

Instruction* DispatchTableInstruction::Clone() const
{
	return new DispatchTableInstruction(*this);
}

uint32_t ByteNotInstruction::GetByteCodeData() const
{
	return c;
}

void ByteNotInstruction::SetValidBytes(StaticBitTable<256>& mask) const
{
	bool isBitSet = mask[c];
	mask.SetAllBits();
	if(!isBitSet) mask.ClearBit(c);
}

Instruction* ByteNotEitherOf2Instruction::Clone() const
{
	return new ByteNotEitherOf2Instruction(*this);
}

uint32_t ByteNotEitherOf2Instruction::GetByteCodeData() const
{
	return c[0] | c[1] << 8;
}

void ByteNotEitherOf2Instruction::SetValidBytes(StaticBitTable<256>& mask) const
{
	bool isC0Set = mask[c[0]];
	bool isC1Set = mask[c[1]];
	mask.SetAllBits();
	if(!isC0Set) mask.ClearBit(c[0]);
	if(!isC1Set) mask.ClearBit(c[1]);
}

Instruction* ByteNotEitherOf3Instruction::Clone() const
{
	return new ByteNotEitherOf3Instruction(*this);
}

uint32_t ByteNotEitherOf3Instruction::GetByteCodeData() const
{
	return c[0] | c[1]<<8 | c[2]<<16;
}

void ByteNotEitherOf3Instruction::SetValidBytes(StaticBitTable<256>& mask) const
{
	bool isC0Set = mask[c[0]];
	bool isC1Set = mask[c[1]];
	bool isC2Set = mask[c[2]];
	mask.SetAllBits();
	if(!isC0Set) mask.ClearBit(c[0]);
	if(!isC1Set) mask.ClearBit(c[1]);
	if(!isC2Set) mask.ClearBit(c[2]);
}

Instruction* ByteNotRangeInstruction::Clone() const
{
	return new ByteNotRangeInstruction(*this);
}

uint32_t ByteNotRangeInstruction::GetByteCodeData() const
{
	return byteRange.min | byteRange.max<<8;
}

void ByteNotRangeInstruction::SetValidBytes(StaticBitTable<256>& mask) const
{
	for(uint32_t i = 0; i < byteRange.min; ++i) mask.SetBit(i);
	for(uint32_t i = byteRange.max+1; i < 256; ++i) mask.SetBit(i);
}

Instruction* ByteBitMaskInstruction::Clone() const
{
	return new ByteBitMaskInstruction(*this);
}

void ByteBitMaskInstruction::BuildByteCode(ByteCodeBuilder& builder) const
{
	static_assert(sizeof(bitMask) == 32, "Ensure bitMask has expected size");
	
	size_t populationCount = bitMask.GetPopulationCount();

	if(populationCount == 0)
	{
		builder.AddInstruction(this, InstructionType::Fail, false);
	}
	else if(populationCount == 256)
	{
		builder.AddInstruction(this, InstructionType::AnyByte);
	}
	else if(populationCount <= 3)
	{
		unsigned char c[4];
		uint32_t cIndex = 0;
		for(uint32_t i = 0; i < 256 && cIndex != populationCount; ++i)
		{
			if(bitMask[i]) c[cIndex++] = i;
		}

		if(cIndex >= 2 && c[0]+cIndex-1 == c[cIndex-1])
		{
			unsigned char low = c[0];
			unsigned char high = c[cIndex-1];
			builder.AddInstruction(this, InstructionType::ByteRange, low | high<<8);
		}
		else
		{
			switch(cIndex)
			{
			case 1:	builder.AddInstruction(this, InstructionType::Byte, c[0]); break;
			case 2: builder.AddInstruction(this, InstructionType::ByteEitherOf2, c[0] | c[1] << 8); break;
			case 3: builder.AddInstruction(this, InstructionType::ByteEitherOf3, c[0] | c[1] << 8 | c[2]<<16); break;
			}
		}
	}
	else if(populationCount >= 253)
	{
		unsigned char c[4];
		uint32_t cIndex = 0;
		for(uint32_t i = 0; i < 256 && cIndex != 256-populationCount; ++i)
		{
			if(!bitMask[i]) c[cIndex++] = i;
		}
		
		if(cIndex >= 2 && c[0]+cIndex-1 == c[cIndex-1])
		{
			unsigned char low = c[0];
			unsigned char high = c[cIndex-1];
			builder.AddInstruction(this, InstructionType::ByteNotRange, low | high<<8);
		}
		else
		{
			switch(cIndex)
			{
			case 1:	builder.AddInstruction(this, InstructionType::ByteNot, c[0]); break;
			case 2: builder.AddInstruction(this, InstructionType::ByteNotEitherOf2, c[0] | c[1] << 8); break;
			case 3: builder.AddInstruction(this, InstructionType::ByteNotEitherOf3, c[0] | c[1] << 8 | c[2]<<16); break;
			}
		}
	}
	else
	{
		uint32_t offset = builder.GetOffsetForData(&bitMask, sizeof(bitMask));
		builder.AddInstruction(this, InstructionType::ByteBitMask, offset);
	}
}

void ByteBitMaskInstruction::SetValidBytes(StaticBitTable<256>& mask) const
{
	mask |= bitMask;
}

Instruction* CallInstruction::Clone() const
{
	CallInstruction* result = new CallInstruction(*this);
	result->Link();
	return result;
}

void CallInstruction::BuildByteCode(ByteCodeBuilder& builder) const
{
	ByteCodeCallData data;
	data.callIndex		= callTarget->index;
	data.falseIndex		= falseTarget ? falseTarget->index : TypeData<uint32_t>::Maximum();
	data.trueIndex		= trueTarget  ? trueTarget->index  : TypeData<uint32_t>::Maximum();

	uint32_t offset = builder.GetOffsetForData(&data, sizeof(data));
	builder.AddInstruction(this, type, offset);
}

Instruction* CallInstruction::ProcessReachable(int flags)
{
	callTarget->TagReachable(flags);
	if(falseTarget) falseTarget->TagReachable(flags);
	return trueTarget;
}

void CallInstruction::Link()
{
	callTarget->referenceList.Append(InstructionReference{&callTarget, this});
	if(falseTarget) falseTarget->referenceList.Append(InstructionReference{&falseTarget, this});
	if(trueTarget) trueTarget->referenceList.Append(InstructionReference{&trueTarget, this});
}

void CallInstruction::Unlink()
{
	callTarget->referenceList.Remove(&callTarget);
	if(falseTarget) falseTarget->referenceList.Remove(&falseTarget);
	if(trueTarget) trueTarget->referenceList.Remove(&trueTarget);
}

Instruction* FailInstruction::Clone() const
{
	return new FailInstruction(*this);
}

Instruction* FindByteInstruction::Clone() const
{
	return new FindByteInstruction(*this);
}

uint32_t FindByteInstruction::GetByteCodeData() const
{
	return c | target->index<<8;
}

void FindByteInstruction::Link()
{
	referenceList.Append(InstructionReference{InstructionReference::SELF_REFERENCE, this});
	target->referenceList.Append(InstructionReference{&target, this});
}

void FindByteInstruction::Unlink()
{
	referenceList.Remove(InstructionReference::SELF_REFERENCE);
	target->referenceList.Remove(&target);
}

Instruction* PossessInstruction::Clone() const
{
	PossessInstruction* result = new PossessInstruction(*this);
	result->Link();
	return result;
}

void PossessInstruction::BuildByteCode(ByteCodeBuilder& builder) const
{
	ByteCodeCallData data;
	data.callIndex		= callTarget->index;
	data.falseIndex		= TypeData<uint32_t>::Maximum();
	data.trueIndex		= trueTarget->index;

	uint32_t offset = builder.GetOffsetForData(&data, sizeof(data));
	builder.AddInstruction(this, type, offset);
}

Instruction* PossessInstruction::ProcessReachable(int flags)
{
	callTarget->TagReachable(flags);
	return trueTarget;
}

void PossessInstruction::Link()
{
	callTarget->referenceList.Append(InstructionReference{&callTarget, this});
	trueTarget->referenceList.Append(InstructionReference{&trueTarget, this});
}

void PossessInstruction::Unlink()
{
	callTarget->referenceList.Remove(&callTarget);
	trueTarget->referenceList.Remove(&trueTarget);
}

Instruction* PropagateBackwardsInstruction::Clone() const
{
	return new PropagateBackwardsInstruction(*this);
}

void PropagateBackwardsInstruction::BuildByteCode(ByteCodeBuilder& builder) const
{
	uint32_t offset = builder.GetOffsetForData(&bitMask, sizeof(bitMask));
	builder.AddInstruction(this, type, offset);
}

SearchByteInstruction::SearchByteInstruction(unsigned char aC, unsigned aOffset)
: Instruction(InstructionType::SearchByte)
{
	bytes[0] = aC;
	bytes[1] = 0;
	bytes[2] = 0;
	bytes[3] = 0;
	bytes[4] = 0;
	bytes[5] = 0;
	bytes[6] = 0;
	bytes[7] = 0;
	offset = aOffset;
}

SearchByteInstruction::SearchByteInstruction(unsigned char numberOfBytes, unsigned char aBytes[8], unsigned aOffset)
: Instruction((InstructionType) ((int) InstructionType::SearchByteEitherOf2 - 2 + numberOfBytes))
{
	JASSERT(1 <= numberOfBytes && numberOfBytes <= 8);
	memcpy(bytes, aBytes, 8);
	offset = aOffset;
}

SearchByteInstruction::SearchByteInstruction(InstructionType type, unsigned char aBytes[8], unsigned aOffset)
: Instruction(type)
{
	memcpy(bytes, aBytes, 8);
	offset = aOffset;
}

SearchByteInstruction::SearchByteInstruction(InstructionType type, unsigned char aBytes[8], unsigned aOffset, const Table<NibbleMask>& aNibbleMaskList)
: Instruction(type),
  nibbleMaskList(aNibbleMaskList)
{
	memcpy(bytes, aBytes, 8);
	offset = aOffset;
}

Instruction* SearchByteInstruction::Clone() const
{
	return new SearchByteInstruction(*this);
}

void SearchByteInstruction::BuildByteCode(ByteCodeBuilder& builder) const
{
	if(type == InstructionType::SearchByte)
	{
		builder.AddInstruction(this, type, bytes[0] | offset<<8);
	}
	else
	{
		char buffer[sizeof(ByteCodeSearchByteData) + sizeof(ByteCodeSearchMultiByteData)];
		ByteCodeSearchByteData* data = (ByteCodeSearchByteData*) buffer;
		
		memcpy(data->bytes, bytes, 8);
		data->offset = offset;

		size_t sizeOfData = sizeof(ByteCodeSearchByteData);
		static constexpr EnumSet<InstructionType, uint64_t> SMALLER_DATA
		{
			InstructionType::SearchByteEitherOf2,
			InstructionType::SearchByteEitherOf3,
			InstructionType::SearchByteEitherOf4,
			InstructionType::SearchBytePair,
			InstructionType::SearchBytePair2,
			InstructionType::SearchByteRange,
			InstructionType::SearchByteRangePair,
			InstructionType::SearchByteTriplet,
		};
		if(SMALLER_DATA.Contains(type)) sizeOfData -= 4;
		
		static constexpr EnumSet<InstructionType, uint64_t> MULTI_BYTE_SEARCH
		{
			InstructionType::SearchBytePair,
			InstructionType::SearchBytePair2,
			InstructionType::SearchBytePair3,
			InstructionType::SearchBytePair4,
			InstructionType::SearchByteRangePair,
			InstructionType::SearchByteTriplet,
			InstructionType::SearchByteTriplet2,
		};
		
		if(MULTI_BYTE_SEARCH.Contains(type))
		{
			ByteCodeSearchMultiByteData* multiByteData = (ByteCodeSearchMultiByteData*) &buffer[sizeOfData];
			multiByteData->Set(nibbleMaskList);

			// Special case to order SearchBytePair2
			if(type == InstructionType::SearchBytePair2)
			{
				if(data->bytes[0] == data->bytes[1] || data->bytes[2] == data->bytes[3])
				{
					multiByteData->numberOfNibbleMasks = 0;
					multiByteData->isPath = false;
				}
				else if(nibbleMaskList.GetCount() == 2)
				{
					if((nibbleMaskList[0].GetMaskForByte(data->bytes[0]) & 1) == 0)
					{
						Swap(data->bytes[0], data->bytes[1]);
					}
					if((nibbleMaskList[1].GetMaskForByte(data->bytes[2]) & 1) == 0)
					{
						Swap(data->bytes[2], data->bytes[3]);
					}
				}
			}
			
			if(multiByteData->numberOfNibbleMasks == 0
			   && (type == InstructionType::SearchBytePair3 || type == InstructionType::SearchBytePair4))
			{
				int bytes = int(type) - int(InstructionType::SearchBytePair) + 1;
				NibbleMask nibbleMaskList[2];
				
				for(int i = 0; i < bytes; ++i)
				{
					nibbleMaskList[0].AddByte(data->bytes[i]);
					nibbleMaskList[1].AddByte(data->bytes[bytes+i]);
				}
				multiByteData->numberOfNibbleMasks = 2;
				multiByteData->isPath = false;
				nibbleMaskList[0].CopyMaskToTarget(multiByteData->nibbleMask[0]);
				nibbleMaskList[1].CopyMaskToTarget(multiByteData->nibbleMask[2]);
			}

			sizeOfData += sizeof(uint32_t) + 32*multiByteData->numberOfNibbleMasks;
		}

		uint32_t dataOffset = builder.GetOffsetForData(buffer, sizeOfData);
		builder.AddInstruction(this, type, dataOffset);
	}
}

Instruction* SearchDataInstruction::Clone() const
{
	return new SearchDataInstruction(*this);
}

SearchDataInstruction::SearchDataInstruction(InstructionType type, uint32_t aLength, uint32_t aData[256])
: Instruction(type),
  length(aLength)
{
	CopyMemory(data, aData, 256);
}

SearchDataInstruction::SearchDataInstruction(InstructionType type, uint32_t aLength, uint32_t aData[256], const Table<NibbleMask>& aNibbleMaskList)
: Instruction(type),
  length(aLength),
  nibbleMaskList(aNibbleMaskList)
{
	CopyMemory(data, aData, 256);
}

void SearchDataInstruction::BuildByteCode(ByteCodeBuilder& builder) const
{
	struct Data
	{
		ByteCodeSearchData 			searchData;
		ByteCodeSearchMultiByteData	multiByteData;
	};
	
	Data byteCodeData;
	
	byteCodeData.searchData.length = length;
	CopyMemory(byteCodeData.searchData.data, data, 256);

	size_t dataSize = sizeof(ByteCodeSearchData);
	
	if(type == InstructionType::SearchShiftOr)
	{
		if(nibbleMaskList.GetCount())
		{
			byteCodeData.multiByteData.Set(nibbleMaskList);
		}
		else if(length <= 2)
		{
			byteCodeData.multiByteData.Set(data, length+1);
		}
		else
		{
			byteCodeData.multiByteData.SetUnused();
		}
		dataSize += sizeof(uint32_t) + 32*byteCodeData.multiByteData.numberOfNibbleMasks;
	}
	
	uint32_t dataOffset = builder.GetOffsetForData(&byteCodeData, dataSize);
	builder.AddInstruction(this, type, dataOffset);
}

Instruction* MatchInstruction::Clone() const
{
	return new MatchInstruction(*this);
}

Instruction* ProgressCheckInstruction::Clone() const
{
	return new ProgressCheckInstruction(*this);
}

Instruction* RecurseInstruction::Clone() const
{
	RecurseInstruction* result = new RecurseInstruction(*this);
	result->Link();
	return result;
}

uint32_t RecurseInstruction::GetByteCodeData() const
{
	return callTarget->index<<8 | recurseValue;
}

Instruction* RecurseInstruction::ProcessReachable(int flags)
{
	callTarget->TagReachable(flags);
	return Instruction::ProcessReachable(flags);
}

void RecurseInstruction::Link()
{
	callTarget->referenceList.Append(InstructionReference{&callTarget, this});
	Instruction::Link();
}

void RecurseInstruction::Unlink()
{
	callTarget->referenceList.Remove(&callTarget);
	Instruction::Unlink();
}

Instruction* ReturnIfRecurseValueInstruction::Clone() const
{
	return new ReturnIfRecurseValueInstruction(*this);
}

Instruction* SaveInstruction::Clone() const
{
	return new SaveInstruction(*this);
}

void SaveInstruction::BuildByteCode(ByteCodeBuilder& builder) const
{
	uint32_t data = saveIndex + (saveOffset<<8);
	builder.AddInstruction(this, neverRecurse || this->LeadsToMatch(true) ? InstructionType::SaveNoRecurse : InstructionType::Save, data);
}

Instruction* SplitInstruction::Clone() const
{
	SplitInstruction* result = new SplitInstruction(*this);
	result->Link();
	return result;
}

Instruction* SplitInstruction::ProcessReachable(int flags)
{
	for(Instruction* target : targetList)
	{
		target->TagReachable(flags);
	}
	return nullptr;
}

void SplitInstruction::Link()
{
	for(Instruction* &target : targetList)
	{
		target->referenceList.Append(InstructionReference{&target, this});
	}
}

void SplitInstruction::Unlink()
{
	for(Instruction* &target : targetList)
	{
		target->referenceList.Remove(&target);
	}
}

Instruction* StepBackInstruction::Clone() const
{
	return new StepBackInstruction(*this);
}


Instruction* SuccessInstruction::Clone() const
{
	return new SuccessInstruction(*this);
}

void SplitInstruction::BuildByteCode(ByteCodeBuilder& builder) const
{
	uint32_t numberOfTargets = (uint32_t) targetList.GetCount();
	
	if(numberOfTargets == 0)
	{
		builder.AddInstruction(this, InstructionType::Fail);
		return;
	}
	else if(numberOfTargets == 1)
	{
		builder.AddInstruction(this, InstructionType::Jump, targetList[0]->index);
		return;
	}
	
	uint32_t byteCodeData[numberOfTargets+1];
	
	if(numberOfTargets == 2)
	{
		Instruction* nextInstruction = GetNext();
		if(targetList[0] == nextInstruction)
		{
			builder.AddInstruction(this, targetList[0]->LeadsToMatch(true) ? InstructionType::SplitNextMatchN : InstructionType::SplitNextN, targetList[1]->index);
			return;
		}
		else if(targetList[1] == nextInstruction)
		{
			builder.AddInstruction(this, targetList[0]->LeadsToMatch(true) ? InstructionType::SplitNMatchNext : InstructionType::SplitNNext, targetList[0]->index);
			return;
		}
		else if(targetList[0]->LeadsToMatch(true))
		{
			byteCodeData[0] = targetList[0]->index;
			byteCodeData[1] = targetList[1]->index;
			uint32_t offset = builder.GetOffsetForData(byteCodeData, 2*sizeof(uint32_t));
			builder.AddInstruction(this, InstructionType::SplitMatch, offset);
			return;
		}
	}

	byteCodeData[0] = numberOfTargets;
	for(size_t i = 0; i < numberOfTargets; ++i)
	{
		byteCodeData[i+1] = targetList[i]->index;
	}
	
	uint32_t offset = builder.GetOffsetForData(byteCodeData, sizeof(uint32_t) * (numberOfTargets+1));
	
	builder.AddInstruction(this, InstructionType::Split, offset);
}

//==========================================================================
