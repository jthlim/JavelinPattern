//============================================================================

#include "Javelin/Pattern/Internal/PatternReverseProcessor.h"
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

#define VERBOSE_DEBUG_PATTERN	0

//============================================================================

class ThompsonNfaReverseProcessor final : public ReverseProcessor
{
public:
	ThompsonNfaReverseProcessor(const void* data, size_t length);
	
	virtual const void* Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char **captures, bool matchIsAnchored) const;
	
private:
	PatternData			patternData;
	uint32_t			numberOfInstructions;
	uint32_t			startingInstruction;
	ExpandedJumpTables	expandedJumpTables;
	
	void Set(const void* data, size_t length);
	
	struct State;
	struct ProcessData;
	
	void ProcessState(const State* currentState, State* nextState, const unsigned char* &p, ProcessData& processData) const;
	const void* Process(const unsigned char* pIn, ProcessData& processData) const;
};

//============================================================================

struct ThompsonNfaReverseProcessor::ProcessData
{
	const bool					matchIsAnchored;
	const void*					match = nullptr;
	const unsigned char *const 	pStart;
	const unsigned char *const	pStop;
	const unsigned char *const 	pEnd;
	const PatternData			patternData;
	State*						currentState;
	const ExpandedJumpTables&	expandedJumpTables;
	
	ProcessData(bool aMatchIsAnchored, const void* data, size_t length, size_t stop, const ThompsonNfaReverseProcessor& processor)
	: matchIsAnchored(aMatchIsAnchored),
	  pStart((const unsigned char*) data),
	  pStop((const unsigned char*) data + stop),
	  pEnd((const unsigned char*) data + length),
	  patternData(processor.patternData),
	  expandedJumpTables(processor.expandedJumpTables)
	{
	}
};

//============================================================================

struct ThompsonNfaReverseProcessor::State
{
	uint32_t		numberOfThreads;
	uint32_t*		threadList;
	uint32_t*		updateCache;
	uint64_t		numberOfStates;
	uint32_t		stateList[1];			// List of already visited states

	static size_t GetSize(uint32_t maximumNumberOfThreads) 				{ return (sizeof(State) + maximumNumberOfThreads*(2*sizeof(uint32_t)) + 7) & -8; }
	static size_t GetUpdateCacheSize(uint32_t maximumNumberOfThreads) 	{ return (maximumNumberOfThreads*(sizeof(uint32_t)) + 7) & -8; }

	void Dump(ICharacterWriter& output) const;
	bool HasNoThreads() const 											{ return numberOfThreads == 0;	}
	
	void ResetThreads()
	{
		numberOfStates  = 0;
		numberOfThreads = 0;
	}

	JINLINE bool Check(uint32_t pc)
	{
		uint32_t index = updateCache[pc];
		if(index < numberOfStates)
		{
			if(stateList[index] == pc) return false;
		}
		updateCache[pc] = uint32_t(numberOfStates);
		stateList[numberOfStates++] = pc;
		return true;
	}

	void Prepare(uint32_t* aUpdateCache, uint32_t maximumNumberOfThreads)
	{
		updateCache = aUpdateCache;
		threadList = (uint32_t*) &stateList[maximumNumberOfThreads];
		ResetThreads();
	}

	void AddThread(uint32_t pc)
	{
#if VERBOSE_DEBUG_PATTERN
		StandardOutput.PrintF("Adding pc: %u\n", pc);
#endif

		threadList[numberOfThreads++] = pc;
	}

	void AddThread(uint32_t pc, const unsigned char* p, ProcessData& processData);
};

