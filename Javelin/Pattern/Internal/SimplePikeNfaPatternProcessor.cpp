//============================================================================

#include "Javelin/Pattern/Internal/PatternProcessor.h"
#include "Javelin/Container/BitTable.h"
#include "Javelin/Container/EnumSet.h"
#include "Javelin/Container/StackBuffer.h"
#include "Javelin/Pattern/Internal/PatternByteCode.h"
#include "Javelin/Stream/ICharacterWriter.h"
#include "Javelin/Template/Memory.h"

#include "Javelin/Stream/StandardWriter.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class SimplePikeNfaPatternProcessor final : public PatternProcessor
{
public:
	SimplePikeNfaPatternProcessor(const void* data, size_t length);
	
	virtual const void* FullMatch(const void* data, size_t length) const;
	virtual const void* FullMatch(const void* data, size_t length, const char **captures) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const;
	virtual Interval<const void*> LocatePartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const;
	
private:
	bool				matchRequiresEndOfInput;
	PatternData			patternData;
	uint32_t			numberOfCaptures;
	uint32_t			numberOfInstructions;
	uint32_t			partialMatchStartingInstruction;
	uint32_t			fullMatchStartingInstruction;
	ExpandedJumpTables	expandedJumpTables;
	
	void Set(const void* data, size_t length);
	
	struct ThreadState;
	struct StateList;
	struct ProcessData;
	
	void ProcessStateList(StateList* currentState, StateList* nextState, const unsigned char* &p, ProcessData& processData) const;
	const void* Process(const unsigned char* pIn, ProcessData& processData) const;
};

struct SimplePikeNfaPatternProcessor::ThreadState
{
	uint32_t				pc;
	const unsigned char*	captures[4];
};

//============================================================================

struct SimplePikeNfaPatternProcessor::ProcessData
{
	const bool					isFullMatch;
	const void*					match = nullptr;
	const unsigned char *const 	pStart;
	const unsigned char *const 	pSearchStart;
	const unsigned char *const 	pEnd;
	uint32_t const				startingInstruction;
	ThreadState					captures;
	const PatternData			patternData;
	StateList*					currentState;
	const ExpandedJumpTables&	expandedJumpTables;
	
	ProcessData(bool aIsFullMatch, const void* data, size_t length, size_t offset, uint32_t aStartingInstruction, const SimplePikeNfaPatternProcessor& processor)
	: isFullMatch(aIsFullMatch),
	  pStart((const unsigned char*) data),
	  pSearchStart((const unsigned char*) data + offset),
	  pEnd((const unsigned char*) data + length),
	  startingInstruction(aStartingInstruction),
	  patternData(processor.patternData),
	  expandedJumpTables(processor.expandedJumpTables)
	{
		memset(captures.captures, 0, sizeof(captures.captures));
	}
};

//============================================================================

struct SimplePikeNfaPatternProcessor::StateList
{
	// StateList is a dynamically sized:
	// lastUpdatePointers[numberOfStates]
	// and threadStateList[numberOfStates]
	
	ThreadState*	updateThread;
	ThreadState*	threadList;
	uint32_t*		updateCache;
	uint64_t		numberOfStates;			// Becomes (1<<32) to stop any further states from being added
	uint32_t		stateList[1];			// List of already visited states

	static uint32_t GetSize(uint32_t maximumNumberOfThreads) 				{ return (uint32_t) (sizeof(StateList) + maximumNumberOfThreads*(sizeof(uint32_t) + sizeof(ThreadState))); }
	static uint32_t GetUpdateCacheSize(uint32_t maximumNumberOfThreads) 	{ return (maximumNumberOfThreads*(sizeof(uint32_t)) + 7) & -8; }

	void Dump(ICharacterWriter& output);
	
	bool HasNoThreads() const { return updateThread == threadList; }
	
