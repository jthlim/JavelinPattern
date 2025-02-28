//==========================================================================

#include "Javelin/Pattern/Internal/PatternInstructionList.h"
#include "Javelin/Container/EnumSet.h"
#include "Javelin/Container/Map.h"
#include "Javelin/Math/MathFunctions.h"
#include "Javelin/Pattern/Internal/PatternByteCode.h"
#include "Javelin/Pattern/Internal/PatternCompiler.h"
#include "Javelin/Pattern/Internal/PatternComponent.h"
#include "Javelin/Pattern/Internal/PatternInstruction.h"
#include "Javelin/Pattern/Internal/PatternNibbleMask.h"
#include "Javelin/Pattern/Pattern.h"
#include "Javelin/Stream/ICharacterWriter.h"
#include "Javelin/Stream/StandardWriter.h"
#include "Javelin/Type/ObjectWithHash.h"

//==========================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//==========================================================================

#define JDUMP_BYTECODE_OPTIMIZATION_INFORMATION		0	// No effect in final builds.
#define JDUMP_SEARCH_INFORMATION					0

//==========================================================================

#if JDUMP_SEARCH_INFORMATION
	#define DEBUG_SEARCH_PRINTF(...)	StandardError.PrintF(__VA_ARGS__)
#else
	#define DEBUG_SEARCH_PRINTF(...)	((void) sizeof(__VA_ARGS__))
#endif

//==========================================================================

static StaticBitTable<256> MakeWordMask()
{
	StaticBitTable<256> mask;
	for(int i = 0; i < 256; ++i) if(Character::IsWordCharacter(i)) mask.SetBit(i);
	return mask;
}

//==========================================================================

struct InstructionList::StateMap
{
	typedef Map<InstructionTable, Instruction*> StatesToInstructionMap;
	typedef Map<Instruction*, InstructionTable> InstructionToStatesMap;
	
	StatesToInstructionMap statesToInstructionMap;
	InstructionToStatesMap instructionToStatesMap;
	
	StateMap()
	: statesToInstructionMap(4),
	  instructionToStatesMap(4)
	{
	}
	
	void Insert(const ObjectWithHash<InstructionTable&> &state, Instruction* instruction)
	{
		statesToInstructionMap.Insert(state, instruction);
		instructionToStatesMap.Insert(instruction, state);
	}
	
	void Remove(const ObjectWithHash<InstructionTable&> &state, Instruction* split)
	{
		JASSERT(instructionToStatesMap[split] == state);
		JASSERT(statesToInstructionMap[state] == split);
		statesToInstructionMap.Remove(state);
		instructionToStatesMap.Remove(split);
	}
	
	StatesToInstructionMap::ConstIterator Find(const ObjectWithHash<InstructionTable&> &lookup) const 	{ return statesToInstructionMap.Find(lookup); }
	StatesToInstructionMap::ConstIterator Find(const InstructionTable& lookup) const 					{ return statesToInstructionMap.Find(lookup); }
	StatesToInstructionMap::ConstIterator End() const													{ return statesToInstructionMap.End(); }
	
	InstructionTable MakeCanonicalStateList(Instruction* original, const InstructionTable& targetList) const;
	InstructionTable MakeCanonicalStateList(SplitInstruction* split) const	{ return MakeCanonicalStateList(split, split->targetList); }
	void RecurseAddTargets(InstructionTable& result, Instruction* original, Instruction* target) const;
};

void InstructionList::StateMap::RecurseAddTargets(InstructionTable& result, Instruction* original, Instruction* target) const
{
	if(target == original) return;
	InstructionToStatesMap::ConstIterator it = instructionToStatesMap.Find(target);
	if(it != instructionToStatesMap.End())
	{
		for(Instruction* state : it->value)
		{
			RecurseAddTargets(result, original, state);
		}
	}
	else
	{
		switch(target->type)
		{
		case InstructionType::Split:
			for(Instruction* splitTarget : ((SplitInstruction*) target)->targetList)
			{
				RecurseAddTargets(result, original, splitTarget);
			}
			break;
			
		case InstructionType::Jump:
			RecurseAddTargets(result, original, ((JumpInstruction*) target)->target);
			break;
		
		case InstructionType::ProgressCheck:
			{
				Instruction* next = target->GetNext();
				if(next == original || result.Contains(next)) break;
				result.AppendUnique(target);
				break;
			}
				
		case InstructionType::Fail:
			break;
			
		default:
			result.AppendUnique(target);
			break;
		}
	}
}

InstructionTable InstructionList::StateMap::MakeCanonicalStateList(Instruction* original, const InstructionTable& targetList) const
{
	InstructionTable result;
	for(Instruction* target : targetList)
	{
		RecurseAddTargets(result, original, target);
	}
	return result;
}

//==========================================================================

InstructionList::~InstructionList()
{
	while(instructionList.HasData()) delete &instructionList.Front();
	while(searchOptimizerList.HasData()) delete &searchOptimizerList.Front();
}

void InstructionList::Dump(ICharacterWriter& output)
{
	IndexInstructions();
	UpdateReferencesForInstructions();
	
	for(Instruction& instruction : instructionList)
	{
		output.PrintF("%p %3z: %z ref: ", &instruction, instruction.index, instruction.referenceList.GetCount());
		instruction.Dump(output);
		output.PrintF("\n");
	}
}

void InstructionList::AddInstruction(Instruction* instruction)
{
	Patch(instruction);
	instructionList.Append(instruction);
}

void InstructionList::RemoveInstruction(Instruction* instruction)
{
	instruction->listNode.Remove();
}

void InstructionList::ReplaceInstruction(Instruction* old, Instruction* newInstruction)
{
	old->listNode.ReplaceWith(&newInstruction->listNode);
}

void InstructionList::InsertAfterInstruction(Instruction* instruction, Instruction* newInstruction)
{
	instruction->listNode.InsertAfter(&newInstruction->listNode);
}

void InstructionList::InsertBeforeInstruction(Instruction* instruction, Instruction* newInstruction)
{
	instruction->listNode.InsertBefore(&newInstruction->listNode);
}

void InstructionList::Patch(Instruction* instruction)
{
	for(Instruction** patch : patchList)
	{
		*patch = instruction;
	}
	patchList.SetCount(0);
}

//==========================================================================

bool InstructionList::ShouldEmitSaveInstructions() const
{
	if(scanDirection == Reverse)
	{
		// If the processor type isn't default, then we can never get
		// to the OnePass reverse processor
		if(processorType != PatternProcessorType::Default) return false;
	}
	return saveCounter == 0;
}

//==========================================================================

void InstructionList::IndexInstructions()
{
	uint32_t offset = 0;
	for(Instruction& instruction : instructionList)
	{
		instruction.index = offset++;
	}
	totalNumberOfInstructions = offset;
}

void InstructionList::UpdateReferencesForInstructions()
{
	for(Instruction& instruction : instructionList)
	{
		instruction.referenceList.SetCount(0);
	}
	
	for(SearchOptimizer& searchOptimizer : searchOptimizerList)
	{
		searchOptimizer.original->referenceList.Append(InstructionReference{&searchOptimizer.original, InstructionReference::SEARCH_OPTIMIZER_REFERENCE});
	}
	
	if(scanDirection == Forwards)
	{
		partialMatchInstruction->referenceList.Append(InstructionReference{&partialMatchInstruction, InstructionReference::START_REFERENCE});		
	}
	fullMatchInstruction->referenceList.Append(InstructionReference{&fullMatchInstruction, InstructionReference::START_REFERENCE});
	
	for(Instruction& instruction : instructionList)
	{
		instruction.Link();
	}
}

void InstructionList::DumpReferenceMismatch(const ReferenceTable& before, size_t instructionIndex, const char* optimizationPass)
{
	StandardOutput.PrintF("Mismatch on instruction %z after %s in:\n", instructionIndex, optimizationPass);
	Dump(StandardOutput);
	StandardOutput.PrintF("Current: ");
	for(InstructionReference ir : before) StandardOutput.PrintF(" %p:%p", ir.storage, ir.instruction);
	StandardOutput.PrintF("\nExpected: ");
	for(InstructionReference ir : instructionList[instructionIndex].referenceList) StandardOutput.PrintF(" %p:%p", ir.storage, ir.instruction);
	StandardOutput.PrintF("\n");
}

bool InstructionList::VerifyReferencesForInstructions(const char* optimizationPass)
{
	Table< ReferenceTable > snapShot;
	snapShot.SetCount(instructionList.GetCount());
	size_t i = 0;
	for(Instruction& instruction : instructionList)
	{
		snapShot[i] = instruction.referenceList;
		++i;
	}
	
	UpdateReferencesForInstructions();
	
	i = 0;
	for(Instruction& instruction : instructionList)
	{
		const ReferenceTable &singleSnapShot = snapShot[i];
		const ReferenceTable &current = instruction.referenceList;
	
		if(singleSnapShot.GetCount() != current.GetCount())
		{
			DumpReferenceMismatch(singleSnapShot, i, optimizationPass);
			return false;
		}
		for(InstructionReference ref : singleSnapShot)
		{
			if(!current.Contains(ref))
			{
				DumpReferenceMismatch(singleSnapShot, i, optimizationPass);
				return false;
			}
		}
		++i;
	}
	
#if JDUMP_BYTECODE_OPTIMIZATION_INFORMATION
	StandardOutput.PrintF("\nAfter optimization pass: %s\n", optimizationPass);
	Dump(StandardOutput);
#endif
	
	return true;
}

//==========================================================================

static constexpr EnumSet<InstructionType, uint64_t> NO_IMPLICIT_NEXT_INSTRUCTION_SET
{
	InstructionType::ByteJumpTable,
	InstructionType::DispatchTable,
	InstructionType::Call,
	InstructionType::Possess,
	InstructionType::Fail,
	InstructionType::Jump,
	InstructionType::Match,
	InstructionType::Split,
	InstructionType::Success,
};

#if DUMP_OPTIMIZATION_STATISTICS
int optimizeStep[32];

static Table<const Instruction*> ConvertToInstructionListToTable(const IntrusiveList<Instruction, &Instruction::listNode>& list)
{
	Table<const Instruction*> result(list.GetCount());
	for(const Instruction& instruction : list) result.Append(&instruction);
	return result;
}
#endif

JINLINE void InstructionList::RunOptimizeStep(void (InstructionList::*step)(), const char* name)
{
#if DUMP_OPTIMIZATION_STATISTICS
	Table<const Instruction*> snapShot = ConvertToInstructionListToTable(instructionList);
#endif

	(this->*step)();
	JASSERT(VerifyReferencesForInstructions(name));

#if DUMP_OPTIMIZATION_STATISTICS
	if(snapShot != ConvertToInstructionListToTable(instructionList))
	{
		optimizeStep[optimizationStepCounter]++;
	}
	++optimizationStepCounter;
#endif
}

void InstructionList::Optimize()
{
	if(options & Pattern::NO_OPTIMIZE)
	{
		if(scanDirection == Reverse)
		{
			CleanReverseProgram();
		}
		return;
	}
	
#define STEP(x) RunOptimizeStep(&InstructionList::x, #x)
	STEP(Optimize_RemoveAssertEndOfInput);									// This also determines matchRequiresEndOfInput flag
	if(scanDirection == Reverse) STEP(Optimize_RemoveLeadingAsserts);
	STEP(Optimize_SimplifyWordBoundaryAsserts);
	if(scanDirection == Forwards) STEP(Optimize_CaptureSearch);
	if(scanDirection == Reverse) STEP(Optimize_SimpleByteConsumersToAdvanceByte);
	STEP(Optimize_LeftFactor);
	STEP(Optimize_RemoveZeroReference);
	STEP(Optimize_RightFactor);
	if(scanDirection == Forwards) STEP(Optimize_DelaySave);
	STEP(Optimize_CollapseSplit);
	STEP(Optimize_RemoveZeroReference);
	STEP(Optimize_ForwardJumpTargets);
	STEP(Optimize_SplitToDispatch);
	if(scanDirection == Reverse)
	{
		CleanReverseProgram();
		if(processorType == PatternProcessorType::NotOnePass)
		{
			STEP(Optimize_CollapseSplit);
			STEP(Optimize_SplitToDispatch);
			STEP(Optimize_DispatchTargetsToAdvanceByte);
		}
		else
		{
			STEP(Optimize_SimpleByteConsumersToAdvanceByte);
		}
	}
	if(scanDirection == Forwards) STEP(Optimize_SplitToMatchReorder);
	STEP(Optimize_ForwardJumpTargets);
	STEP(Optimize_RightFactor);
	STEP(Optimize_RemoveZeroReference);
	if(scanDirection == Forwards) STEP(Optimize_RemoveJumpToNext);
	if(scanDirection == Forwards) STEP(Optimize_DelaySave);
	if(scanDirection == Forwards) STEP(Optimize_DispatchTargetsToAdvanceByte);
	if(scanDirection == Forwards) STEP(Optimize_AccelerateSearch);
	if(scanDirection == Forwards) STEP(Optimize_ByteJumpTableToFindByte);
	if(scanDirection == Reverse) STEP(Optimize_ForwardJumpTargets);
	if(scanDirection == Reverse) STEP(Optimize_RightFactor);
	if(scanDirection == Reverse) STEP(Optimize_SplitToMatchReorder);
	if(scanDirection == Reverse) STEP(Optimize_RemoveZeroReference);
	if(scanDirection == Reverse) STEP(Optimize_RemoveJumpToNext);
	if(scanDirection == Reverse) STEP(Optimize_ForwardJumpTargets);
	if(scanDirection == Reverse) STEP(Optimize_RemoveZeroReference);
	if(scanDirection == Reverse) STEP(Optimize_ByteJumpTableToFindByte);
#undef STEP
	
#if DUMP_OPTIMIZATION_STATISTICS
	StandardOutput.PrintF("OPTIMIZATION STEPS\n");
	for(uint32_t i = 0; i < optimizationStepCounter; ++i)
	{
		StandardOutput.PrintF("%u: %u\n", i, optimizeStep[i]);
	}
	StandardOutput.PrintF("\n");
#endif
}

void InstructionList::CleanReverseProgram()
{
	if(instructionList.HasElement([](const Instruction& a) -> bool {
		return a.type == InstructionType::Split;
	}))
	{
		static constexpr EnumSet<InstructionType, uint64_t> SAVE_SET
		{
			InstructionType::Save,
			InstructionType::SaveNoRecurse
		};
		processorType = PatternProcessorType::NotOnePass;
		RemoveAllInstructionsOfType(SAVE_SET);
	}
	else
	{
		// If there are no branches in the stream, then all of the
		// following instructions are unnecessary as they would have
		// been guaranteed in the forward pass
		static constexpr EnumSet<InstructionType, uint64_t> REMOVE_SET
		{
			InstructionType::AssertStartOfInput,
			InstructionType::AssertEndOfInput,
			InstructionType::AssertStartOfLine,
			InstructionType::AssertEndOfLine,
			InstructionType::AssertWordBoundary,
			InstructionType::AssertNotWordBoundary,
			InstructionType::AssertStartOfSearch,
			InstructionType::ProgressCheck
		};
		RemoveAllInstructionsOfType(REMOVE_SET);
	}
}

void InstructionList::PropagateFail(Instruction* instruction, Instruction* fail)
{
	for(size_t i = instruction->referenceList.GetCount(); i > 0;)
	{
		InstructionReference ref = instruction->referenceList[--i];
		
		Instruction* ancestor = ref.GetInstruction(instruction);
		if(ancestor)
		{
			switch(ancestor->type)
			{
			case InstructionType::ByteJumpTable:
			case InstructionType::DispatchTable:
				if(ref.storage) *ref.storage = fail;
				fail->referenceList.Append(ref);
				break;

			case InstructionType::Split:
				{
					SplitInstruction* split = (SplitInstruction*) ancestor;
					split->Unlink();
					split->targetList.Remove(instruction);
					if(split->targetList.GetCount() == 0)
					{
						PropagateFail(split, fail);
					}
					else if(split->targetList.GetCount() == 1)
					{
						if(split->targetList[0] == split->GetNext())
						{
							split->TransferReferencesTo(split->GetNext());
							RemoveInstruction(split);
						}
						else
						{
							JumpInstruction* jump = new JumpInstruction(split->targetList[0]);
							split->TransferReferencesTo(jump);
							ReplaceInstruction(split, jump);
						}
						delete split;
					}
					else
					{
						split->Link();
					}
				}
				break;

			default:
				if(ref.storage) *ref.storage = fail;
				PropagateFail(ancestor, fail);
				break;
			}
		}
		else if(ref.storage != InstructionReference::SELF_REFERENCE)
		{
			if(ref.storage) *ref.storage = fail;
			fail->referenceList.Append(ref);
		}
	}
	RemoveInstruction(instruction);
	delete instruction;
}

Instruction* InstructionList::ReplaceWithFail(Instruction* old)
{
	Instruction* replacement = new FailInstruction;
	old->Unlink();
	InsertAfterInstruction(old, replacement);
	PropagateFail(old, replacement);
	return replacement;
}

void InstructionList::RemoveAllInstructionsOfType(const EnumSet<InstructionType, uint64_t>& typeSet)
{
	for(LinkedInstructionList::Iterator it = instructionList.ReverseBegin(); it != instructionList.ReverseEnd();)
	{
		Instruction* instruction = &*it;
		--it;
		if(typeSet.Contains(instruction->type))
		{
			instruction->Unlink();
			instruction->TransferReferencesTo(instruction->GetNext());
			RemoveInstruction(instruction);
			delete instruction;
		}
	}
}

