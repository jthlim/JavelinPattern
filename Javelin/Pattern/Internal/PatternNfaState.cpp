//============================================================================

#include "Javelin/Pattern/Internal/PatternNfaState.h"
#include "Javelin/Pattern/Internal/PatternProcessor.h"
#include "Javelin/Container/EnumSet.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

bool NfaState::AddEntry(uint32_t pc, UpdateCache &updateCache)
{
	if((stateFlags & Flag::IS_MATCH) == Flag::IS_MATCH) return false;
	
	uint32_t possibleIndex = updateCache.possibleIndex[pc];
	if(possibleIndex >= numberOfStates || stateList[possibleIndex] != pc)
	{
		updateCache.possibleIndex[pc] = numberOfStates;
		stateList[numberOfStates++] = pc;
		return true;
	}
	return false;
}

bool NfaState::AddEntryNoMatchCheck(uint32_t pc, UpdateCache &updateCache)
{
	uint32_t possibleIndex = updateCache.possibleIndex[pc];
	if(possibleIndex >= numberOfStates || stateList[possibleIndex] != pc)
	{
		updateCache.possibleIndex[pc] = numberOfStates;
		stateList[numberOfStates++] = pc;
		return true;
	}
	return false;
}

void NfaState::AddEntryNoCheck(uint32_t pc)
{
	if(stateFlags & Flag::IS_MATCH) return;
	stateList[numberOfStates++] = pc;
}

bool NfaState::IsSearch() const
{
	if((stateFlags & Flag::IS_SEARCH) == 0) return false;
	return numberOfStates == 1;
}

//============================================================================

void NfaState::ProcessNextStateFlags(OpenHashSet<uint32_t>& progressCheckSet, const PatternData& patternData, uint32_t pc, CharacterRange& range, UpdateCache &updateCache, int nextFlags)
{
Loop:
	const ByteCodeInstruction instruction = patternData[pc];
	switch(instruction.type)
	{
	case InstructionType::AssertStartOfInput:
		nextFlags |= Flag::IS_START_OF_INPUT_MASK;
		++pc;
		goto Loop;
		
	case InstructionType::AssertStartOfLine:
		nextFlags |= Flag::WAS_END_OF_LINE_MASK;
		{
			const CharacterRange NEW_LINE_RANGE{'\n', '\n'};
			NEW_LINE_RANGE.TrimRelevancyInterval(range);
		}
		++pc;
		goto Loop;
		
	case InstructionType::AssertEndOfInput:
		++pc;
		goto Loop;
		
	case InstructionType::AssertEndOfLine:
		++pc;
		goto Loop;
		
	case InstructionType::AssertWordBoundary:
	case InstructionType::AssertNotWordBoundary:
		nextFlags |= Flag::WAS_WORD_CHARACTER_MASK;
		CharacterRangeList::WORD_CHARACTERS.TrimRelevancyInterval(range);
		++pc;
		goto Loop;

	case InstructionType::AssertStartOfSearch:
		nextFlags |= Flag::IS_START_OF_SEARCH_MASK;
		++pc;
		goto Loop;

	case InstructionType::DispatchRange:
		{
			const ByteCodeJumpRangeData* jumpRange = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			for(int i = 0; i < 2; ++i)
			{
				if(jumpRange->pcData[i] != TypeData<uint32_t>::Maximum())
				{
					ProcessNextStateFlags(progressCheckSet, patternData, jumpRange->pcData[i], range, updateCache, nextFlags);
				}
			}
		}
		break;
		
	case InstructionType::DispatchMask:
		{
			const ByteCodeJumpMaskData* jumpMask = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
			for(int i = 0; i < 2; ++i)
			{
				if(jumpMask->pcData[i] != TypeData<uint32_t>::Maximum())
				{
					ProcessNextStateFlags(progressCheckSet, patternData, jumpMask->pcData[i], range, updateCache, nextFlags);
				}
			}
		}
		break;
		
	case InstructionType::DispatchTable:
		{
			const ByteCodeJumpTableData* jumpTable = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
			for(int i = 0; i < jumpTable->numberOfTargets; ++i)
			{
				if(jumpTable->pcData[i] != TypeData<uint32_t>::Maximum())
				{
					ProcessNextStateFlags(progressCheckSet, patternData, jumpTable->pcData[i], range, updateCache, nextFlags);
				}
			}
		}
		break;
		
	case InstructionType::Jump:
		pc = instruction.data;
		goto Loop;
		
	case InstructionType::ProgressCheck:
		if(!progressCheckSet.Contains(pc))
		{
			progressCheckSet.Put(pc);
			++pc;
			goto Loop;
		}
		break;

	case InstructionType::PropagateBackwards:
	case InstructionType::Save:
	case InstructionType::SaveNoRecurse:
		++pc;
		goto Loop;
		
	case InstructionType::Split:
		{
			const ByteCodeSplitData* data = patternData.GetData<ByteCodeSplitData>(instruction.data);
			for(uint32_t i = 0; i < data->numberOfTargets-1; ++i)
			{
				ProcessNextStateFlags(progressCheckSet, patternData, data->targetList[i], range, updateCache, nextFlags);
			}
			pc = data->targetList[data->numberOfTargets-1];
		}
		goto Loop;

	case InstructionType::SplitMatch:
		{
			const uint32_t* data = patternData.GetData<uint32_t>(instruction.data);
			ProcessNextStateFlags(progressCheckSet, patternData, data[0], range, updateCache, nextFlags);
			pc = data[1];
		}
		goto Loop;

	case InstructionType::SplitNextN:
	case InstructionType::SplitNextMatchN:
		ProcessNextStateFlags(progressCheckSet, patternData, pc+1, range, updateCache, nextFlags);
		pc = instruction.data;
		goto Loop;
			
	case InstructionType::SplitNNext:
	case InstructionType::SplitNMatchNext:
		ProcessNextStateFlags(progressCheckSet, patternData, instruction.data, range, updateCache, nextFlags);
		++pc;
		goto Loop;
		
	case InstructionType::AdvanceByte:
	case InstructionType::AnyByte:
	case InstructionType::Byte:
	case InstructionType::ByteEitherOf2:
	case InstructionType::ByteEitherOf3:
	case InstructionType::ByteRange:
	case InstructionType::ByteBitMask:
	case InstructionType::ByteJumpMask:
	case InstructionType::ByteJumpRange:
	case InstructionType::ByteJumpTable:
	case InstructionType::ByteNot:
	case InstructionType::ByteNotEitherOf2:
	case InstructionType::ByteNotEitherOf3:
	case InstructionType::ByteNotRange:
	case InstructionType::FindByte:
	case InstructionType::Match:
		stateFlags |= nextFlags;
		break;

	case InstructionType::SearchByte:
	case InstructionType::SearchByteEitherOf2:
	case InstructionType::SearchByteEitherOf3:
	case InstructionType::SearchByteEitherOf4:
	case InstructionType::SearchByteEitherOf5:
	case InstructionType::SearchByteEitherOf6:
	case InstructionType::SearchByteEitherOf7:
	case InstructionType::SearchByteEitherOf8:
	case InstructionType::SearchBytePair:
	case InstructionType::SearchBytePair2:
	case InstructionType::SearchBytePair3:
	case InstructionType::SearchBytePair4:
	case InstructionType::SearchByteTriplet:
	case InstructionType::SearchByteTriplet2:
	case InstructionType::SearchByteRange:
	case InstructionType::SearchByteRangePair:
	case InstructionType::SearchBoyerMoore:
	case InstructionType::SearchShiftOr:
		stateFlags |= nextFlags;
		++pc;
		goto Loop;
			
	case InstructionType::Fail:
		break;

	case InstructionType::AssertRecurseValue:
	case InstructionType::BackReference:
	case InstructionType::Call:
	case InstructionType::Possess:
	case InstructionType::Recurse:
	case InstructionType::ReturnIfRecurseValue:
	case InstructionType::StepBack:
	case InstructionType::Success:
		JERROR("Unexpected instruction!");
	}
}

