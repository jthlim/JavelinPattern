//==========================================================================

#include "Javelin/Pattern/Internal/PatternInstructionWalker.h"
#include "Javelin/Pattern/Internal/PatternNibbleMask.h"
#include "Javelin/Stream/StandardWriter.h"

//==========================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//==========================================================================

void InstructionWalker::Presence::Update(const StaticBitTable<256>& presenceBits, uint32_t pathMask)
{
	mask |= pathMask;
	bitField |= presenceBits;
	for(int i = 0; i < 256; ++i)
	{
		if(presenceBits[i]) masks[i] |= pathMask;
	}
}

//==========================================================================

InstructionWalker::InstructionWalker()
{
	ClearMemory(parentMasks);
	ClearMemory(childMasks);
	pathIndex = 0;
}

InstructionWalker::~InstructionWalker()
{
}

bool InstructionWalker::ShouldContinue() const
{
	return activeInstructions.HasData() && !HasMatch();
}

void InstructionWalker::AddInitialInstruction(const Instruction* instruction)
{
	if(!activeInstructions.Contains(instruction))
	{
		activeInstructions.Insert(instruction, 1<<pathIndex);
		++pathIndex;
	}
}

void InstructionWalker::Dump(ICharacterWriter& output) const
{
	for(const auto& ip : activeInstructions)
	{
		output.PrintF("%u:%u ", ip.key->index, ip.value);
		ip.key->Dump(output);
		output.PrintF(" ");
	}
	output.PrintF("\n");
}

bool InstructionWalker::HasMatch() const
{
	for(const auto& ip : activeInstructions)
	{
		if(ip.key->CanLeadToMatch()) return true;
	}
	return false;
}

void InstructionWalker::AdvanceAllInstructions()
{
	OpenHashSet<const Instruction*> progressCheckSet;
	Map<const Instruction*, uint32_t> nextState;
	Presence& presence = presenceList.Append();
	for(const auto& ip : activeInstructions)
	{
		ProcessInstruction(progressCheckSet, nextState, presence, ip.key, ip.value);
	}
	activeInstructions = Move(nextState);
}

// Given a path mask, eg. 000110, determine a new path mask, eg. 011000
// If we overflow a 32 bit mask, all instructions go into channel 31
uint32_t InstructionWalker::GetNewPathMask(uint32_t mask)
{
	uint32_t newPathIndex;
	if(pathIndex < 32) newPathIndex = pathIndex++;
	else newPathIndex = 31;

	parentMasks[newPathIndex] |= mask;
	return 1<<newPathIndex;
}

void InstructionWalker::UpdateChildMasks()
{
	if(childMasksUpdated) return;
	childMasksUpdated = true;
	
//	for(int i = 0; i < pathIndex; ++i)
//	{
//		printf("Parent: %i: %x\n", i, parentMasks[i]);
//	}
	for(int32_t i = pathIndex-1; i >= 0; --i)
	{
		uint32_t mask = parentMasks[i];
		childMasks[i] |= (1<<i);
		while(mask)
		{
			uint32_t nextIterationMask = mask & (mask-1);		// Pull off the lowest bit
			uint32_t singleBit = mask & ~nextIterationMask;
			
			uint32_t parentIndex = BitUtility::DetermineHighestNonZeroBit32(singleBit);
			childMasks[parentIndex] |= (1<<i);

			mask = nextIterationMask;
		}
	}
//	for(int i = 0; i < pathIndex; ++i)
//	{
//		printf("Child: %i: %x\n", i, childMasks[i]);
//	}
}

void InstructionWalker::UpdatePresenceMasks(size_t i)
{
	UpdateChildMasks();

	Presence& presence = presenceList[i];
	for(int c = 0; c < 256; ++c)
	{
		presence.masks[c] = GetFullMask(presence.masks[c], presence.mask);
	}
	presence.mask = GetFullMask(presence.mask, 0);
}

uint32_t InstructionWalker::GetFullMask(uint32_t mask, uint32_t stateMask) const
{
	uint32_t result = mask;
	while(mask)
	{
		uint32_t nextIterationMask = mask & (mask-1);		// Pull off the lowest bit
		uint32_t singleBit = mask & ~nextIterationMask;
		
		uint32_t index = BitUtility::DetermineHighestNonZeroBit32(singleBit);
		result |= childMasks[index] & ~stateMask;
		
		mask = nextIterationMask;
	}
	return result;
}

