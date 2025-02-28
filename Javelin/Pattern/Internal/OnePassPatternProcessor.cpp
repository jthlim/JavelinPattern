//============================================================================

#include "Javelin/Pattern/Internal/PatternProcessor.h"
#if !PATTERN_USE_JIT

#include "Javelin/Container/BitTable.h"
#include "Javelin/Pattern/Internal/PatternByteCode.h"
#include "Javelin/Stream/ICharacterWriter.h"
#include "Javelin/Template/Memory.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class OnePassPatternProcessor final : public PatternProcessor
{
public:
	OnePassPatternProcessor(const void* data, size_t length);
	OnePassPatternProcessor(DataBlock&& dataBlock);
	~OnePassPatternProcessor();
	
	virtual const void* FullMatch(const void* data, size_t length) const;
	virtual const void* FullMatch(const void* data, size_t length, const char **captures) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const;
	virtual Interval<const void*> LocatePartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures);

private:
	struct ProcessData
	{
		bool					isFullMatch;
		mutable int32_t			recurseValue;
		const unsigned char* 	pStart;
		const unsigned char*	pSearchStart;
		const unsigned char* 	pEnd;
		const char**			captures;
		
		ProcessData(bool aIsFullMatch, const void* data, size_t length, size_t offset, const char** aCaptures)
		{
			isFullMatch		= aIsFullMatch;
			recurseValue	= -1;
			pStart 			= (const unsigned char*) data;
			pSearchStart	= pStart + offset;
			pEnd			= pStart + length;
			captures		= aCaptures;
		}
	};
	
	bool				matchRequiresEndOfInput;
	uint8_t				numberOfCaptures;
	PatternData			patternData;
	uint32_t			partialMatchStartingInstruction;
	uint32_t			fullMatchStartingInstruction;
	DataBlock			dataStore;
	ExpandedJumpTables	expandedJumpTables;

	const void* Process(uint32_t pc, const unsigned char* p, const ProcessData& processData) const;
	
	void Set(const void* data, size_t length);
};

//============================================================================

OnePassPatternProcessor::OnePassPatternProcessor(const void* data, size_t length)
{
	Set(data, length);
}

OnePassPatternProcessor::OnePassPatternProcessor(DataBlock&& dataBlock)
: dataStore((DataBlock&&) dataBlock)
{
	Set(dataStore.GetData(), dataStore.GetCount());
}

void OnePassPatternProcessor::Set(const void* data, size_t length)
{
	const ByteCodeHeader* header = (const ByteCodeHeader*) data;
	matchRequiresEndOfInput = header->flags.matchRequiresEndOfInput;
	numberOfCaptures = header->numberOfCaptures;
	partialMatchStartingInstruction = header->partialMatchStartingInstruction;
	fullMatchStartingInstruction = header->fullMatchStartingInstruction;
	patternData.p = (const unsigned char*) header->GetForwardProgram();
	expandedJumpTables.Set(patternData, header->numberOfInstructions);
}

OnePassPatternProcessor::~OnePassPatternProcessor()
{
}

//============================================================================