/// Only used for reverse searches -- asserts before a byte match are not necessary
/// As the reverse processor is only called if a forward processor has already
/// ensured that the asserts are valid.
void InstructionList::Optimize_RemoveLeadingAsserts()
{
	Instruction* instruction = &instructionList.Front();
	for(;;)
	{
		if(instruction->referenceList.GetCount() > 1) return;
		
		switch(instruction->type)
		{
		case InstructionType::AssertEndOfInput:
		case InstructionType::AssertEndOfLine:
		case InstructionType::AssertStartOfSearch:
		case InstructionType::AssertNotWordBoundary:
		case InstructionType::AssertStartOfInput:
		case InstructionType::AssertStartOfLine:
		case InstructionType::AssertWordBoundary:
			{
				Instruction* next = instruction->GetNext();
				next->referenceList.Remove(nullptr);
				instruction->TransferReferencesTo(next);
				RemoveInstruction(instruction);
				delete instruction;
				instruction = next;
			}
			break;
				
		case InstructionType::Jump:
			instruction = ((JumpInstruction*) instruction)->target;
			break;
			
		case InstructionType::Save:
			instruction = instruction->GetNext();
			break;
				
		default:
			return;
		}
	}
}

void InstructionList::Optimize_SimplifyWordBoundaryAsserts()
{
	static constexpr EnumSet<InstructionType, uint64_t> AFTER_SKIP_INSTRUCTIONS
	{
		InstructionType::ProgressCheck,
		InstructionType::Save,
		InstructionType::SaveNoRecurse
	};

	static constexpr EnumSet<InstructionType, uint64_t> BEFORE_SKIP_INSTRUCTIONS
	{
		InstructionType::ProgressCheck,
		InstructionType::Save,
		InstructionType::SaveNoRecurse,
		InstructionType::Split,
	};

	static constexpr EnumSet<InstructionType, uint64_t> NON_WORD_CHARACTER_ASSERT_INSTRUCTIONS
	{
		InstructionType::AssertStartOfLine,
		InstructionType::AssertStartOfInput,
		InstructionType::AssertEndOfLine,
		InstructionType::AssertEndOfInput,
	};

	static const StaticBitTable<256> WORD_MASK = MakeWordMask();
	
	for(LinkedInstructionList::Iterator it = instructionList.Begin(); it != instructionList.End(); ++it)
	{
		if(it->type != InstructionType::AssertWordBoundary
			&& it->type != InstructionType::AssertNotWordBoundary) continue;
		
		AssertWordBoundaryBaseInstruction *assertInstruction = static_cast<AssertWordBoundaryBaseInstruction*>(&*it);
		
		Instruction* afterBoundary = it->GetNext();
		while(AFTER_SKIP_INSTRUCTIONS.Contains(afterBoundary->type))
		{
			afterBoundary = afterBoundary->GetNext();	
		}

		bool afterIsWordCharacter = false;
		bool afterIsNotWordCharacter = false;

		StaticBitTable<256> afterValidBytes;
		if(afterBoundary->IsSimpleByteConsumer())
		{
			afterValidBytes = afterBoundary->GetValidBytes();
			afterIsWordCharacter = (afterValidBytes & ~WORD_MASK).HasAllBitsClear();
			afterIsNotWordCharacter = (afterValidBytes & WORD_MASK).HasAllBitsClear();
		}
		else if((afterBoundary->LeadsToMatch(false) && matchRequiresEndOfInput)
				|| NON_WORD_CHARACTER_ASSERT_INSTRUCTIONS.Contains(afterBoundary->type))
		{
			afterIsWordCharacter = false;
			afterIsNotWordCharacter = true;
			afterBoundary = nullptr;
		}
		
		if(afterIsWordCharacter != afterIsNotWordCharacter)
		{
			assertInstruction->isNextWordOnly = afterIsWordCharacter;
			assertInstruction->isNextNotWordOnly = afterIsNotWordCharacter;
		}
		
		if(it->referenceList.GetCount() != 1) continue;
		
		Instruction* beforeBoundary = it->referenceList[0].GetInstruction(assertInstruction);
		while(beforeBoundary->referenceList.GetCount() == 1
			  && BEFORE_SKIP_INSTRUCTIONS.Contains(beforeBoundary->type)
			  && beforeBoundary != assertInstruction)
		{
			beforeBoundary = beforeBoundary->referenceList[0].GetInstruction(beforeBoundary);
		}
		
		bool beforeIsWordCharacter = false;
		bool beforeIsNotWordCharacter = false;
		StaticBitTable<256> beforeValidBytes;
		if(beforeBoundary->IsSimpleByteConsumer())
		{
			beforeValidBytes = beforeBoundary->GetValidBytes();
			beforeIsWordCharacter = (beforeValidBytes & ~WORD_MASK).HasAllBitsClear();
			beforeIsNotWordCharacter = (beforeValidBytes & WORD_MASK).HasAllBitsClear();
		}
		else if(beforeBoundary == &instructionList.Front()
				|| NON_WORD_CHARACTER_ASSERT_INSTRUCTIONS.Contains(beforeBoundary->type))
		{
			beforeIsWordCharacter = false;
			beforeIsNotWordCharacter = true;
			beforeBoundary = nullptr;
		}
		
		if(beforeIsWordCharacter != beforeIsNotWordCharacter)
		{
			assertInstruction->isPreviousWordOnly = beforeIsWordCharacter;
			assertInstruction->isPreviousNotWordOnly = beforeIsNotWordCharacter;
		}

		if(beforeIsNotWordCharacter == beforeIsWordCharacter
		   || afterIsWordCharacter == afterIsNotWordCharacter)
		{
			continue;
		}
		
		int beforeMask = beforeIsNotWordCharacter + 2*beforeIsWordCharacter;
		int afterMask  = afterIsNotWordCharacter  + 2*afterIsWordCharacter;
		
		static const unsigned char FLIP_BITS[4] = { 0, 2, 1, 3 };
		
		if(it->type == InstructionType::AssertWordBoundary)
		{
			// Input    Output
			//  00       00
			//  01 (1)   10 (2)
			//  10 (2)   01 (1)
			//  11 (3)   11 (3)
			
			if(beforeMask & afterMask)
			{
				// This is a failure case!
				if(beforeBoundary)
				{
					ReplaceWithFail(beforeBoundary);
				}
				else
				{
					Instruction* assertInstruction = &*it;
					--it;
					ReplaceWithFail(assertInstruction);
				}
			}
			else if(beforeMask & FLIP_BITS[afterMask])
			{
				// This is a redundant assert
				Instruction* remove = &*it;
				--it;
				
				remove->GetNext()->referenceList.Remove(nullptr);
				remove->TransferReferencesTo(remove->GetNext());
				RemoveInstruction(remove);
				delete remove;
			}
			else if(beforeMask | afterMask)
			{
				// There is some information that we can use that isn't redundant
				if((afterIsWordCharacter || afterIsNotWordCharacter) && beforeBoundary == nullptr) continue;
				if((beforeIsWordCharacter || beforeIsNotWordCharacter) && afterBoundary == nullptr) continue;

				Instruction* remove = &*it;
				remove->GetNext()->referenceList.Remove(nullptr);
				remove->TransferReferencesTo(remove->GetNext());
				RemoveInstruction(remove);
				delete remove;
				
				if(beforeIsWordCharacter)
				{
					StaticBitTable<256> validMask = afterValidBytes & ~WORD_MASK;
					Instruction* replacement = new ByteBitMaskInstruction(validMask);
					if(afterBoundary->IsSingleReference())
					{
						afterBoundary->TransferReferencesTo(replacement);
						ReplaceInstruction(afterBoundary, replacement);
						delete afterBoundary;
						it = replacement;
					}
					else
					{
						JumpInstruction* skip = new JumpInstruction(afterBoundary->GetNext());
						InsertBeforeInstruction(afterBoundary, replacement);
						InsertBeforeInstruction(afterBoundary, skip);
						afterBoundary->referenceList.Remove(nullptr);
						replacement->referenceList.Append(InstructionReference::PREVIOUS);
						skip->referenceList.Append(InstructionReference::PREVIOUS);
					}
				}
				else if(beforeIsNotWordCharacter)
				{
					StaticBitTable<256> validMask = afterValidBytes & WORD_MASK;
					Instruction* replacement = new ByteBitMaskInstruction(validMask);
					if(afterBoundary->IsSingleReference())
					{
						afterBoundary->TransferReferencesTo(replacement);
						ReplaceInstruction(afterBoundary, replacement);
						delete afterBoundary;
						it = replacement;
					}
					else
					{
						JumpInstruction* skip = new JumpInstruction(afterBoundary->GetNext());
						InsertBeforeInstruction(afterBoundary, replacement);
						InsertBeforeInstruction(afterBoundary, skip);
						afterBoundary->referenceList.Remove(nullptr);
						replacement->referenceList.Append(InstructionReference::PREVIOUS);
						skip->referenceList.Append(InstructionReference::PREVIOUS);
					}
				}
				else if(afterIsWordCharacter)
				{
					StaticBitTable<256> validMask = beforeValidBytes & ~WORD_MASK;
					Instruction* replacement = new ByteBitMaskInstruction(validMask);
					beforeBoundary->TransferReferencesTo(replacement);
					ReplaceInstruction(beforeBoundary, replacement);
					delete beforeBoundary;
					it = replacement;
				}
				else if(afterIsNotWordCharacter)
				{
					StaticBitTable<256> validMask = beforeValidBytes & WORD_MASK;
					Instruction* replacement = new ByteBitMaskInstruction(validMask);
					beforeBoundary->TransferReferencesTo(replacement);
					ReplaceInstruction(beforeBoundary, replacement);
					delete beforeBoundary;
					it = replacement;
				}
			}
		}
		else
		{
			// AssertType == NotWordBoundary
			if(beforeMask & afterMask)
			{
				// This is a redundant assert
				Instruction* remove = &*it;
				--it;
				
				remove->GetNext()->referenceList.Remove(nullptr);
				remove->TransferReferencesTo(remove->GetNext());
				RemoveInstruction(remove);
				delete remove;
			}
			else if(beforeMask & FLIP_BITS[afterMask])
			{
				// This is a failure case!
				if(beforeBoundary)
				{
					ReplaceWithFail(beforeBoundary);
				}
				else
				{
					Instruction* assertInstruction = &*it;
					--it;
					ReplaceWithFail(assertInstruction);
				}
			}
			else if(beforeMask | afterMask)
			{
				// There is some information that we can use that isn't redundant
				if((afterIsWordCharacter || afterIsNotWordCharacter) && beforeBoundary == nullptr) continue;
				if((beforeIsWordCharacter || beforeIsNotWordCharacter) && afterBoundary == nullptr) continue;
				
				Instruction* remove = &*it;
				remove->GetNext()->referenceList.Remove(nullptr);
				remove->TransferReferencesTo(remove->GetNext());
				RemoveInstruction(remove);
				delete remove;
				
				if(beforeIsWordCharacter)
				{
					StaticBitTable<256> validMask = afterValidBytes & WORD_MASK;
					Instruction* replacement = new ByteBitMaskInstruction(validMask);
					if(afterBoundary->IsSingleReference())
					{
						afterBoundary->TransferReferencesTo(replacement);
						ReplaceInstruction(afterBoundary, replacement);
						delete afterBoundary;
						it = replacement;
					}
					else
					{
						JumpInstruction* skip = new JumpInstruction(afterBoundary->GetNext());
						InsertBeforeInstruction(afterBoundary, replacement);
						InsertBeforeInstruction(afterBoundary, skip);
						afterBoundary->referenceList.Remove(nullptr);
						replacement->referenceList.Append(InstructionReference::PREVIOUS);
						skip->referenceList.Append(InstructionReference::PREVIOUS);
					}
				}
				else if(beforeIsNotWordCharacter)
				{
					StaticBitTable<256> validMask = afterValidBytes & ~WORD_MASK;
					Instruction* replacement = new ByteBitMaskInstruction(validMask);
					if(afterBoundary->IsSingleReference())
					{
						afterBoundary->TransferReferencesTo(replacement);
						ReplaceInstruction(afterBoundary, replacement);
						delete afterBoundary;
						it = replacement;
					}
					else
					{
						JumpInstruction* skip = new JumpInstruction(afterBoundary->GetNext());
						InsertBeforeInstruction(afterBoundary, replacement);
						InsertBeforeInstruction(afterBoundary, skip);
						afterBoundary->referenceList.Remove(nullptr);
						replacement->referenceList.Append(InstructionReference::PREVIOUS);
						skip->referenceList.Append(InstructionReference::PREVIOUS);
					}
				}
				else if(afterIsWordCharacter)
				{
					StaticBitTable<256> validMask = beforeValidBytes & WORD_MASK;
					Instruction* replacement = new ByteBitMaskInstruction(validMask);
					beforeBoundary->TransferReferencesTo(replacement);
					ReplaceInstruction(beforeBoundary, replacement);
					delete beforeBoundary;
					it = replacement;
				}
				else if(afterIsNotWordCharacter)
				{
					StaticBitTable<256> validMask = beforeValidBytes & ~WORD_MASK;
					Instruction* replacement = new ByteBitMaskInstruction(validMask);
					beforeBoundary->TransferReferencesTo(replacement);
					ReplaceInstruction(beforeBoundary, replacement);
					delete beforeBoundary;
					it = replacement;
				}
			}
		}
	}
}

void InstructionList::Optimize_RemoveAssertEndOfInput()
{
	if(IsForwards())
	{
		if(hasEndAnchor) matchRequiresEndOfInput = true;
	}
	else
	{
		if(hasStartAnchor) matchRequiresEndOfInput = true;
	}
	
	static constexpr EnumSet<InstructionType, uint64_t> FORWARD_END_SET{ InstructionType::AssertEndOfInput };
	static constexpr EnumSet<InstructionType, uint64_t> REVERSE_END_SET{ InstructionType::AssertStartOfInput, InstructionType::AssertStartOfSearch };
	const EnumSet<InstructionType, uint64_t>& END_SET = IsForwards() ? FORWARD_END_SET : REVERSE_END_SET;
	
	bool hasByteConsumer = false;
	for(LinkedInstructionList::Iterator it = instructionList.ReverseBegin(); it != instructionList.ReverseEnd(); --it)
	{
		if(!END_SET.Contains(it->type))
		{
			if(it->HasMultipleReferences()) break;		// This detects merging points.
			if(it->IsByteConsumer()) hasByteConsumer = true;
		}
		else
		{
			if(hasByteConsumer)
			{
				it = ReplaceWithFail(&*it);
			}
			else
			{
				matchRequiresEndOfInput = true;
				Instruction* next = it->GetNext();
				next->referenceList.Remove(nullptr);
				it->TransferReferencesTo(next);
				RemoveInstruction(&*it);
				delete &*it;
				it = next;
			}
		}
	}
}

static void RecurseAddTargets(InstructionTable &targets, Instruction* original, Instruction* next)
{
	if(next == original) return;
	switch(next->type)
	{
	case InstructionType::Split:
		{
			SplitInstruction* split = (SplitInstruction*) next;
			for(size_t i = 0; i < split->targetList.GetCount(); ++i)
			{
				RecurseAddTargets(targets, original, split->targetList[i]);
			}
		}
		break;
			
	case InstructionType::Jump:
		{
			JumpInstruction* jump = (JumpInstruction*) next;
			RecurseAddTargets(targets, original, jump->target);
		}
		break;
			
	case InstructionType::ProgressCheck:
		{
			Instruction* nextnext = next->GetNext();
			if(nextnext == original || targets.Contains(nextnext)) break;
			targets.AppendUnique(next);
		}
		break;

	case InstructionType::Fail:
		break;
			
	default:
		targets.AppendUnique(next);
		break;
	}
}

void InstructionList::Optimize_SplitToSplit(SplitInstruction* split)
{
	InstructionTable targets;
	for(size_t i = 0; i < split->targetList.GetCount(); ++i)
	{
		RecurseAddTargets(targets, split, split->targetList[i]);
	}
	if(split->targetList == targets) return;
	
	split->Unlink();
	split->targetList = Move(targets);
	split->Link();
}

void InstructionList::Optimize_SplitToMatchReorder()
{
	if(!matchRequiresEndOfInput && scanDirection == Forwards)
	{
		instructionList.Perform([](Instruction& instruction) { instruction.startReachable = 0; });
		if(fullMatchInstruction == partialMatchInstruction)
		{
			fullMatchInstruction->TagReachable(3);
		}
		else
		{
			fullMatchInstruction->TagReachable(1);
			partialMatchInstruction->TagReachable(2);
		}
		
		for(LinkedInstructionList::Iterator it = instructionList.Begin(); it != instructionList.End();)
		{
			Instruction* instruction = &*it;
			++it;
			if(instruction->type != InstructionType::Split) continue;
			if(instruction->startReachable != 2) continue;
			
			SplitInstruction* split = (SplitInstruction*) instruction;
			if(split->targetList[0]->LeadsToMatch(false))
			{
				// The top priority branch always leads to a match
				// Replace with a jump.
				JumpInstruction* jump = new JumpInstruction(split->targetList[0]);
				split->TransferReferencesTo(jump);
				split->Unlink();
				ReplaceInstruction(split, jump);
				delete split;
			}
		}
	}
	else
	{
		for(Instruction& instruction : instructionList)
		{
			if(instruction.type != InstructionType::Split) continue;
			
			SplitInstruction* split = (SplitInstruction*) &instruction;
			InstructionTable targets = split->targetList;
			for(size_t i = 0; i < targets.GetCount(); ++i)
			{
				Instruction* instruction = targets[i];
				if(instruction->LeadsToMatch(scanDirection == Reverse))
				{
					targets.RemoveIndex(i);
					targets.InsertAtIndex(0, instruction);
				}
			}
			
			if(split->targetList == targets) continue;

			split->Unlink();
			split->targetList = Move(targets);
			split->Link();
		}
	}
}

void InstructionList::Optimize_SplitToSplit()
{
	for(Instruction& instruction : instructionList)
	{
		if(instruction.type != InstructionType::Split) continue;
		
		SplitInstruction* split = (SplitInstruction*) &instruction;
		Optimize_SplitToSplit(split);
	}
}

