//============================================================================

#include "Javelin/Pattern/Internal/PatternReverseProcessor.h"
#include "Javelin/Container/BitTable.h"
#include "Javelin/Pattern/Internal/PatternByteCode.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class BitFieldGlushkovNfaReverseProcessor final : public ReverseProcessor
{
public:
	BitFieldGlushkovNfaReverseProcessor(const void* data, size_t length);
	
	virtual const void* Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char **captures, bool matchIsAnchored) const;
	
	static bool CanUse(const void* data, size_t length);
	
private:
	uint32_t	matchStateMask;
	DataBlock	lookupDataBlock;
	const void* (*anchoredProcessFunction)(uint32_t matchStateMask, const void* data, const void* stopSearch, const void* startSearch);
	const void* (*unanchoredProcessFunction)(uint32_t matchStateMask, const void* data, const void* stopSearch, const void* startSearch);
	
	struct State
	{
		uint32_t pc;
		uint32_t nextPc;
		
		bool operator==(const State& a) const { return pc == a.pc && nextPc == a.nextPc; }
		bool operator<(const State& a) const { return pc < a.pc || (pc == a.pc && nextPc < a.nextPc); }
		
		friend size_t GetHash(const State& a)  { return GetHash((uint64_t) a.nextPc << 32 | a.pc); }
	};
	
	template<size_t N> struct UpdateStateHelper
	{
		static uint32_t UpdateState(uint32_t c, uint32_t oldState, const void* data);
	};

	template<typename LOOKUP_TYPE, size_t N> struct ProcessHelper
	{
		static void PrepareData(DataBlock& dataBlock, const Table<uint32_t>& stateMaskList, uint32_t characterMask[257]);
		template<bool IS_ANCHORED> static const void* MatchFunction( uint32_t matchStateMask, const void* data, const void* stopSearch, const void* startSearch);
	};
	
	static const int END_OF_INPUT = 256;
	static const uint32_t STARTING_STATE = 1;
	
	void Set(const void* data, size_t length);
	
	static const int MAXIMUM_NUMBER_OF_BITS = 32;
	static void DetermineStateList(Table<State>& stateList, const ByteCodeHeader* header);
	
	typedef Map<State, uint32_t> StateToMaskMap;
	
	static void BuildStateToMaskMap(StateToMaskMap& stateToMaskMap, const Table<State>& stateInstructionList);

	// The processing determines the state bits *between* instructions.
	// This means that byte-jump-* instructions need to be split into separate conceptual states.
	void ProcessNext(uint32_t& stateBits, Table<bool>& progressChecks, const PatternData& patternData, uint32_t pc, uint32_t stateIndex, const StateToMaskMap& stateToMaskMap, int c);
};

template<typename LOOKUP_TYPE, size_t N>
void BitFieldGlushkovNfaReverseProcessor::ProcessHelper<LOOKUP_TYPE, N>::PrepareData(DataBlock& dataBlock, const Table<uint32_t>& stateMaskList, uint32_t characterMask[257])
{
	dataBlock.SetCount(sizeof(LOOKUP_TYPE)*256*N + 256*sizeof(LOOKUP_TYPE));
	LOOKUP_TYPE (*p)[N][256] = (LOOKUP_TYPE (*)[N][256]) dataBlock.GetData();
	LOOKUP_TYPE* pCharacterMask = (LOOKUP_TYPE*) (p+1);
	
	for(size_t i = 0; i < N; ++i)
	{
		uint32_t masks[8];
		for(int j = 0; j < 8; ++j)
		{
			masks[j] = i*8 + j < stateMaskList.GetCount() ? stateMaskList[i*8+j] : 0;
		}
		
		for(int j = 0; j < 256; ++j)
		{
			uint32_t value = 0;
			if(j & 1) value |= masks[0];
			if(j & 2) value |= masks[1];
			if(j & 4) value |= masks[2];
			if(j & 8) value |= masks[3];
			if(j & 0x10) value |= masks[4];
			if(j & 0x20) value |= masks[5];
			if(j & 0x40) value |= masks[6];
			if(j & 0x80) value |= masks[7];
			(*p)[i][j] = value;
		}
	}
	for(int c = 0; c < 256; ++c)
	{
		pCharacterMask[c] = characterMask[c];
	}
}