void NfaState::AddNextState(const PatternData& patternData, uint32_t pc, CharacterRange& range, UpdateCache &updateCache, int nextFlags)
{
Loop:
	const ByteCodeInstruction instruction = patternData[pc];
	switch(instruction.type)
	{
	case InstructionType::Jump:
		pc = instruction.data;
		goto Loop;
		
	case InstructionType::ProgressCheck:
		if(AddEntryNoMatchCheck(pc, updateCache))
		{
			++pc;
			goto Loop;
		}
		else
		{
			return;
		}
			
	case InstructionType::PropagateBackwards:
	case InstructionType::Save:
	case InstructionType::SaveNoRecurse:
		++pc;
		goto Loop;
		
	case InstructionType::FindByte:
	case InstructionType::SearchByte:
	case InstructionType::SearchByteEitherOf2:
	case InstructionType::SearchByteEitherOf3:
	case InstructionType::SearchByteEitherOf4:
	case InstructionType::SearchByteEitherOf5:
	case InstructionType::SearchByteEitherOf6:
	case InstructionType::SearchByteEitherOf7:
	case InstructionType::SearchByteEitherOf8:
	case InstructionType::SearchBytePair:
	case InstructionType::SearchBytePair2:
	case InstructionType::SearchBytePair3:
	case InstructionType::SearchBytePair4:
	case InstructionType::SearchByteTriplet:
	case InstructionType::SearchByteTriplet2:
	case InstructionType::SearchByteRange:
	case InstructionType::SearchByteRangePair:
	case InstructionType::SearchBoyerMoore:
	case InstructionType::SearchShiftOr:
		// This will be double checked later that there's only a single state for searches to work.
		if(numberOfStates == 0) nextFlags |= Flag::IS_SEARCH;
		break;

	case InstructionType::Split:
		{
			const ByteCodeSplitData* data = patternData.GetData<ByteCodeSplitData>(instruction.data);
			for(uint32_t i = 0; i < data->numberOfTargets-1; ++i)
			{
				AddNextState(patternData, data->targetList[i], range, updateCache, nextFlags);
			}
			pc = data->targetList[data->numberOfTargets-1];
		}
		goto Loop;
		
	case InstructionType::SplitMatch:
		{
			const uint32_t* data = patternData.GetData<uint32_t>(instruction.data);
			AddNextState(patternData, data[0], range, updateCache, nextFlags);
			pc = data[1];
		}
		goto Loop;
			
	case InstructionType::SplitNextN:
	case InstructionType::SplitNextMatchN:
		AddNextState(patternData, pc+1, range, updateCache, nextFlags);
		pc = instruction.data;
		goto Loop;
			
	case InstructionType::SplitNNext:
	case InstructionType::SplitNMatchNext:
		AddNextState(patternData, instruction.data, range, updateCache, nextFlags);
		++pc;
		goto Loop;
		
	default:
		break;
	}
	if((nextFlags & Flag::STATE_ADDITION_DISABLED) == 0)
	{
		if(AddEntry(pc, updateCache))
		{
			OpenHashSet<uint32_t> progressCheckSet;
			ProcessNextStateFlags(progressCheckSet, patternData, pc, range, updateCache, nextFlags);
		}
	}
	else
	{
		OpenHashSet<uint32_t> progressCheckSet;
		ProcessNextStateFlags(progressCheckSet, patternData, pc, range, updateCache, nextFlags);
	}
}

void NfaState::ClearIrrelevantFlags()
{
	stateFlags &= (stateFlags >> 8) | (0x00ffff00 | Flag::IS_SEARCH | PARTIAL_MATCH_IS_ALLOWED);
}

void NfaState::Process(NfaState& outResult, const PatternData& patternData, CharacterRange& range, uint32_t flags, UpdateCache &updateCache) const
{
	outResult.numberOfStates = 0;
	outResult.stateFlags = flags;
	
	for(uint32_t i = 0; i < numberOfStates; ++i)
	{
		uint32_t pc = stateList[i];
		if(patternData[pc].type == InstructionType::ProgressCheck) continue;
		ProcessCurrentState(outResult, patternData, pc, range, flags, updateCache);
	}
	outResult.ClearIrrelevantFlags();
}