void InstructionList::Optimize_LeftFactor()
{
	// eg: "abc|abd" -> "ab(?:c|d)"
	for(LinkedInstructionList::Iterator it = instructionList.Begin(); it != instructionList.End(); ++it)
	{
	Repeat:
		if(it->type != InstructionType::Split || it->referenceList.GetCount() == 0) continue;
		
		SplitInstruction* split = (SplitInstruction*) &*it;
		Optimize_SplitToSplit(split);
		LinkedInstructionList::Iterator insertAfter(it);
		StateMap emptyStateMap;
		Optimize_SplitToByteConsumers(insertAfter, split, emptyStateMap);
		if(split->targetList.GetCount() < 2)
		{
			// Replace with either fail or jump;
			if(split->targetList.IsEmpty())
			{
				it = ReplaceWithFail(split);
			}
			else
			{
				JumpInstruction* jump = new JumpInstruction(split->targetList[0]);
				split->targetList[0]->referenceList.Remove(&split->targetList[0]);
				ReplaceInstruction(split, jump);
				split->TransferReferencesTo(jump);
				delete split;
				it = jump;
			}
			continue;
		}
		
		size_t start = 0;
		while(start < split->targetList.GetCount())
		{
			size_t end = start+1;
			Instruction* startTarget = split->targetList[start];
			for(; end < split->targetList.GetCount(); ++end)
			{
				Instruction* target = split->targetList[end];
				if(*startTarget != *target) break;
			}
			
			// Two cases
			// a). All targets equal. ie.
			//     01 split 2 4 6            ->      01 byte x
			//     02 byte x                         02 split i1 i2 i3
			//     03 i1                             03 i1
			//     04 byte x                         04 i2
			//     05 i2                             05 i3
			//     06 byte x
			//     07 i3
			//
			// In the case of Jump, Split or Match instructions, this becomes simply (ie. if they have no implicit next instruction)
			//     01 jump
			//     02 i1
			//     03 i2
			//     04 i3
			//
			// b) Some targets equal, ie.
			//     01 split 2 4 6 8       ->         01 split 2 4 8
			//     02 byte x                         02 byte x
			//     03 i1                             03 i1
			//     04 byte y                         04 byte y
			//     05 i2                             05 split i2 i3
			//     06 byte y                         06 i2
			//     07 i3                             07 i3
			//     08 byte z                         08 byte z
			//     09 i4                             09 i4
			//
			// The second case is important for the CollapseSplit optimization
			
			if(start == 0 && end == split->targetList.GetCount())
			{
				Instruction* commonTarget = startTarget->Clone();
				
				for(size_t j = 0; j < split->targetList.GetCount(); ++j)
				{
					Instruction* &target = split->targetList[j];
					Instruction* newTarget = target->GetNext();
					JASSERT(newTarget->referenceList.Contains(nullptr));

					target->referenceList.Remove(&target);
					target = newTarget;
					newTarget->referenceList.Append(InstructionReference{&target, split});
				}
				split->TransferReferencesTo(commonTarget);
				split->referenceList.SetCount(0);
				
				if(!NO_IMPLICIT_NEXT_INSTRUCTION_SET.Contains(commonTarget->type))
				{
					split->referenceList.Append(InstructionReference::PREVIOUS);
				}
				InsertBeforeInstruction(split, commonTarget);
				goto Repeat;
			}
			else if(start+1 != end)
			{
				SplitInstruction* newSplit = nullptr;
				if(!NO_IMPLICIT_NEXT_INSTRUCTION_SET.Contains(startTarget->type))
				{
					// Create a new split instruction
					newSplit = new SplitInstruction;
					newSplit->targetList.Reserve(end-start+1);
					
					for(size_t j = start; j < end; ++j)
					{
						Instruction* &target = split->targetList[j];
						Instruction* afterTarget = target->GetNext();
						
						newSplit->targetList.Append(afterTarget);
						afterTarget->referenceList.Append(InstructionReference{&newSplit->targetList.Back(), newSplit});
					}

					newSplit->referenceList.Append(InstructionReference::PREVIOUS);
					InsertAfterInstruction(split, newSplit);
				}
				
				Instruction* commonTarget = startTarget->Clone();
				InsertAfterInstruction(split, commonTarget);
				split->targetList[start]->referenceList.Remove(&split->targetList[start]);
				split->targetList[start] = commonTarget;
				commonTarget->referenceList.Append(InstructionReference{&split->targetList[start], split});
				
				// Redo references for array being changed in size.
				for(size_t j = start+1; j < split->targetList.GetCount(); ++j)
				{
					Instruction* &target = split->targetList[j];
					target->referenceList.Remove(&target);
				}

				split->targetList.RemoveCount(start+1, end-start-1);
				for(size_t x = start+1; x < split->targetList.GetCount(); ++x)
				{
					Instruction* &target = split->targetList[x];
					target->referenceList.Append(InstructionReference{&target, split});
				}
				
				++start;
			}
			else
			{
				start = end;
			}
		}
	}
}

void InstructionList::Optimize_RightFactor()
{
	// eg: "axy|bxy " -> "(?:a|b)xy"

	IndexInstructions();
	
	for(LinkedInstructionList::Iterator it = instructionList.ReverseBegin(); it != instructionList.ReverseEnd(); --it)
	{
		if(it->type != InstructionType::Jump) continue;
		
		JumpInstruction* jump = (JumpInstruction*) &*it;
		
	Repeat:
		if(!jump->referenceList.Contains(nullptr))
		{
			jump->TransferReferencesTo(jump->target);
			continue;
		}
		Instruction* previous = jump->GetPrevious();
		
		Instruction* target = jump->target;
		for(InstructionReference reference : target->referenceList)
		{
			Instruction* targetPrevious = reference.GetInstruction(target);
			if(targetPrevious == nullptr) continue;
			if(targetPrevious->type == InstructionType::Jump)
			{
				targetPrevious = targetPrevious->GetPrevious();
			}
			if(targetPrevious->index <= jump->index) continue;
			
			if(*previous == *targetPrevious)
			{
				// Forward all other jump references to target now
				// StandardOutput.PrintF("Forwarding %u -> %u\n", previous->index, targetPrevious->index);
				jump->referenceList.Remove(nullptr);
				jump->TransferReferencesTo(target);
				previous->TransferReferencesTo(jump);
				jump->Unlink();
				jump->target = targetPrevious;
				jump->Link();
				RemoveInstruction(previous);
				delete previous;
				goto Repeat;
			}
		}
	}
}

void InstructionList::Optimize_SplitToByteConsumers(LinkedInstructionList::Iterator& insertAfter, SplitInstruction* split, StateMap& stateMap)
{
	// If we have split a, b, c, d, e, f
	// And a,b,c e,f are byte consumers, then change this to:
	
	//    split x, d, y
	// x: split a, b, c
	// y: split e, f
	if(split->targetList.GetCount() <= 2) return;
	
	uint32_t start = 0;
	while(start < split->targetList.GetCount())
	{
		if(split->targetList[start]->IsByteConsumer())
		{
			uint32_t end = start+1;
			for(; end < split->targetList.GetCount(); ++end)
			{
				Instruction* target = split->targetList[end];
				if(!target->IsByteConsumer()) break;
			}
			
			if((start == 0 && end == split->targetList.GetCount())
			   || (end-start) < 2)
			{
				// No changes required
			}
			else
			{
				SplitInstruction* newSplit = new SplitInstruction;
				InstructionTable newTargetList;
				newTargetList.Reserve(end-start);
				for(uint32_t j = start; j < end; ++j)
				{
					Instruction* target = split->targetList[j];
					newTargetList.Append(target);
				}
				newSplit->targetList = stateMap.MakeCanonicalStateList(nullptr, newTargetList);
				ObjectWithHash<InstructionTable&> targetListWithHash(newSplit->targetList);
				
				StateMap::StatesToInstructionMap::ConstIterator it = stateMap.Find(targetListWithHash);
				for(uint32_t j = start; j < split->targetList.GetCount(); ++j)
				{
					Instruction* &target = split->targetList[j];
					target->referenceList.Remove(&target);
				}
				
				if(it != stateMap.End())
				{
					delete newSplit;
					Instruction* target = it->value;
					split->targetList[start] = target;
				}
				else
				{
					stateMap.Insert(targetListWithHash, newSplit);
					newSplit->Link();
					split->targetList[start] = newSplit;
					InsertAfterInstruction(&*insertAfter, newSplit);
					++insertAfter;
				}
				split->targetList.RemoveCount(start+1, end-start-1);
				for(uint32_t j = start; j < split->targetList.GetCount(); ++j)
				{
					Instruction* &target = split->targetList[j];
					target->referenceList.Append(InstructionReference{&target, split});
				}
			}
			start = end+1;
		}
		else
		{
			++start;
		}
	}
}

void InstructionList::Optimize_SplitToByteFilters(LinkedInstructionList::Iterator& insertAfter, SplitInstruction* split)
{
	// If we have split a, b, c, d, e, f
	// And a,b,c e,f are byte consumers, then change this to:
	
	//    split x d y
	// x: split a b c
	// y: split e f
	if(split->optimizationPhase == SplitInstruction::OptimizationPhase::SplitToByteFiltersDone) return;
	split->optimizationPhase = SplitInstruction::OptimizationPhase::SplitToByteFiltersDone;
	if(split->targetList.GetCount() <= 2) return;
	
	uint32_t start = 0;
	while(start < split->targetList.GetCount())
	{
		if(GetByteFilterStartingFrom(split->targetList[start]))
		{
			uint32_t end = start+1;
			for(; end < split->targetList.GetCount(); ++end)
			{
				Instruction* target = split->targetList[end];
				if(!GetByteFilterStartingFrom(target))
				{
					if(matchRequiresEndOfInput
					   && end == split->targetList.GetCount()-1
					   && GetMatchStartingFrom(target) != nullptr) continue;
					break;
				}
			}
			
			if((start == 0 && end == split->targetList.GetCount())
			   || (end-start) < 2)
			{
				// No changes required
			}
			else
			{
				SplitInstruction* newSplit = new SplitInstruction;
				newSplit->targetList.Reserve(end-start);
				newSplit->optimizationPhase = SplitInstruction::OptimizationPhase::SplitToByteFiltersDone;
				for(uint32_t j = start; j < split->targetList.GetCount(); ++j)
				{
					Instruction* &target = split->targetList[j];
					target->referenceList.Remove(&target);
				}
				for(uint32_t j = start; j < end; ++j)
				{
					Instruction* target = split->targetList[j];
					newSplit->targetList.Append(target);
					target->referenceList.Append(InstructionReference{&newSplit->targetList[j-start], newSplit});
				}
				split->targetList[start] = newSplit;
				newSplit->referenceList.Append(InstructionReference{&split->targetList[start], split});
				split->targetList.RemoveCount(start+1, end-start-1);
				for(uint32_t j = start+1; j < split->targetList.GetCount(); ++j)
				{
					Instruction* &target = split->targetList[j];
					target->referenceList.Append(InstructionReference{&target, split});
				}
				InsertAfterInstruction(&*insertAfter, newSplit);
				++insertAfter;
			}
			start = end+1;
		}
		else
		{
			++start;
		}
	}
}

static Instruction* GetAfterTargetInstruction(Instruction* next)
{
	if(next->type != InstructionType::Jump) return next;
	return ((JumpInstruction*) next)->target;
}

void InstructionList::Optimize_CollapseSplit(StateMap& stateMap, StateSet* growthStateList, SplitInstruction* split)
{
	// Avoiding growth pattern checks:
	// Make sure "\\w.\\bnnn" is collapsed
	// Make sure "(?i)Tom|Sawyer|Huckleberry|Finn" is collapsed
	// Make sure [a-q][^u-z]{13} is not collapsed too far
	
//	StandardOutput.PrintF("Split: ");
//	split->targetList.Dump(StandardOutput);
	
	InstructionTable canonicalStateList = stateMap.MakeCanonicalStateList(split);
	ObjectWithHash<InstructionTable&> canonicalStateListWithHash(canonicalStateList);
//	StandardOutput.PrintF("Canonical state list: ");
//	canonicalStateList.Dump(StandardOutput);

	StateMap::StatesToInstructionMap::ConstIterator it = stateMap.Find(canonicalStateListWithHash);
	if(it != stateMap.End())
	{
		if(it->value != split)
		{
			JumpInstruction* replacement = new JumpInstruction(it->value);
			replacement->index = split->index;
			split->TransferReferencesTo(replacement);
			ReplaceInstruction(split, replacement);
			split->Unlink();
			
			StateMap::InstructionToStatesMap::Iterator it2 = stateMap.instructionToStatesMap.Find(split);
			if(it2 != stateMap.instructionToStatesMap.End())
			{
				InstructionTable t2 = it2->value;
				ObjectWithHash<InstructionTable&> t2WithHash(t2);
				stateMap.Remove(t2WithHash, split);
				stateMap.Insert(t2WithHash, replacement);
			}
			delete split;
			return;
		}
		
		if(growthStateList[AVOID_STATES_REQUIRE_CONSECUTIVE_GROWTH_COUNT-1].Find(canonicalStateListWithHash) != growthStateList[AVOID_STATES_REQUIRE_CONSECUTIVE_GROWTH_COUNT-1].End())
		{
			// Lets see whether the split can make use of existing collapses
			InstructionTable newTargetList;
			newTargetList.Append(canonicalStateList[0]);
			canonicalStateList.RemoveIndex(0);
			while(canonicalStateList.GetCount())
			{
				// Cannot use canonicalStateListWithHash as it is changing in this loop.
				StateMap::StatesToInstructionMap::ConstIterator it = stateMap.Find(canonicalStateList);
				if(it != stateMap.End())
				{
					newTargetList.Append(it->value);
					break;
				}
				newTargetList.Append(canonicalStateList[0]);
				canonicalStateList.RemoveIndex(0);
			}
			
			if(split->targetList != newTargetList)
			{
				// Remove all references.
				split->Unlink();
				split->targetList = (InstructionTable&&) newTargetList;
				split->Link();
			}
			return;
		}
	}
	else
	{
		stateMap.Insert(canonicalStateListWithHash, split);
	}
	
	Optimize_SplitToSplit(split);
	
	LinkedInstructionList::Iterator insertAfter = split;
	Optimize_SplitToByteConsumers(insertAfter, split, stateMap);
	uint32_t numberOfTargets = (uint32_t) split->targetList.GetCount();
	
	StaticBitTable<256> bitMask;
	Instruction* afterTargetInstruction[numberOfTargets];
	
	bool sameTarget = true;
	bool hasOverlap = false;
	for(size_t j = 0; j < numberOfTargets; ++j)
	{
		Instruction* target = split->targetList[j];
		if(!target->IsByteConsumer()) return;
		if(!target->IsSimpleByteConsumer())
		{
			sameTarget = false;
		}
	}
	
	for(size_t j = 0; j < numberOfTargets; ++j)
	{
		Instruction* target = split->targetList[j];
		
		StaticBitTable<256> targetBitMask = target->GetValidBytes();
		if((bitMask & targetBitMask).HasAnyBitSet()) hasOverlap = true;
		bitMask |= targetBitMask;
		
		if(sameTarget)
		{
			afterTargetInstruction[j] = GetAfterTargetInstruction(target->GetNext());
			if(afterTargetInstruction[0] != afterTargetInstruction[j]
			   && (!NO_IMPLICIT_NEXT_INSTRUCTION_SET.Contains(afterTargetInstruction[0]->type) || *afterTargetInstruction[0] != *afterTargetInstruction[j])) sameTarget = false;
		}
	}
	
	if(sameTarget)
	{
		// Replace with bitmask or byte range.
		for(size_t j = 0; j < numberOfTargets; ++j)
		{
			Instruction* &target = split->targetList[j];
			target->referenceList.Remove(&target);
		}
		
		Instruction* replacement;
		if(bitMask.HasAllBitsClear())
		{
			replacement = new FailInstruction;
		}
		else if(bitMask.HasAllBitsSet())
		{
			replacement = new AnyByteInstruction;
		}
		else
		{
			ByteBitMaskInstruction* bitMaskInstruction = new ByteBitMaskInstruction;
			bitMaskInstruction->bitMask = bitMask;
			replacement = bitMaskInstruction;
		}
		
		split->TransferReferencesTo(replacement);
		stateMap.Remove(canonicalStateListWithHash, split);
		stateMap.Insert(canonicalStateListWithHash, replacement);
		ReplaceInstruction(split, replacement);
		
		// Insert jump instruction to afterTarget
		Instruction* target = GetAfterTargetInstruction(split->targetList[0]->GetNext());
		JumpInstruction* jump = new JumpInstruction(target);
		
		jump->referenceList.Append(InstructionReference::PREVIOUS);
		
		InsertAfterInstruction(replacement, jump);
		delete split;
	}
	else if(!hasOverlap)
	{
		ByteJumpTableInstruction* jumpTableInstruction = new ByteJumpTableInstruction;
		jumpTableInstruction->targetList.Reserve(256);
		jumpTableInstruction->referenceList.Reserve(jumpTableInstruction->referenceList.GetCount() + numberOfTargets);
		
		split->TransferReferencesTo(jumpTableInstruction);
		stateMap.Remove(canonicalStateListWithHash, split);
		stateMap.Insert(canonicalStateListWithHash, jumpTableInstruction);
		
		if(bitMask.GetPopulationCount() != 256)
		{
			jumpTableInstruction->targetList.Append(nullptr);
		}
		
		for(size_t j = 0; j < numberOfTargets; ++j)
		{
			Instruction* &target = split->targetList[j];
			
			for(int p = 0; p < target->GetNumberOfPlanes(); ++p)
			{
				StaticBitTable<256> bitMask = target->GetValidBytesForPlane(p);
				if(bitMask.GetPopulationCount() != 0)
				{
					Instruction* afterTargetInstruction = GetAfterTargetInstruction(target->GetTargetForPlane(p));
					Instruction* newTarget = (afterTargetInstruction == split) ? jumpTableInstruction : afterTargetInstruction;
					
					size_t index = jumpTableInstruction->targetList.FindIndexForValue(newTarget);
					if(index == TypeData<size_t>::Maximum())
					{
						index = jumpTableInstruction->targetList.GetCount();
						jumpTableInstruction->targetList.Append(newTarget);
						newTarget->referenceList.Append(InstructionReference{&jumpTableInstruction->targetList[index], jumpTableInstruction});
					}
					for(uint32_t k = 0; k < 256; ++k)
					{
						if(bitMask[k]) jumpTableInstruction->targetTable[k] = index;
					}
				}
			}
			
			target->referenceList.Remove(&target);
		}
		
		ReplaceInstruction(split, jumpTableInstruction);
		delete split;
		stateMap.Remove(canonicalStateListWithHash, jumpTableInstruction);
		Instruction* result = Optimize_SimplifyByteJumpTable(jumpTableInstruction);
		stateMap.Insert(canonicalStateListWithHash, result);
	}
	else
	{
		// Have a split, with targets that have overlapping regions!
		ByteJumpTableInstruction* jumpTableInstruction = new ByteJumpTableInstruction;
		jumpTableInstruction->referenceList.Reserve(split->referenceList.GetCount() + 256);
		jumpTableInstruction->targetList.Reserve(256);
		
		split->TransferReferencesTo(jumpTableInstruction);
		stateMap.Remove(canonicalStateListWithHash, split);
		stateMap.Insert(canonicalStateListWithHash, jumpTableInstruction);
		
		InstructionTable targetList[256];
		InstructionTable addedSplitList;
		Javelin::Map<InstructionTable, uint32_t> jumpStateMap(16);
		
		if(bitMask.GetPopulationCount() != 256)
		{
			InstructionTable emptyTargetList;
			jumpStateMap.Insert(emptyTargetList, 0);
			jumpTableInstruction->targetList.Append(nullptr);
		}
		
		for(size_t j = 0; j < split->targetList.GetCount(); ++j)
		{
			Instruction* target = split->targetList[j];
			for(int p = 0; p < target->GetNumberOfPlanes(); ++p)
			{
				StaticBitTable<256> targetBitMask = split->targetList[j]->GetValidBytesForPlane(p);
				
				Instruction* afterTargetInstruction = GetAfterTargetInstruction(target->GetTargetForPlane(p));
				Instruction* newTarget = (afterTargetInstruction == split) ? jumpTableInstruction : afterTargetInstruction;
				for(uint32_t x = 0; x < 256; ++x)
				{
					if(targetBitMask[x]) targetList[x].Append(newTarget);
				}
			}
		}
		
		uint32_t index;
		for(uint32_t j = 0; j < 256; ++j)
		{
			if(j == 0 || targetList[j] != targetList[j-1])
			{
				InstructionTable byteTargets = stateMap.MakeCanonicalStateList(nullptr, targetList[j]);
				ObjectWithHash<InstructionTable&> byteTargetsWithHash(byteTargets);

				Javelin::Map<InstructionTable, uint32_t>::Iterator it = jumpStateMap.Find(byteTargetsWithHash);
				if(it == jumpStateMap.End())
				{
					Instruction* target = nullptr;
					switch(byteTargets.GetCount())
					{
					case 0:
						target = nullptr;
						break;
						
					case 1:
						target = byteTargets[0];
						break;
						
					default:
						// Create a split instruction
						{
							StateMap::StatesToInstructionMap::ConstIterator it = stateMap.Find(byteTargetsWithHash);
							if(it != stateMap.End())
							{
								target = it->value;
							}
							else
							{
								SplitInstruction* newSplit = new SplitInstruction;
								newSplit->targetList = byteTargets;
								newSplit->Link();
								InsertAfterInstruction(split, newSplit);
								++insertAfter;
								target = newSplit;
								stateMap.Insert(byteTargetsWithHash, newSplit);
								addedSplitList.Append(newSplit);
							}
						}
						break;
					}
					index = (uint32_t) jumpTableInstruction->targetList.GetCount();
					jumpStateMap.Insert(byteTargetsWithHash, index);
					jumpTableInstruction->targetList.Append(target);
					if(target) target->referenceList.Append(InstructionReference{&jumpTableInstruction->targetList[index], jumpTableInstruction});
				}
				else
				{
					index = it->value;
				}
			}
			jumpTableInstruction->targetTable[j] = index;
		}
		
		if(addedSplitList.GetCount() >= 2)
		{
			int insertAt = AVOID_STATES_REQUIRE_CONSECUTIVE_GROWTH_COUNT-1;
			while(insertAt > 0 && !growthStateList[insertAt-1].Contains(canonicalStateListWithHash)) --insertAt;
			
			for(Instruction* addedSplit : addedSplitList)
			{
				SplitInstruction* split = (SplitInstruction*) addedSplit;
				growthStateList[insertAt].Insert(split->targetList);
			}
		}
		
		for(size_t j = 0; j < numberOfTargets; ++j)
		{
			Instruction* &target = split->targetList[j];
			target->referenceList.Remove(&target);
		}
		
		ReplaceInstruction(split, jumpTableInstruction);
		delete split;
		stateMap.Remove(canonicalStateListWithHash, jumpTableInstruction);
		Instruction* result = Optimize_SimplifyByteJumpTable(jumpTableInstruction);
		stateMap.Insert(canonicalStateListWithHash, result);
	}
}