	JINLINE bool Check(uint32_t pc)
	{
		uint32_t index = updateCache[pc];
		if(index < numberOfStates)
		{
			if(numberOfStates >= 0x100000000ll || stateList[index] == pc) return false;
		}
		updateCache[pc] = uint32_t(numberOfStates);
		stateList[numberOfStates++] = pc;
		return true;
	}

	void Prepare(uint32_t* aUpdateCache, uint32_t maximumNumberOfThreads)
	{
		updateCache = aUpdateCache;
		threadList = (ThreadState*) &stateList[maximumNumberOfThreads];
		ResetThreads();
	}
	
	void ResetThreads()
	{
		numberOfStates  = 0;
		updateThread 	= threadList;
	}
	
	void AddStartingInstruction(uint32_t pc, const unsigned char* p, ProcessData& processData)
	{
		ThreadState dummy;
		memset(dummy.captures, 0, sizeof(dummy.captures));
		AddThread(pc, dummy, p, false, processData);
	}

	void AddThread(uint32_t pc, const ThreadState& threadState)
	{
		updateThread->pc = pc;
		memcpy(updateThread->captures, threadState.captures, sizeof(threadState.captures));
		++updateThread;
	}

	void AddThread(uint32_t pc, ThreadState& threadState, const unsigned char* p, bool recurseSave, ProcessData& processData);
};