void InstructionWalker::ProcessInstruction(OpenHashSet<const Instruction*> &progressCheckSet, Map<const Instruction*, uint32_t>& nextState, Presence& presence, const Instruction* instruction, uint32_t pathMask)
{
	switch(instruction->type)
	{
	case InstructionType::Jump:
		ProcessInstruction(progressCheckSet, nextState, presence, static_cast<const JumpInstruction*>(instruction)->target, pathMask);
		break;
			
	case InstructionType::Call:
		{
			const CallInstruction* callInstruction = static_cast<const CallInstruction*>(instruction);
			if(callInstruction->falseTarget) ProcessInstruction(progressCheckSet, nextState, presence, callInstruction->falseTarget, pathMask);
			if(callInstruction->trueTarget) ProcessInstruction(progressCheckSet, nextState, presence, callInstruction->trueTarget, pathMask);
		}
		break;

	case InstructionType::Possess:
		{
			const PossessInstruction* possessInstruction = static_cast<const PossessInstruction*>(instruction);
			ProcessInstruction(progressCheckSet, nextState, presence, possessInstruction->callTarget, pathMask);
		}
		break;

	case InstructionType::AssertStartOfInput:
	case InstructionType::AssertEndOfInput:
	case InstructionType::AssertStartOfLine:
	case InstructionType::AssertEndOfLine:
	case InstructionType::AssertWordBoundary:
	case InstructionType::AssertNotWordBoundary:
	case InstructionType::AssertRecurseValue:
	case InstructionType::AssertStartOfSearch:
	case InstructionType::ReturnIfRecurseValue:
	case InstructionType::Save:
		ProcessInstruction(progressCheckSet, nextState, presence, instruction->GetNext(), pathMask);
		break;

	case InstructionType::ProgressCheck:
		if(!progressCheckSet.Contains(instruction))
		{
			progressCheckSet.Put(instruction);
			ProcessInstruction(progressCheckSet, nextState, presence, instruction->GetNext(), pathMask);
		}
		break;
			
	case InstructionType::Fail:
		// This removes FAIL paths from the instruction walk.
		for(Presence& p : presenceList)
		{
			p.mask &= ~pathMask;
			for(int i = 0; i < 256; ++i)
			{
				p.masks[i] &= ~pathMask;
				if(p.masks[i] == 0) p.bitField.ClearBit(i);
			}
		}
		break;
			
	case InstructionType::Split:
		{
			const SplitInstruction* split = static_cast<const SplitInstruction*>(instruction);
			if(split->targetList.IsEmpty()) break;
			ProcessInstruction(progressCheckSet, nextState, presence, split->targetList[0], pathMask);
			
			for(size_t i = 1; i < split->targetList.GetCount(); ++i)
			{
				ProcessInstruction(progressCheckSet, nextState, presence, split->targetList[i], GetNewPathMask(pathMask));
			}
		}
		break;
			
	default:
		{
			JASSERT(instruction->IsByteConsumer());
			uint32_t numberOfPlanes = instruction->GetNumberOfPlanes();
			for(uint32_t p = 0; p < numberOfPlanes; ++p)
			{
				presence.Update(instruction->GetValidBytesForPlane(p), pathMask);
				
				uint32_t newPathMask;
				if(p == 0) newPathMask = pathMask;
				else newPathMask = GetNewPathMask(pathMask);
				
				const Instruction* target = instruction->GetTargetForPlane(p);
				JASSERT(target);
				nextState[target] |= newPathMask;
			}
		}
		break;
			
	case InstructionType::BackReference:
	case InstructionType::Recurse:
		JERROR("Unexpected instruction");
		break;

	}
}

//==========================================================================