void InstructionList::Optimize_CollapseSplit()
{
	StateMap stateMap;
	StateSet growthStateList[AVOID_STATES_REQUIRE_CONSECUTIVE_GROWTH_COUNT];
	
	// Looking for two cases here
	// 1) split -> byte -> same target --- change to BitMask
	// 2) split -> byte -> different targets --- change to ByteJumpTable
	
	// This is done in reverse so that references to instructions can be reduced and save can be delayed.
	
	// eg. processing forward for "([0-9]+)-([0-9]+)-([0-9]+)" gives:
	//	After optimization pass: CollapseSplit
	//	000060c000009400   0: 1 ref: jump 2
	//	000060c000009340   1: 1 ref: any-byte
	//	0000611000007840   2: 3 ref: split 3 1
	//	000060d00000b230   3: 2 ref: save-no-recurse 0
	//	000060d00000b160   4: 1 ref: save-no-recurse 2
	//	000060c000009880   5: 1 ref: byte-range {'0', '9'}
	//	000061500000b480   6: 2 ref: byte-jump-table: fail 6 8
	//	000060c0000097c0   7: 0 ref: byte '-'
	// ...
	
	
	LinkedInstructionList::Iterator instructionIterator = instructionList.ReverseBegin();
	while(instructionIterator != instructionList.ReverseEnd())
	{
		Instruction* instruction = &*instructionIterator;
		if(instruction->referenceList.GetCount() == 0)
		{
			--instructionIterator;
			continue;
			
		}
		
		// This strange iteration is because we want to place extra instructions after the split that we're replacing
		// We only ever delete the split we're processing, so using the split's next as an anchor while processing is stable
		++instructionIterator;
		if(instruction->type == InstructionType::Save)
		{
			Optimize_DelaySave((SaveInstruction*) instruction, true);
		}
		else if(instruction->type == InstructionType::Split)
		{
			SplitInstruction* split = (SplitInstruction*) instruction;
			Optimize_CollapseSplit(stateMap, growthStateList, split);
		}
		--instructionIterator;
		if(instructionIterator == instruction) --instructionIterator;
	}
}

void InstructionList::Optimize_RemoveZeroReference()
{
	instructionList.Perform([](Instruction& instruction) { instruction.startReachable = 0; });

	if(scanDirection == Forwards)
	{
		if(partialMatchInstruction == fullMatchInstruction)
		{
			fullMatchInstruction->TagReachable(3);
		}
		else
		{
			fullMatchInstruction->TagReachable(1);
			partialMatchInstruction->TagReachable(2);
		}
	}
	else
	{
		fullMatchInstruction->TagReachable(1);
	}
	
	// First pass, break all links from unreachable instructions
	for(Instruction& instruction : instructionList)
	{
		if(!instruction.startReachable)
		{
			instruction.Unlink();
		}
	}
	
	// Now remove them from the table.
	int matchReachable = 0;
	for(LinkedInstructionList::Iterator it = instructionList.ReverseBegin(); it != instructionList.ReverseEnd();)
	{
		Instruction* current = &*it;
		--it;
		if(!current->startReachable)
		{
			for(InstructionReference ref : current->referenceList)
			{
				if(ref.instruction == InstructionReference::SEARCH_OPTIMIZER_REFERENCE)
				{
					SearchOptimizer* optimizer = (SearchOptimizer*) (uintptr_t(ref.storage) - offsetof(&SearchOptimizer::original));
					delete optimizer;
				}
			}

			RemoveInstruction(current);
			delete current;
		}
		else if(current->type == InstructionType::Match
				|| current->type == InstructionType::Success)
		{
			matchReachable |= current->startReachable;
		}
	}

	if((matchReachable & 1) == 0) ConvertToFail(fullMatchInstruction);
	if(scanDirection == Forwards && (matchReachable & 2) == 0) ConvertToFail(partialMatchInstruction);
}

void InstructionList::ConvertToFail(Instruction* &startingInstruction)
{
	InstructionReference reference{&startingInstruction, InstructionReference::START_REFERENCE};
	
	startingInstruction->referenceList.Remove(reference);
	Instruction* failInstruction;
	if(instructionList.Front().type == InstructionType::Fail) failInstruction = &instructionList.Front();
	else
	{
		failInstruction = new FailInstruction;
		instructionList.PushFront(failInstruction);
	}
	startingInstruction = failInstruction;
	failInstruction->referenceList.Append(reference);
}

void InstructionList::Optimize_SimpleByteConsumersToAdvanceByte()
{
	instructionList.Perform([](Instruction& instruction) { instruction.splitReachable = false; });
	
	// Tag all instructions reached by splits
	for(Instruction& instruction : instructionList)
	{
		if(instruction.type == InstructionType::Split)
		{
			SplitInstruction& split = (SplitInstruction&) instruction;
			for(Instruction* target : split.targetList)
			{
				target->splitReachable = true;
			}
		}
	}
	
	// Now replace all simple byte consumer instructions not reachable by splits with advance byte
	for(LinkedInstructionList::Iterator it = instructionList.Begin(); it != instructionList.End();)
	{
		Instruction* instruction = &*it;
		++it;
		if(instruction->splitReachable == true) return;
		if(instruction->IsSimpleByteConsumer() && instruction->type != InstructionType::Fail)
		{
			AdvanceByteInstruction* advanceByte = new AdvanceByteInstruction;
			instruction->TransferReferencesTo(advanceByte);
			ReplaceInstruction(instruction, advanceByte);
			delete instruction;
		}
		else if(instruction->type == InstructionType::ByteJumpTable)
		{
			JumpTableInstruction* jumpTable = (JumpTableInstruction*) instruction;
			if(jumpTable->targetList[0] == nullptr)
			{
				jumpTable->Unlink();
				jumpTable->targetList.Remove(nullptr);
				for(int i = 0; i < 256; ++i)
				{
					if(jumpTable->targetTable[i] > 0) jumpTable->targetTable[i]--;
				}
				jumpTable->Link();
			}
			if(jumpTable->targetList.GetCount() == 1)
			{
				// Convert to any byte then jump if necessary
				AnyByteInstruction* anyByte = new AnyByteInstruction;

				if(jumpTable->targetList[0] == jumpTable->GetNext())
				{
					jumpTable->GetNext()->referenceList.Append(InstructionReference::PREVIOUS);
				}
				else
				{
					JumpInstruction* jump = new JumpInstruction(jumpTable->targetList[0]);
					jumpTable->TransferReferencesTo(anyByte);
					jump->referenceList.Append(InstructionReference::PREVIOUS);
					InsertAfterInstruction(jumpTable, jump);
				}
				
				ReplaceInstruction(jumpTable, anyByte);
				jumpTable->Unlink();
				delete jumpTable;
			}
		}
	}
}

void InstructionList::Optimize_RemoveJumpToNext()
{
	for(LinkedInstructionList::Iterator it = instructionList.ReverseBegin(); it != instructionList.ReverseEnd();)
	{
		LinkedInstructionList::Iterator current = it;
		--it;
		if(current->type != InstructionType::Jump) continue;
		
		JumpInstruction* jump = (JumpInstruction*) &*current;
		if(jump->target == jump->GetNext())
		{
			Instruction* target = jump->target;
			target->referenceList.Remove(&jump->target);
			jump->TransferReferencesTo(target);
			
			RemoveInstruction(jump);
			delete jump;
		}
	}
}

Instruction* InstructionList::GetMatchStartingFrom(Instruction* instruction) const
{
	for(;;)
	{
		switch(instruction->type)
		{
		case InstructionType::Jump:
			instruction = ((JumpInstruction*) instruction)->target;
			continue;

		case InstructionType::AssertEndOfInput:
			if(matchRequiresEndOfInput)
			{
				instruction = instruction->GetNext();
				continue;
			}
			return nullptr;
				
		case InstructionType::Save:
		case InstructionType::ProgressCheck:
		case InstructionType::PropagateBackwards:
			instruction = instruction->GetNext();
			continue;

		case InstructionType::DispatchTable:
			{
				DispatchTableInstruction* dispatch = (DispatchTableInstruction*) instruction;
				return dispatch->targetList[0] != nullptr ? dispatch : nullptr;
			}
				
		case InstructionType::Match:
		case InstructionType::Success:
			return instruction;
				
		default:
			return nullptr;
		}
	}
}

Instruction* InstructionList::GetByteFilterStartingFrom(Instruction* instruction) const
{
	for(;;)
	{
		switch(instruction->type)
		{
		case InstructionType::AnyByte:
		case InstructionType::Byte:
		case InstructionType::ByteEitherOf2:
		case InstructionType::ByteEitherOf3:
		case InstructionType::ByteRange:
		case InstructionType::ByteBitMask:
		case InstructionType::ByteJumpTable:
		case InstructionType::ByteNot:
		case InstructionType::ByteNotEitherOf2:
		case InstructionType::ByteNotEitherOf3:
		case InstructionType::ByteNotRange:
		case InstructionType::DispatchTable:
		case InstructionType::Fail:
			return instruction;
				
		case InstructionType::Jump:
			instruction = ((JumpInstruction*) instruction)->target;
			continue;
				
		case InstructionType::AssertStartOfInput:
		case InstructionType::AssertEndOfInput:
		case InstructionType::AssertStartOfLine:
		case InstructionType::AssertEndOfLine:
		case InstructionType::AssertWordBoundary:
		case InstructionType::AssertNotWordBoundary:
		case InstructionType::Save:
		case InstructionType::ProgressCheck:
		case InstructionType::PropagateBackwards:
			instruction = instruction->GetNext();
			continue;
				
		default:
			return nullptr;
		}
	}
}

Instruction* InstructionList::GetAfterByteFilterStartingFrom(Instruction* instruction) const
{
	for(;;)
	{
		switch(instruction->type)
		{
		case InstructionType::Jump:
			instruction = ((JumpInstruction*) instruction)->target;
			continue;
			
		case InstructionType::ProgressCheck:
		case InstructionType::Save:
			instruction = instruction->GetNext();
			continue;

		case InstructionType::Match:
		case InstructionType::Success:
		case InstructionType::DispatchTable:
		case InstructionType::Split:
			return instruction;
				
		default:
			return nullptr;
		}
	}
}