void NfaState::ProcessCurrentState(NfaState& outResult, const PatternData& patternData, uint32_t pc, CharacterRange& range, uint32_t flags, UpdateCache &updateCache) const
{
Loop:
	const ByteCodeInstruction instruction = patternData[pc];
	switch(instruction.type)
	{
	case InstructionType::AdvanceByte:
		outResult.AddNextState(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
		break;
		
	case InstructionType::AnyByte:
		if(range.min < 256)
		{
			if(range.max > 255) range.max = 255;
			outResult.AddNextState(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
		}
		break;
		
	case InstructionType::AssertStartOfInput:
		if((stateFlags & Flag::IS_START_OF_INPUT) == 0) break;
		++pc;
		goto Loop;
		
	case InstructionType::AssertStartOfLine:
		if((stateFlags & Flag::WAS_END_OF_LINE) == 0) break;
		++pc;
		goto Loop;
		
	case InstructionType::AssertEndOfInput:
		if(range.min < 256)
		{
			if(range.max >= 255) range.max = 255;
			break;
		}
		++pc;
		goto Loop;
		
	case InstructionType::AssertEndOfLine:
		if(range.min == '\n' || range.min == END_OF_INPUT)
		{
			range.max = range.min;
		}
		else if(range.min < '\n')
		{
			if(range.max >= '\n') range.max = '\n' - 1;
			break;
		}
		else
		{
			JASSERT(range.min < 256);
			if(range.max > 255) range.max = 255;
			break;
		}
		++pc;
		goto Loop;
			
	case InstructionType::AssertStartOfSearch:
		if((stateFlags & Flag::IS_START_OF_SEARCH) == 0) break;
		++pc;
		goto Loop;
		
	case InstructionType::AssertWordBoundary:
		CharacterRangeList::WORD_CHARACTERS.TrimRelevancyInterval(range);
		if(((stateFlags ^ ConvertFlagsToNextFlags(flags)) & Flag::WAS_WORD_CHARACTER) == 0) break;
		++pc;
		goto Loop;
		
	case InstructionType::AssertNotWordBoundary:
		CharacterRangeList::WORD_CHARACTERS.TrimRelevancyInterval(range);
		if((stateFlags ^ ConvertFlagsToNextFlags(flags)) & Flag::WAS_WORD_CHARACTER) break;
		++pc;
		goto Loop;
		
	case InstructionType::Byte:
		if(range.min == instruction.data)
		{
			range.max = range.min;
			outResult.AddNextState(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
		}
		else if(range.min < instruction.data)
		{
			if(range.max >= instruction.data) range.max = instruction.data-1;
		}
		else if(range.max < 256)
		{
			if(range.max > 255) range.max = 255;
		}
		break;
		
	case InstructionType::ByteEitherOf2:
		if(range.min < 256)
		{
			int c0 = instruction.data & 0xff;
			int c1 = (instruction.data >> 8) & 0xff;
			CharacterRangeList rangeList;
			rangeList.Append(c0);
			rangeList.Append(c1);
			rangeList.TrimRelevancyInterval(range);
			if(range.min == c0 || range.min == c1)
			{
				outResult.AddNextState(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
		}
		break;
		
	case InstructionType::ByteEitherOf3:
		if(range.min < 256)
		{
			int c0 = instruction.data & 0xff;
			int c1 = (instruction.data >> 8) & 0xff;
			int c2 = (instruction.data >> 16) & 0xff;
			CharacterRangeList rangeList;
			rangeList.Append(c0);
			rangeList.Append(c1);
			rangeList.Append(c2);
			rangeList.TrimRelevancyInterval(range);
			if(range.min == c0 || range.min == c1 || range.min == c2)
			{
				outResult.AddNextState(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
		}
		break;
		
	case InstructionType::ByteRange:
		if(range.min < 256)
		{
			unsigned char low = instruction.data & 0xff;
			unsigned char high = (instruction.data >> 8) & 0xff;
			if(low <= range.min && range.min <= high)
			{
				if(range.max > high) range.max = high;
				outResult.AddNextState(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
			else if(range.min < low)
			{
				if(range.max >= low) range.max = low-1;
			}
			else
			{
				if(range.max > 255) range.max = 255;
			}
		}
		break;
		
	case InstructionType::ByteBitMask:
		if(range.min < 256)
		{
			const StaticBitTable<256>& data = *patternData.GetData<StaticBitTable<256>>(instruction.data);
			if(data[range.min])
			{
				outResult.AddNextState(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
			
			int high = range.min+1;
			int maxTest = Minimum(255u, (uint32_t) range.max);
			while(high <= maxTest && data[high] == data[range.min]) ++high;
			range.max = high-1;
		}
		break;
		
	case InstructionType::ByteJumpRange:
		if(range.min < 256)
		{
			const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			const CharacterRange jumpRange{data->range.min, data->range.max};
			jumpRange.TrimRelevancyInterval(range);
			if(range.max > 255) range.max = 255;
			
			uint32_t newPc = data->pcData[data->range.Contains(range.min)];
			if(newPc != TypeData<uint32_t>::Maximum())
			{
				outResult.AddNextState(patternData, newPc, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
		}
		break;
		
	case InstructionType::ByteJumpMask:
		if(range.min < 256)
		{
			const ByteCodeJumpMaskData* data = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
			uint32_t newPc = data->pcData[data->bitMask[range.min]];
			if(newPc != TypeData<uint32_t>::Maximum())
			{
				outResult.AddNextState(patternData, newPc, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
			int high = range.min+1;
			int maxTest = Minimum(255u, (uint32_t) range.max);
			while(high <= maxTest && data->bitMask[high] == data->bitMask[range.min]) ++high;
			range.max = high-1;
		}
		break;
		
	case InstructionType::ByteJumpTable:
		if(range.min < 256)
		{
			const ByteCodeJumpTableData* data = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
			uint32_t newPc = data->pcData[data->jumpTable[range.min]];
			if(newPc != TypeData<uint32_t>::Maximum())
			{
				outResult.AddNextState(patternData, newPc, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
			int high = range.min+1;
			int maxTest = Minimum(255u, (uint32_t) range.max);
			while(high <= maxTest && data->jumpTable[high] == data->jumpTable[range.min]) ++high;
			range.max = high-1;
		}
		break;
		
	case InstructionType::ByteNot:
		if(range.min < 256)
		{
			int c0 = instruction.data;
			if(range.min < c0)
			{
				if(range.max >= instruction.data) range.max = instruction.data-1;
			}
			else if(range.min == c0)
			{
				range.max = range.min;
			}
			else
			{
				if(range.max > 255) range.max = 255;
			}
			if(range.min != c0) outResult.AddNextState(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
		}
		break;
			
	case InstructionType::ByteNotEitherOf2:
		if(range.min < 256)
		{
			int c0 = instruction.data & 0xff;
			int c1 = (instruction.data >> 8) & 0xff;
			CharacterRangeList rangeList;
			rangeList.Append(c0);
			rangeList.Append(c1);
			rangeList.TrimRelevancyInterval(range);
			if(range.min != c0 && range.min != c1)
			{
				outResult.AddNextState(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
		}
		break;
			
	case InstructionType::ByteNotEitherOf3:
		if(range.min < 256)
		{
			int c0 = instruction.data & 0xff;
			int c1 = (instruction.data >> 8) & 0xff;
			int c2 = (instruction.data >> 16) & 0xff;
			CharacterRangeList rangeList;
			rangeList.Append(c0);
			rangeList.Append(c1);
			rangeList.Append(c2);
			rangeList.TrimRelevancyInterval(range);
			if(range.min != c0 && range.min != c1 && range.min != c2)
			{
				outResult.AddNextState(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
		}
		break;
			
	case InstructionType::ByteNotRange:
		if(range.min < 256)
		{
			unsigned char low = instruction.data & 0xff;
			unsigned char high = (instruction.data >> 8) & 0xff;
			if(range.min < low)
			{
				if(range.max >= low) range.max = low-1;
				outResult.AddNextState(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
			else if(range.min <= high)
			{
				if(range.max > high) range.max = high;
			}
			else
			{
				if(range.max > 255) range.max = 255;
				outResult.AddNextState(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
		}
		break;
			
	case InstructionType::DispatchRange:
		{
			const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			if(range.min < data->range.min)
			{
				if(range.max >= data->range.min) range.max = data->range.min-1;
			}
			else
			{
				if(data->range.Contains(range.min))
				{
					if(range.max >= data->range.max) range.max = data->range.max;
				}
			}
			pc = data->pcData[data->range.Contains(range.min)];
			if(pc == TypeData<uint32_t>::Maximum()) return;
		}
		goto Loop;
		
	case InstructionType::DispatchMask:
		{
			
			const ByteCodeJumpMaskData* data = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
			int high = range.min+1;
			int maxTest = Minimum(255u, (uint32_t) range.max);
			while(high <= maxTest && data->bitMask[high] == data->bitMask[range.min]) ++high;
			range.max = high-1;
			pc = data->pcData[range.min < 256 ? data->bitMask[range.min] : 0];
			if(pc == TypeData<uint32_t>::Maximum()) return;
		}
		goto Loop;
		
	case InstructionType::DispatchTable:
		{
			const ByteCodeJumpTableData* data = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
			int high = range.min+1;
			int maxTest = Minimum(255u, (uint32_t) range.max);
			while(high <= maxTest && data->jumpTable[high] == data->jumpTable[range.min]) ++high;
			range.max = high-1;
			pc = data->pcData[range.min < 256 ? data->jumpTable[range.min] : 0];
			if(pc == TypeData<uint32_t>::Maximum()) return;
		}
		goto Loop;
		
	case InstructionType::Fail:
		break;
		
	case InstructionType::FindByte:
		if(range.min < 256)
		{
			int c0 = instruction.data & 0xff;
			if(range.min < c0)
			{
				if(range.max >= c0) range.max = c0-1;
			}
			else if(range.min == c0)
			{
				range.max = range.min;
			}
			else
			{
				if(range.max > 255) range.max = 255;
			}
			
			outResult.AddNextState(patternData, range.min == c0 ? instruction.data >> 8 : pc, range, updateCache, ConvertFlagsToNextFlags(flags));
		}
		break;
			
	case InstructionType::Jump:
		pc = instruction.data;
		goto Loop;

	case InstructionType::ProgressCheck:
		if(outResult.AddEntryNoMatchCheck(pc, updateCache))
		{
			++pc;
			goto Loop;
		}
		break;

	case InstructionType::PropagateBackwards:
	case InstructionType::Save:
	case InstructionType::SaveNoRecurse:
		++pc;
		goto Loop;
			
	case InstructionType::Match:
		if((stateFlags & PARTIAL_MATCH_IS_ALLOWED) || range.min == END_OF_INPUT)
		{
			outResult.stateFlags |= Flag::IS_MATCH;
		}
		else
		{
			if(range.max > 255) range.max = 255;
		}
		break;
		
	case InstructionType::Split:
		{
			const ByteCodeSplitData* data = patternData.GetData<ByteCodeSplitData>(instruction.data);
			for(uint32_t i = 0; i < data->numberOfTargets-1; ++i)
			{
				ProcessCurrentState(outResult, patternData, data->targetList[i], range, flags, updateCache);
			}
			pc = data->targetList[data->numberOfTargets-1];
		}
		goto Loop;

	case InstructionType::SplitMatch:
		{
			const uint32_t* data = patternData.GetData<uint32_t>(instruction.data);
			ProcessCurrentState(outResult, patternData, data[0], range, flags, updateCache);
			pc = data[1];
		}
		goto Loop;
			
	case InstructionType::SplitNextN:
	case InstructionType::SplitNextMatchN:
		ProcessCurrentState(outResult, patternData, pc+1, range, flags, updateCache);
		pc = instruction.data;
		goto Loop;
		
	case InstructionType::SplitNNext:
	case InstructionType::SplitNMatchNext:
		ProcessCurrentState(outResult, patternData, instruction.data, range, flags, updateCache);
		++pc;
		goto Loop;
		
	case InstructionType::SearchByte:
		{
			int offset = (instruction.data >> 8);
			int c = instruction.data & 0xff;

			if(offset == 0)
			{
				if(range.min < c)
				{
					if(range.max >= c) range.max = c-1;
				}
				else if(range.min == c)
				{
					range.max = range.min;
				}
				else if(range.min < 256)
				{
					if(range.max > 255) range.max = 255;
				}
				if(range.min != c)
				{
					outResult.AddNextState(patternData, pc, range, updateCache, ConvertFlagsToNextFlags(flags));
				}
			}
			++pc;
			goto Loop;
		}
		break;
			
	case InstructionType::SearchBytePair:
	case InstructionType::SearchByteTriplet:
	case InstructionType::SearchByteEitherOf2:
	case InstructionType::SearchBytePair2:
	case InstructionType::SearchByteTriplet2:
	case InstructionType::SearchByteEitherOf3:
	case InstructionType::SearchBytePair3:
	case InstructionType::SearchByteEitherOf4:
	case InstructionType::SearchBytePair4:
	case InstructionType::SearchByteEitherOf5:
	case InstructionType::SearchByteEitherOf6:
	case InstructionType::SearchByteEitherOf7:
	case InstructionType::SearchByteEitherOf8:
		if(range.min < 256)
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			if(data->offset == 0)
			{
				int numberOfBytes = instruction.type >= InstructionType::SearchByteTriplet ?
										int(instruction.type) - int(InstructionType::SearchByteTriplet) + 1 :
										instruction.type >= InstructionType::SearchBytePair ?
											int(instruction.type) - int(InstructionType::SearchBytePair) + 1 :
											int(instruction.type) - int(InstructionType::SearchByte) + 1;
				CharacterRangeList rangeList;
				bool match = false;
				for(int i = 0; i < numberOfBytes; ++i)
				{
					if(range.min == data->bytes[i]) match = true;
					rangeList.Add(data->bytes[i]);
				}
				rangeList.TrimRelevancyInterval(range);
				if(!match)
				{
					outResult.AddNextState(patternData, pc, range, updateCache, ConvertFlagsToNextFlags(flags));
				}
			}
			++pc;
			goto Loop;
		}
		break;
			
	case InstructionType::SearchByteRange:
	case InstructionType::SearchByteRangePair:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(data->offset == 0)
			{
				const CharacterRange searchRange{data->bytes[0], data->bytes[1]};
				searchRange.TrimRelevancyInterval(range);
				if(!searchRange.Contains(range.min))
				{
					outResult.AddNextState(patternData, pc, range, updateCache, ConvertFlagsToNextFlags(flags));
				}
			}
			++pc;
			goto Loop;
		}
		break;
			
	case InstructionType::SearchShiftOr:
		if(range.min < 256)
		{
			const ByteCodeSearchData* data = patternData.GetData<ByteCodeSearchData>(instruction.data);
			int high = range.min+1;
			int maxTest = Minimum(255u, (uint32_t) range.max);
			while(high <= maxTest && ((data->data[high] ^ data->data[range.min]) & 1) == 0) ++high;
			range.max = high-1;
			
			if((data->data[range.min] & 1) != 0)
			{
				outResult.AddNextState(patternData, pc, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
			++pc;
			goto Loop;
		}
		break;
		
	case InstructionType::SearchBoyerMoore:
		if(range.min < 256)
		{
			ProcessCurrentState(outResult, patternData, pc+1, range, flags, updateCache);
			outResult.AddNextState(patternData, pc, range, updateCache, ConvertFlagsToNextFlags(flags));
		}
		break;
			
	case InstructionType::AssertRecurseValue:
	case InstructionType::BackReference:
	case InstructionType::Call:
	case InstructionType::Possess:
	case InstructionType::Recurse:
	case InstructionType::ReturnIfRecurseValue:
	case InstructionType::StepBack:
	case InstructionType::Success:
		JERROR("Unexpected instruction!");
	}
}

//============================================================================

void NfaState::ProcessNextStateFlagsReverse(OpenHashSet<uint32_t>& progressCheckSet, const PatternData& patternData, uint32_t pc, CharacterRange& range, UpdateCache &updateCache, int nextFlags)
{
Loop:
	const ByteCodeInstruction instruction = patternData[pc];
	switch(instruction.type)
	{
	case InstructionType::AssertStartOfInput:
		++pc;
		goto Loop;
		
	case InstructionType::AssertStartOfLine:
		++pc;
		goto Loop;
		
	case InstructionType::AssertEndOfInput:
		nextFlags |= Flag::IS_END_OF_INPUT_MASK;
		++pc;
		goto Loop;
		
	case InstructionType::AssertEndOfLine:
		nextFlags |= Flag::WAS_END_OF_LINE_MASK;
		{
			const CharacterRange NEW_LINE_RANGE{'\n', '\n'};
			NEW_LINE_RANGE.TrimRelevancyInterval(range);
		}
		++pc;
		goto Loop;
		
	case InstructionType::AssertWordBoundary:
	case InstructionType::AssertNotWordBoundary:
		nextFlags |= Flag::WAS_WORD_CHARACTER_MASK;
		CharacterRangeList::WORD_CHARACTERS.TrimRelevancyInterval(range);
		++pc;
		goto Loop;

	case InstructionType::AssertStartOfSearch:
		nextFlags |= Flag::IS_START_OF_SEARCH_MASK;
		++pc;
		goto Loop;

	case InstructionType::DispatchRange:
		{
			const ByteCodeJumpRangeData* jumpRange = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			if(jumpRange->pcData[0] != TypeData<uint32_t>::Maximum())
			{
				nextFlags |= Flag::IS_START_OF_SEARCH_MASK;
			}
			for(int i = 0; i < 2; ++i)
			{
				if(jumpRange->pcData[i] != TypeData<uint32_t>::Maximum())
				{
					ProcessNextStateFlagsReverse(progressCheckSet, patternData, jumpRange->pcData[i], range, updateCache, nextFlags);
				}
			}
		}
		break;
		
	case InstructionType::DispatchMask:
		{
			const ByteCodeJumpMaskData* jumpMask = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
			if(jumpMask->pcData[0] != TypeData<uint32_t>::Maximum())
			{
				nextFlags |= Flag::IS_START_OF_SEARCH_MASK;
			}
			for(int i = 0; i < 2; ++i)
			{
				if(jumpMask->pcData[i] != TypeData<uint32_t>::Maximum())
				{
					ProcessNextStateFlagsReverse(progressCheckSet, patternData, jumpMask->pcData[i], range, updateCache, nextFlags);
				}
			}
		}
		break;
		
	case InstructionType::DispatchTable:
		{
			const ByteCodeJumpTableData* jumpTable = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
			if(jumpTable->pcData[0] != TypeData<uint32_t>::Maximum())
			{
				nextFlags |= Flag::IS_START_OF_SEARCH_MASK;
			}
			for(int i = 0; i < jumpTable->numberOfTargets; ++i)
			{
				if(jumpTable->pcData[i] != TypeData<uint32_t>::Maximum())
				{
					ProcessNextStateFlagsReverse(progressCheckSet, patternData, jumpTable->pcData[i], range, updateCache, nextFlags);
				}
			}
		}
		break;
		
	case InstructionType::Jump:
		pc = instruction.data;
		goto Loop;
		
	case InstructionType::ProgressCheck:
		if(!progressCheckSet.Contains(pc))
		{
			progressCheckSet.Put(pc);
			++pc;
			goto Loop;
		}
		break;
	case InstructionType::PropagateBackwards:
	case InstructionType::Save:
	case InstructionType::SaveNoRecurse:
		++pc;
		goto Loop;
		
	case InstructionType::Split:
		{
			const ByteCodeSplitData* data = patternData.GetData<ByteCodeSplitData>(instruction.data);
			for(uint32_t i = 0; i < data->numberOfTargets-1; ++i)
			{
				ProcessNextStateFlagsReverse(progressCheckSet, patternData, data->targetList[i], range, updateCache, nextFlags);
			}
			pc = data->targetList[data->numberOfTargets-1];
		}
		goto Loop;

	case InstructionType::SplitMatch:
		{
			const uint32_t* data = patternData.GetData<uint32_t>(instruction.data);
			ProcessNextStateFlagsReverse(progressCheckSet, patternData, data[0], range, updateCache, nextFlags);
			pc = data[1];
		}
		goto Loop;

	case InstructionType::SplitNextN:
	case InstructionType::SplitNextMatchN:
		ProcessNextStateFlagsReverse(progressCheckSet, patternData, pc+1, range, updateCache, nextFlags);
		pc = instruction.data;
		goto Loop;
			
	case InstructionType::SplitNNext:
	case InstructionType::SplitNMatchNext:
		ProcessNextStateFlagsReverse(progressCheckSet, patternData, instruction.data, range, updateCache, nextFlags);
		++pc;
		goto Loop;
		
	case InstructionType::AdvanceByte:
	case InstructionType::AnyByte:
	case InstructionType::Byte:
	case InstructionType::ByteEitherOf2:
	case InstructionType::ByteEitherOf3:
	case InstructionType::ByteRange:
	case InstructionType::ByteBitMask:
	case InstructionType::ByteJumpMask:
	case InstructionType::ByteJumpRange:
	case InstructionType::ByteJumpTable:
	case InstructionType::ByteNot:
	case InstructionType::ByteNotEitherOf2:
	case InstructionType::ByteNotEitherOf3:
	case InstructionType::ByteNotRange:
	case InstructionType::FindByte:
	case InstructionType::Match:
		stateFlags |= nextFlags;
		break;

	case InstructionType::SearchByte:
	case InstructionType::SearchByteEitherOf2:
	case InstructionType::SearchByteEitherOf3:
	case InstructionType::SearchByteEitherOf4:
	case InstructionType::SearchByteEitherOf5:
	case InstructionType::SearchByteEitherOf6:
	case InstructionType::SearchByteEitherOf7:
	case InstructionType::SearchByteEitherOf8:
	case InstructionType::SearchBytePair:
	case InstructionType::SearchBytePair2:
	case InstructionType::SearchBytePair3:
	case InstructionType::SearchBytePair4:
	case InstructionType::SearchByteTriplet:
	case InstructionType::SearchByteTriplet2:
	case InstructionType::SearchByteRange:
	case InstructionType::SearchByteRangePair:
	case InstructionType::SearchBoyerMoore:
	case InstructionType::SearchShiftOr:
		stateFlags |= nextFlags;
		++pc;
		goto Loop;
			
	case InstructionType::Fail:
		break;

	case InstructionType::AssertRecurseValue:
	case InstructionType::BackReference:
	case InstructionType::Call:
	case InstructionType::Possess:
	case InstructionType::Recurse:
	case InstructionType::ReturnIfRecurseValue:
	case InstructionType::StepBack:
	case InstructionType::Success:
		JERROR("Unexpected instruction!");
	}
}

void NfaState::AddNextStateReverse(const PatternData& patternData, uint32_t pc, CharacterRange& range, UpdateCache &updateCache, int nextFlags)
{
Loop:
	const ByteCodeInstruction instruction = patternData[pc];
	switch(instruction.type)
	{
	case InstructionType::Jump:
		pc = instruction.data;
		goto Loop;
		
	case InstructionType::ProgressCheck:
		if(AddEntryNoMatchCheck(pc, updateCache))
		{
			++pc;
			goto Loop;
		}
		else
		{
			return;
		}
			
	case InstructionType::PropagateBackwards:
	case InstructionType::Save:
	case InstructionType::SaveNoRecurse:
		++pc;
		goto Loop;
		
	case InstructionType::FindByte:
	case InstructionType::SearchByte:
	case InstructionType::SearchByteEitherOf2:
	case InstructionType::SearchByteEitherOf3:
	case InstructionType::SearchByteEitherOf4:
	case InstructionType::SearchByteEitherOf5:
	case InstructionType::SearchByteEitherOf6:
	case InstructionType::SearchByteEitherOf7:
	case InstructionType::SearchByteEitherOf8:
	case InstructionType::SearchBytePair:
	case InstructionType::SearchBytePair2:
	case InstructionType::SearchBytePair3:
	case InstructionType::SearchBytePair4:
	case InstructionType::SearchByteTriplet:
	case InstructionType::SearchByteTriplet2:
	case InstructionType::SearchByteRange:
	case InstructionType::SearchByteRangePair:
	case InstructionType::SearchBoyerMoore:
	case InstructionType::SearchShiftOr:
		nextFlags |= Flag::IS_SEARCH;
		break;

	case InstructionType::Split:
		{
			const ByteCodeSplitData* data = patternData.GetData<ByteCodeSplitData>(instruction.data);
			for(uint32_t i = 0; i < data->numberOfTargets-1; ++i)
			{
				AddNextStateReverse(patternData, data->targetList[i], range, updateCache, nextFlags);
			}
			pc = data->targetList[data->numberOfTargets-1];
		}
		goto Loop;
		
	case InstructionType::SplitMatch:
		{
			const uint32_t* data = patternData.GetData<uint32_t>(instruction.data);
			AddNextStateReverse(patternData, data[0], range, updateCache, nextFlags);
			pc = data[1];
		}
		goto Loop;
			
	case InstructionType::SplitNextN:
	case InstructionType::SplitNextMatchN:
		AddNextStateReverse(patternData, pc+1, range, updateCache, nextFlags);
		pc = instruction.data;
		goto Loop;
			
	case InstructionType::SplitNNext:
	case InstructionType::SplitNMatchNext:
		AddNextStateReverse(patternData, instruction.data, range, updateCache, nextFlags);
		++pc;
		goto Loop;
		
	default:
		break;
	}
	if(AddEntryNoMatchCheck(pc, updateCache))
	{
		OpenHashSet<uint32_t> progressCheckSet;
		ProcessNextStateFlagsReverse(progressCheckSet, patternData, pc, range, updateCache, nextFlags);
	}
}

void NfaState::ProcessReverse(NfaState& outResult, const PatternData& patternData, CharacterRange& range, uint32_t flags, UpdateCache &updateCache) const
{
	outResult.numberOfStates = 0;
	outResult.stateFlags = flags;
	
	for(uint32_t i = 0; i < numberOfStates; ++i)
	{
		uint32_t pc = stateList[i];
		if(patternData[pc].type == InstructionType::ProgressCheck) continue;
		ProcessCurrentStateReverse(outResult, patternData, pc, range, flags, updateCache);
	}
	outResult.ClearIrrelevantFlags();
}

void NfaState::ProcessCurrentStateReverse(NfaState& outResult, const PatternData& patternData, uint32_t pc, CharacterRange& range, uint32_t flags, UpdateCache &updateCache) const
{
Loop:
	const ByteCodeInstruction instruction = patternData[pc];
	switch(instruction.type)
	{
	case InstructionType::AdvanceByte:
		outResult.AddNextStateReverse(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
		break;
		
	case InstructionType::AnyByte:
		if(range.min < 256)
		{
			if(range.max > 255) range.max = 255;
			outResult.AddNextStateReverse(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
		}
		break;
		
	case InstructionType::AssertStartOfInput:
		if(range.min < 256)
		{
			if(range.max > 255) range.max = 255;
			break;
		}
		JASSERT(range.max == range.min);
		++pc;
		goto Loop;
		
	case InstructionType::AssertStartOfLine:
		if(range.min < '\n')
		{
			if(range.max >= '\n') range.max = '\n'-1;
			break;
		}
		else if(range.min == '\n')
		{
			range.max = range.min;
		}
		else if(range.min < 256)
		{
			if(range.max > 255) range.max = 255;
			break;
		}
		++pc;
		goto Loop;
		
	case InstructionType::AssertEndOfInput:
		if((stateFlags & Flag::IS_END_OF_INPUT) == 0) break;
		++pc;
		goto Loop;
		
	case InstructionType::AssertEndOfLine:
		if((stateFlags & (Flag::IS_END_OF_INPUT | Flag::WAS_END_OF_LINE)) == 0) break;
		++pc;
		goto Loop;
			
	case InstructionType::AssertStartOfSearch:
		if((stateFlags & Flag::IS_START_OF_SEARCH) == 0) break;
		++pc;
		goto Loop;
		
	case InstructionType::AssertWordBoundary:
		CharacterRangeList::WORD_CHARACTERS.TrimRelevancyInterval(range);
		if(((stateFlags ^ ConvertFlagsToNextFlags(flags)) & Flag::WAS_WORD_CHARACTER) == 0) break;
		++pc;
		goto Loop;
		
	case InstructionType::AssertNotWordBoundary:
		CharacterRangeList::WORD_CHARACTERS.TrimRelevancyInterval(range);
		if((stateFlags ^ ConvertFlagsToNextFlags(flags)) & Flag::WAS_WORD_CHARACTER) break;
		++pc;
		goto Loop;
		
	case InstructionType::Byte:
		if(range.min == instruction.data)
		{
			range.max = range.min;
			outResult.AddNextStateReverse(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
		}
		else if(range.min < instruction.data)
		{
			if(range.max >= instruction.data) range.max = instruction.data-1;
		}
		else if(range.max < 256)
		{
			if(range.max > 255) range.max = 255;
		}
		break;
		
	case InstructionType::ByteEitherOf2:
		if(range.min < 256)
		{
			int c0 = instruction.data & 0xff;
			int c1 = (instruction.data >> 8) & 0xff;
			CharacterRangeList rangeList;
			rangeList.Append(c0);
			rangeList.Append(c1);
			rangeList.TrimRelevancyInterval(range);
			if(range.min == c0 || range.min == c1)
			{
				outResult.AddNextStateReverse(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
		}
		break;
		
	case InstructionType::ByteEitherOf3:
		if(range.min < 256)
		{
			int c0 = instruction.data & 0xff;
			int c1 = (instruction.data >> 8) & 0xff;
			int c2 = (instruction.data >> 16) & 0xff;
			CharacterRangeList rangeList;
			rangeList.Append(c0);
			rangeList.Append(c1);
			rangeList.Append(c2);
			rangeList.TrimRelevancyInterval(range);
			if(range.min == c0 || range.min == c1 || range.min == c2)
			{
				outResult.AddNextStateReverse(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
		}
		break;
		
	case InstructionType::ByteRange:
		if(range.min < 256)
		{
			unsigned char low = instruction.data & 0xff;
			unsigned char high = (instruction.data >> 8) & 0xff;
			if(low <= range.min && range.min <= high)
			{
				if(range.max > high) range.max = high;
				outResult.AddNextStateReverse(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
			else if(range.min < low)
			{
				if(range.max >= low) range.max = low-1;
			}
			else
			{
				if(range.max > 255) range.max = 255;
			}
		}
		break;
		
	case InstructionType::ByteBitMask:
		if(range.min < 256)
		{
			const StaticBitTable<256>& data = *patternData.GetData<StaticBitTable<256>>(instruction.data);
			if(data[range.min])
			{
				outResult.AddNextStateReverse(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
			
			int high = range.min+1;
			int maxTest = Minimum(255u, (uint32_t) range.max);
			while(high <= maxTest && data[high] == data[range.min]) ++high;
			range.max = high-1;
		}
		break;
		
	case InstructionType::ByteJumpRange:
		if(range.min < 256)
		{
			const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			const CharacterRange jumpRange{data->range.min, data->range.max};
			jumpRange.TrimRelevancyInterval(range);
			if(range.max > 255) range.max = 255;
			
			uint32_t newPc = data->pcData[data->range.Contains(range.min)];
			if(newPc != TypeData<uint32_t>::Maximum())
			{
				outResult.AddNextStateReverse(patternData, newPc, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
		}
		break;
		
	case InstructionType::ByteJumpMask:
		if(range.min < 256)
		{
			const ByteCodeJumpMaskData* data = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
			uint32_t newPc = data->pcData[data->bitMask[range.min]];
			if(newPc != TypeData<uint32_t>::Maximum())
			{
				outResult.AddNextStateReverse(patternData, newPc, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
			int high = range.min+1;
			int maxTest = Minimum(255u, (uint32_t) range.max);
			while(high <= maxTest && data->bitMask[high] == data->bitMask[range.min]) ++high;
			range.max = high-1;
		}
		break;
		
	case InstructionType::ByteJumpTable:
		if(range.min < 256)
		{
			const ByteCodeJumpTableData* data = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
			uint32_t newPc = data->pcData[data->jumpTable[range.min]];
			if(newPc != TypeData<uint32_t>::Maximum())
			{
				outResult.AddNextStateReverse(patternData, newPc, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
			int high = range.min+1;
			int maxTest = Minimum(255u, (uint32_t) range.max);
			while(high <= maxTest && data->jumpTable[high] == data->jumpTable[range.min]) ++high;
			range.max = high-1;
		}
		break;
		
	case InstructionType::ByteNot:
		if(range.min < 256)
		{
			int c0 = instruction.data;
			if(range.min < c0)
			{
				if(range.max >= instruction.data) range.max = instruction.data-1;
			}
			else if(range.min == c0)
			{
				range.max = range.min;
			}
			else
			{
				if(range.max > 255) range.max = 255;
			}
			if(range.min != c0) outResult.AddNextStateReverse(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
		}
		break;
			
	case InstructionType::ByteNotEitherOf2:
		if(range.min < 256)
		{
			int c0 = instruction.data & 0xff;
			int c1 = (instruction.data >> 8) & 0xff;
			CharacterRangeList rangeList;
			rangeList.Append(c0);
			rangeList.Append(c1);
			rangeList.TrimRelevancyInterval(range);
			if(range.min != c0 && range.min != c1)
			{
				outResult.AddNextStateReverse(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
		}
		break;
			
	case InstructionType::ByteNotEitherOf3:
		if(range.min < 256)
		{
			int c0 = instruction.data & 0xff;
			int c1 = (instruction.data >> 8) & 0xff;
			int c2 = (instruction.data >> 16) & 0xff;
			CharacterRangeList rangeList;
			rangeList.Append(c0);
			rangeList.Append(c1);
			rangeList.Append(c2);
			rangeList.TrimRelevancyInterval(range);
			if(range.min != c0 && range.min != c1 && range.min != c2)
			{
				outResult.AddNextStateReverse(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
		}
		break;
			
	case InstructionType::ByteNotRange:
		if(range.min < 256)
		{
			unsigned char low = instruction.data & 0xff;
			unsigned char high = (instruction.data >> 8) & 0xff;
			
			if(range.min < low)
			{
				if(range.max >= low) range.max = low-1;
				outResult.AddNextStateReverse(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
			else if(range.min <= high)
			{
				if(range.max > high) range.max = high;
			}
			else
			{
				if(range.max > 255) range.max = 255;
				outResult.AddNextStateReverse(patternData, pc+1, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
		}
		break;
			
	case InstructionType::DispatchRange:
		{
			const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			if(stateFlags & Flag::IS_START_OF_SEARCH)
			{
				pc = data->pcData[0];
			}
			else
			{
				if(range.min < data->range.min)
				{
					if(range.max >= data->range.min) range.max = data->range.min-1;
				}
				else
				{
					if(data->range.Contains(range.min))
					{
						if(range.max >= data->range.max) range.max = data->range.max;
					}
				}
				pc = data->pcData[data->range.Contains(range.min)];
			}
			if(pc == TypeData<uint32_t>::Maximum()) return;
		}
		goto Loop;
		
	case InstructionType::DispatchMask:
		{
			
			const ByteCodeJumpMaskData* data = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
			if(stateFlags & Flag::IS_START_OF_SEARCH)
			{
				pc = data->pcData[0];
			}
			else
			{
				int high = range.min+1;
				int maxTest = Minimum(255u, (uint32_t) range.max);
				while(high <= maxTest && data->bitMask[high] == data->bitMask[range.min]) ++high;
				range.max = high-1;
				pc = data->pcData[range.min < 256 ? data->bitMask[range.min] : 0];
			}
			if(pc == TypeData<uint32_t>::Maximum()) return;
		}
		goto Loop;
		
	case InstructionType::DispatchTable:
		{
			const ByteCodeJumpTableData* data = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
			if(stateFlags & Flag::IS_START_OF_SEARCH)
			{
				pc = data->pcData[0];
			}
			else
			{
				int high = range.min+1;
				int maxTest = Minimum(255u, (uint32_t) range.max);
				while(high <= maxTest && data->jumpTable[high] == data->jumpTable[range.min]) ++high;
				range.max = high-1;
				pc = data->pcData[range.min < 256 ? data->jumpTable[range.min] : 0];
			}
			if(pc == TypeData<uint32_t>::Maximum()) return;
		}
		goto Loop;
		
	case InstructionType::Fail:
		break;
		
	case InstructionType::FindByte:
		if(range.min < 256)
		{
			int c0 = instruction.data & 0xff;
			if(range.min < c0)
			{
				if(range.max >= c0) range.max = c0-1;
			}
			else if(range.min == c0)
			{
				range.max = range.min;
			}
			else
			{
				if(range.max > 255) range.max = 255;
			}
			
			outResult.AddNextStateReverse(patternData, range.min == c0 ? instruction.data >> 8 : pc, range, updateCache, ConvertFlagsToNextFlags(flags));
		}
		break;
			
	case InstructionType::Jump:
		pc = instruction.data;
		goto Loop;

	case InstructionType::ProgressCheck:
		if(outResult.AddEntryNoMatchCheck(pc, updateCache))
		{
			++pc;
			goto Loop;
		}
		break;

	case InstructionType::PropagateBackwards:
	case InstructionType::Save:
	case InstructionType::SaveNoRecurse:
		++pc;
		goto Loop;
			
	case InstructionType::Match:
		if((stateFlags & PARTIAL_MATCH_IS_ALLOWED) || range.min >= 256)
		{
			outResult.stateFlags |= Flag::IS_MATCH;
		}
		else
		{
			if(range.max > 255) range.max = 255;
		}
		break;
		
	case InstructionType::Split:
		{
			const ByteCodeSplitData* data = patternData.GetData<ByteCodeSplitData>(instruction.data);
			for(uint32_t i = 0; i < data->numberOfTargets-1; ++i)
			{
				ProcessCurrentStateReverse(outResult, patternData, data->targetList[i], range, flags, updateCache);
			}
			pc = data->targetList[data->numberOfTargets-1];
		}
		goto Loop;

	case InstructionType::SplitMatch:
		{
			const uint32_t* data = patternData.GetData<uint32_t>(instruction.data);
			ProcessCurrentStateReverse(outResult, patternData, data[0], range, flags, updateCache);
			pc = data[1];
		}
		goto Loop;
			
	case InstructionType::SplitNextN:
	case InstructionType::SplitNextMatchN:
		ProcessCurrentStateReverse(outResult, patternData, pc+1, range, flags, updateCache);
		pc = instruction.data;
		goto Loop;
		
	case InstructionType::SplitNNext:
	case InstructionType::SplitNMatchNext:
		ProcessCurrentStateReverse(outResult, patternData, instruction.data, range, flags, updateCache);
		++pc;
		goto Loop;
		
	case InstructionType::SearchByte:
		{
			int offset = (instruction.data >> 8);
			int c = instruction.data & 0xff;

			if(offset == 0)
			{
				if(range.min < c)
				{
					if(range.max >= c) range.max = c-1;
				}
				else if(range.min == c)
				{
					range.max = range.min;
				}
				else if(range.min < 256)
				{
					if(range.max > 255) range.max = 255;
				}
				if(range.min != c)
				{
					outResult.AddNextStateReverse(patternData, pc, range, updateCache, ConvertFlagsToNextFlags(flags));
				}
			}
			++pc;
			goto Loop;
		}
		break;
			
	case InstructionType::SearchBytePair:
	case InstructionType::SearchByteTriplet:
	case InstructionType::SearchByteEitherOf2:
	case InstructionType::SearchBytePair2:
	case InstructionType::SearchByteTriplet2:
	case InstructionType::SearchByteEitherOf3:
	case InstructionType::SearchBytePair3:
	case InstructionType::SearchByteEitherOf4:
	case InstructionType::SearchBytePair4:
	case InstructionType::SearchByteEitherOf5:
	case InstructionType::SearchByteEitherOf6:
	case InstructionType::SearchByteEitherOf7:
	case InstructionType::SearchByteEitherOf8:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			if(data->offset == 0)
			{
				int numberOfBytes = instruction.type >= InstructionType::SearchByteTriplet ?
										int(instruction.type) - int(InstructionType::SearchByteTriplet) + 1 :
										instruction.type >= InstructionType::SearchBytePair ?
											int(instruction.type) - int(InstructionType::SearchBytePair) + 1 :
											int(instruction.type) - int(InstructionType::SearchByte) + 1;
				CharacterRangeList rangeList;
				bool match = false;
				for(int i = 0; i < numberOfBytes; ++i)
				{
					if(range.min == data->bytes[i]) match = true;
					rangeList.Add(data->bytes[i]);
				}
				rangeList.TrimRelevancyInterval(range);
				if(!match)
				{
					outResult.AddNextStateReverse(patternData, pc, range, updateCache, ConvertFlagsToNextFlags(flags));
				}
			}
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchByteRange:
	case InstructionType::SearchByteRangePair:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(data->offset == 0)
			{
				const CharacterRange searchRange{data->bytes[0], data->bytes[1]};
				searchRange.TrimRelevancyInterval(range);
				if(!searchRange.Contains(range.min))
				{
					outResult.AddNextStateReverse(patternData, pc, range, updateCache, ConvertFlagsToNextFlags(flags));
				}
			}
			++pc;
			goto Loop;
		}
		break;
			
	case InstructionType::SearchShiftOr:
		if(range.min < 256)
		{
			const ByteCodeSearchData* data = patternData.GetData<ByteCodeSearchData>(instruction.data);
			int high = range.min+1;
			int maxTest = Minimum(255u, (uint32_t) range.max);
			while(high <= maxTest && ((data->data[high] ^ data->data[range.min]) & 1) == 0) ++high;
			range.max = high-1;
			
			if((data->data[range.min] & 1) != 0)
			{
				outResult.AddNextStateReverse(patternData, pc, range, updateCache, ConvertFlagsToNextFlags(flags));
			}
			++pc;
			goto Loop;
		}
		break;
		
	case InstructionType::SearchBoyerMoore:
		if(range.min < 256)
		{
			ProcessCurrentStateReverse(outResult, patternData, pc+1, range, flags, updateCache);
			outResult.AddNextStateReverse(patternData, pc, range, updateCache, ConvertFlagsToNextFlags(flags));
		}
		break;
			
	case InstructionType::AssertRecurseValue:
	case InstructionType::BackReference:
	case InstructionType::Call:
	case InstructionType::Possess:
	case InstructionType::Recurse:
	case InstructionType::ReturnIfRecurseValue:
	case InstructionType::StepBack:
	case InstructionType::Success:
		JERROR("Unexpected instruction!");
	}
}

//============================================================================