template<>
JINLINE uint32_t BitFieldGlushkovNfaReverseProcessor::UpdateStateHelper<1>::UpdateState(uint32_t c, uint32_t oldState, const void* data)
{
	const uint8_t (*LOOKUP)[1][256] = (uint8_t (*)[1][256]) data;
	const uint8_t* CHARACTER_MASK = (const uint8_t*) (LOOKUP+1);
	return (*LOOKUP)[0][oldState] & CHARACTER_MASK[c];
}

template<>
JINLINE uint32_t BitFieldGlushkovNfaReverseProcessor::UpdateStateHelper<2>::UpdateState(uint32_t c, uint32_t oldState, const void* data)
{
	const uint16_t (*LOOKUP)[2][256] = (uint16_t (*)[2][256]) data;
	const uint16_t* CHARACTER_MASK = (const uint16_t*) (LOOKUP+1);
	return ((*LOOKUP)[0][oldState & 0xff]
			  | (*LOOKUP)[1][oldState >> 8])
			& CHARACTER_MASK[c];
}

template<>
JINLINE uint32_t BitFieldGlushkovNfaReverseProcessor::UpdateStateHelper<3>::UpdateState(uint32_t c, uint32_t oldState, const void* data)
{
	const uint32_t (*LOOKUP)[3][256] = (uint32_t (*)[3][256]) data;
	const uint32_t* CHARACTER_MASK = (const uint32_t*) (LOOKUP+1);
	return ((*LOOKUP)[0][oldState & 0xff]
			  | (*LOOKUP)[1][oldState >> 8 & 0xff]
			  | (*LOOKUP)[2][oldState >> 16])
			& CHARACTER_MASK[c];
}

template<>
JINLINE uint32_t BitFieldGlushkovNfaReverseProcessor::UpdateStateHelper<4>::UpdateState(uint32_t c, uint32_t oldState, const void* data)
{
	const uint32_t (*LOOKUP)[4][256] = (uint32_t (*)[4][256]) data;
	const uint32_t* CHARACTER_MASK = (const uint32_t*) (LOOKUP+1);
	return ((*LOOKUP)[0][oldState & 0xff]
			  | (*LOOKUP)[1][oldState >> 8 & 0xff]
			  | (*LOOKUP)[2][oldState >> 16 & 0xff]
			  | (*LOOKUP)[3][oldState >> 24])
			& CHARACTER_MASK[c];
}

template<typename LOOKUP_TYPE, size_t N>
template<bool IS_ANCHORED>
const void* BitFieldGlushkovNfaReverseProcessor::ProcessHelper<LOOKUP_TYPE, N>::MatchFunction(uint32_t matchStateMask, const void* data, const void* stopSearch, const void* startSearch)
{
	uint32_t state = STARTING_STATE;
	const uint8_t* p = (const uint8_t*) startSearch;
	const uint8_t* result = nullptr;
	
	while(p > stopSearch)
	{
		if(!IS_ANCHORED)
		{
			if(state & matchStateMask) result = p;
		}
		if(state == 0)
		{
			JASSERT(result != nullptr);
			return result;
		}
		uint8_t c = *--p;
		state = UpdateStateHelper<N>::UpdateState(c, state, data);
	}

	if(state & matchStateMask) return p;
	JASSERT(result != nullptr);
	return result;
}

//============================================================================

BitFieldGlushkovNfaReverseProcessor::BitFieldGlushkovNfaReverseProcessor(const void* data, size_t length)
{
	Set(data, length);
}