void InstructionList::Optimize_SplitToDispatch()
{
	for(LinkedInstructionList::Iterator it = instructionList.ReverseBegin(); it != instructionList.ReverseEnd(); --it)
	{
		if(it->type != InstructionType::Split) continue;
		if(it->referenceList.GetCount() == 0) continue;

		SplitInstruction* split = (SplitInstruction*) &*it;
		if(split->targetList.GetCount() > 256) continue;

		Optimize_SplitToSplit(split);
		InstructionTable canonicalTargetList = split->targetList;
		Optimize_SplitToByteFilters(it, split);
		split = (SplitInstruction*) &*it;
		
		// If there is a match target and if it is the last one.
		Instruction* endOfInputInstruction = nullptr;
		bool hasMatch = false;
		bool hasOverlap = false;
		StaticBitTable<256> bitMask;
		size_t numberOfTargets = split->targetList.GetCount();

		if(scanDirection == Forwards)
		{
			// This split is ignored because it's going to be converted to a SplitMatch (or variant)
			if(numberOfTargets == 2
			   && (split->targetList[0]->LeadsToMatch(false)
				   || (matchRequiresEndOfInput && split->targetList[1]->LeadsToMatch(false))))
			{
				continue;
			}
		}
		
		if(matchRequiresEndOfInput)
		{
			for(size_t j = 0; j < numberOfTargets; ++j)
			{
				Instruction* target = split->targetList[j];
				if(GetMatchStartingFrom(target) != nullptr)
				{
					hasMatch = true;
					break;
				}
			}
		}
		else
		{
			if(GetMatchStartingFrom(split->targetList.Back()) != nullptr)
			{
				hasMatch = true;
			}
		}
		
		int equivalentCount = 0;
		for(size_t j = 0; j < numberOfTargets; ++j)
		{
			Instruction* target = split->targetList[j];
			Instruction* byteTarget = GetByteFilterStartingFrom(target);
			
			if(endOfInputInstruction == nullptr)
			{
				if(GetMatchStartingFrom(target) != nullptr)
				{
					endOfInputInstruction = target;
				}
			}
			
			if(hasMatch && !byteTarget && GetMatchStartingFrom(target) != nullptr) continue;
			if(!byteTarget) goto Next;

			if(hasMatch && !matchRequiresEndOfInput)
			{
				if(byteTarget->GetNumberOfPlanes() > 1) goto Next;
				
				Instruction* afterByteFilter = GetAfterByteFilterStartingFrom(byteTarget->GetTargetForPlane(0));
				if(!afterByteFilter) goto Next;
				if(afterByteFilter->type != InstructionType::Match
				   && afterByteFilter->type != InstructionType::Success
				   && !(afterByteFilter->type == InstructionType::DispatchTable && ((DispatchTableInstruction*) afterByteFilter)->targetList[0] != nullptr)
				   && *split != *afterByteFilter) goto Next;
			}
			
			StaticBitTable<256> targetBitMask = byteTarget->GetValidBytes();
			if(bitMask == targetBitMask || targetBitMask.HasAllBitsSet()) equivalentCount++;
			if((bitMask & targetBitMask).HasAnyBitSet()) hasOverlap = true;
			bitMask |= targetBitMask;
		}
		
		if(equivalentCount >= numberOfTargets-1) continue;
		
		if(bitMask.HasAllBitsClear())
		{
			Instruction* replacement = new FailInstruction;
			split->TransferReferencesTo(replacement);
			split->Unlink();
			ReplaceInstruction(split, replacement);
			delete split;
			it = replacement;
		}
		else if(!hasOverlap)
		{
			DispatchTableInstruction* dispatchTableInstruction = new DispatchTableInstruction;
			split->TransferReferencesTo(dispatchTableInstruction);
			dispatchTableInstruction->index = split->index;
			
			uint32_t numberOfTargets = (uint32_t) split->targetList.GetCount();
			dispatchTableInstruction->targetList.Reserve(numberOfTargets+1);
			if(endOfInputInstruction)
			{
				dispatchTableInstruction->targetList.Append(endOfInputInstruction);
				endOfInputInstruction->referenceList.Append(InstructionReference{&dispatchTableInstruction->targetList[0], dispatchTableInstruction});
			}
			else
			{
				dispatchTableInstruction->targetList.Append(nullptr);
			}
			
			for(size_t j = 0; j < numberOfTargets; ++j)
			{
				Instruction* &target = split->targetList[j];
				target->referenceList.Remove(&target);
				
				Instruction* byteTarget = GetByteFilterStartingFrom(target);
				if(!byteTarget) continue;
				
				StaticBitTable<256> bitMask = byteTarget->GetValidBytes();
				if(bitMask.GetPopulationCount() != 0)
				{
					size_t index;
					if(dispatchTableInstruction->targetList.Contains(target))
					{
						index = dispatchTableInstruction->targetList.FindIndexForValue(target);
					}
					else
					{
						index = dispatchTableInstruction->targetList.GetCount();
						dispatchTableInstruction->targetList.Append(target);
						target->referenceList.Append(InstructionReference{&dispatchTableInstruction->targetList[index], dispatchTableInstruction});
					}

					for(uint32_t k = 0; k < 256; ++k)
					{
						if(bitMask[k]) dispatchTableInstruction->targetTable[k] = index;
					}
				}
				
			}
			
			ReplaceInstruction(split, dispatchTableInstruction);
			delete split;
			it = dispatchTableInstruction;
		}
		else
		{
			// Have a split, with targets that have overlapping regions!
			DispatchTableInstruction* dispatchTableInstruction = new DispatchTableInstruction;
			dispatchTableInstruction->referenceList.Reserve(split->referenceList.GetCount() + 256);
			dispatchTableInstruction->targetList.Reserve(256);
			
			split->TransferReferencesTo(dispatchTableInstruction);
			
			InstructionTable targetList[256];
			Javelin::Map<InstructionTable, uint32_t> stateMap;

			if(endOfInputInstruction != nullptr)
			{
				dispatchTableInstruction->targetList.Append(endOfInputInstruction);
				endOfInputInstruction->referenceList.Append(InstructionReference{&dispatchTableInstruction->targetList[0], dispatchTableInstruction});
				stateMap.Insert(dispatchTableInstruction->targetList, 0);
			}
			else
			{
				dispatchTableInstruction->targetList.Append(nullptr);
			}
			
			{
				InstructionTable emptyTargetList;
				stateMap.Insert(emptyTargetList, 0);
			}
			
			for(size_t j = 0; j < numberOfTargets; ++j)
			{
				Instruction* &target = split->targetList[j];
				Instruction* byteTarget = GetByteFilterStartingFrom(target);
				if(!byteTarget) continue;
				
				for(int p = 0; p < byteTarget->GetNumberOfPlanes(); ++p)
				{
					StaticBitTable<256> targetBitMask;
					byteTarget->SetValidBytesForPlane(p, targetBitMask);
					
					for(uint32_t x = 0; x < 256; ++x)
					{
						if(targetBitMask[x]) targetList[x].Append(target);
					}
				}
				
			}
			
			uint32_t index;
			for(uint32_t j = 0; j < 256; ++j)
			{
				if(j == 0 || targetList[j] != targetList[j-1])
				{
					const InstructionTable& byteTargets = targetList[j];
					Javelin::Map<InstructionTable, uint32_t>::Iterator it = stateMap.Find(byteTargets);
					if(it == stateMap.End())
					{
						Instruction* target = nullptr;
						switch(byteTargets.GetCount())
						{
						case 0:
							target = nullptr;
							break;
							
						case 1:
							target = byteTargets[0];
							break;
							
						default:
							// Create a split instruction
							{
								SplitInstruction* newSplit = new SplitInstruction;
								newSplit->targetList = byteTargets;
								newSplit->Link();
								InsertAfterInstruction(split, newSplit);
								target = newSplit;
							}
							break;
						}
						index = (uint32_t) dispatchTableInstruction->targetList.GetCount();
						stateMap.Insert(targetList[j], index);
						dispatchTableInstruction->targetList.Append(target);
						if(target) target->referenceList.Append(InstructionReference{&dispatchTableInstruction->targetList[index], dispatchTableInstruction});
					}
					else
					{
						index = it->value;
					}
				}
				dispatchTableInstruction->targetTable[j] = index;
			}
			
			for(size_t j = 0; j < split->targetList.GetCount(); ++j)
			{
				Instruction* &target = split->targetList[j];
				target->referenceList.Remove(&target);
			}
			
			ReplaceInstruction(split, dispatchTableInstruction);
			delete split;
			it = dispatchTableInstruction;
		}
		
	Next:
		;
	}
}

void InstructionList::Optimize_DelaySave(bool allowSplittingMultipleReferences)
{
	IndexInstructions();
	for(LinkedInstructionList::Iterator it = instructionList.ReverseBegin(); it != instructionList.ReverseEnd(); )
	{
		LinkedInstructionList::Iterator current = it;
		--it;
		if(current->type != InstructionType::Save) continue;
		Optimize_DelaySave((SaveInstruction*) &*current, allowSplittingMultipleReferences);
	}
}

void InstructionList::Optimize_DelaySave(SaveInstruction* saveInstruction, bool allowSplittingMultipleReferences)
{
	JASSERT(saveInstruction->type == InstructionType::Save);
	while(instructionList.End() != saveInstruction)
	{
		Instruction* nextInstruction = saveInstruction->GetNext();
		
		if(nextInstruction->type == InstructionType::Split)
		{
			if(nextInstruction->referenceList.GetCount() != 1) return;

			// Only delay past a split if:
			// 1. All targets have single reference
			// 2. All targets are higher index than the instruction.
			SplitInstruction* split = (SplitInstruction*) nextInstruction;
			for(Instruction* target : split->targetList)
			{
				if(target->index < split->index) return;
				if(target->referenceList.GetCount() > 1) return;
			}
	
			nextInstruction->referenceList.Remove(nullptr);
			saveInstruction->TransferReferencesTo(nextInstruction);
			RemoveInstruction(saveInstruction);
			
			// OK.. propagate our save instruction through every branch of the split!
			for(Instruction* target : split->targetList)
			{
				SaveInstruction* newSave = (SaveInstruction*) saveInstruction->Clone();
				target->TransferReferencesTo(newSave);
				target->referenceList.Append(InstructionReference::PREVIOUS);
				InsertBeforeInstruction(target, newSave);
				Optimize_DelaySave(newSave, allowSplittingMultipleReferences);
			}
			
			delete saveInstruction;
			return;
		}
		else if(nextInstruction->type == InstructionType::Jump)
		{
			if(!allowSplittingMultipleReferences) return;
			
			JumpInstruction* jump = (JumpInstruction*) nextInstruction;
			if(jump->referenceList.GetCount() != 1) return;
			
			// Can we hoist a byte jump target?
			Instruction* target = jump->target;
			if(target->IsSimpleByteConsumer())
			{
				saveInstruction->saveOffset++;
			}
			else if(target->type == InstructionType::Save)
			{
				return;
			}
			else if(!target->IsEmptyWidth())
			{
				return;
			}
			
			// Need to transform:
			//      save                byte x
			//      jump n              save
			//   n: byte x         ->   jump n+1

			Instruction* duplicate = target->Clone();
			duplicate->index = target->index;
			saveInstruction->TransferReferencesTo(duplicate);
			InsertBeforeInstruction(saveInstruction, duplicate);
			saveInstruction->referenceList.Append(InstructionReference::PREVIOUS);
			jump->Unlink();
			jump->target = target->GetNext();
			jump->Link();

			continue;
		}
		else
		{
			if(nextInstruction->referenceList.GetCount() != 1
			   && !allowSplittingMultipleReferences) return;

			if(nextInstruction->IsSimpleByteConsumer())
			{
				saveInstruction->saveOffset++;
			}
			else if(nextInstruction->type == InstructionType::Save
					&& ((SaveInstruction*) nextInstruction)->saveOffset < saveInstruction->saveOffset)
			{
				return;
			}
			else if(!nextInstruction->IsEmptyWidth())
			{
				return;
			}
			
			if(nextInstruction->referenceList.GetCount() == 1)
			{
				nextInstruction->referenceList.Remove(nullptr);
				saveInstruction->TransferReferencesTo(nextInstruction);
				saveInstruction->referenceList.Append(InstructionReference::PREVIOUS);
				
				RemoveInstruction(saveInstruction);
				
				// Special case
				if(nextInstruction->type == InstructionType::Fail)
				{
					delete saveInstruction;
					return;
				}
				
				InsertAfterInstruction(nextInstruction, saveInstruction);
			}
			else
			{
				Instruction* duplicate = nextInstruction->Clone();
				duplicate->index = nextInstruction->index;
				JumpInstruction* jump = new JumpInstruction(nextInstruction->GetNext());
				jump->index = nextInstruction->index;
				saveInstruction->TransferReferencesTo(duplicate);
				InsertBeforeInstruction(saveInstruction, duplicate);
				InsertAfterInstruction(saveInstruction, jump);
				saveInstruction->referenceList.Append(InstructionReference::PREVIOUS);
				nextInstruction->referenceList.Remove(nullptr);
				jump->referenceList.Append(InstructionReference::PREVIOUS);
			}
		}
	}
}

Instruction* InstructionList::Optimize_SimplifyByteJumpTable(ByteJumpTableInstruction* jumpTableInstruction)
{
	// If a ByteJumpTable has a single target of itself, then convert to fail
	// If it has a single target that is the next instruction, then convert to bit mask
	
	if(jumpTableInstruction->targetList.GetCount() == 1)
	{
		Instruction* &target = jumpTableInstruction->targetList[0];
		target->referenceList.Remove(&target);
		Instruction* replacement;
		if(target == jumpTableInstruction)
		{
			// Replace jump table with fail!
			replacement = new FailInstruction;
			ReplaceInstruction(jumpTableInstruction, replacement);
		}
		else
		{
			replacement = new AnyByteInstruction;
			ReplaceInstruction(jumpTableInstruction, replacement);
			if(target != jumpTableInstruction->GetNext())
			{
				JumpInstruction* jump = new JumpInstruction(target);
				InsertAfterInstruction(replacement, jump);
			}
			replacement->GetNext()->referenceList.Append(InstructionReference::PREVIOUS);
		}
		jumpTableInstruction->TransferReferencesTo(replacement);
		delete jumpTableInstruction;
		return replacement;
	}
	
	if(jumpTableInstruction->targetList.GetCount() != 2) return jumpTableInstruction;
	
	if(jumpTableInstruction->targetList[0] != nullptr &&
	   jumpTableInstruction->targetList[1] != nullptr) return jumpTableInstruction;
	
	Instruction* target = jumpTableInstruction->targetList[0] != nullptr ? jumpTableInstruction->targetList[0] : jumpTableInstruction->targetList[1];
	if(target != jumpTableInstruction->GetNext()) return jumpTableInstruction;

	uint8_t targetIndex = jumpTableInstruction->targetList[0] != nullptr ? 0 : 1;

	// We've got a jump table that just jumps to the next instruction!
	// -> Replace it with a Byte, ByteRange, ByteEitherOf2, ByteEitherOf3 or ByteBitMaskInstruction
	
	StaticBitTable<256> bitMask;
	for(size_t i = 0; i < 256; ++i)
	{
		if(jumpTableInstruction->targetTable[i] == targetIndex) bitMask.SetBit(i);
	}
	
	Instruction* replacement;
	if(bitMask.IsContiguous())
	{
		if(bitMask.GetPopulationCount() == 1)
		{
			replacement = new ByteInstruction(bitMask.CountTrailingZeros());
		}
		else
		{
			Interval<size_t> range = bitMask.GetContiguousRange();
			replacement = new ByteRangeInstruction(range.min, range.max-1);
		}
	}
	else switch(bitMask.GetPopulationCount())
	{
	case 0:
		replacement = new FailInstruction;
		break;

// TODO
//	case 2:
//	case 3:
			
	default:
		replacement = new ByteBitMaskInstruction(bitMask);
		break;
	}
	
	jumpTableInstruction->TransferReferencesTo(replacement);
	target->referenceList.Remove(&jumpTableInstruction->targetList[targetIndex]);
	target->referenceList.Append(InstructionReference::PREVIOUS);
	ReplaceInstruction(jumpTableInstruction, replacement);
	delete jumpTableInstruction;
	return replacement;
}

void InstructionList::Optimize_DispatchTargetsToAdvanceByte()
{
	// If an instruction already has been filtered by dispatch table,
	// convert it to a (quicker) advance byte instruction.
	
	Map<Instruction*, int> dispatchReferenceCount;
	for(Instruction&  instruction : instructionList)
	{
		if(instruction.type != InstructionType::DispatchTable) continue;
		
		DispatchTableInstruction* dispatchInstruction = (DispatchTableInstruction*) &instruction;
		for(Instruction* target : dispatchInstruction->targetList)
		{
			if(target) dispatchReferenceCount[target]++;
		}
	}
	
	for(LinkedInstructionList::Iterator it = instructionList.ReverseBegin(); it != instructionList.ReverseEnd(); --it)
	{
		if(!it->IsSimpleByteConsumer()) continue;
		
		Map<Instruction*, int>::Iterator entry = dispatchReferenceCount.Find(&*it);
		if(entry != dispatchReferenceCount.End())
		{
			if(it->referenceList.GetCount() == entry->value)
			{
				Instruction* replacement = new AdvanceByteInstruction;
				it->TransferReferencesTo(replacement);
				ReplaceInstruction(&*it, replacement);
				delete &*it;
				it = replacement;
			}
		}
	}
}

void InstructionList::Optimize_ByteJumpTableToFindByte()
{
	for(LinkedInstructionList::Iterator it = instructionList.Begin(); it != instructionList.End(); ++it)
	{
		if(it->type != InstructionType::ByteJumpTable) continue;
		
		ByteJumpTableInstruction* jumpTableInstruction = (ByteJumpTableInstruction*) &*it;
		size_t index = jumpTableInstruction->targetList.FindIndexForValue(jumpTableInstruction);
		if(index == TypeData<size_t>::Maximum()) continue;

		StaticBitTable<256> afterMask;
		
		for(int j = 0; j < 256; ++j)
		{
			if(jumpTableInstruction->targetTable[j] != index)
			{
				afterMask.SetBit(j);
			}
		}

		size_t count = afterMask.GetPopulationCount();
		
		if(count == 0)
		{
			// This means that we're stuck permanently in this state, with no exit. Convert to fail
			it = ReplaceWithFail(jumpTableInstruction);
			continue;
		}
		
		if(count > 32) continue;
		
		if(afterMask.IsContiguous())
		{
			Interval<size_t> range = afterMask.GetContiguousRange();
			if(range.GetSize() == 1)
			{
				
				Instruction* target = jumpTableInstruction->targetList[jumpTableInstruction->targetTable[range.min]];
				FindByteInstruction* replacement = new FindByteInstruction(range.min, target);
				replacement->referenceList.Append(InstructionReference{InstructionReference::SELF_REFERENCE, replacement});
				target->referenceList.Append(InstructionReference{&replacement->target, replacement});
				jumpTableInstruction->TransferReferencesTo(replacement);
				
				ReplaceInstruction(jumpTableInstruction, replacement);
				jumpTableInstruction->Unlink();
				delete jumpTableInstruction;
				it = replacement;
			}
			else if(scanDirection == Forwards)
			{
				uint8_t bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
				bytes[0] = range.min;
				bytes[1] = range.max-1;
				Instruction* searcher = new SearchByteInstruction(InstructionType::SearchByteRange, bytes, 0);
				InsertBeforeInstruction(jumpTableInstruction, searcher);
				jumpTableInstruction->TransferReferencesTo(searcher);
				jumpTableInstruction->referenceList.Append(InstructionReference::PREVIOUS);
			}
		}
		else if(count <= 8 && scanDirection == Forwards)
		{
			int count = 0;
			uint8_t bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
			for(int j = 0; j < 256; ++j)
			{
				if(jumpTableInstruction->targetTable[j] != index)
				{
					bytes[count++] = j;
				}
			}

			JASSERT(2 <= count && count <= 8);
			Instruction* searcher = new SearchByteInstruction(count, bytes, 0);
			InsertBeforeInstruction(jumpTableInstruction, searcher);
			jumpTableInstruction->TransferReferencesTo(searcher);
			jumpTableInstruction->referenceList.Append(InstructionReference::PREVIOUS);	
		}
	}
}