const void* OnePassPatternProcessor::Process(uint32_t pc, const unsigned char* p, const ProcessData& processData) const
{
Loop:
	const ByteCodeInstruction instruction = patternData[pc];
	switch(instruction.type)
	{
	case InstructionType::AdvanceByte:
		++p;
		++pc;
		goto Loop;
		
	case InstructionType::AnyByte:
		if(p == processData.pEnd) return nullptr;
		++p;
		++pc;
		goto Loop;
			
	case InstructionType::AssertStartOfInput:
		if(p != processData.pStart) return nullptr;
		++pc;
		goto Loop;
			
	case InstructionType::AssertEndOfInput:
		if(p != processData.pEnd) return nullptr;
		++pc;
		goto Loop;
			
	case InstructionType::AssertStartOfLine:
		if(p != processData.pStart && p[-1] != '\n') return nullptr;
		++pc;
		goto Loop;
		
	case InstructionType::AssertEndOfLine:
		if(p != processData.pEnd && *p != '\n') return nullptr;
		++pc;
		goto Loop;
		
	case InstructionType::AssertWordBoundary:
		if(p != processData.pEnd && Character::IsWordCharacter(*p))
		{
			if(p != processData.pStart
			   && Character::IsWordCharacter(p[-1])) return nullptr;
		}
		else
		{
			if(p == processData.pStart) return nullptr;
			if(!Character::IsWordCharacter(p[-1])) return nullptr;
		}
		++pc;
		goto Loop;
			
	case InstructionType::AssertNotWordBoundary:
		if(p != processData.pEnd && Character::IsWordCharacter(*p))
		{
			if(p == processData.pStart) return nullptr;
			if(!Character::IsWordCharacter(p[-1])) return nullptr;
		}
		else
		{
			if(p != processData.pStart
			   && Character::IsWordCharacter(p[-1])) return nullptr;
		}
		++pc;
		goto Loop;
			
	case InstructionType::AssertStartOfSearch:
		if(p != processData.pSearchStart) return nullptr;
		++pc;
		goto Loop;
		
	case InstructionType::AssertRecurseValue:
		if(processData.recurseValue != instruction.data) return nullptr;
		++pc;
		goto Loop;
			
	case InstructionType::BackReference:
		{
			uint32_t index = instruction.data*2;
			ssize_t length = processData.captures[index+1] - processData.captures[index];
			if(length >= 0
			   && p+length <= processData.pEnd
			   && memcmp(processData.captures[index], p, length) == 0)
			{
				p += length;
				++pc;
				goto Loop;
			}
		}
		return nullptr;
			
	case InstructionType::Byte:
		if(p == processData.pEnd) return nullptr;
		if(*p != instruction.data) return nullptr;
		++p;
		++pc;
		goto Loop;
			
	case InstructionType::ByteEitherOf2:
		if(p == processData.pEnd) return nullptr;
		if(*p != (instruction.data & 0xff)
		   && *p != ((instruction.data>>8) & 0xff)) return nullptr;
		++p;
		++pc;
		goto Loop;
			
	case InstructionType::ByteEitherOf3:
		if(p == processData.pEnd) return nullptr;
		if(*p != (instruction.data & 0xff)
		   && *p != ((instruction.data>>8) & 0xff)
		   && *p != ((instruction.data>>16) & 0xff)) return nullptr;
		++p;
		++pc;
		goto Loop;
		
	case InstructionType::ByteRange:
		if(p == processData.pEnd) return nullptr;
		else
		{
			unsigned char low = instruction.data & 0xff;
			unsigned char high = (instruction.data >> 8) & 0xff;
			uint32_t delta = high - low;
			if(uint32_t(*p - low) > delta) return nullptr;
		}
		++p;
		++pc;
		goto Loop;
			
	case InstructionType::ByteBitMask:
		if(p == processData.pEnd) return nullptr;
		{
			const StaticBitTable<256>& data = *patternData.GetData<StaticBitTable<256>>(instruction.data);
			if(!data[*p]) return nullptr;
		}
		++p;
		++pc;
		goto Loop;
			
	case InstructionType::ByteJumpTable:
	case InstructionType::ByteJumpMask:
		if(p == processData.pEnd) return nullptr;
		else
		{
			const uint32_t* data = expandedJumpTables.GetJumpTable(pc);
			pc = data[*p];
			if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
			++p;
			goto Loop;
		}
			
	case InstructionType::ByteJumpRange:
		if(p == processData.pEnd) return nullptr;
		else
		{
			const ByteCodeJumpRangeData* jumpRangeData = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			pc = jumpRangeData->pcData[jumpRangeData->range.Contains(*p)];
			if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
			++p;
			goto Loop;
		}
			
	case InstructionType::ByteNot:
		if(p == processData.pEnd) return nullptr;
		if(*p == instruction.data) return nullptr;
		++p;
		++pc;
		goto Loop;
		
	case InstructionType::ByteNotEitherOf2:
		if(p == processData.pEnd) return nullptr;
		if(*p == (instruction.data & 0xff)
		   || *p == ((instruction.data>>8) & 0xff)) return nullptr;
		++p;
		++pc;
		goto Loop;
		
	case InstructionType::ByteNotEitherOf3:
		if(p == processData.pEnd) return nullptr;
		if(*p == (instruction.data & 0xff)
		   || *p == ((instruction.data>>8) & 0xff)
		   || *p == ((instruction.data>>16) & 0xff)) return nullptr;
		++p;
		++pc;
		goto Loop;
		
	case InstructionType::ByteNotRange:
		if(p == processData.pEnd) return nullptr;
		else
		{
			unsigned char low = instruction.data & 0xff;
			unsigned char high = (instruction.data >> 8) & 0xff;
			if(low <= *p && *p <= high) return nullptr;
		}
		++p;
		++pc;
		goto Loop;

	case InstructionType::Call:
		{
			const ByteCodeCallData* callData = patternData.GetData<ByteCodeCallData>(instruction.data);
			const void* result = Process(callData->callIndex, p, processData);
			pc = (&callData->falseIndex)[result ? 1 : 0];
			if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
			goto Loop;
		}
			
	case InstructionType::DispatchTable:
		if(p == processData.pEnd)
		{
			const ByteCodeJumpTableData* data = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
			pc = data->pcData[0];
		}
		else
		{
			const uint32_t* data = expandedJumpTables.GetJumpTable(pc);
			pc = data[*p];
		}
		if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
		goto Loop;
			
	case InstructionType::DispatchMask:
		if(p == processData.pEnd)
		{
			const ByteCodeJumpMaskData* data = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
			pc = data->pcData[0];
		}
		else
		{
			const uint32_t* data = expandedJumpTables.GetJumpTable(pc);
			pc = data[*p];
		}
		if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
		goto Loop;
			
	case InstructionType::DispatchRange:
		if(p == processData.pEnd)
		{
			const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			pc = data->pcData[0];
		}
		else
		{
			const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			pc = data->pcData[data->range.Contains(*p)];
		}
		if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
		goto Loop;
		
	case InstructionType::Fail:
		return nullptr;

	case InstructionType::FindByte:
		{
			unsigned char c = instruction.data & 0xff;
			p = (const unsigned char*) FindByte(p, c, processData.pEnd);
			if(!p) return nullptr;
			++p;
			pc = instruction.data >> 8;
			goto Loop;
		}
	
	case InstructionType::Possess:
		{
			const ByteCodeCallData* callData = patternData.GetData<ByteCodeCallData>(instruction.data);
			const void* result = Process(callData->callIndex, p, processData);
			if(result)
			{
				p = (const unsigned char*) result;
				pc = callData->trueIndex;
			}
			else
			{
				pc = callData->falseIndex;
			}
			if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
			goto Loop;
		}
			
	case InstructionType::PropagateBackwards:
		if(processData.captures != nullptr)
		{
			const StaticBitTable<256>& bitTable = *patternData.GetData<StaticBitTable<256>>(instruction.data);
			while(bitTable[p[-1]]) --p;
		}
		++pc;
		goto Loop;
		
	case InstructionType::Recurse:
		{
			int recurseValue = instruction.data & 0xff;
			int recursePc = instruction.data >> 8;
			
			int oldRecurseValue = processData.recurseValue;
			processData.recurseValue = recurseValue;
			
			const char* oldCaptures[numberOfCaptures*2];
			memcpy(oldCaptures, processData.captures, numberOfCaptures*2*sizeof(const char*));
			
			if(Process(recursePc, p, processData))
			{
				p = (const unsigned char*) processData.captures[recurseValue*2+1];
				memcpy(processData.captures, oldCaptures, numberOfCaptures*2*sizeof(const char*));
				processData.recurseValue = oldRecurseValue;
			}
			else
			{
				memcpy(processData.captures, oldCaptures, numberOfCaptures*2*sizeof(const char*));
				processData.recurseValue = oldRecurseValue;
				return nullptr;
			}
			++pc;
			goto Loop;
		}
			
	case InstructionType::ReturnIfRecurseValue:
		if(processData.recurseValue == instruction.data) return p;
		++pc;
		goto Loop;
			
	case InstructionType::SearchByte:
		{
			unsigned char c = instruction.data & 0xff;
			unsigned offset = instruction.data >> 8;

			const unsigned char* pSearch = p+offset;
			if(pSearch >= processData.pEnd) return nullptr;

			size_t remaining = processData.pEnd - pSearch;
			pSearch = (const unsigned char*) memchr(pSearch, c, remaining);
			if(!pSearch) return nullptr;
			p = pSearch-offset;
			++pc;
			goto Loop;
		}

	case InstructionType::SearchByteEitherOf2:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindByteEitherOf2(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchByteEitherOf3:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindByteEitherOf3(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchByteEitherOf4:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindByteEitherOf4(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchByteEitherOf5:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindByteEitherOf5(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchByteEitherOf6:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindByteEitherOf6(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchByteEitherOf7:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindByteEitherOf7(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchByteEitherOf8:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindByteEitherOf8(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchBytePair:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindBytePair(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchBytePair2:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindBytePair2(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchBytePair3:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindBytePair3(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchBytePair4:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindBytePair4(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchByteRange:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindByteRange(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchByteRangePair:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindByteRangePair(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchByteTriplet:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindByteTriplet(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchByteTriplet2:
		{
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			const unsigned char* pSearch = p+data->offset;
			if(pSearch >= processData.pEnd) return nullptr;
			pSearch = (const unsigned char*) FindByteTriplet2(pSearch, data->GetAllBytes(), processData.pEnd);
			if(!pSearch) return nullptr;
			p = pSearch-data->offset;
			++pc;
			goto Loop;
		}
			
	case InstructionType::SearchBoyerMoore:
		{
			const ByteCodeSearchData* searchData = patternData.GetData<ByteCodeSearchData>(instruction.data);
			p = (const unsigned char*) FindBoyerMoore(p, searchData, processData.pEnd);
			if(!p) return nullptr;
			++pc;
			goto Loop;
		}

	case InstructionType::SearchShiftOr:
		{
			const ByteCodeSearchData* searchData = patternData.GetData<ByteCodeSearchData>(instruction.data);
			p = (const unsigned char*) FindShiftOr(p, searchData, processData.pEnd);
			if(!p) return nullptr;
			++pc;
			goto Loop;
		}
			
	case InstructionType::Jump:
		pc = instruction.data;
		goto Loop;
			
	case InstructionType::Match:
		if(processData.isFullMatch)	return p == processData.pEnd ? p : nullptr;
		else return p;
			
	case InstructionType::ProgressCheck:
		++pc;
		goto Loop;
		
	case InstructionType::Save:
	case InstructionType::SaveNoRecurse:
		if(processData.captures != nullptr)
		{
			uint32_t saveIndex = instruction.data & 0xff;
			uint32_t saveOffset = instruction.data >> 8;
			processData.captures[saveIndex] = (const char*) p - saveOffset;
		}
		++pc;
		goto Loop;
			
	case InstructionType::SplitMatch:
		{
			const uint32_t* splitData = patternData.GetData<uint32_t>(instruction.data);
			pc = (!processData.isFullMatch || p == processData.pEnd) ? splitData[0] : splitData[1];
			goto Loop;
		}
			
	case InstructionType::SplitNextMatchN:
		pc = (!processData.isFullMatch || p == processData.pEnd) ? pc+1 : instruction.data;
		goto Loop;

	case InstructionType::SplitNMatchNext:
		pc = (!processData.isFullMatch || p == processData.pEnd) ? instruction.data : pc+1;
		goto Loop;
			
	case InstructionType::StepBack:
		p -= instruction.data;
		if(p < processData.pStart) return nullptr;
		++pc;
		goto Loop;
		
	case InstructionType::Success:
		return p;
			
	case InstructionType::Split:
	case InstructionType::SplitNextN:
	case InstructionType::SplitNNext:
		JERROR("Unhandled instruction");
		return nullptr;
	}
}

//============================================================================

const void* OnePassPatternProcessor::FullMatch(const void* data, size_t length) const
{
	ProcessData processData(true, data, length, 0, nullptr);
	return Process(fullMatchStartingInstruction, (const unsigned char*) data, processData);
}

const void* OnePassPatternProcessor::FullMatch(const void* data, size_t length, const char **captures) const
{
	ProcessData processData(true, data, length, 0, captures);
	return Process(fullMatchStartingInstruction, (const unsigned char*) data, processData);
}

const void* OnePassPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset) const
{
	ProcessData processData(matchRequiresEndOfInput, data, length, offset, nullptr);
	return Process(partialMatchStartingInstruction, processData.pSearchStart, processData);
}

const void* OnePassPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const
{
	ProcessData processData(matchRequiresEndOfInput, data, length, offset, captures);
	return Process(partialMatchStartingInstruction, processData.pSearchStart, processData);
}

Interval<const void*> OnePassPatternProcessor::LocatePartialMatch(const void* data, size_t length, size_t offset) const
{
	JASSERT(numberOfCaptures > 0);
	const char* captures[numberOfCaptures*2];
	captures[0] = nullptr;
	captures[1] = nullptr;
	ProcessData processData(matchRequiresEndOfInput, data, length, offset, captures);
	Process(partialMatchStartingInstruction, processData.pSearchStart, processData);
	return {captures[0], captures[1]};
}

const void* OnePassPatternProcessor::PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures)
{
	ProcessData processData(matchRequiresEndOfInput, data, length, offset, captures);
	return Process(fullMatchStartingInstruction, processData.pSearchStart, processData);
}

//============================================================================

PatternProcessor* PatternProcessor::CreateOnePassProcessor(DataBlock&& dataBlock)
{
	return new OnePassPatternProcessor((DataBlock&&) dataBlock);
}

PatternProcessor* PatternProcessor::CreateOnePassProcessor(const void* data, size_t length, bool makeCopy)
{
	if(makeCopy)
	{
		DataBlock dataBlock(data, length);
		return new OnePassPatternProcessor((DataBlock&&) dataBlock);
	}
	else
	{
		return new OnePassPatternProcessor(data, length);
	}
}

//============================================================================
#endif // !PATTERN_USE_JIT
//============================================================================