Table<NibbleMask> InstructionWalker::BuildNibbleMaskList(size_t index, size_t length, bool allowSinglePath)
{
	Table<NibbleMask> result;
	if(!allowSinglePath && pathIndex <= 1) return result;

	for(int b = 0; b < length; ++b)
	{
		UpdatePresenceMasks(index+b);
	}

//	for(int b = 0; b < length; ++b)
//	{
//		const Presence& presence = GetPresence(index+b);
//		StandardOutput.PrintF("Presence %d - mask: %x", b, presence.mask);
//		for(int i = 0; i < 256; ++i)
//		{
//			if(i % 16 == 0) StandardOutput.PrintF("\n\t");
//			StandardOutput.PrintF("0x%x, ", presence.masks[i]);
//		}
//		StandardOutput.PrintF("\n\n");
//	}
//	
	Table<uint32_t> pathMaskList = GetPathMaskList(index, length);
	if(!allowSinglePath && pathMaskList.GetCount() <= 1) return result;

//	StandardOutput.PrintF("PathMaskList\n");
//	for(size_t i = 0; i < pathMaskList.GetCount(); ++i)
//	{
//		StandardOutput.PrintF("%z: %u\n", i, pathMaskList[i]);
//	}
//	
	BuildNibbleMaskListFromPathMaskList(result, pathMaskList, index, length);
	
//	if(result.GetCount())
//	{
//		StandardOutput.PrintF("NibbleMasks\n");
//		for(int i = 0; i < result.GetCount(); ++i)
//		{
//			StandardOutput.PrintF("%z lw: %A\n", i, &String::CreateHexStringFromBytes(result[i].GetNibbleMask(), 16));
//			StandardOutput.PrintF("%z hi: %A\n", i, &String::CreateHexStringFromBytes((const char*) result[i].GetNibbleMask() + 16, 16));
//		}
//	}
	if(!allowSinglePath && result.GetCount() > 0 && result[0].GetNumberOfBitsUsed() <= 1)
	{
		result.Reset();
	}
	
//	if(pathMaskList.GetCount() >= 8)
//	{
//		StandardOutput.PrintF("Mask for bytes\n");
//		for(int c = 0; c < 256; ++c)
//		{
//			if((c & 15) == 0) StandardOutput.PrintF("\n\t");
//			StandardOutput.PrintF("%02x, ", result[0].GetMaskForByte(c));
//		}
//		StandardOutput.PrintF("\n");
//	}
	
	return result;
}

Table<uint32_t> InstructionWalker::GetPathMaskList(size_t index, size_t length) const
{
	Table<uint32_t> result;
	
	Table<OpenHashSet<uint32_t>> presenceMaskList;
	presenceMaskList.SetCount(length);
	for(int b = 0; b < length; ++b)
	{
		const Presence& presence = GetPresence(index+b);
		for(int i = 0; i < 256; ++i)
		{
			if(presence.masks[i] != 0)
			{
				presenceMaskList[b].Put(presence.masks[i]);
			}
		}
	}
	
	uint32_t maskTest = uint32_t((1ull << pathIndex) - 1);
	for(uint32_t i = 0; i < pathIndex; ++i)
	{
		uint32_t mask = 1 << i;
		if((mask & maskTest) == 0) continue;

		bool found = false;
		uint32_t clearMaskTest = (1u << 31) - 1;
		for(const OpenHashSet<uint32_t>& presenceMasks : presenceMaskList)
		{
			for(uint32_t presenceMask : presenceMasks)
			{
				if(mask & presenceMask)
				{
					found = true;
					clearMaskTest &= presenceMask;
				}
			}
		}
		
		if(found)
		{
			result.Append(mask);
			maskTest &= ~clearMaskTest;
		}
	}
	
	return result;
}