// This builds all of the lookup tables.
void BitFieldGlushkovNfaReverseProcessor::Set(const void* data, size_t length)
{
	ByteCodeHeader* header = (ByteCodeHeader*) data;
	PatternData patternData(header->GetReverseProgram());

	// First step through pattern and determine state instructions
	Table<State> stateList(MAXIMUM_NUMBER_OF_BITS);
	DetermineStateList(stateList, header);

	StateToMaskMap stateToMaskMap(stateList.GetCount());
	BuildStateToMaskMap(stateToMaskMap, stateList);
	
	Table<bool> progressChecks;
	progressChecks.SetCount(header->numberOfProgressChecks);
	progressChecks.SetAll(false);
	
	uint32_t characterMask[257];
	memset(characterMask, 0, sizeof(characterMask));
	
	Table<uint32_t> stateMask;
	stateMask.SetCount(stateList.GetCount());
	stateMask.SetAll(0);
	matchStateMask = 0;

	for(size_t i = 0; i < stateList.GetCount(); ++i)
	{
		for(int c = 0; c < 257; ++c)
		{
			uint32_t state = 0;
			progressChecks.SetAll(false);
			ProcessNext(state, progressChecks, patternData, stateList[i].nextPc, i, stateToMaskMap, c);
			
			// states is a bitmask of states reachable by character c from state i
			characterMask[c] |= state;
			stateMask[i] |= state;
		}
	}

#define HANDLE_CASE(N, T)	\
	case N:	\
		ProcessHelper<T, N>::PrepareData(lookupDataBlock, stateMask, characterMask);		\
		anchoredProcessFunction = &ProcessHelper<T, N>::MatchFunction<true>;	\
		unanchoredProcessFunction = &ProcessHelper<T, N>::MatchFunction<false>;	\
		break
	
	switch((stateList.GetCount()+7) / 8)
	{
	HANDLE_CASE(1, uint8_t);
	HANDLE_CASE(2, uint16_t);
	HANDLE_CASE(3, uint32_t);
	HANDLE_CASE(4, uint32_t);
		
	default:
		JERROR("Unhandled list size!");
	}
}

