//============================================================================

#include "Javelin/Pattern/Internal/PatternProcessor.h"
#if !PATTERN_USE_JIT

#include "Javelin/Container/BitTable.h"
#include "Javelin/Pattern/Internal/PatternByteCode.h"
#include "Javelin/Template/Memory.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class BitStateBackTrackingPatternProcessor final : public PatternProcessor
{
private:
	static constexpr size_t MAXIMUM_BITS = 32768;
	
public:
	BitStateBackTrackingPatternProcessor(const void* data, size_t length);
	BitStateBackTrackingPatternProcessor(DataBlock&& dataBlock);
	~BitStateBackTrackingPatternProcessor();
	
	virtual bool CanUseFullMatchProgram(size_t inputLength) const 					{ return fullMatchAlwaysFits || CanUsePartialMatchProgram(inputLength); }
	virtual bool CanUsePartialMatchProgram(size_t inputLength) const final 			{ return (inputLength+1) * numberOfBitStateInstructions <= MAXIMUM_BITS; }
	
	virtual const void* FullMatch(const void* data, size_t length) const;
	virtual const void* FullMatch(const void* data, size_t length, const char **captures) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const;
	virtual Interval<const void*> LocatePartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures);

private:
	struct ProcessData;
	
	bool			fullMatchAlwaysFits;
	bool			matchRequiresEndOfInput;
	uint8_t			numberOfCaptures;
	Table<uint32_t>	bitStateLookup;
	PatternData		patternData;
	uint32_t		numberOfBitStateInstructions;
	uint32_t		partialMatchStartingInstruction;
	uint32_t		fullMatchStartingInstruction;
	DataBlock		dataStore;
	ExpandedJumpTables	expandedJumpTables;
	
	const void* Process(uint32_t pc, const unsigned char* p, ProcessData& processData) const;
	
	void Set(const void* data, size_t length);
};

struct BitStateBackTrackingPatternProcessor::ProcessData
{
	bool							isFullMatch;
	const unsigned char* 			pStart;
	const unsigned char*			pSearchStart;
	const unsigned char* 			pEnd;
	const char**					captures;
	StaticBitTable<MAXIMUM_BITS>	stateTracker;
	
	ProcessData(bool aIsFullMatch, uint32_t numberOfBitStateInstructions, const void* data, size_t length, size_t offset, const char** aCaptures)
	: stateTracker(NO_INITIALIZE)
	{
		isFullMatch		= aIsFullMatch;
		pStart 			= (const unsigned char*) data;
		pSearchStart	= pStart + offset;
		pEnd			= pStart + length;
		captures		= aCaptures;
		stateTracker.ClearFirstNBits(numberOfBitStateInstructions*(length-offset+1));
	}
};

//============================================================================

BitStateBackTrackingPatternProcessor::BitStateBackTrackingPatternProcessor(const void* data, size_t length)
{
	Set(data, length);
}

BitStateBackTrackingPatternProcessor::BitStateBackTrackingPatternProcessor(DataBlock&& dataBlock)
: dataStore((DataBlock&&) dataBlock)
{
	Set(dataStore.GetData(), dataStore.GetCount());
}

void BitStateBackTrackingPatternProcessor::Set(const void* data, size_t length)
{
	ByteCodeHeader* header = (ByteCodeHeader*) data;
	matchRequiresEndOfInput = header->flags.matchRequiresEndOfInput;
	numberOfCaptures = header->numberOfCaptures;
	uint32_t numberOfInstructions = header->numberOfInstructions;
	partialMatchStartingInstruction = header->partialMatchStartingInstruction;
	fullMatchStartingInstruction = header->fullMatchStartingInstruction;
	patternData.p = (const unsigned char*) header->GetForwardProgram();
	expandedJumpTables.Set(patternData, header->numberOfInstructions);

	numberOfBitStateInstructions = 0;
	bitStateLookup.SetCount(numberOfInstructions);
	for(uint32_t i = 0; i < numberOfInstructions; ++i)
	{
		if(patternData[i].isSingleReference)
		{
			bitStateLookup[i] = TypeData<uint32_t>::Maximum();
		}
		else
		{
			bitStateLookup[i] = numberOfBitStateInstructions++;
		}
	}

	fullMatchAlwaysFits = (header->maximumMatchLength+1) * numberOfBitStateInstructions <= MAXIMUM_BITS;
}

BitStateBackTrackingPatternProcessor::~BitStateBackTrackingPatternProcessor()
{
}

//============================================================================

static constexpr int MakeCase(InstructionType type, bool isSingleReference)
{
	return (int) type*2 + isSingleReference;
}