void InstructionList::Optimize_ForwardJumpTargets()
{
	for(Instruction& instruction : instructionList)
	{
		if(instruction.type != InstructionType::Jump) continue;
		
		Instruction* target = ((JumpInstruction&) instruction).target;
		while(target->type == InstructionType::Jump) target = ((JumpInstruction*) target)->target;
		
		for(size_t i = instruction.referenceList.GetCount(); i > 0;)
		{
			--i;
			
			InstructionReference ref = instruction.referenceList[i];
			if(!ref.storage) continue;
			
			*ref.storage = target;
			instruction.referenceList.RemoveIndex(i);
			target->referenceList.Append(ref);
		}
	}
}

//==========================================================================

static AnyByteResult IsAnyByteSequence(AnyByteInstruction* anyByte)
{
	// Looking for:
	// x:   any-byte
	// x+1: split x, x+2
	//
	// or:
	// x:   any-byte
	// x+1: split x+2, x
	Instruction* next = anyByte->GetNext();
	if(next->type != InstructionType::Split) return AnyByteResult::False;

	SplitInstruction* split = (SplitInstruction*) next;
	if(split->targetList.GetCount() != 2) return AnyByteResult::False;
	if(split->targetList[0] == anyByte)
	{
		return (split->targetList[1] == split->GetNext()) ? AnyByteResult::Maximal : AnyByteResult::False;
	}
	if(split->targetList[1] == anyByte)
	{
		return (split->targetList[0] == split->GetNext()) ? AnyByteResult::Minimal : AnyByteResult::False;
	}
	return AnyByteResult::False;
}

static AnyByteResult IsJumpAnyByteSequence(JumpInstruction* jump)
{
	// Looking for:
	// x:   jump x+2
	// x+1: any-byte
	// x+2: split x+1, x+3
	//
	// or:
	// x:   jump x
	// x+1: any-byte
	// x+2: split x+3, x+1
	Instruction* next = jump->GetNext();
	if(next->type != InstructionType::AnyByte) return AnyByteResult::False;
	Instruction* anyByte = next;
	
	Instruction* afterAnyByte = anyByte->GetNext();
	if(afterAnyByte->type != InstructionType::Split) return AnyByteResult::False;
	SplitInstruction* split = (SplitInstruction*) afterAnyByte;
	if(split->targetList.GetCount() != 2) return AnyByteResult::False;
	if(jump->target != split) return AnyByteResult::False;
	
	if(split->targetList[0] == anyByte && split->targetList[1] == split->GetNext()) return AnyByteResult::Minimal;
	else if(split->targetList[1] == anyByte && split->targetList[0] == split->GetNext()) return AnyByteResult::Maximal;
	return AnyByteResult::False;
}

bool InstructionList::RequiresAnyByteMinimalForPartialMatch(IComponent* headComponent) const
{
	if(isFixedLength)
	{
		if(hasStartAnchor || hasEndAnchor) return false;
	}
	
	if(!headComponent->RequiresAnyByteMinimalForPartialMatch()) return false;
	
	for(const Instruction& instruction : instructionList)
	{
		switch(instruction.type)
		{
		case InstructionType::AnyByte:		// Any bytes followed immediately by AnyByteMiminal/AnyByteMaximal represent .+ or .+?
			if(IsAnyByteSequence((AnyByteInstruction*) &instruction) != AnyByteResult::False) return false;
			continue;
				
		case InstructionType::AssertStartOfInput:
			return false;
				
		case InstructionType::AssertStartOfSearch:
			return false;
			
		case InstructionType::Save:
			continue;

		case InstructionType::Jump:
			return IsJumpAnyByteSequence((JumpInstruction*) &instruction) == AnyByteResult::False;
				
		default:
			return true;
		}
	}
	return true;
}

//==========================================================================

void InstructionList::Optimize_CaptureSearch()
{
//	Dump(StandardOutput);
//	
	for(Instruction& instruction : instructionList)
	{
		AnyByteResult anyByteResult;
		Instruction* original = nullptr;
		Instruction* afterAnyByte = nullptr;
		switch(instruction.type)
		{
		case InstructionType::AnyByte:
			anyByteResult = IsAnyByteSequence((AnyByteInstruction*) &instruction);
			switch(anyByteResult)
			{
			case AnyByteResult::False:
				continue;
					
			default:
				original = instruction.GetNext();
				afterAnyByte = original->GetNext();
				break;
			}
			break;

		default:
			continue;
		}
	
		AddToSearchOptimizer(original, afterAnyByte, anyByteResult, true, StaticBitTable<256>{});
	}
}

void InstructionList::AddToSearchOptimizer(Instruction* split, Instruction* afterAnyByte, AnyByteResult mode, bool canSkipBackwardsPropagate, const StaticBitTable<256>& ReversePropagate)
{
	
	// Build the BM table!
	SearchOptimizer* optimizer = new SearchOptimizer;
	optimizer->isMaximal = mode == AnyByteResult::Maximal;
	optimizer->original = split;
	optimizer->canSkipBackwardsPropagate = canSkipBackwardsPropagate;
	optimizer->backwardsPropagate = ReversePropagate;
	searchOptimizerList.Append(optimizer);
	
	InstructionWalker& walker = optimizer->instructionWalker;
	walker.AddInitialInstruction(afterAnyByte);
	
	while(walker.ShouldContinue())
	{
		walker.AdvanceAllInstructions();
	}
	
	if(walker.GetPresenceListCount() == 0)
	{
		delete optimizer;
	}
	else
	{
		split->referenceList.Append(InstructionReference{&optimizer->original, InstructionReference::SEARCH_OPTIMIZER_REFERENCE});
	}
}

// This is the relative distribution of bytes in 181G of data (my home directory) in fixed point .16 format
// weighted [4:1] [text file types:all data]
// It's not entirely scientific, but it does reveal searching for "z" is better than searching for "a"
static constexpr uint32_t DISTRIBUTION_TABLE[256] =
{
	
	1876,  176,   88,  116,  152,   48,   45,   46,   61,  390, 1594,   37,   40,  259,   72,   42,
	  49,   35,   33,   32,   38,   35,   32,   32,   39,   34,   31,   31,   33,   31,   32,   38,
 	7964,   65,  816,  129,   88,   52,   94,  130,  390,  384,  556,   85,  426,  473,  721,  822,		//  !@#$%&'()*+,-./
	 327,  364,  226,  212,  212,  205,  220,  222,  234,  187,  306,  331,  572,  499,  598,   53,		// 0123456789:;<=>?
	  72,  464,  206,  353,  334,  419,  203,  173,  165,  394,   69,   91,  275,  209,  283,  324,		// @ABCDEFGHIJKLMNO
	 244,   70,  297,  412,  472,  164,  119,  108,   92,   85,   62,  114,   59,  113,   29, 1048,		// PQRSTUVWXYZ[\]^_
	  36, 2386,  576, 1559, 1112, 3269,  662,  689,  786, 2040,  142,  270, 1513,  778, 1893, 1798,		// `abcdefghijklmno
	 973,   88, 1832, 1972, 2590,  773,  382,  292,  280,  398,   96,  113,  776,  111,   31,   33,		// pqrstuvwxyz{|}~
	  49,   32,   30,   33,   33,   31,   28,   27,   33,   42,   29,   41,   30,   32,   28,   28,
	  35,   35,   35,   27,   31,   31,   30,   28,   31,   27,   28,   26,   28,   26,   28,   31,
	  34,   27,   27,   26,   28,   28,   26,   27,   31,   32,   63,   28,   31,   30,   28,   31,
	  31,   30,   30,   27,   30,   33,   28,   30,   31,   33,   29,   28,   29,   32,   28,   35,
	  42,   32,   30,   30,   31,   30,   30,   37,   32,   29,   26,   26,   31,   27,   27,   28,
	  33,   28,   28,   27,   26,   30,   29,   28,   29,   25,   27,   27,   27,   27,   27,   29,
	  42,   34,   33,   30,   32,   29,   27,   29,   37,   34,   29,   31,   28,   31,   29,   30,
	  41,   31,   35,   31,   29,   29,   35,   30,   39,   38,   30,   30,   33,   35,   37,  148,
};

uint32_t InstructionList::CalculateSearchEffectivenessValue(const uint32_t* data, size_t offset)
{
	uint32_t result = 0;
	for(int i = 0; i < 256; ++i)
	{
		result += (offset - data[i]) * DISTRIBUTION_TABLE[i];
	}
	return result;
}

// Concepts:
// Performance of an accelerator is determined by two factors:
// 1. How quickly it can find a potential match
// 2. How likely the potential match is going to lead to a real match.
//
// Example. If the pattern in "abcde", then:
//  Searching for single byte "a" is fast (~25GB/sec), but will lead to a large number of false matches
//  Searching for single byte "b" is fast (~25GB/sec), and will generally lead to less false matches
//  Searching for pair byte "bc" is reasonably fast (16.5GB/sec), and will lead to dramatically less false matches
//  BoyerMoore is slower (~1.5GB/sec) and will sometimes lead to a match
//  Searching for ShiftOr "abcde" is moderate speed (2.5GB/sec), but will always lead to a match
//
// From these observations, use a heuristic which is roughly the product
// of the scan speed and match probability to select the best search algorithm


static uint32_t ScaleBaseSpeedForProbability(uint32_t baseSpeed, float probability)
{
	constexpr float    PROBABILITY_FOR_LENGTH	= 0.90f;
	constexpr uint32_t RELAXATION_LENGTH 		= 224;
	
	float length = Math::Log(1.0f-PROBABILITY_FOR_LENGTH) / Math::Log(1.0f-probability);
	return uint32_t(baseSpeed * (1.0 - exp(-length/RELAXATION_LENGTH)));
}

void InstructionList::SearchOptimizer::ReplaceOriginal(Instruction* searcher, bool replaceByteJumpTableWithAdvanceByte, bool hasZeroOffsetCheck, Instruction*& partialMatchInstruction)
{
	Instruction* localOriginal = original;
	InsertBeforeInstruction(localOriginal, searcher);
	localOriginal->TransferReferencesTo(searcher);
	localOriginal->referenceList.Append(InstructionReference::PREVIOUS);
	
	if(backwardsPropagate.HasAnyBitSet()
	   && (!hasZeroOffsetCheck || !canSkipBackwardsPropagate))
	{
		Instruction* propagateBackwards = new PropagateBackwardsInstruction(backwardsPropagate);
		InsertAfterInstruction(searcher, propagateBackwards);
		propagateBackwards->referenceList.Append(InstructionReference::PREVIOUS);
		
		// Special case -- Don't allow partial match to go before a propagate Reverse
		if(partialMatchInstruction == searcher)
		{
			searcher->referenceList.Remove(&partialMatchInstruction);
			partialMatchInstruction = localOriginal;
			localOriginal->referenceList.Append(InstructionReference{&partialMatchInstruction,InstructionReference::START_REFERENCE});
		}
		return;
	}
	
	// Common case -- search at offset 0 to a byte jump table that only has itself and the next instruction as targets
	if(replaceByteJumpTableWithAdvanceByte
	   && localOriginal->type == InstructionType::ByteJumpTable
	   && localOriginal->referenceList.GetCount() == 1)
	{
		JumpTableInstruction* jumpTableInstruction = (JumpTableInstruction*) localOriginal;
		Instruction* next = jumpTableInstruction->GetNext();
		if(jumpTableInstruction->targetList.GetCount() == 2
		   && ((jumpTableInstruction->targetList[0] == searcher && jumpTableInstruction->targetList[1] == jumpTableInstruction->GetNext())
			   || (jumpTableInstruction->targetList[1] == searcher && jumpTableInstruction->targetList[0] == jumpTableInstruction->GetNext())))
		{
			Instruction* advance = new AdvanceByteInstruction;
			ReplaceInstruction(localOriginal, advance);
			advance->referenceList.Append(InstructionReference::PREVIOUS);
			next->referenceList.Append(InstructionReference::PREVIOUS);
			localOriginal->Unlink();
			delete localOriginal;
		}
	}
}