void BitFieldGlushkovNfaReverseProcessor::ProcessNext(uint32_t& stateBits, Table<bool>& progressChecks, const PatternData& patternData, uint32_t pc, uint32_t stateIndex, const StateToMaskMap& stateToMaskMap, int c)
{
Loop:
	const ByteCodeInstruction& instruction = patternData[pc];
	switch(instruction.type)
	{
	case InstructionType::AnyByte:
	case InstructionType::AdvanceByte:
		JASSERT(stateToMaskMap.Contains(State{pc, pc+1}));
		stateBits |= stateToMaskMap[State{pc, pc+1}];
		break;
			
	case InstructionType::Byte:
		if(instruction.data == c)
		{
			JASSERT(stateToMaskMap.Contains(State{pc, pc+1}));
			stateBits |= stateToMaskMap[State{pc, pc+1}];
		}
		break;
			
	case InstructionType::ByteEitherOf2:
		if((instruction.data & 0xff) == c ||
		   ((instruction.data >> 8) & 0xff) == c)
		{
			JASSERT(stateToMaskMap.Contains(State{pc, pc+1}));
			stateBits |= stateToMaskMap[State{pc, pc+1}];
		}
		break;
		
	case InstructionType::ByteEitherOf3:
		if((instruction.data & 0xff) == c ||
		   ((instruction.data >> 8) & 0xff) == c ||
		   ((instruction.data >> 16) & 0xff) == c)
		{
			JASSERT(stateToMaskMap.Contains(State{pc, pc+1}));
			stateBits |= stateToMaskMap[State{pc, pc+1}];
		}
		break;
		
	case InstructionType::ByteRange:
		{
			unsigned char low = instruction.data & 0xff;
			unsigned char high = (instruction.data >> 8) & 0xff;
			uint32_t delta = high - low;
			if(uint32_t(c - low) <= delta)
			{
				JASSERT(stateToMaskMap.Contains(State{pc, pc+1}));
				stateBits |= stateToMaskMap[State{pc, pc+1}];
			}
		}
		break;

	case InstructionType::ByteBitMask:
		if(c != END_OF_INPUT)
		{
			const StaticBitTable<256>& data = *patternData.GetData<StaticBitTable<256>>(instruction.data);
			if(data[c])
			{
				JASSERT(stateToMaskMap.Contains(State{pc, pc+1}));
				stateBits |= stateToMaskMap[State{pc, pc+1}];
			}
		}
		break;
			
	case InstructionType::ByteNot:
		if(c != END_OF_INPUT && c != instruction.data)
		{
			JASSERT(stateToMaskMap.Contains(State{pc, pc+1}));
			stateBits |= stateToMaskMap[State{pc, pc+1}];
		}
		break;
			
	case InstructionType::ByteNotEitherOf2:
		if((instruction.data & 0xff) != c &&
		   ((instruction.data >> 8) & 0xff) != c)
		{
			JASSERT(stateToMaskMap.Contains(State{pc, pc+1}));
			stateBits |= stateToMaskMap[State{pc, pc+1}];
		}
		break;
			
	case InstructionType::ByteNotEitherOf3:
		if((instruction.data & 0xff) != c &&
		   ((instruction.data >> 8) & 0xff) != c &&
		   ((instruction.data >> 16) & 0xff) != c)
		{
			JASSERT(stateToMaskMap.Contains(State{pc, pc+1}));
			stateBits |= stateToMaskMap[State{pc, pc+1}];
		}
		break;
			
	case InstructionType::ByteNotRange:
		if(c != END_OF_INPUT)
		{
			unsigned char low = instruction.data & 0xff;
			unsigned char high = (instruction.data >> 8) & 0xff;
			if(c < low || c > high)
			{
				JASSERT(stateToMaskMap.Contains(State{pc, pc+1}));
				stateBits |= stateToMaskMap[State{pc, pc+1}];
			}
		}
		break;
			
	case InstructionType::ByteJumpTable:
		if(c != END_OF_INPUT)
		{
			const ByteCodeJumpTableData* data = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
			uint32_t targetPc = data->pcData[data->jumpTable[c]];
			if(targetPc != TypeData<uint32_t>::Maximum())
			{
				JASSERT(stateToMaskMap.Contains(State{pc, targetPc}));
				stateBits |= stateToMaskMap[State{pc, targetPc}];
			}
		}
		break;

	case InstructionType::ByteJumpMask:
		if(c != END_OF_INPUT)
		{
			const ByteCodeJumpMaskData* data = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
			uint32_t targetPc = data->pcData[data->bitMask[c]];
			if(targetPc != TypeData<uint32_t>::Maximum())
			{
				JASSERT(stateToMaskMap.Contains(State{pc, targetPc}));
				stateBits |= stateToMaskMap[State{pc, targetPc}];
			}
		}
		break;
		
	case InstructionType::ByteJumpRange:
		{
			const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			uint32_t targetPc = data->pcData[data->range.Contains(c)];
			if(targetPc != TypeData<uint32_t>::Maximum())
			{
				JASSERT(stateToMaskMap.Contains(State{pc, targetPc}));
				stateBits |= stateToMaskMap[State{pc, targetPc}];
			}
		}
		break;
			
	case InstructionType::DispatchMask:
		{
			const ByteCodeJumpMaskData* data = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
			pc = data->pcData[c == END_OF_INPUT ? 0 : data->bitMask[c]];
			if(pc == TypeData<uint32_t>::Maximum()) break;
			goto Loop;
		}

	case InstructionType::DispatchTable:
		{
			const ByteCodeJumpTableData* data = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
			pc = data->pcData[c == END_OF_INPUT ? 0 : data->jumpTable[c]];
			if(pc == TypeData<uint32_t>::Maximum()) break;
			goto Loop;
		}
			
	case InstructionType::DispatchRange:
		{
			const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			pc = data->pcData[c == END_OF_INPUT ? 0 : data->range.Contains(c)];
			if(pc == TypeData<uint32_t>::Maximum()) break;
			goto Loop;
		}
		break;
			
	case InstructionType::Fail:
		break;
		
	case InstructionType::FindByte:
		{
			uint32_t targetPc = (instruction.data & 0xff) == c ? instruction.data >> 8 : pc;
			JASSERT(stateToMaskMap.Contains(State{pc, targetPc}));
			stateBits |= stateToMaskMap[State{pc, targetPc}];
		}
		break;
		
	case InstructionType::Jump:
		pc = instruction.data;
		goto Loop;
		
	case InstructionType::Match:
		matchStateMask |= 1 << stateIndex;
		break;
			
	case InstructionType::ProgressCheck:
		if(progressChecks[instruction.data]) break;
		progressChecks[instruction.data] = true;
		++pc;
		goto Loop;
			
	case InstructionType::Split:
		{
			const ByteCodeSplitData* splitData = patternData.GetData<ByteCodeSplitData>(instruction.data);
			uint32_t numberOfTargets = splitData->numberOfTargets;
			for(uint32_t i = 0; i < numberOfTargets-1; ++i)
			{
				ProcessNext(stateBits, progressChecks, patternData, splitData->targetList[i], stateIndex, stateToMaskMap, c);
			}
			pc = splitData->targetList[numberOfTargets-1];
			goto Loop;
		}
		
	case InstructionType::SplitMatch:
		{
			const uint32_t* splitData = patternData.GetData<uint32_t>(instruction.data);
			ProcessNext(stateBits, progressChecks, patternData, splitData[0], stateIndex, stateToMaskMap, c);
			pc = splitData[1];
			goto Loop;
		}
		
	case InstructionType::SplitNextN:
	case InstructionType::SplitNextMatchN:
	case InstructionType::SplitNNext:
	case InstructionType::SplitNMatchNext:
		ProcessNext(stateBits, progressChecks, patternData, instruction.data, stateIndex, stateToMaskMap, c);
		++pc;
		goto Loop;
			
	case InstructionType::AssertStartOfInput:
	case InstructionType::AssertEndOfInput:
	case InstructionType::AssertStartOfLine:
	case InstructionType::AssertEndOfLine:
	case InstructionType::AssertWordBoundary:
	case InstructionType::AssertNotWordBoundary:
	case InstructionType::AssertStartOfSearch:
	case InstructionType::AssertRecurseValue:
	case InstructionType::BackReference:
	case InstructionType::Call:
	case InstructionType::Possess:
	case InstructionType::PropagateBackwards:
	case InstructionType::Recurse:
	case InstructionType::ReturnIfRecurseValue:
	case InstructionType::SaveNoRecurse:
	case InstructionType::Save:
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
	case InstructionType::SearchByteRange:
	case InstructionType::SearchByteRangePair:
	case InstructionType::SearchBoyerMoore:
	case InstructionType::SearchShiftOr:
	case InstructionType::SearchByteTriplet:
	case InstructionType::SearchByteTriplet2:
	case InstructionType::StepBack:
	case InstructionType::Success:
		JERROR("Unexpected instruction");
	}
}