void SimplePikeNfaPatternProcessor::StateList::AddThread(uint32_t pc, ThreadState& threadState, const unsigned char* p, bool recurseSave, ProcessData& processData)
{
Loop:
	if(!Check(pc)) return;

	const ByteCodeInstruction instruction = processData.patternData[pc];
	
	switch(instruction.type)
	{
	case InstructionType::AssertStartOfInput:
		if(p == processData.pStart)	goto ProcessNextInstruction;
		break;
		
	case InstructionType::AssertEndOfInput:
		if(p == processData.pEnd) goto ProcessNextInstruction;
		break;
		
	case InstructionType::AssertStartOfLine:
		if(p == processData.pStart || p[-1] == '\n') goto ProcessNextInstruction;
		break;
		
	case InstructionType::AssertEndOfLine:
		if(p == processData.pEnd || *p == '\n') goto ProcessNextInstruction;
		break;
		
	case InstructionType::AssertWordBoundary:
		if(Character::IsWordCharacter(*p))
		{
			if(p == processData.pStart) goto ProcessNextInstruction;
			if(!Character::IsWordCharacter(p[-1])) goto ProcessNextInstruction;
		}
		else
		{
			if(p != processData.pStart
			   && Character::IsWordCharacter(p[-1])) goto ProcessNextInstruction;
		}
		break;
		
	case InstructionType::AssertNotWordBoundary:
		if(Character::IsWordCharacter(*p))
		{
			if(p != processData.pStart
			   && Character::IsWordCharacter(p[-1])) goto ProcessNextInstruction;
		}
		else
		{
			if(p == processData.pStart) goto ProcessNextInstruction;
			if(!Character::IsWordCharacter(p[-1])) goto ProcessNextInstruction;
		}
		break;
		
	case InstructionType::AssertStartOfSearch:
		if(p != processData.pSearchStart) break;
		goto ProcessNextInstruction;
		
	case InstructionType::DispatchTable:
		if(p == processData.pEnd)
		{
			const ByteCodeJumpTableData* data = processData.patternData.GetData<ByteCodeJumpTableData>(instruction.data);
			pc = data->pcData[0];
		}
		else
		{
			const uint32_t* data = processData.expandedJumpTables.GetJumpTable(pc);
			pc = data[*p];
		}
		if(pc != TypeData<uint32_t>::Maximum()) goto Loop;
		break;
		
	case InstructionType::DispatchMask:
		if(p == processData.pEnd)
		{
			const ByteCodeJumpMaskData* data = processData.patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
			pc = data->pcData[0];
		}
		else
		{
			const uint32_t* data = processData.expandedJumpTables.GetJumpTable(pc);
			pc = data[*p];
		}
		if(pc != TypeData<uint32_t>::Maximum()) goto Loop;
		break;
		
	case InstructionType::DispatchRange:
		{
			const ByteCodeJumpRangeData* data = processData.patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			if(p == processData.pEnd)
			{
				pc = data->pcData[0];
			}
			else
			{
				pc = data->pcData[data->range.Contains(*p)];
			}
			if(pc != TypeData<uint32_t>::Maximum()) goto Loop;
			break;
		}
			
	case InstructionType::Fail:
		break;
		
	case InstructionType::Jump:
		pc = instruction.data;
		goto Loop;
		
	case InstructionType::Match:
		if(processData.isFullMatch && p != processData.pEnd) break;
		processData.match = p;
		processData.captures = threadState;
		// Prevent processing of lower priority threads
		numberOfStates = 0x100000000ll;
		processData.currentState->ResetThreads();
		return;
		
	case InstructionType::Save:
	case InstructionType::SaveNoRecurse:
		{
			uint32_t saveIndex = instruction.data & 0xff;
			uint32_t saveOffset = instruction.data >> 8;
			
			if(recurseSave)
			{
				const unsigned char* old = threadState.captures[saveIndex];
				threadState.captures[saveIndex] = p-saveOffset;
				
				AddThread(pc+1, threadState, p, true, processData);
				threadState.captures[saveIndex] = old;
			}
			else
			{
				threadState.captures[saveIndex] = p-saveOffset;
				
				goto ProcessNextInstruction;
			}
		}
		break;
			
	case InstructionType::SearchByte:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			unsigned char c = instruction.data & 0xff;
			unsigned offset = instruction.data >> 8;
			if(p+offset >= processData.pEnd) return;

			if(p[offset] == c) AddThread(pc+1, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		break;
			
	case InstructionType::SearchByteEitherOf2:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(p+data->offset >= processData.pEnd) return;
			
			if(p[data->offset] == data->bytes[0]
			   || p[data->offset] == data->bytes[1])
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;
			
	case InstructionType::SearchByteEitherOf3:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(p+data->offset >= processData.pEnd) return;
			
			if(p[data->offset] == data->bytes[0]
			   || p[data->offset] == data->bytes[1]
			   || p[data->offset] == data->bytes[2])
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;
			
	case InstructionType::SearchByteEitherOf4:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(p+data->offset >= processData.pEnd) return;
			
			if(p[data->offset] == data->bytes[0]
			   || p[data->offset] == data->bytes[1]
			   || p[data->offset] == data->bytes[2]
			   || p[data->offset] == data->bytes[3])
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;

	case InstructionType::SearchByteEitherOf5:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(p+data->offset >= processData.pEnd) return;
			
			if(p[data->offset] == data->bytes[0]
			   || p[data->offset] == data->bytes[1]
			   || p[data->offset] == data->bytes[2]
			   || p[data->offset] == data->bytes[3]
			   || p[data->offset] == data->bytes[4])
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;

	case InstructionType::SearchByteEitherOf6:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(p+data->offset >= processData.pEnd) return;
			
			if(p[data->offset] == data->bytes[0]
			   || p[data->offset] == data->bytes[1]
			   || p[data->offset] == data->bytes[2]
			   || p[data->offset] == data->bytes[3]
			   || p[data->offset] == data->bytes[4]
			   || p[data->offset] == data->bytes[5])
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;

	case InstructionType::SearchByteEitherOf7:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(p+data->offset >= processData.pEnd) return;
			
			if(p[data->offset] == data->bytes[0]
			   || p[data->offset] == data->bytes[1]
			   || p[data->offset] == data->bytes[2]
			   || p[data->offset] == data->bytes[3]
			   || p[data->offset] == data->bytes[4]
			   || p[data->offset] == data->bytes[5]
			   || p[data->offset] == data->bytes[6])
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;

	case InstructionType::SearchByteEitherOf8:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(p+data->offset >= processData.pEnd) return;
			
			if(p[data->offset] == data->bytes[0]
			   || p[data->offset] == data->bytes[1]
			   || p[data->offset] == data->bytes[2]
			   || p[data->offset] == data->bytes[3]
			   || p[data->offset] == data->bytes[4]
			   || p[data->offset] == data->bytes[5]
			   || p[data->offset] == data->bytes[6]
			   || p[data->offset] == data->bytes[7])
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;
			
	case InstructionType::SearchBytePair:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(p+data->offset+1 >= processData.pEnd) return;
			
			if(p[data->offset] == data->bytes[0]
			   && p[data->offset+1] == data->bytes[1])
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;
			
	case InstructionType::SearchBytePair2:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(p+data->offset+1 >= processData.pEnd) return;
			
			if((p[data->offset] == data->bytes[0] || p[data->offset] == data->bytes[1])
			   && (p[data->offset+1] == data->bytes[2] || p[data->offset+1] == data->bytes[3]))
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;
			
	case InstructionType::SearchBytePair3:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(p+data->offset+1 >= processData.pEnd) return;
			
			if((p[data->offset] == data->bytes[0] || p[data->offset] == data->bytes[1] || p[data->offset] == data->bytes[2])
			   && (p[data->offset+1] == data->bytes[3] || p[data->offset+1] == data->bytes[4] || p[data->offset+1] == data->bytes[5]))
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;
			
	case InstructionType::SearchBytePair4:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(p+data->offset+1 >= processData.pEnd) return;
			
			if((p[data->offset] == data->bytes[0] || p[data->offset] == data->bytes[1] || p[data->offset] == data->bytes[2] || p[data->offset] == data->bytes[3])
			   && (p[data->offset+1] == data->bytes[4] || p[data->offset+1] == data->bytes[5] || p[data->offset+1] == data->bytes[6] || p[data->offset+1] == data->bytes[7]))
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;
			
	case InstructionType::SearchByteRange:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			if(p[data->offset] >= data->bytes[0] && p[data->offset] <= data->bytes[1])
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;
			
	case InstructionType::SearchByteRangePair:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(p+data->offset+1 >= processData.pEnd) return;
			
			if(p[data->offset] >= data->bytes[0]
				&& p[data->offset] <= data->bytes[1]
			    && p[data->offset+1] >= data->bytes[2]
			    && p[data->offset+1] <= data->bytes[3])
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;
			
	case InstructionType::SearchByteTriplet:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(p+data->offset+2 >= processData.pEnd) return;
			
			if(p[data->offset] == data->bytes[0]
			   	&& p[data->offset+1] == data->bytes[1]
				&& p[data->offset+2] == data->bytes[2])
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;
			
	case InstructionType::SearchByteTriplet2:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchByteData* data = processData.patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			if(p+data->offset+1 >= processData.pEnd) return;
			
			if((p[data->offset] == data->bytes[0] || p[data->offset] == data->bytes[1])
			   && (p[data->offset+1] == data->bytes[2] || p[data->offset+1] == data->bytes[3])
			   && (p[data->offset+2] == data->bytes[4] || p[data->offset+2] == data->bytes[5]))
			{
				AddThread(pc+1, threadState, p, true, processData);
			}
			else AddThread(pc, threadState);
		}
		break;
			
	case InstructionType::SearchBoyerMoore:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchData* searchData = processData.patternData.GetData<ByteCodeSearchData>(instruction.data);
			if(p+searchData->length >= processData.pEnd) return;

			if(searchData->data[p[searchData->length]] == 0) AddThread(pc+1, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		break;
			
	case InstructionType::SearchShiftOr:
		if(processData.patternData[pc+1].type == InstructionType::PropagateBackwards)
		{
			const StaticBitTable<256>& bitMask = *processData.patternData.GetData<StaticBitTable<256>>(processData.patternData[pc+1].data);
			if(bitMask[*p]) AddThread(pc+2, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		else
		{
			const ByteCodeSearchData* searchData = processData.patternData.GetData<ByteCodeSearchData>(instruction.data);
			if(p+searchData->length >= processData.pEnd) return;
			
			uint32_t mask = -1;
			for(uint32_t i = 0; i <= searchData->length; ++i)
			{
				mask = (mask<<1) | searchData->data[p[i]];
			}
			if(((mask >> searchData->length) & 1) == 0) AddThread(pc+1, threadState, p, true, processData);
			else AddThread(pc, threadState);
		}
		break;
			
	case InstructionType::Split:
		{
			const ByteCodeSplitData* splitData = processData.patternData.GetData<ByteCodeSplitData>(instruction.data);
			
			uint32_t numberOfTargets = splitData->numberOfTargets;
			for(uint32_t i = 0; i < numberOfTargets-1; ++i)
			{
				AddThread(splitData->targetList[i], threadState, p, true, processData);
			}
			pc = splitData->targetList[numberOfTargets-1];
			goto Loop;
		}
		
	case InstructionType::SplitMatch:
		{
			const uint32_t* splitData = processData.patternData.GetData<uint32_t>(instruction.data);
			pc = (!processData.isFullMatch || p == processData.pEnd) ? splitData[0] : splitData[1];
			goto Loop;
		}
			
	case InstructionType::SplitNextN:
		AddThread(pc+1, threadState, p, true, processData);
		pc = instruction.data;
		goto Loop;
		
	case InstructionType::SplitNNext:
		AddThread(instruction.data, threadState, p, true, processData);
		goto ProcessNextInstruction;
		
	case InstructionType::SplitNextMatchN:
		pc = (!processData.isFullMatch || p == processData.pEnd) ? pc+1 : instruction.data;
		goto Loop;
			
	case InstructionType::SplitNMatchNext:
		pc = (!processData.isFullMatch || p == processData.pEnd) ? instruction.data : pc+1;
		goto Loop;
		
	default:
		AddThread(pc, threadState);
		break;
			
	case InstructionType::ProgressCheck:
	case InstructionType::PropagateBackwards:
	ProcessNextInstruction:
		++pc;
		goto Loop;
	}
}

void SimplePikeNfaPatternProcessor::StateList::Dump(ICharacterWriter& output)
{
	output.PrintF("StateList:");
	
	for(ThreadState* thread = threadList; thread < updateThread; ++thread)
	{
		output.PrintF(" %u", thread->pc);
	}
	output.PrintF("\n");
}

//============================================================================

SimplePikeNfaPatternProcessor::SimplePikeNfaPatternProcessor(const void* data, size_t length)
{
	Set(data, length);
}

void SimplePikeNfaPatternProcessor::Set(const void* data, size_t length)
{
	ByteCodeHeader* header = (ByteCodeHeader*) data;
	numberOfCaptures = header->numberOfCaptures*2;
	matchRequiresEndOfInput = header->flags.matchRequiresEndOfInput;
	numberOfInstructions = (header->numberOfInstructions + 1) & -2;
	partialMatchStartingInstruction = header->partialMatchStartingInstruction;
	fullMatchStartingInstruction = header->fullMatchStartingInstruction;
	patternData.p = (const unsigned char*) header->GetForwardProgram();
	expandedJumpTables.Set(patternData, numberOfInstructions);
}

//============================================================================

void SimplePikeNfaPatternProcessor::ProcessStateList(StateList* currentState, StateList* nextState, const unsigned char* &p, ProcessData& processData) const
{
	for(ThreadState* thread = currentState->threadList; thread < currentState->updateThread; ++thread)
	{
		ThreadState& threadState = *thread;
		uint32_t pc = threadState.pc;
		
	Loop:
		const ByteCodeInstruction instruction = patternData[pc];
		
		switch(instruction.type)
		{
		case InstructionType::AdvanceByte:
		case InstructionType::AnyByte:
			nextState->AddThread(pc+1, threadState, p+1, false, processData);
			break;
			
		case InstructionType::Byte:
			if(instruction.data == *p)
			{
				nextState->AddThread(pc+1, threadState, p+1, false, processData);
			}
			break;
			
		case InstructionType::ByteEitherOf2:
			if((instruction.data & 0xff) == *p ||
			   ((instruction.data >> 8) & 0xff) == *p)
			{
				nextState->AddThread(pc+1, threadState, p+1, false, processData);
			}
			break;
			
		case InstructionType::ByteEitherOf3:
			if((instruction.data & 0xff) == *p ||
			   ((instruction.data >> 8) & 0xff) == *p ||
			   ((instruction.data >> 16) & 0xff) == *p)
			{
				nextState->AddThread(pc+1, threadState, p+1, false, processData);
			}
			break;
			
		case InstructionType::ByteRange:
			{
				unsigned char low = instruction.data & 0xff;
				unsigned char high = (instruction.data >> 8) & 0xff;
				uint32_t delta = high - low;
				if(uint32_t(*p - low) <= delta)
				{
					nextState->AddThread(pc+1, threadState, p+1, false, processData);
				}
			}
			break;
			
		case InstructionType::ByteBitMask:
			{
				const StaticBitTable<256>& data = *patternData.GetData<StaticBitTable<256>>(instruction.data);
				if(data[*p])
				{
					nextState->AddThread(pc+1, threadState, p+1, false, processData);
				}
			}
			break;

		case InstructionType::ByteJumpTable:
		case InstructionType::ByteJumpMask:
			{
				const uint32_t* data = expandedJumpTables.GetJumpTable(pc);
				uint32_t nextPc = data[*p];
				if(nextPc != TypeData<uint32_t>::Maximum())
				{
					nextState->AddThread(nextPc, threadState, p+1, false, processData);
				}
			}
			break;

		case InstructionType::ByteJumpRange:
			{
				const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
				uint32_t nextPc = data->pcData[data->range.Contains(*p)];
				if(nextPc != TypeData<uint32_t>::Maximum())
				{
					nextState->AddThread(nextPc, threadState, p+1, false, processData);
				}
			}
			break;
				
		case InstructionType::ByteNot:
			if(instruction.data != *p)
			{
				nextState->AddThread(pc+1, threadState, p+1, false, processData);
			}
			break;
			
		case InstructionType::ByteNotEitherOf2:
			if((instruction.data & 0xff) != *p &&
			   ((instruction.data >> 8) & 0xff) != *p)
			{
				nextState->AddThread(pc+1, threadState, p+1, false, processData);
			}
			break;
			
		case InstructionType::ByteNotEitherOf3:
			if((instruction.data & 0xff) != *p &&
			   ((instruction.data >> 8) & 0xff) != *p &&
			   ((instruction.data >> 16) & 0xff) != *p)
			{
				nextState->AddThread(pc+1, threadState, p+1, false, processData);
			}
			break;
			
		case InstructionType::ByteNotRange:
			{
				unsigned char low = instruction.data & 0xff;
				unsigned char high = (instruction.data >> 8) & 0xff;
				if(*p < low || *p > high)
				{
					nextState->AddThread(pc+1, threadState, p+1, false, processData);
				}
			}
			break;
				
		case InstructionType::FindByte:
			if(nextState->HasNoThreads() && thread+1 >= currentState->updateThread)
			{
				unsigned char c = instruction.data & 0xff;
				p = (const unsigned char*) FindByte(p, c, processData.pEnd);
				if(!p) return;
				nextState->AddThread(instruction.data>>8, threadState, p+1, false, processData);
			}
			else
			{
				unsigned char c = instruction.data & 0xff;
				nextState->AddThread((*p == c) ? instruction.data>>8 : pc, threadState, p+1, false, processData);
			}
			break;
				
		case InstructionType::SearchByte:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				unsigned char c = instruction.data & 0xff;
				unsigned offset = instruction.data >> 8;
				
				const unsigned char* pSearch = p+offset;
				if(pSearch >= processData.pEnd) return;
				
				size_t remaining = processData.pEnd - pSearch;
				pSearch = (const unsigned char*) memchr(pSearch, c, remaining);
				if(!pSearch) return;
				p = pSearch-offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
				
		case InstructionType::SearchByteEitherOf2:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindByteEitherOf2(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
				
		case InstructionType::SearchByteEitherOf3:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindByteEitherOf3(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
				
		case InstructionType::SearchByteEitherOf4:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindByteEitherOf4(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
			
		case InstructionType::SearchByteEitherOf5:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindByteEitherOf5(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
			
		case InstructionType::SearchByteEitherOf6:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindByteEitherOf6(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
			
		case InstructionType::SearchByteEitherOf7:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindByteEitherOf7(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
			
		case InstructionType::SearchByteEitherOf8:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindByteEitherOf8(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
			
		case InstructionType::SearchBytePair:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindBytePair(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
				
		case InstructionType::SearchBytePair2:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindBytePair2(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
				
		case InstructionType::SearchBytePair3:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindBytePair3(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
			
		case InstructionType::SearchBytePair4:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindBytePair4(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
			
		case InstructionType::SearchByteRange:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindByteRange(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
			
		case InstructionType::SearchByteRangePair:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindByteRangePair(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
			
		case InstructionType::SearchByteTriplet:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindByteTriplet(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
				
		case InstructionType::SearchByteTriplet2:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const unsigned char* pSearch = p+data->offset;
				if(pSearch >= processData.pEnd) return;
				
				pSearch = (const unsigned char*) FindByteTriplet2(pSearch, data->GetAllBytes(), processData.pEnd);
				if(!pSearch) return;
				p = pSearch-data->offset-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
				
		case InstructionType::SearchBoyerMoore:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchData* searchData = patternData.GetData<ByteCodeSearchData>(instruction.data);
				const unsigned char* pSearch = (const unsigned char*) FindBoyerMoore(p, searchData, processData.pEnd);
				if(!pSearch) return;
				p = pSearch-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
				
		case InstructionType::SearchShiftOr:
			if(nextState->HasNoThreads()
			   && thread == currentState->threadList
			   && thread+1 >= currentState->updateThread)
			{
				const ByteCodeSearchData* searchData = patternData.GetData<ByteCodeSearchData>(instruction.data);
				const unsigned char* pSearch = (const unsigned char*) FindShiftOr(p, searchData, processData.pEnd);
				if(!pSearch) return;
				p = pSearch-1;
				goto AfterSuccessfulSearch;
			}
			nextState->AddThread(pc, threadState, p+1, false, processData);
			break;
				
		AfterSuccessfulSearch:
			if(patternData[pc+1].type == InstructionType::PropagateBackwards)
			{
				const StaticBitTable<256>& bitTable = *patternData.GetData<StaticBitTable<256>>(patternData[pc+1].data);
				while(bitTable[*p]) --p;
				nextState->AddThread(pc+2, threadState, p+1, false, processData);
			}
			else
			{
				nextState->AddThread(pc+1, threadState, p+1, false, processData);
			}
			break;
				
		case InstructionType::AssertEndOfInput:
		case InstructionType::AssertEndOfLine:
		case InstructionType::AssertWordBoundary:
		case InstructionType::AssertNotWordBoundary:
		case InstructionType::AssertRecurseValue:
		case InstructionType::AssertStartOfInput:
		case InstructionType::AssertStartOfLine:
		case InstructionType::AssertStartOfSearch:
		case InstructionType::BackReference:
		case InstructionType::Call:
		case InstructionType::DispatchMask:
		case InstructionType::DispatchRange:
		case InstructionType::DispatchTable:
		case InstructionType::Fail:
		case InstructionType::Jump:
		case InstructionType::Match:
		case InstructionType::Save:
		case InstructionType::SaveNoRecurse:
		case InstructionType::Split:
		case InstructionType::SplitMatch:
		case InstructionType::SplitNNext:
		case InstructionType::SplitNMatchNext:
		case InstructionType::SplitNextN:
		case InstructionType::SplitNextMatchN:
		case InstructionType::StepBack:
		case InstructionType::Possess:
		case InstructionType::ProgressCheck:
		case InstructionType::PropagateBackwards:
		case InstructionType::Recurse:
		case InstructionType::ReturnIfRecurseValue:
		case InstructionType::Success:
			JERROR("Unexpected instruction");
		}
	}
}

const void* SimplePikeNfaPatternProcessor::Process(const unsigned char* pIn, ProcessData& processData) const
{
	// Make numberOfInstructions even to preserve alignment requirements
	uint32_t processSize = StateList::GetSize(numberOfInstructions);
	uint32_t updateCacheSize = StateList::GetUpdateCacheSize(numberOfInstructions);
	return StackBuffer(2*processSize+updateCacheSize, [=, &processData](unsigned char* pBuffer) -> const void*
	{
		StateList* currentState = (StateList*) pBuffer;
		StateList* nextState = (StateList*) (pBuffer + processSize);
		uint32_t* updateCache = (uint32_t*) (pBuffer + 2*processSize);
		
		currentState->Prepare(updateCache, numberOfInstructions);
		nextState->Prepare(updateCache, numberOfInstructions);

		const unsigned char* p = pIn;
		const unsigned char* pEnd = processData.pEnd;

		processData.currentState = currentState;
		nextState->AddStartingInstruction(processData.startingInstruction, p, processData);
		
		for(; p < pEnd; ++p)
		{
			if(nextState->HasNoThreads()) return processData.match;
			Swap(currentState, nextState);
			nextState->ResetThreads();

			processData.currentState = currentState;
			ProcessStateList(currentState, nextState, p, processData);
		}

		return processData.match;
	});
}

const void* SimplePikeNfaPatternProcessor::FullMatch(const void* data, size_t length) const
{
	ProcessData processData(true, data, length, 0, fullMatchStartingInstruction, *this);
	return Process(processData.pStart, processData);
}

const void* SimplePikeNfaPatternProcessor::FullMatch(const void* data, size_t length, const char **captures) const
{
	ProcessData processData(true, data, length, 0, fullMatchStartingInstruction, *this);
	const void* result = Process(processData.pStart, processData);
	if(result)
	{
		memcpy(captures, processData.captures.captures, sizeof(const char*)*numberOfCaptures);
	}
	return result;
}

const void* SimplePikeNfaPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset) const
{
	ProcessData processData(matchRequiresEndOfInput, data, length, offset, partialMatchStartingInstruction, *this);
	return Process(processData.pSearchStart, processData);
}

const void* SimplePikeNfaPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const
{
	ProcessData processData(matchRequiresEndOfInput, data, length, offset, partialMatchStartingInstruction, *this);
	const void* result = Process(processData.pSearchStart, processData);
	if(result)
	{
		memcpy(captures, processData.captures.captures, sizeof(const char*)*numberOfCaptures);
	}
	return result;
}

Interval<const void*> SimplePikeNfaPatternProcessor::LocatePartialMatch(const void* data, size_t length, size_t offset) const
{
	ProcessData processData(matchRequiresEndOfInput, data, length, offset, partialMatchStartingInstruction, *this);
	Process(processData.pSearchStart, processData);
	return {processData.captures.captures[0], processData.captures.captures[1]};
}

const void* SimplePikeNfaPatternProcessor::PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const
{
	ProcessData processData(matchRequiresEndOfInput, data, length, offset, fullMatchStartingInstruction, *this);
	const void* result = Process(processData.pSearchStart, processData);
	JASSERT(result);
	memcpy(captures, processData.captures.captures, sizeof(const char*)*numberOfCaptures);
	return result;
}

//============================================================================

PatternProcessor* PatternProcessor::CreateSimplePikeNfaProcessor(const void* data, size_t length)
{
	return new SimplePikeNfaPatternProcessor(data, length);
}

//============================================================================