void InstructionList::Optimize_AccelerateSearch()
{
	constexpr uint64_t PROBABILITY_SCALER                    = 476;           // This scales the probability table of [A-Za-z] to be 1.0 in fixed .24 format
	constexpr uint64_t SHIFT_OR_PROBABILITY_SCALER           = 2;
	
	constexpr uint32_t PAIR_TAG                              = 0x80000000;
	constexpr uint32_t TRIPLET_TAG                           = 0xc0000000;
	
	constexpr uint32_t MIN_SHIFT_OR_THRESHOLD                = 3;
	constexpr uint32_t MIN_BOYER_MOORE_THRESHOLD             = 3;
	constexpr uint32_t MAX_SHIFT_OR_THRESHOLD                = 24;
	constexpr uint64_t BOYER_MOORE_SCAN_SPEED_PER_CHARACTER  = 320;
	constexpr uint32_t SCAN_SPEED_THRESHOLD                  = 800;
	
	constexpr uint32_t SHIFT_OR_RAMP_LENGTH					 = 6;
	
	// These values were measured performance in MB/sec
	constexpr uint32_t FIND_BYTE_SCAN_SPEED[]                = { 25000, 20000, 16500, 13000, 10100, 9200, 7700, 6900 };
	constexpr uint32_t FIND_BYTE_PAIR_SCAN_SPEED[]           = { 16500, 11400, 8500, 6500 };
	constexpr uint32_t FIND_BYTE_TRIPLET_SCAN_SPEED[]		 = { 10600, 7000 };
	constexpr uint32_t FIND_BYTE_RANGE_SCAN_SPEED            = 22000;
	constexpr uint32_t FIND_BYTE_RANGE_PAIR_SCAN_SPEED       = 13000;
	constexpr uint32_t SHIFT_OR_SCAN_SPEED                   = 2500;
	
	constexpr uint32_t FIND_BYTE_PAIR_PROBABILITY_SCALE		 = 1;
	constexpr uint32_t FIND_BYTE_TRIPLET_PROBABILITY_SCALE	 = 1;
	
	constexpr uint32_t MAX_SHIFT_OR_PROBABILITY				 = 13421772;      // this is 80% in Fixed.24
	constexpr uint32_t MAX_SEARCH_BYTE_PROBABILITY           = 15099494;      // this is 90% in Fixed.24
	constexpr uint32_t MIN_SEARCH_BYTE_PROBABILITY           = 12582912;      // this is 75% in Fixed.24
	
	for(SearchOptimizer& optimizer : searchOptimizerList)
	{
		if(optimizer.original->type == InstructionType::FindByte
		   || optimizer.original->type == InstructionType::Fail)
		{
//			StandardOutput.PrintF("Converted to find byte already! ignore\n");
			continue;
		}

		bool useShiftOr = false;
		uint32_t bestScanSpeed = 0;	// Units is MB/sec * probabilityOfMatch
		uint32_t bestSearchIndex = 0;
		uint32_t searchData[256];
		memset(searchData, -1, sizeof(searchData));
		
		if(optimizer.instructionWalker.GetPresenceListCount() >= MIN_BOYER_MOORE_THRESHOLD)
		{
			for(size_t i = 0; i < optimizer.instructionWalker.GetPresenceListCount(); ++i)
			{
				const StaticBitTable<256>& presenceBits = optimizer.instructionWalker.GetPresenceBits(i);
				int lastCharacterProbability = 0;
				for(int j = 0; j < 256; ++j)
				{
					if(presenceBits[j])
					{
						lastCharacterProbability += DISTRIBUTION_TABLE[j];
						searchData[j] = uint32_t(i);
					}
				}
				
				int score = CalculateSearchEffectivenessValue(searchData, i);
				int scanSpeed = BOYER_MOORE_SCAN_SPEED_PER_CHARACTER * score * (0x10000 - lastCharacterProbability) >> 32;
				if(scanSpeed > bestScanSpeed)
				{
					bestSearchIndex = uint32_t(i);
					bestScanSpeed = scanSpeed;
				}
//				StandardOutput.PrintF("Search effectiveness: %z: %d\n", i, score);
			}
		}

		// Can we use SearchByte?
//		StandardOutput.PrintF("prefix length: %z\n", optimizer.presenceBitsList.GetCount());
		{
			struct SearchData
			{
				bool	 isContinugous;
				uint32_t count;
				uint64_t score;
			};
			
			Table<SearchData> searchDataList(optimizer.instructionWalker.GetPresenceListCount());
			uint32_t bestIndex = TypeData<uint32_t>::Maximum();
			uint32_t bestSearchByteScanSpeed = 0;
			bool isFail = false;
			for(size_t p = 0; p < optimizer.instructionWalker.GetPresenceListCount(); ++p)
			{
				const StaticBitTable<256>& presenceBits = optimizer.instructionWalker.GetPresenceBits(p);
				Optional<Interval<size_t>> contiguousRangeOptional = presenceBits.GetContiguousRange();
				SearchData data = { false, 0, 0 };
				if(contiguousRangeOptional.HasValue())
				{
					data.isContinugous = true;
					Interval<size_t> range = contiguousRangeOptional;
					for(size_t i = range.min; i < range.max; ++i)
					{
						data.score += DISTRIBUTION_TABLE[i];
					}
					data.count = range.GetSize();
				}
				else
				{
					for(int i = 0; i < 256; ++i)
					{
						if(presenceBits[i])
						{
							data.score += DISTRIBUTION_TABLE[i];
							++data.count;
						}
					}
					if(data.score == 0) isFail = true;
				}
				searchDataList.Append(data);
			}

			if(isFail)
			{
				// This pattern is impossible!
				ReplaceWithFail(optimizer.original);
				continue;
			}

			if(MIN_SHIFT_OR_THRESHOLD <= searchDataList.GetCount()
			   && searchDataList.GetCount() < MAX_SHIFT_OR_THRESHOLD)
			{
				uint32_t scanSpeed = SHIFT_OR_SCAN_SPEED;

				if(!isFixedLength)
				{
					// Reduce the shift or scan speed by the probability of each character.
					// ie. the higher the probabilty, the lower the usefulness of the search
					// eg. if there's a ".{2,4}(abc|def)", it's going to end up having a LOT of checks that fail
					// Conversely, if most characters are acutally meant to match, then the accelerator doens't matter
					// Limit the maximum probability to achieve the effect that long strings 
					
					// totalProbability is the chance that all positions match
					uint64_t totalProbability = 1<<24;
					bool isLeading = true;
					uint32_t effectiveLength = 0;
					for(size_t i = 0; i < searchDataList.GetCount(); ++i)
					{
						uint32_t probability = searchDataList[i].score * PROBABILITY_SCALER;
						if(isLeading)
						{
							if(probability > MAX_SHIFT_OR_PROBABILITY) continue;
							else isLeading = false;
						}
						
						++effectiveLength;
						probability *= SHIFT_OR_PROBABILITY_SCALER;

						if(probability > MAX_SHIFT_OR_PROBABILITY) probability = MAX_SHIFT_OR_PROBABILITY;
						totalProbability = totalProbability * probability >> 24;
					}
					
					scanSpeed = scanSpeed  * ((1ull<<24) - totalProbability) >> 24;
					
					// Linear ramp up
					if(effectiveLength < SHIFT_OR_RAMP_LENGTH)
					{
						scanSpeed = scanSpeed * effectiveLength / SHIFT_OR_RAMP_LENGTH;
					}
				}
				
				DEBUG_SEARCH_PRINTF("ShiftOrScanSpeed: %u, BoyerMooreScanSpeed: %u\n", scanSpeed, bestScanSpeed);
				if(scanSpeed > bestScanSpeed)
				{
					useShiftOr = true;
					bestScanSpeed = scanSpeed;
				}
			}

			for(size_t i = 0; i < searchDataList.GetCount(); ++i)
			{
				if(searchDataList[i].isContinugous)
				{
					if(searchDataList[i].count > 32) continue;
				}
				else
				{
					if(searchDataList[i].count > 8) continue;
				}

				
				// For each of the search byte approaches, the performance is only attained when it can skip a large number of bytes
				// Given a probability, we can calculate the the length required for a > 90% match probability:
				// (1-p)^length = (1.0-0.9)
				// length.log(1-p) = log(0.1)
				// length = log(0.1) / log(1-p)
				
				// Do we have a SearchByteRangeTriplet candidate
				if(i+2 < searchDataList.GetCount()
				   && searchDataList[i].count <= 2
				   && searchDataList[i+1].count <= 2
				   && searchDataList[i+2].count <= 2)
				{
					uint32_t length = Maximum(searchDataList[i].count, searchDataList[i+1].count, searchDataList[i+2].count);
					uint32_t baseScanSpeed = FIND_BYTE_TRIPLET_SCAN_SPEED[length-1];
					
					int64_t scoreScale1 = searchDataList[i].score * PROBABILITY_SCALER;
					int64_t scoreScale2 = searchDataList[i+1].score * PROBABILITY_SCALER;
					int64_t scoreScale3 = searchDataList[i+2].score * PROBABILITY_SCALER;
					if(scoreScale1 > (1<<24)) scoreScale1 = 1<<24;
					if(scoreScale2 > (1<<24)) scoreScale2 = 1<<24;
					if(scoreScale3 > (1<<24)) scoreScale3 = 1<<24;
					uint64_t scoreScale12 = (scoreScale1 * scoreScale2) >> 24;
					uint64_t scoreScale123 = (scoreScale12 * scoreScale3) >> 24;
					
					float probability = scoreScale123 * (FIND_BYTE_TRIPLET_PROBABILITY_SCALE / 16777216.0f);
					if(probability < 1.0)
					{
						uint32_t scanSpeed = ScaleBaseSpeedForProbability(baseScanSpeed, probability);
						
						if(searchDataList.GetCount() > 3)
						{
							uint64_t probabilityScale = 1<<24;
							for(size_t j = 0; j < i; ++j)
							{
								uint64_t probability = searchDataList[i].score * PROBABILITY_SCALER;
								if(probability > MAX_SEARCH_BYTE_PROBABILITY) probability = MAX_SEARCH_BYTE_PROBABILITY;
								if(probability < MIN_SEARCH_BYTE_PROBABILITY) probability = MIN_SEARCH_BYTE_PROBABILITY;
								probabilityScale = probabilityScale * probability >> 24;
							}
							for(size_t j = i+3; j < searchDataList.GetCount(); ++j)
							{
								uint64_t probability = searchDataList[i].score * PROBABILITY_SCALER;
								if(probability > MAX_SEARCH_BYTE_PROBABILITY) probability = MAX_SEARCH_BYTE_PROBABILITY;
								if(probability < MIN_SEARCH_BYTE_PROBABILITY) probability = MIN_SEARCH_BYTE_PROBABILITY;
								probabilityScale = probabilityScale * probability >> 24;
							}
							
							scanSpeed = scanSpeed * ((1<<24) - probabilityScale) >> 24;
						}
						
						DEBUG_SEARCH_PRINTF("SearchByteTriplet scan speed: %u, %z (p: %g)\n", scanSpeed, i, probability);
						if(scanSpeed > bestSearchByteScanSpeed)
						{
							bestSearchByteScanSpeed = scanSpeed;
							bestIndex = i | TRIPLET_TAG;
						}
					}
				}
				
				// Not an else if intentionally since byte pair could well be faster
				// Do we have a SearchByteRangePair candidate
				if(searchDataList[i].isContinugous
				   && i+1 < searchDataList.GetCount()
				   && searchDataList[i+1].isContinugous
				   && searchDataList[i+1].count <= 32)
				{
					uint32_t baseScanSpeed = (searchDataList[i].count == 1 && searchDataList[i+1].count == 1) ?
												FIND_BYTE_PAIR_SCAN_SPEED[0] :
												FIND_BYTE_RANGE_PAIR_SCAN_SPEED;
					
					int64_t scoreScale1 = searchDataList[i].score * PROBABILITY_SCALER;
					int64_t scoreScale2 = searchDataList[i+1].score * PROBABILITY_SCALER;
					if(scoreScale1 > (1<<24)) scoreScale1 = 1<<24;
					if(scoreScale2 > (1<<24)) scoreScale2 = 1<<24;
					uint64_t scoreScale12 = (scoreScale1 * scoreScale2) >> 24;
					
					float probability = scoreScale12 * (FIND_BYTE_PAIR_PROBABILITY_SCALE / 16777216.0f);
					if(probability < 1.0)
					{
						uint32_t scanSpeed = ScaleBaseSpeedForProbability(baseScanSpeed, probability);
						
						if(searchDataList.GetCount() > 2)
						{
							uint64_t probabilityScale = 1<<24;
							for(size_t j = 0; j < i; ++j)
							{
								uint64_t probability = searchDataList[i].score * PROBABILITY_SCALER;
								if(probability > MAX_SEARCH_BYTE_PROBABILITY) probability = MAX_SEARCH_BYTE_PROBABILITY;
								if(probability < MIN_SEARCH_BYTE_PROBABILITY) probability = MIN_SEARCH_BYTE_PROBABILITY;
								probabilityScale = probabilityScale * probability >> 24;
							}
							for(size_t j = i+2; j < searchDataList.GetCount(); ++j)
							{
								uint64_t probability = searchDataList[i].score * PROBABILITY_SCALER;
								if(probability > MAX_SEARCH_BYTE_PROBABILITY) probability = MAX_SEARCH_BYTE_PROBABILITY;
								if(probability < MIN_SEARCH_BYTE_PROBABILITY) probability = MIN_SEARCH_BYTE_PROBABILITY;
								probabilityScale = probabilityScale * probability >> 24;
							}
							
							scanSpeed = scanSpeed * ((1<<24) - probabilityScale) >> 24;
						}
						
						DEBUG_SEARCH_PRINTF("SearchByteRangePair scan speed: %u, %z (p: %g)\n", scanSpeed, i, probability);
						if(scanSpeed > bestSearchByteScanSpeed)
						{
							bestSearchByteScanSpeed = scanSpeed;
							bestIndex = i | PAIR_TAG;
						}
					}
				}
				// Or a SearchBytePair candidate
				else if(searchDataList[i].count <= 4
				   && i+1 < searchDataList.GetCount()
				   && searchDataList[i+1].count <= 4)
				{
					JASSERT(searchDataList[i].count > 0 && searchDataList[i+1].count > 0);

					uint32_t length = Maximum(searchDataList[i].count, searchDataList[i+1].count);
					uint32_t baseScanSpeed = FIND_BYTE_PAIR_SCAN_SPEED[length-1];
					
					int64_t scoreScale1 = searchDataList[i].score * PROBABILITY_SCALER;
					int64_t scoreScale2 = searchDataList[i+1].score * PROBABILITY_SCALER;
					if(scoreScale1 > (1<<24)) scoreScale1 = 1<<24;
					if(scoreScale2 > (1<<24)) scoreScale2 = 1<<24;
					uint64_t scoreScale12 = (scoreScale1 * scoreScale2) >> 24;

					float probability = scoreScale12 * (FIND_BYTE_PAIR_PROBABILITY_SCALE / 16777216.0f);
					if(probability < 1.0)
					{
						uint32_t scanSpeed = ScaleBaseSpeedForProbability(baseScanSpeed, probability);
						
						if(searchDataList.GetCount() > 2)
						{
							uint64_t probabilityScale = 1<<24;
							for(size_t j = 0; j < i; ++j)
							{
								uint64_t probability = searchDataList[i].score * PROBABILITY_SCALER;
								if(probability > MAX_SEARCH_BYTE_PROBABILITY) probability = MAX_SEARCH_BYTE_PROBABILITY;
								if(probability < MIN_SEARCH_BYTE_PROBABILITY) probability = MIN_SEARCH_BYTE_PROBABILITY;
								probabilityScale = probabilityScale * probability >> 24;
							}
							for(size_t j = i+2; j < searchDataList.GetCount(); ++j)
							{
								uint64_t probability = searchDataList[i].score * PROBABILITY_SCALER;
								if(probability > MAX_SEARCH_BYTE_PROBABILITY) probability = MAX_SEARCH_BYTE_PROBABILITY;
								if(probability < MIN_SEARCH_BYTE_PROBABILITY) probability = MIN_SEARCH_BYTE_PROBABILITY;
								probabilityScale = probabilityScale * probability >> 24;
							}
							
							scanSpeed = scanSpeed * ((1<<24) - probabilityScale) >> 24;
						}
						
						DEBUG_SEARCH_PRINTF("SearchBytePair scan speed: %u, %z (p: %g)\n", scanSpeed, i, probability);
						if(scanSpeed > bestSearchByteScanSpeed)
						{
							bestSearchByteScanSpeed = scanSpeed;
							bestIndex = i | PAIR_TAG;
						}
					}
				}
				// Otherwise it's SearchByte
				else if(bestSearchByteScanSpeed == 0 || (bestIndex & ~PAIR_TAG) != i-1)
				{
					uint32_t baseScanSpeed = (searchDataList[i].isContinugous && searchDataList[i].count != 1) ?
												FIND_BYTE_RANGE_SCAN_SPEED :
												FIND_BYTE_SCAN_SPEED[searchDataList[i].count-1];
					float probability = searchDataList[i].score * (PROBABILITY_SCALER / 16777216.0f);
					if(probability < 1.0)
					{
						uint32_t scanSpeed = ScaleBaseSpeedForProbability(baseScanSpeed, probability);
						
						if(searchDataList.GetCount() > 1)
						{
							uint64_t probabilityScale = 1<<24;
							for(size_t j = 0; j < i; ++j)
							{
								uint64_t probability = searchDataList[i].score * PROBABILITY_SCALER;
								if(probability > MAX_SEARCH_BYTE_PROBABILITY) probability = MAX_SEARCH_BYTE_PROBABILITY;
								if(probability < MIN_SEARCH_BYTE_PROBABILITY) probability = MIN_SEARCH_BYTE_PROBABILITY;
								probabilityScale = probabilityScale * probability >> 24;
							}
							for(size_t j = i+1; j < searchDataList.GetCount(); ++j)
							{
								uint64_t probability = searchDataList[i].score * PROBABILITY_SCALER;
								if(probability > MAX_SEARCH_BYTE_PROBABILITY) probability = MAX_SEARCH_BYTE_PROBABILITY;
								if(probability < MIN_SEARCH_BYTE_PROBABILITY) probability = MIN_SEARCH_BYTE_PROBABILITY;
								probabilityScale = probabilityScale * probability >> 24;
							}
							
							scanSpeed = scanSpeed * ((1<<24) - probabilityScale) >> 24;
						}
						
						if(bestIndex & PAIR_TAG) scanSpeed = scanSpeed * 3 / 4;
						DEBUG_SEARCH_PRINTF("SearchByte scan speed: %u, %z (p: %g)\n", scanSpeed, i, probability);
						if(scanSpeed > bestSearchByteScanSpeed)
						{
							bestSearchByteScanSpeed = scanSpeed;
							bestIndex = i;
						}
					}
				}
			}

			DEBUG_SEARCH_PRINTF("SearchScanSpeed: %u, SearchByteScanSpeed: %u\n", bestScanSpeed, bestSearchByteScanSpeed);
			
			if(bestSearchByteScanSpeed > bestScanSpeed)
			{
				Instruction* searcher;
				Instruction* original = optimizer.original;

				if((bestIndex & TRIPLET_TAG) == TRIPLET_TAG)
				{
					uint32_t index = bestIndex & ~TRIPLET_TAG;
					uint8_t bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
					
					uint32_t tripletCount = Maximum(searchDataList[index].count, searchDataList[index+1].count, searchDataList[index+2].count);
						
					for(int b = 0; b < 3; ++b)
					{
						StaticBitTable<256> presenceBits = optimizer.instructionWalker.GetPresenceBits(index+b);
						for(int i = 0; i < tripletCount; ++i)
						{
							if(presenceBits.HasAllBitsClear())
							{
								bytes[b*tripletCount+i] = bytes[b*tripletCount+i-1];
							}
							else
							{
								bytes[b*tripletCount+i] = presenceBits.CountTrailingZeros();
								presenceBits.ClearBit(bytes[b*tripletCount+i]);
							}
						}
					}
					
					DEBUG_SEARCH_PRINTF("Using search byte triplet %u\n", index);
					Table<NibbleMask> nibbleMaskList = optimizer.instructionWalker.BuildNibbleMaskList(index, 3, false);
					searcher = new SearchByteInstruction(InstructionType(int(InstructionType::SearchByteTriplet) + (tripletCount-1)), bytes, index, nibbleMaskList);
				}
				else if(bestIndex & PAIR_TAG)
				{
					uint32_t index = bestIndex & ~PAIR_TAG;
					uint8_t bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
					
					Table<NibbleMask> nibbleMaskList = optimizer.instructionWalker.BuildNibbleMaskList(index, 2, false);
					if(searchDataList[index].isContinugous && searchDataList[index+1].isContinugous
					   && (searchDataList[index].count > 1 || searchDataList[index+1].count > 1))
					{
						Interval<size_t> range0 = optimizer.instructionWalker.GetPresenceBits(index).GetContiguousRange();
						Interval<size_t> range1 = optimizer.instructionWalker.GetPresenceBits(index+1).GetContiguousRange();
						bytes[0] = range0.min;
						bytes[1] = range0.max-1;
						bytes[2] = range1.min;
						bytes[3] = range1.max-1;
						
						DEBUG_SEARCH_PRINTF("Using search byte range pair %u\n", index);
						searcher = new SearchByteInstruction(InstructionType::SearchByteRangePair, bytes, index, nibbleMaskList);
					}
					else
					{
						uint32_t pairCount = Maximum(searchDataList[index].count, searchDataList[index+1].count);
						
						for(int b = 0; b < 2; ++b)
						{
							StaticBitTable<256> presenceBits = optimizer.instructionWalker.GetPresenceBits(index+b);
							for(int i = 0; i < pairCount; ++i)
							{
								if(presenceBits.HasAllBitsClear())
								{
									bytes[b*pairCount+i] = bytes[b*pairCount+i-1];
								}
								else
								{
									bytes[b*pairCount+i] = presenceBits.CountTrailingZeros();
									presenceBits.ClearBit(bytes[b*pairCount+i]);
								}
							}
						}
						
						DEBUG_SEARCH_PRINTF("Using search byte pair %u\n", index);
						searcher = new SearchByteInstruction(InstructionType(int(InstructionType::SearchBytePair) + (pairCount-1)), bytes, index, nibbleMaskList);
					}
				}
				else
				{
//						StandardOutput.PrintF("Using index: %u\n", bestIndex);
					// Let FindByte optimization take over instead
					if(bestIndex == 0 && searchDataList[bestIndex].count == 1 && original->type == InstructionType::ByteJumpTable) continue;
				
					if(searchDataList[bestIndex].isContinugous && searchDataList[bestIndex].count != 1)
					{
						unsigned char bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
						Interval<size_t> range0 = optimizer.instructionWalker.GetPresenceBits(bestIndex).GetContiguousRange();

						bytes[0] = range0.min;
						bytes[1] = range0.max-1;

						DEBUG_SEARCH_PRINTF("Using search byte range %u\n", bestIndex);
						searcher = new SearchByteInstruction(InstructionType::SearchByteRange, bytes, bestIndex);
					}
					else
					{
						StaticBitTable<256> presenceBits = optimizer.instructionWalker.GetPresenceBits(bestIndex);
						
						int numberOfBytes = 0;
						unsigned char bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
						while(!presenceBits.HasAllBitsClear())
						{
							unsigned char byte = presenceBits.CountTrailingZeros();
							bytes[numberOfBytes++] = byte;
							presenceBits.ClearBit(byte);
						}
						
						DEBUG_SEARCH_PRINTF("Using search byte %u\n", bestIndex);
						searcher = new SearchByteInstruction(numberOfBytes, bytes, bestIndex);
					}
				}

				// Only convert to advance byte if bestIndex == 0
				optimizer.ReplaceOriginal(searcher, (bestIndex & ~TRIPLET_TAG) == 0, bestIndex == 0 , partialMatchInstruction);
				continue;
			}
		}

		if(bestScanSpeed < SCAN_SPEED_THRESHOLD) continue;
		
		Instruction* searcher;
		if(useShiftOr)
		{
			uint32_t length = optimizer.instructionWalker.GetPresenceListCount();
			uint32_t maskValue = (1<<length)-1;
			for(int j = 0; j < 256; ++j) searchData[j] = maskValue;

			for(size_t i = 0; i < length; ++i)
			{
				int mask = ~(1<<i);
				const StaticBitTable<256>& presenceBits = optimizer.instructionWalker.GetPresenceBits(i);
				for(int j = 0; j < 256; ++j)
				{
					if(presenceBits[j]) searchData[j] &= mask;
				}
			}
			DEBUG_SEARCH_PRINTF("Using ShiftOr\n");
			Table<NibbleMask> nibbleMaskList = optimizer.instructionWalker.BuildNibbleMaskList(0, length <= 3 ? length : 3, length <= 3);
			searcher = new SearchDataInstruction(InstructionType::SearchShiftOr, length-1, searchData, nibbleMaskList);
		}
		else
		{
			memset(searchData, -1, sizeof(searchData));
			for(size_t i = 0; i <= bestSearchIndex; ++i)
			{
				const StaticBitTable<256>& presenceBits = optimizer.instructionWalker.GetPresenceBits(i);
				for(int j = 0; j < 256; ++j)
				{
					if(presenceBits[j]) searchData[j] = i;
				}
			}
			for(int j = 0; j < 256; ++j)
			{
				searchData[j] = bestSearchIndex - searchData[j];
			}
			
			DEBUG_SEARCH_PRINTF("Using BoyerMoore\n");
			searcher = new SearchDataInstruction(InstructionType::SearchBoyerMoore, bestSearchIndex, searchData);
		}
		
		optimizer.ReplaceOriginal(searcher, useShiftOr, false, partialMatchInstruction);
	}
}