void BitFieldGlushkovNfaReverseProcessor::DetermineStateList(Table<State>& stateList, const ByteCodeHeader* header)
{
	PatternData patternData(header->GetReverseProgram());
	uint32_t numberOfInstructions = header->numberOfReverseInstructions;

	stateList.Append(State{static_cast<uint32_t>(-1),header->reverseMatchStartingInstruction});
	for(uint32_t i = 0; i < numberOfInstructions; ++i)
	{
		const ByteCodeInstruction& instruction = patternData[i];
		switch(instruction.type)
		{
		case InstructionType::AnyByte:
		case InstructionType::AdvanceByte:
		case InstructionType::Byte:
		case InstructionType::ByteEitherOf2:
		case InstructionType::ByteEitherOf3:
		case InstructionType::ByteRange:
		case InstructionType::ByteBitMask:
		case InstructionType::ByteNot:
		case InstructionType::ByteNotEitherOf2:
		case InstructionType::ByteNotEitherOf3:
		case InstructionType::ByteNotRange:
			stateList.Append(State{i, i+1});
			break;

		case InstructionType::FindByte:
			stateList.Append(State{i, i});
			stateList.Append(State{i, uint32_t(instruction.data>>8)});
			break;

		case InstructionType::ByteJumpTable:
			{
				const ByteCodeJumpTableData* data = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
				for(uint32_t j = 0; j < data->numberOfTargets; ++j)
				{
					uint32_t targetPc = data->pcData[j];
					if(targetPc != TypeData<uint32_t>::Maximum())
					{
						stateList.Append(State{i, targetPc});
					}
				}
			}
			break;
				
		case InstructionType::ByteJumpMask:
			{
				const ByteCodeJumpMaskData* data = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
				for(uint32_t j = 0; j < 2; ++j)
				{
					uint32_t targetPc = data->pcData[j];
					if(targetPc != TypeData<uint32_t>::Maximum())
					{
						stateList.Append(State{i, targetPc});
					}
				}
			}
			break;
				
		case InstructionType::ByteJumpRange:
			{
				const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
				for(uint32_t j = 0; j < 2; ++j)
				{
					uint32_t targetPc = data->pcData[j];
					if(targetPc != TypeData<uint32_t>::Maximum())
					{
						stateList.Append(State{i, targetPc});
					}
				}
			}
			break;

		case InstructionType::DispatchMask:
		case InstructionType::DispatchRange:
		case InstructionType::DispatchTable:
		case InstructionType::Fail:
		case InstructionType::Jump:
		case InstructionType::Match:
		case InstructionType::ProgressCheck:
		case InstructionType::Split:
		case InstructionType::SplitMatch:
		case InstructionType::SplitNextN:
		case InstructionType::SplitNextMatchN:
		case InstructionType::SplitNNext:
		case InstructionType::SplitNMatchNext:
			break;

		case InstructionType::AssertStartOfInput:
		case InstructionType::AssertEndOfInput:
		case InstructionType::AssertStartOfLine:
		case InstructionType::AssertEndOfLine:
		case InstructionType::AssertWordBoundary:
		case InstructionType::AssertNotWordBoundary:
		case InstructionType::AssertStartOfSearch:
			// Cannot be handled by this processor (yet?)
			stateList.SetCount(0);
			return;
				
		case InstructionType::AssertRecurseValue:
		case InstructionType::BackReference:
		case InstructionType::Call:
		case InstructionType::Possess:
		case InstructionType::PropagateBackwards:
		case InstructionType::Recurse:
		case InstructionType::ReturnIfRecurseValue:
		case InstructionType::Save:
		case InstructionType::SaveNoRecurse:
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
		case InstructionType::SearchByteRange:
		case InstructionType::SearchByteRangePair:
		case InstructionType::SearchBoyerMoore:
		case InstructionType::SearchShiftOr:
		case InstructionType::SearchByteTriplet:
		case InstructionType::SearchByteTriplet2:
		case InstructionType::StepBack:
		case InstructionType::Success:
			JERROR("Unexpected instruction");
			stateList.SetCount(0);
			return;
		}
	}
}