// ByteCodeInstruction defines type::isSingleReference::data
// The Process loop switches on type::isSingleReference to
// avoid having to check bitState unnecessarily.
#define CASE(x) 																												\
	case MakeCase(x, false):																									\
		{																														\
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));	\
			if(processData.stateTracker[bitIndex]) return nullptr;																\
			processData.stateTracker.SetBit(bitIndex);																			\
		}																														\
		[[fallthrough]];																										\
	case MakeCase(x, true):

const void* BitStateBackTrackingPatternProcessor::Process(uint32_t pc, const unsigned char* p, ProcessData& processData) const
{
Loop:
	const ByteCodeInstruction instruction = patternData[pc];
	switch(instruction.value >> 24)
	{
	CASE(InstructionType::AdvanceByte)
		++p;
		++pc;
		goto Loop;
			
	CASE(InstructionType::AnyByte)
		if(p == processData.pEnd) return nullptr;
		++p;
		++pc;
		goto Loop;
			
	CASE(InstructionType::AssertStartOfInput)
		if(p != processData.pStart) return nullptr;
		++pc;
		goto Loop;
			
	CASE(InstructionType::AssertEndOfInput)
		if(p != processData.pEnd) return nullptr;
		++pc;
		goto Loop;
			
	CASE(InstructionType::AssertStartOfLine)
		if(p != processData.pStart && p[-1] != '\n') return nullptr;
		++pc;
		goto Loop;
		
	CASE(InstructionType::AssertEndOfLine)
		if(p != processData.pEnd && *p != '\n') return nullptr;
		++pc;
		goto Loop;
		
	CASE(InstructionType::AssertStartOfSearch)
		if(p != processData.pSearchStart) return nullptr;
		++pc;
		goto Loop;
			
	CASE(InstructionType::AssertWordBoundary)
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
			
	CASE(InstructionType::AssertNotWordBoundary)
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
			
	CASE(InstructionType::Byte)
		if(p == processData.pEnd) return nullptr;
		if(*p != instruction.data) return nullptr;
		++p;
		++pc;
		goto Loop;
			
	CASE(InstructionType::ByteEitherOf2)
		if(p == processData.pEnd) return nullptr;
		if(*p != (instruction.data & 0xff)
		   && *p != ((instruction.data>>8) & 0xff)) return nullptr;
		++p;
		++pc;
		goto Loop;
			
	CASE(InstructionType::ByteEitherOf3)
		if(p == processData.pEnd) return nullptr;
		if(*p != (instruction.data & 0xff)
		   && *p != ((instruction.data>>8) & 0xff)
		   && *p != ((instruction.data>>16) & 0xff)) return nullptr;
		++p;
		++pc;
		goto Loop;
		
	CASE(InstructionType::ByteRange)
		if(p == processData.pEnd) return nullptr;
		else
		{
			unsigned char low = instruction.data & 0xff;
			unsigned char high = (instruction.data >> 8) & 0xff;
			uint32_t delta = high-low;
			if(uint32_t(*p - low) > delta) return nullptr;
		}
		++p;
		++pc;
		goto Loop;
			
	CASE(InstructionType::ByteBitMask)
		if(p == processData.pEnd) return nullptr;
		{
			const StaticBitTable<256>& data = *patternData.GetData<StaticBitTable<256>>(instruction.data);
			if(!data[*p]) return nullptr;
		}
		++p;
		++pc;
		goto Loop;
			
	CASE(InstructionType::ByteJumpTable)
		if(p == processData.pEnd) return nullptr;
		else
		{
			const uint32_t* data = expandedJumpTables.GetJumpTable(pc);
			pc = data[*p];
			if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
			++p;
			goto Loop;
		}
			
	CASE(InstructionType::ByteJumpMask)
		if(p == processData.pEnd) return nullptr;
		else
		{
			const uint32_t* data = expandedJumpTables.GetJumpTable(pc);
			pc = data[*p];
			if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
			++p;
			goto Loop;
		}
			
	CASE(InstructionType::ByteJumpRange)
		if(p == processData.pEnd) return nullptr;
		else
		{
			const ByteCodeJumpRangeData* jumpRangeData = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			pc = jumpRangeData->pcData[jumpRangeData->range.Contains(*p)];
			if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
			++p;
			goto Loop;
		}
			
	CASE(InstructionType::ByteNot)
		if(p == processData.pEnd) return nullptr;
		if(*p == instruction.data) return nullptr;
		++p;
		++pc;
		goto Loop;
		
	CASE(InstructionType::ByteNotEitherOf2)
		if(p == processData.pEnd) return nullptr;
		if(*p == (instruction.data & 0xff)
		   || *p == ((instruction.data>>8) & 0xff)) return nullptr;
		++p;
		++pc;
		goto Loop;
		
	CASE(InstructionType::ByteNotEitherOf3)
		if(p == processData.pEnd) return nullptr;
		if(*p == (instruction.data & 0xff)
		   || *p == ((instruction.data>>8) & 0xff)
		   || *p == ((instruction.data>>16) & 0xff)) return nullptr;
		++p;
		++pc;
		goto Loop;
		
	CASE(InstructionType::ByteNotRange)
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

	CASE(InstructionType::DispatchTable)
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
			
	CASE(InstructionType::DispatchMask)
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
			
	CASE(InstructionType::DispatchRange)
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
		
	CASE(InstructionType::Fail)
		return nullptr;

	case MakeCase(InstructionType::FindByte, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));

			unsigned char c = instruction.data & 0xff;
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p == processData.pEnd) return nullptr;
				if(*p == c) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			++p;
			pc = instruction.data >> 8;
			goto Loop;
		}
			
	case MakeCase(InstructionType::FindByte, true):
		{
			unsigned char c = instruction.data & 0xff;
			p = (const unsigned char*) FindByte(p, c, processData.pEnd);
			if(!p) return nullptr;
			++p;
			pc = instruction.data >> 8;
			goto Loop;
		}
			
	CASE(InstructionType::PropagateBackwards)
		if(processData.captures != nullptr)
		{
			const StaticBitTable<256>& bitTable = *patternData.GetData<StaticBitTable<256>>(instruction.data);
			while(bitTable[p[-1]]) --p;
		}
		++pc;
		goto Loop;
			
	case MakeCase(InstructionType::SearchByte, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			
			unsigned char c = instruction.data & 0xff;
			unsigned offset = instruction.data >> 8;

			// Note: don't offset bitIndex!
			p += offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p == processData.pEnd) return nullptr;
				if(*p == c) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchByte, true):
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
			
	case MakeCase(InstructionType::SearchByteEitherOf2, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p == processData.pEnd) return nullptr;
				if(*p == data->bytes[0] || *p == data->bytes[1]) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchByteEitherOf2, true):
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
			
	case MakeCase(InstructionType::SearchByteEitherOf3, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p == processData.pEnd) return nullptr;
				if(*p == data->bytes[0] || *p == data->bytes[1] || *p == data->bytes[2]) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchByteEitherOf3, true):
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
			
	case MakeCase(InstructionType::SearchByteEitherOf4, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p == processData.pEnd) return nullptr;
				if(*p == data->bytes[0] || *p == data->bytes[1] || *p == data->bytes[2] || *p == data->bytes[3]) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchByteEitherOf4, true):
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
			
	case MakeCase(InstructionType::SearchByteEitherOf5, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p == processData.pEnd) return nullptr;
				if(*p == data->bytes[0] || *p == data->bytes[1] || *p == data->bytes[2] || *p == data->bytes[3]
				   || *p == data->bytes[4]) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchByteEitherOf5, true):
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
			
	case MakeCase(InstructionType::SearchByteEitherOf6, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p == processData.pEnd) return nullptr;
				if(*p == data->bytes[0] || *p == data->bytes[1] || *p == data->bytes[2] || *p == data->bytes[3]
				   || *p == data->bytes[4] || *p == data->bytes[5]) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchByteEitherOf6, true):
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
			
	case MakeCase(InstructionType::SearchByteEitherOf7, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p == processData.pEnd) return nullptr;
				if(*p == data->bytes[0] || *p == data->bytes[1] || *p == data->bytes[2] || *p == data->bytes[3]
				   || *p == data->bytes[4] || *p == data->bytes[5] || *p == data->bytes[6]) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchByteEitherOf7, true):
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
			
	case MakeCase(InstructionType::SearchByteEitherOf8, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p == processData.pEnd) return nullptr;
				if(*p == data->bytes[0] || *p == data->bytes[1] || *p == data->bytes[2] || *p == data->bytes[3]
				   || *p == data->bytes[4] || *p == data->bytes[5] || *p == data->bytes[6] || *p == data->bytes[7]) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchByteEitherOf8, true):
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
			
	case MakeCase(InstructionType::SearchBytePair, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p+1 >= processData.pEnd) return nullptr;
				if(p[0] == data->bytes[0] && p[1] == data->bytes[1]) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchBytePair, true):
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
			
	case MakeCase(InstructionType::SearchBytePair2, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p+1 >= processData.pEnd) return nullptr;
				if((p[0] == data->bytes[0] || p[0] == data->bytes[1])
				   && (p[1] == data->bytes[2] || p[1] == data->bytes[3])) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchBytePair2, true):
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
			
	case MakeCase(InstructionType::SearchBytePair3, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p+1 >= processData.pEnd) return nullptr;
				if((p[0] == data->bytes[0] || p[0] == data->bytes[1] || p[0] == data->bytes[2])
				   && (p[1] == data->bytes[3] || p[1] == data->bytes[4] || p[1] == data->bytes[5])) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchBytePair3, true):
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
			
	case MakeCase(InstructionType::SearchBytePair4, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p+1 >= processData.pEnd) return nullptr;
				if((p[0] == data->bytes[0] || p[0] == data->bytes[1] || p[0] == data->bytes[2] || p[0] == data->bytes[3])
				   && (p[1] == data->bytes[4] || p[1] == data->bytes[5] || p[1] == data->bytes[6] || p[1] == data->bytes[7])) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchBytePair4, true):
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
			
	case MakeCase(InstructionType::SearchByteTriplet, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p+1 >= processData.pEnd) return nullptr;
				if(p[0] == data->bytes[0] && p[1] == data->bytes[1] && p[2] == data->bytes[2]) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchByteTriplet, true):
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
			
	case MakeCase(InstructionType::SearchByteTriplet2, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p+1 >= processData.pEnd) return nullptr;
				if((p[0] == data->bytes[0] || p[0] == data->bytes[1])
				   && (p[1] == data->bytes[2] || p[1] == data->bytes[3])
				   && (p[2] == data->bytes[4] || p[2] == data->bytes[5])) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchByteTriplet2, true):
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
			
	case MakeCase(InstructionType::SearchByteRange, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p >= processData.pEnd) return nullptr;
				if(p[0] >= data->bytes[0] && p[0] <= data->bytes[1]) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchByteRange, true):
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
			
	case MakeCase(InstructionType::SearchByteRangePair, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += data->offset;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p+1 >= processData.pEnd) return nullptr;
				if(p[0] >= data->bytes[0]
				   && p[0] <= data->bytes[1]
				   && p[1] >= data->bytes[2]
				   && p[1] <= data->bytes[3]) break;
				
				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= data->offset;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchByteRangePair, true):
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
			
	case MakeCase(InstructionType::SearchBoyerMoore, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchData* searchData = patternData.GetData<ByteCodeSearchData>(instruction.data);
			
			// Note: don't offset bitIndex!
			p += searchData->length;
			if(p >= processData.pEnd) return nullptr;
			
			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p >= processData.pEnd) return nullptr;
				uint32_t adjust = *p;
				if(adjust == 0) break;
				
				p += adjust;
				bitIndex += adjust*numberOfBitStateInstructions;
			}
			p -= searchData->length;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchBoyerMoore, true):
		{
			const ByteCodeSearchData* searchData = patternData.GetData<ByteCodeSearchData>(instruction.data);
			p = (const unsigned char*) FindBoyerMoore(p, searchData, processData.pEnd);
			if(!p) return nullptr;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchShiftOr, false):
		{
			uint32_t bitIndex = bitStateLookup[pc] + (numberOfBitStateInstructions * (uint32_t) (p-processData.pSearchStart));
			const ByteCodeSearchData* searchData = patternData.GetData<ByteCodeSearchData>(instruction.data);
			
			uint32_t testMask = 1 << searchData->length;
			uint32_t state = (uint32_t) -1;

			for(;;)
			{
				if(processData.stateTracker[bitIndex]) return nullptr;
				processData.stateTracker.SetBit(bitIndex);
				
				if(p >= processData.pEnd) return nullptr;
				
				state = (state << 1) | searchData->data[*p];
				if((state & testMask) == 0) break;

				++p;
				bitIndex += numberOfBitStateInstructions;
			}
			p -= searchData->length;
			++pc;
			goto Loop;
		}
			
	case MakeCase(InstructionType::SearchShiftOr, true):
		{
			const ByteCodeSearchData* searchData = patternData.GetData<ByteCodeSearchData>(instruction.data);
			p = (const unsigned char*) FindShiftOr(p, searchData, processData.pEnd);
			if(!p) return nullptr;
			++pc;
			goto Loop;
		}
			
	CASE(InstructionType::Jump)
		pc = instruction.data;
		goto Loop;
			
	CASE(InstructionType::Match)
		if(processData.isFullMatch)	return p == processData.pEnd ? p : nullptr;
		else return p;
			
	CASE(InstructionType::ProgressCheck)
		++pc;
		goto Loop;
			
	CASE(InstructionType::SaveNoRecurse)
		if(processData.captures != nullptr)
		{
			uint32_t saveIndex = instruction.data & 0xff;
			uint32_t saveOffset = instruction.data >> 8;
			processData.captures[saveIndex] = (const char*) p - saveOffset;
		}
		++pc;
		goto Loop;
			
	CASE(InstructionType::Save)
		if(processData.captures == nullptr)
		{
			++pc;
			goto Loop;
		}
		else
		{
			uint32_t saveIndex = instruction.data & 0xff;
			uint32_t saveOffset = instruction.data >> 8;
			const char* backup = processData.captures[saveIndex];
			processData.captures[saveIndex] = (const char*) p - saveOffset;
			const void* result = Process(pc+1, p, processData);
			if(result != nullptr) return result;
			else processData.captures[saveIndex] = backup;
			return nullptr;
		}
			
	CASE(InstructionType::Split)
		{
			const ByteCodeSplitData* split = patternData.GetData<ByteCodeSplitData>(instruction.data);
			uint32_t numberOfTargets = split->numberOfTargets;
			for(size_t i = 0; i < numberOfTargets-1; ++i)
			{
				const void* result = Process(split->targetList[i], p, processData);
				if(result != nullptr) return result;
			}
			pc = split->targetList[numberOfTargets-1];
			goto Loop;
		}
		
	CASE(InstructionType::SplitMatch)
		{
			const uint32_t* splitData = patternData.GetData<uint32_t>(instruction.data);
			pc = (!processData.isFullMatch || p == processData.pEnd) ? splitData[0] : splitData[1];
			goto Loop;
		}
			
	CASE(InstructionType::SplitNextN)
		{
			const void* result = Process(pc+1, p, processData);
			if(result != nullptr) return result;
			pc = instruction.data;
			goto Loop;
		}
		
	CASE(InstructionType::SplitNNext)
		{
			const void* result = Process(instruction.data, p, processData);
			if(result != nullptr) return result;
			++pc;
			goto Loop;
		}
			
	CASE(InstructionType::SplitNextMatchN)
		pc = (!processData.isFullMatch || p == processData.pEnd) ? pc+1 : instruction.data;
		goto Loop;
		
	CASE(InstructionType::SplitNMatchNext)
		pc = (!processData.isFullMatch || p == processData.pEnd) ? instruction.data : pc+1;
		goto Loop;
		
	default:
		JERROR("Unhandled instruction");
		return nullptr;
	}
}