void ThompsonNfaReverseProcessor::State::AddThread(uint32_t pc, const unsigned char* p, ProcessData& processData)
{
Loop:
#if VERBOSE_DEBUG_PATTERN
	StandardOutput.PrintF("Adding to next thread pc: %u\n", pc);
#endif
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
		if(p != processData.pStop) break;
		goto ProcessNextInstruction;

	case InstructionType::DispatchTable:
		if(p == processData.pStop)
		{
			const ByteCodeJumpTableData* data = processData.patternData.GetData<ByteCodeJumpTableData>(instruction.data);
			pc = data->pcData[0];
		}
		else
		{
			const uint32_t* data = processData.expandedJumpTables.GetJumpTable(pc);
			pc = data[p[-1]];
		}
		if(pc != TypeData<uint32_t>::Maximum()) goto Loop;
		break;
		
	case InstructionType::DispatchMask:
		if(p == processData.pStop)
		{
			const ByteCodeJumpMaskData* data = processData.patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
			pc = data->pcData[0];
		}
		else
		{
			const uint32_t* data = processData.expandedJumpTables.GetJumpTable(pc);
			pc = data[p[-1]];
		}
		if(pc != TypeData<uint32_t>::Maximum()) goto Loop;
		break;
		
	case InstructionType::DispatchRange:
		{
			const ByteCodeJumpRangeData* data = processData.patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			if(p == processData.pStop)
			{
				pc = data->pcData[0];
			}
			else
			{
				pc = data->pcData[data->range.Contains(p[-1])];
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
		if(processData.matchIsAnchored && p != processData.pStop) break;
		processData.match = p;
		return;
		
			
	case InstructionType::Split:
		{
			const ByteCodeSplitData* splitData = processData.patternData.GetData<ByteCodeSplitData>(instruction.data);
			
			uint32_t numberOfTargets = splitData->numberOfTargets;
			for(uint32_t i = 0; i < numberOfTargets-1; ++i)
			{
				AddThread(splitData->targetList[i], p, processData);
			}
			pc = splitData->targetList[numberOfTargets-1];
			goto Loop;
		}
		
	case InstructionType::SplitMatch:
		{
			const uint32_t* splitData = processData.patternData.GetData<uint32_t>(instruction.data);
			AddThread(splitData[0], p, processData);
			pc = splitData[1];
			goto Loop;
		}
			
	case InstructionType::SplitNextN:
	case InstructionType::SplitNextMatchN:
		AddThread(pc+1, p, processData);
		pc = instruction.data;
		goto Loop;
		
	case InstructionType::SplitNNext:
	case InstructionType::SplitNMatchNext:
		AddThread(instruction.data, p, processData);
		goto ProcessNextInstruction;
		
	case InstructionType::ProgressCheck:
	ProcessNextInstruction:
		++pc;
		goto Loop;

	default:
		AddThread(pc);
		break;
			
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

void ThompsonNfaReverseProcessor::State::Dump(ICharacterWriter& output) const
{
	output.PrintF("ThreadList:");
	for(size_t i = 0; i < numberOfThreads; ++i)
	{
		output.PrintF(" %u", threadList[i]);
	}
	output.PrintF("\n");
}

//============================================================================

ThompsonNfaReverseProcessor::ThompsonNfaReverseProcessor(const void* data, size_t length)
{
	Set(data, length);
}

void ThompsonNfaReverseProcessor::Set(const void* data, size_t length)
{
	ByteCodeHeader* header = (ByteCodeHeader*) data;
	numberOfInstructions = header->numberOfReverseInstructions;
	startingInstruction = header->reverseMatchStartingInstruction;
	patternData.p = (const unsigned char*) header->GetReverseProgram();
	expandedJumpTables.Set(patternData, numberOfInstructions);
}

//============================================================================

void ThompsonNfaReverseProcessor::ProcessState(const State* currentState, State* nextState, const unsigned char* &p, ProcessData& processData) const
{
#if VERBOSE_DEBUG_PATTERN
	StandardOutput.PrintF("\nProcessing byte: '%c'\n", p[-1]);
	currentState->Dump(StandardOutput);
#endif
	
	for(uint32_t pcIndex = 0; pcIndex < currentState->numberOfThreads; ++pcIndex)
	{
		uint32_t pc = currentState->threadList[pcIndex];
		
#if VERBOSE_DEBUG_PATTERN
		StandardOutput.PrintF("Processing pc: %u\n", pc);
#endif
	Loop:
		const ByteCodeInstruction instruction = patternData[pc];
		
		switch(instruction.type)
		{
		case InstructionType::AdvanceByte:
		case InstructionType::AnyByte:
			nextState->AddThread(pc+1, p-1, processData);
			break;
			
		case InstructionType::Byte:
			if(instruction.data == p[-1])
			{
				nextState->AddThread(pc+1, p-1, processData);
			}
			break;
			
		case InstructionType::ByteEitherOf2:
			if((instruction.data & 0xff) == p[-1] ||
			   ((instruction.data >> 8) & 0xff) == p[-1])
			{
				nextState->AddThread(pc+1, p-1, processData);
			}
			break;
			
		case InstructionType::ByteEitherOf3:
			if((instruction.data & 0xff) == p[-1] ||
			   ((instruction.data >> 8) & 0xff) == p[-1] ||
			   ((instruction.data >> 16) & 0xff) == p[-1])
			{
				nextState->AddThread(pc+1, p-1, processData);
			}
			break;
			
		case InstructionType::ByteRange:
			{
				unsigned char low = instruction.data & 0xff;
				unsigned char high = (instruction.data >> 8) & 0xff;
				uint32_t delta = high - low;
				if(uint32_t(p[-1] - low) <= delta)
				{
					nextState->AddThread(pc+1, p-1, processData);
				}
			}
			break;
			
		case InstructionType::ByteBitMask:
			{
				const StaticBitTable<256>& data = *patternData.GetData<StaticBitTable<256>>(instruction.data);
				if(data[p[-1]])
				{
					nextState->AddThread(pc+1, p-1, processData);
				}
			}
			break;

		case InstructionType::ByteJumpTable:
		case InstructionType::ByteJumpMask:
			{
				const uint32_t* data = expandedJumpTables.GetJumpTable(pc);
				uint32_t nextPc = data[p[-1]];
				if(nextPc != TypeData<uint32_t>::Maximum())
				{
					nextState->AddThread(nextPc, p-1, processData);
				}
			}
			break;

		case InstructionType::ByteJumpRange:
			{
				const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
				uint32_t nextPc = data->pcData[data->range.Contains(p[-1])];
				if(nextPc != TypeData<uint32_t>::Maximum())
				{
					nextState->AddThread(nextPc, p-1, processData);
				}
			}
			break;
				
		case InstructionType::ByteNot:
			if(instruction.data != p[-1])
			{
				nextState->AddThread(pc+1, p-1, processData);
			}
			break;
			
		case InstructionType::ByteNotEitherOf2:
			if((instruction.data & 0xff) != p[-1] &&
			   ((instruction.data >> 8) & 0xff) != p[-1])
			{
				nextState->AddThread(pc+1, p-1, processData);
			}
			break;
			
		case InstructionType::ByteNotEitherOf3:
			if((instruction.data & 0xff) != p[-1] &&
			   ((instruction.data >> 8) & 0xff) != p[-1] &&
			   ((instruction.data >> 16) & 0xff) != p[-1])
			{
				nextState->AddThread(pc+1, p-1, processData);
			}
			break;
			
		case InstructionType::ByteNotRange:
			{
				unsigned char low = instruction.data & 0xff;
				unsigned char high = (instruction.data >> 8) & 0xff;
				if(p[-1] < low || p[-1] > high)
				{
					nextState->AddThread(pc+1, p-1, processData);
				}
			}
			break;
				
		case InstructionType::FindByte:
			if(nextState->HasNoThreads()
			   && currentState->numberOfThreads == 1)
			{
				unsigned char c = instruction.data & 0xff;
				p = (const unsigned char*) FindByteReverse(p, c, processData.pStop);
				if(!p) return;
				nextState->AddThread(instruction.data>>8, p-1, processData);
			}
			else
			{
				unsigned char c = instruction.data & 0xff;
				nextState->AddThread((p[-1] == c) ? instruction.data>>8 : pc, p-1, processData);
			}
			break;
				
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
		case InstructionType::DispatchMask:
		case InstructionType::DispatchRange:
		case InstructionType::DispatchTable:
		case InstructionType::Fail:
		case InstructionType::Jump:
		case InstructionType::Match:
		case InstructionType::Possess:
		case InstructionType::ProgressCheck:
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
		case InstructionType::Split:
		case InstructionType::SplitMatch:
		case InstructionType::SplitNextN:
		case InstructionType::SplitNextMatchN:
		case InstructionType::SplitNNext:
		case InstructionType::SplitNMatchNext:
			JERROR("Unexpected instruction");
		}
	}
}

const void* ThompsonNfaReverseProcessor::Process(const unsigned char* pIn, ProcessData& processData) const
{
	size_t processSize = State::GetSize(numberOfInstructions);
	size_t updateCacheSize = State::GetUpdateCacheSize(numberOfInstructions);
	return StackBuffer(2*processSize+updateCacheSize, [=, &processData](unsigned char* pBuffer) -> const void*
	{
		State* currentState = (State*) pBuffer;
		State* nextState = (State*) (pBuffer + processSize);
		uint32_t* updateCache = (uint32_t*) (pBuffer + 2*processSize);
		
		currentState->Prepare(updateCache, numberOfInstructions);
		nextState->Prepare(updateCache, numberOfInstructions);

		const unsigned char* p = pIn;
		const unsigned char* pStop = processData.pStop;

		processData.currentState = currentState;
		nextState->AddThread(startingInstruction, pIn, processData);
		
		for(; p > pStop; --p)
		{
			if(nextState->HasNoThreads()) return processData.match;
			Swap(currentState, nextState);
			nextState->ResetThreads();

			processData.currentState = currentState;
			ProcessState(currentState, nextState, p, processData);
		}

		return processData.match;
	});
}

//============================================================================

const void* ThompsonNfaReverseProcessor::Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char **captures, bool matchIsAnchored) const
{
	ProcessData processData(matchIsAnchored, data, length, startOffset, *this);
	return Process((const unsigned char*) matchEnd, processData);
}

//============================================================================

ReverseProcessor* ReverseProcessor::CreateThompsonNfaReverseProcessor(const void* data, size_t length)
{
	return new ThompsonNfaReverseProcessor(data, length);
}

//============================================================================