void BitFieldGlushkovNfaReverseProcessor::BuildStateToMaskMap(StateToMaskMap& stateToMaskMap, const Table<State>& stateList)
{
	for(size_t i = 0; i < stateList.GetCount(); ++i)
	{
		stateToMaskMap.Insert(stateList[i], 1<<i);
	}
}

bool BitFieldGlushkovNfaReverseProcessor::CanUse(const void* data, size_t length)
{
	ByteCodeHeader* header = (ByteCodeHeader*) data;
	Table<State> stateList(MAXIMUM_NUMBER_OF_BITS+1);
	DetermineStateList(stateList, header);
	return stateList.GetCount() > 0 && stateList.GetCount() <= MAXIMUM_NUMBER_OF_BITS;
}

bool ReverseProcessor::CanUseBitFieldGlushkovNfaReverseProcessor(const void* data, size_t length)
{
	return BitFieldGlushkovNfaReverseProcessor::CanUse(data, length);
}

//============================================================================

const void* BitFieldGlushkovNfaReverseProcessor::Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char **captures, bool matchIsAnchored) const
{
	if(matchIsAnchored)
	{
		return (*anchoredProcessFunction)(matchStateMask, lookupDataBlock.GetData(), (uint8_t*) data+startOffset, matchEnd);
	}
	else
	{
		return (*unanchoredProcessFunction)(matchStateMask, lookupDataBlock.GetData(), (uint8_t*) data+startOffset, matchEnd);
	}
}

//============================================================================

ReverseProcessor* ReverseProcessor::CreateBitFieldGlushkovNfaReverseProcessor(const void* data, size_t length)
{
	return new BitFieldGlushkovNfaReverseProcessor(data, length);
}

//============================================================================