void InstructionWalker::BuildNibbleMaskListFromPathMaskList(Table<NibbleMask>& result, const Table<uint32_t>& pathMaskList, size_t index, size_t length) const
{
	result.SetCount(length);
	
	for(uint32_t mask : pathMaskList)
	{
		JASSERT(length <= 4);
		NibbleMask nibbleMaskList[4];
		bool requiresRecursiveAdd = false;

		for(int b = 0; b < length; ++b)
		{
			const Presence& presence = GetPresence(index+b);
			NibbleMask* nibbleMask = new(Placement(nibbleMaskList+b)) NibbleMask(presence.masks, mask);
			
//			StandardOutput.PrintF("NibbleMask for %u, %d: %A\n", mask, b, &String::CreateHexStringFromBytes(nibbleMask.GetNibbleMask(), 32));

			if(nibbleMask->GetNumberOfBitsUsed() == 0) goto SkipMask;
			if(nibbleMask->GetNumberOfBitsUsed() > 1) requiresRecursiveAdd = true;
		}
	
		if(requiresRecursiveAdd)
		{
			if(!AddToNibbleMaskPathList(result, nibbleMaskList))
			{
				result.Reset();
				return;
			}
		}
		else
		{
			if(!AddSingleNibbleMaskListToResult(result, nibbleMaskList))
			{
				result.Reset();
				return;
			}
		}
		
	SkipMask:
		;
	}
}

//==========================================================================

bool InstructionWalker::AddToNibbleMaskPathList(Table<NibbleMask>& result, const NibbleMask* newNibbleMaskPath)
{
	JASSERT(result.GetCount() <= 4);
	int buffer[4];
	return AddToNibbleMaskPathList(result, newNibbleMaskPath, buffer, 0);
}

bool InstructionWalker::AddToNibbleMaskPathList(Table<NibbleMask>& result, const NibbleMask* newNibbleMaskPath, int* newNibbleBitIndexList, int index)
{
	if(index < result.GetCount())
	{
		int numberOfBitsUsed = newNibbleMaskPath[index].GetNumberOfBitsUsed();
		for(int i = 0; i < numberOfBitsUsed; ++i)
		{
			newNibbleBitIndexList[index] = i;
			if(!AddToNibbleMaskPathList(result, newNibbleMaskPath, newNibbleBitIndexList, index+1)) return false;
		}
		return true;
	}
	
	JASSERT(result.GetCount() <= 4);
	NibbleMask singleNibbleMask[4];
	for(int i = 0; i < result.GetCount(); ++i)
	{
		JASSERT(result[i].GetNumberOfBitsUsed() == result[0].GetNumberOfBitsUsed());
		singleNibbleMask[i] = newNibbleMaskPath[i] >> newNibbleBitIndexList[i];
	}

	return AddSingleNibbleMaskListToResult(result, singleNibbleMask);
}

bool InstructionWalker::AddSingleNibbleMaskListToResult(Table<NibbleMask>& result, const NibbleMask* singleNibbleMaskList)
{
	if(result[0].GetNumberOfBitsUsed() < 8)
	{
		// Easy case - empty available buckets!
		for(int i = 0; i < result.GetCount(); ++i)
		{
			result[i].Merge(singleNibbleMaskList[i], 1);
		}
		return true;
	}
	
	// More difficult case. We need to figure out the best masks to merge.
	uint32_t minimumCost = TypeData<uint32_t>::Maximum();
	int bestI = -1;
	int bestJ = -1;
	
	for(int i = 0; i < 8; ++i)
	{
		for(int j = i+1; j < 9; ++j)
		{
			const NibbleMask* secondData = j == 8 ? singleNibbleMaskList : result.GetData();
			uint32_t cost = CalculateMergeCost(result.GetData(), i, secondData, j&7, result.GetCount());
			if(cost < minimumCost)
			{
				minimumCost = cost;
				bestI = i;
				bestJ = j;
			}
		}
	}
	
	// TODO: Return false in some cases?
	
	// Now update the necessary channels.
//	StandardOutput.PrintF("Merge path %d & %d\n", bestI, bestJ);
	if(bestJ != 8)
	{
		for(int i = 0; i < result.GetCount(); ++i)
		{
			result[i].MergeInternalChannel(bestJ, bestI);
		}
	}
	for(int i = 0; i < result.GetCount(); ++i)
	{
		result[i].MergeToChannel(singleNibbleMaskList[i], bestI);
	}
	
	return true;
}

uint32_t InstructionWalker::CalculateMergeCost(const NibbleMask* first, int firstBit, const NibbleMask* second, int secondBit, int length)
{
	uint32_t cost = 1;
	for(int i = 0; i < length; ++i)
	{
		cost += first[i].CalculateMergeCost(firstBit, second[i], secondBit);
	}
	return cost;
}

//==========================================================================