//==========================================================================

bool InstructionList::NeedsProgressChecks() const
{
	return hasBackTrackingComponents || processorType == PatternProcessorType::BackTracking;
}

void InstructionList::Build(uint32_t aOptions, ScanDirection aScanDirection, IComponent* headComponent, Compiler& compiler, bool aHasBackTrackingComponents, PatternProcessorType aProcessorType)
{
	options = aOptions;
	scanDirection = aScanDirection;
	hasBackTrackingComponents = aHasBackTrackingComponents;
	processorType = aProcessorType;
	minimumLength = headComponent->GetMinimumLength();
	maximumLength = headComponent->GetMaximumLength();
	isFixedLength = headComponent->IsFixedLength();
	
	headComponent->BuildInstructions(*this);
	compiler.ResolveRecurseInstructions();
	if(instructionList.GetCount() == 0
	   || (instructionList.Back().type != InstructionType::Match
		   && instructionList.Back().type != InstructionType::Success))
	{
		AddInstruction(new MatchInstruction);
	}
	
	bool isAnchored = (options & Pattern::ANCHORED) != 0;
	hasStartAnchor = isAnchored || instructionList.Front().HasStartAnchor();
	hasEndAnchor = isAnchored || headComponent->HasEndAnchor();
	
	fullMatchInstruction = &instructionList.Front();
	partialMatchInstruction = (scanDirection == Forwards) ? fullMatchInstruction : nullptr;

	// This is required to detect AssertStartOfInput opportunities
	UpdateReferencesForInstructions();
	
	if(scanDirection == Forwards
	   && !isAnchored
	   && RequiresAnyByteMinimalForPartialMatch(headComponent))
	{
		Optimize_DelaySave(false);
		InsertPartialMatchAnyByteMinimal();
		
		if(options & Pattern::NO_FULL_MATCH) fullMatchInstruction = partialMatchInstruction;
		UpdateReferencesForInstructions();
	}
	
	Optimize();
}

void InstructionList::InsertPartialMatchAnyByteMinimal()
{
	StateMap map;
	InstructionTable targetList;
	map.RecurseAddTargets(targetList, nullptr, fullMatchInstruction);
	
	Instruction* splitTarget = fullMatchInstruction;
	Instruction** anyByteInsertionPoint = &partialMatchInstruction;
	if(targetList.ContainsInstruction(InstructionType::AssertStartOfInput))
	{
		// .*?(abc|^def)ghi
		// becomes:
		// ((abc|^def) | .*? abc)hgi
		
		SplitInstruction* initialSplit = new SplitInstruction;
		*anyByteInsertionPoint = initialSplit;
		initialSplit->targetList.Append(splitTarget);
		anyByteInsertionPoint = &initialSplit->targetList.Append();
		InsertBeforeInstruction(splitTarget, initialSplit);
		
		splitTarget = new SplitInstruction;
		for(Instruction* instruction : targetList)
		{
			if(instruction->type != InstructionType::AssertStartOfInput)
			{
				((SplitInstruction*) splitTarget)->targetList.Append(instruction);
			}
		}
		InsertAfterInstruction(initialSplit, splitTarget);
		
		targetList = ((SplitInstruction*) splitTarget)->targetList;
	}
	
	if (targetList.ContainsInstruction(InstructionType::AssertStartOfLine))
	{
		// (?m).*?(abc|^def)ghi
		// becomes:
		// ((abc|^def) | .*? abc|\ndef))hgi
		SplitInstruction* initialSplit = new SplitInstruction;
		*anyByteInsertionPoint = initialSplit;
		initialSplit->targetList.Append(splitTarget);
		anyByteInsertionPoint = &initialSplit->targetList.Append();
		InsertBeforeInstruction(splitTarget, initialSplit);
		
		SplitInstruction* innerSplit = new SplitInstruction;
		for(Instruction* instruction : targetList)
		{
			if(instruction->type != InstructionType::AssertStartOfLine)
			{
				innerSplit->targetList.Append(instruction);
				continue;
			}
			
			ByteInstruction* newLineInstruction = new ByteInstruction('\n');
			SplitInstruction* afterNewlineSplit = new SplitInstruction;
			
			for(Instruction* target : targetList)
			{
				if(target->type == InstructionType::AssertStartOfLine)
				{
					if(target == instruction)
					{
						afterNewlineSplit->targetList.Append(target->GetNext());
						break;
					}
				}
				else
				{
					afterNewlineSplit->targetList.Append(target);
				}
			}
			
			InsertBeforeInstruction(splitTarget, newLineInstruction);
			InsertBeforeInstruction(splitTarget, afterNewlineSplit);
			innerSplit->targetList.Append(newLineInstruction);
		}
		InsertAfterInstruction(initialSplit, innerSplit);
		splitTarget = innerSplit;
		targetList = innerSplit->targetList;
	}
	
	static const StaticBitTable<256> WORD_MASK = MakeWordMask();

	// First check special case: \b\w*, \b\w+, \b\W*, \b\W+
	if(targetList.GetCount() == 1 && targetList[0]->type == InstructionType::AssertWordBoundary)
	{
		InstructionTable testList;
		testList.Append(targetList[0]->GetNext());

		Tuple<Instruction*, bool> repeatedByteInstruction = testList.GetRepeatedByteInstruction();
		if(repeatedByteInstruction.Get<Instruction*>() != nullptr)
		{
			StaticBitTable<256> validBytes = repeatedByteInstruction.Get<Instruction*>()->GetValidBytes();
			Instruction* afterRepeat = GetByteFilterStartingFrom(repeatedByteInstruction.Get<Instruction*>()->GetNext()->GetNext());
			
			// Transform \b\w+x -> (?:\b\w+|search-\W.*?(propagate-back-\w))x
			if((validBytes == WORD_MASK || validBytes == ~WORD_MASK)
				&& (repeatedByteInstruction.Get<bool>() || (afterRepeat && (afterRepeat->GetValidBytes() & ~validBytes).HasAllBitsClear())))
			{
				Instruction*				validByteMaskInstruction		= new ByteBitMaskInstruction(validBytes);
				SplitInstruction*			repeatValidSplitInstruction		= new SplitInstruction;
				Instruction*				notValidByteMaskInstruction		= new ByteBitMaskInstruction(~validBytes);
				SplitInstruction*			repeatInvalidSplitInstruction	= new SplitInstruction;
				SplitInstruction*			initialSplitInstruction			= new SplitInstruction;
				
				*anyByteInsertionPoint = initialSplitInstruction;
				
				initialSplitInstruction->targetList.Append(targetList[0]);
				initialSplitInstruction->targetList.Append(repeatValidSplitInstruction);
				
				repeatValidSplitInstruction->targetList.Append(validByteMaskInstruction);
				repeatValidSplitInstruction->targetList.Append(notValidByteMaskInstruction);
				
				repeatInvalidSplitInstruction->targetList.Append(testList[0]);
				repeatInvalidSplitInstruction->targetList.Append(notValidByteMaskInstruction);
				repeatInvalidSplitInstruction->targetList.Append(validByteMaskInstruction);
				
				InsertBeforeInstruction(splitTarget, initialSplitInstruction);
				InsertBeforeInstruction(splitTarget, validByteMaskInstruction);
				InsertBeforeInstruction(splitTarget, repeatValidSplitInstruction);
				InsertBeforeInstruction(splitTarget, notValidByteMaskInstruction);
				InsertBeforeInstruction(splitTarget, repeatInvalidSplitInstruction);
				
				Instruction* afterRepeatInstruction = repeatedByteInstruction.Get<Instruction*>()->GetNext()->GetNext();
				Instruction* splitBreakOutInstruction = afterRepeatInstruction;
				if(repeatedByteInstruction.Get<bool>())
				{
					// At least one!. Lets leverage that:
					Instruction* singleInstance = repeatedByteInstruction.Get<Instruction*>()->Clone();
					InsertBeforeInstruction(splitTarget, singleInstance);
					InsertBeforeInstruction(splitTarget, new JumpInstruction(afterRepeatInstruction));
					splitBreakOutInstruction = singleInstance;
				}
				
				AddToSearchOptimizer(repeatInvalidSplitInstruction, splitBreakOutInstruction, AnyByteResult::Minimal, repeatedByteInstruction.Get<bool>(), validBytes);
				return;
			}
		}
	}
	
	if(targetList.ContainsInstruction(InstructionType::AssertWordBoundary))
	{
		// .*?(a|\b|c)\w ->
		// (a|\b|c|.*?(a|\W|c))\w
		
		// Only do this if the byte filters after an assert word boundary make it advantageous
		bool isAdvantageous = false;
		for(Instruction* instruction : targetList)
		{
			if(instruction->type != InstructionType::AssertWordBoundary) continue;
			
			Instruction* byteFilter = GetByteFilterStartingFrom(instruction);
			if(!byteFilter) continue;
			
			StaticBitTable<256> targetBitMask = byteFilter->GetValidBytes();
			if((targetBitMask & WORD_MASK).HasAllBitsClear() || (targetBitMask & ~WORD_MASK).HasAllBitsClear())
			{
				isAdvantageous = true;
				break;
			}
		}
		
		if(isAdvantageous)
		{
			SplitInstruction* initialSplit = new SplitInstruction;
			*anyByteInsertionPoint = initialSplit;
			initialSplit->targetList.Append(splitTarget);
			anyByteInsertionPoint = &initialSplit->targetList.Append();
			InsertBeforeInstruction(splitTarget, initialSplit);
			
			SplitInstruction* innerSplit = new SplitInstruction;
			for(Instruction* instruction : targetList)
			{
				if(instruction->type != InstructionType::AssertWordBoundary)
				{
					innerSplit->targetList.Append(instruction);
					continue;
				}
				
				Instruction* byteFilter = GetByteFilterStartingFrom(instruction);
				if(!byteFilter)
				{
					innerSplit->targetList.Append(instruction);
					continue;
				}
				
				StaticBitTable<256> targetBitMask = byteFilter->GetValidBytes();
				if((targetBitMask & WORD_MASK).HasAllBitsClear())
				{
					ByteBitMaskInstruction* wordCharacterInstruction = new ByteBitMaskInstruction;
					wordCharacterInstruction->bitMask = WORD_MASK;
					JumpInstruction* jumpInstruction = new JumpInstruction;
					jumpInstruction->target = instruction->GetNext();
					
					InsertBeforeInstruction(splitTarget, wordCharacterInstruction);
					InsertBeforeInstruction(splitTarget, jumpInstruction);
					innerSplit->targetList.Append(wordCharacterInstruction);
					
				}
				else if((targetBitMask & ~WORD_MASK).HasAllBitsClear())
				{
					ByteBitMaskInstruction* nonWordCharacterInstruction = new ByteBitMaskInstruction;
					nonWordCharacterInstruction->bitMask = ~WORD_MASK;
					JumpInstruction* jumpInstruction = new JumpInstruction;
					jumpInstruction->target = instruction->GetNext();
					
					InsertBeforeInstruction(splitTarget, nonWordCharacterInstruction);
					InsertBeforeInstruction(splitTarget, jumpInstruction);
					innerSplit->targetList.Append(nonWordCharacterInstruction);
				}
				else
				{
					innerSplit->targetList.Append(instruction);
				}
			}
			InsertAfterInstruction(initialSplit, innerSplit);
			splitTarget = innerSplit;
			targetList = innerSplit->targetList;
		}
	}
	
	// Detect if the pattern starts with a single byte repeat, eg. \w+
	// Transform .*?\w+abc -> \w+abc|.*?(propagate-back-\w)\wabc
	// Transform .*?\w*abc -> \w*abc|.*?(propagate-back-\w)abc
	Tuple<Instruction*, bool> repeatedByteInstruction = targetList.GetRepeatedByteInstruction();
	if(repeatedByteInstruction.Get<Instruction*>() != nullptr)
	{
		const StaticBitTable<256> validBytes = repeatedByteInstruction.Get<Instruction*>()->GetValidBytes();
		
		if(validBytes.HasAllBitsSet())
		{
			*anyByteInsertionPoint = splitTarget;
			return;
		}
		
		Instruction*				validByteMaskInstruction		= new ByteBitMaskInstruction(validBytes);
		SplitInstruction*			repeatValidSplitInstruction		= new SplitInstruction;
		Instruction*				notValidByteMaskInstruction		= new ByteBitMaskInstruction(~validBytes);
		SplitInstruction*			repeatInvalidSplitInstruction	= new SplitInstruction;
		
		*anyByteInsertionPoint = repeatInvalidSplitInstruction;
		
		repeatValidSplitInstruction->targetList.Append(validByteMaskInstruction);
		repeatValidSplitInstruction->targetList.Append(notValidByteMaskInstruction);
		
		repeatInvalidSplitInstruction->targetList.Append(splitTarget);
		repeatInvalidSplitInstruction->targetList.Append(notValidByteMaskInstruction);
		repeatInvalidSplitInstruction->targetList.Append(validByteMaskInstruction);
		
		InsertBeforeInstruction(splitTarget, validByteMaskInstruction);
		InsertBeforeInstruction(splitTarget, repeatValidSplitInstruction);
		InsertBeforeInstruction(splitTarget, notValidByteMaskInstruction);
		InsertBeforeInstruction(splitTarget, repeatInvalidSplitInstruction);
		
		Instruction* afterRepeatInstruction = repeatedByteInstruction.Get<Instruction*>()->GetNext()->GetNext();
		Instruction* splitBreakOutInstruction = afterRepeatInstruction;
		if(repeatedByteInstruction.Get<bool>())
		{
			// At least one!. Lets leverage that:
			Instruction* singleInstance = repeatedByteInstruction.Get<Instruction*>()->Clone();
			InsertBeforeInstruction(splitTarget, singleInstance);
			InsertBeforeInstruction(splitTarget, new JumpInstruction(afterRepeatInstruction));
			splitBreakOutInstruction = singleInstance;
		}
		
		AddToSearchOptimizer(repeatInvalidSplitInstruction, splitBreakOutInstruction, AnyByteResult::Minimal, repeatedByteInstruction.Get<bool>(), validBytes);
		return;
	}
	
	// This inserts an any-byte-minimal instruction
	//
	// 0: jump 2
	// 1: any-byte
	// 2: split 3, 1
	//
	
	SplitInstruction* split = new SplitInstruction;
	JumpInstruction* jump = new JumpInstruction(split);
	if(anyByteInsertionPoint) *anyByteInsertionPoint = jump;
	
	Instruction* anyByteInstruction = new AnyByteInstruction;
	
	split->targetList.Append(splitTarget);
	split->targetList.Append(anyByteInstruction);
	
	InsertBeforeInstruction(splitTarget, jump);
	InsertBeforeInstruction(splitTarget, anyByteInstruction);
	InsertBeforeInstruction(splitTarget, split);
}

void InstructionList::WriteByteCode(DataBlockWriter& writer, const String& pattern, uint32_t numberOfCaptures)
{
	IndexInstructions();
	
	ByteCodeBuilder builder(pattern,
							options,
							totalNumberOfInstructions,
							partialMatchInstruction->index,
							fullMatchInstruction->index,
							numberOfCaptures,
							minimumLength,
							maximumLength,
							processorType,
							hasStartAnchor,
							matchRequiresEndOfInput,
							isFixedLength,
							scanDirection == Forwards,
							hasResetCapture);

	for(Instruction& instruction : instructionList)
	{
		instruction.BuildByteCode(builder);
	}
	builder.WriteByteCode(writer);
}

void InstructionList::AppendByteCode(DataBlockWriter& writer)
{
	IndexInstructions();
	ByteCodeBuilder builder(String::EMPTY_STRING,
							options,
							totalNumberOfInstructions,
							0,
							fullMatchInstruction->index,
							0,
							minimumLength,
							maximumLength,
							processorType,
							hasStartAnchor,
							matchRequiresEndOfInput,
							isFixedLength,
							scanDirection == Forwards,
							hasResetCapture);
	
	for(Instruction& instruction : instructionList)
	{
		instruction.BuildByteCode(builder);
	}
	
	builder.AppendByteCode(writer);
}

//==========================================================================