//============================================================================

const void* BitStateBackTrackingPatternProcessor::FullMatch(const void* data, size_t length) const
{
	ProcessData processData(true, numberOfBitStateInstructions, data, length, 0, nullptr);
	return Process(fullMatchStartingInstruction, (const unsigned char*) data, processData);
}

const void* BitStateBackTrackingPatternProcessor::FullMatch(const void* data, size_t length, const char **captures) const
{
	ProcessData processData(true, numberOfBitStateInstructions, data, length, 0, captures);
	return Process(fullMatchStartingInstruction, (const unsigned char*) data, processData);
}

const void* BitStateBackTrackingPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset) const
{
	ProcessData processData(matchRequiresEndOfInput, numberOfBitStateInstructions, data, length, offset, nullptr);
	return Process(partialMatchStartingInstruction, processData.pSearchStart, processData);
}

const void* BitStateBackTrackingPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const
{
	ProcessData processData(matchRequiresEndOfInput, numberOfBitStateInstructions, data, length, offset, captures);
	return Process(partialMatchStartingInstruction, processData.pSearchStart, processData);
}

Interval<const void*> BitStateBackTrackingPatternProcessor::LocatePartialMatch(const void* data, size_t length, size_t offset) const
{
	JASSERT(numberOfCaptures > 0);
	const char* captures[numberOfCaptures*2];
	captures[0] = nullptr;
	captures[1] = nullptr;
	ProcessData processData(matchRequiresEndOfInput, numberOfBitStateInstructions, data, length, offset, captures);
	Process(partialMatchStartingInstruction, processData.pSearchStart, processData);
	return {captures[0], captures[1]};
}

const void* BitStateBackTrackingPatternProcessor::PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures)
{
	ProcessData processData(matchRequiresEndOfInput, numberOfBitStateInstructions, data, length, offset, captures);
	return Process(fullMatchStartingInstruction, processData.pSearchStart, processData);
}

//============================================================================

PatternProcessor* PatternProcessor::CreateBitStateProcessor(const void* data, size_t length)
{
	return new BitStateBackTrackingPatternProcessor(data, length);
}

//============================================================================
#endif // !PATTERN_USE_JIT
//============================================================================
