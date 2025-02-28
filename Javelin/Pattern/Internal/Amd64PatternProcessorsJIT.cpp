//============================================================================
//
// This provides AMD64 JIT versions of:
//  BackTrackingPatternProcessor
//  BitStateBackTrackingPatternProcessor
//  OnePassPatternProcessor
//
//============================================================================

#include "Javelin/Pattern/Internal/PatternProcessor.h"
#if 0 && PATTERN_USE_JIT && defined(JABI_AMD64_SYSTEM_V)

#include "Javelin/Pattern/Internal/Amd64PatternProcessorsFindMethods.h"
#include "Javelin/Pattern/Internal/PatternReverseProcessor.h"
#include "Javelin/Container/BitTable.h"
#include "Javelin/Container/EnumSet.h"
#include "Javelin/Math/BitUtility.h"
#include "Javelin/Pattern/Pattern.h"
#include "Javelin/Pattern/Internal/PatternByteCode.h"
#include "Javelin/Pattern/Internal/PatternNoStackGrowthHandler.h"
#include "Javelin/Pattern/Internal/PatternNibbleMask.h"
#include "Javelin/Pattern/Internal/PatternProcessorType.h"
#include "Javelin/System/Machine.h"
#include "Javelin/Template/Memory.h"
#include "Javelin/Type/UnalignedPointer.h"
#include <sys/mman.h>

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;
using namespace Javelin::PatternInternal::Amd64FindMethods;

//============================================================================

namespace
{
	class Program : public DataBlock
	{
	public:
		Program(PatternProcessorType aType) : type(aType) { }
		
		enum Condition
		{
			Above   		= 7,
			AboveOrEqual	= 3,
			Below    		= 2,
			BelowOrEqual	= 6,
			BitNotSet		= 3,
			BitSet			= 2,
			Carry			= 2,
			Equal    		= 4,
			NoCarry			= 3,
			NotEqual 		= 5,
			NotZero			= 5,
			Sign			= 8,
			Zero			= 4,
		};
		
		void Call(int location);
		void LoadConstantPointer(const void* p);
		void Jump(int location);
		void Fail()						{ Jump(1);	}
		void PushP();
		void PopP();
		void IncrementP();
		void ComparePToPStart();
		void ComparePToPSearchStart();
		void ComparePToPStop();
		void CheckPToPStop(int pc);
		void ExitIfNot0();
		
		void JumpIf(int condition, int location);
		uint32_t JumpIfLong(int condition);
		uint32_t JumpIfShort(int condition);
		uint32_t JumpShort();
		void JumpIfAndMarkPatch(int condition, int pc);
		
		void FailIf(int condition)		{ JumpIf(condition, 1); }
		void FailIfNotEqual()			{ FailIf(NotEqual); }
		void FailIfEqual()				{ FailIf(Equal); }
		void FailIfAbove()				{ FailIf(Above); }
		void FailIfBelow()				{ FailIf(Below); }
		
		void CallAndMarkPatch(int pc)	{ Call(0); MarkPatch(pc); }
		void JumpAndMarkPatch(int pc)	{ Jump(0); MarkPatch(pc); }
		void MarkPatch(uint32_t destination) { patchList.Append(PatchEntry{uint32_t(GetCount()), destination}); }
		void PatchOffset(uint32_t afterInstruction, uint32_t destination);
		void PatchShort(uint32_t location);
		void PatchLong(uint32_t location)	{ PatchOffset(location, uint32_t(GetCount())); }
		void CompareIsPartialMatch();
		void TestValue();
		void TestCaptures();
		void Nop(int count);
		void BitTest();
		void CalculateCapture(int32_t offset);
		void PushCapture(uint32_t captureIndex);
		void PopCapture(uint32_t captureIndex);
		void WriteCapture(uint32_t captureIndex);
		void WriteZeroOffsetCapture(uint32_t captureIndex);
		void ReadPreviousByte();
		void ReadByte();
        void ReadLongWithOffset(int32_t offset);
		void ReadByteForPc(int pc);
		void ReadAlternateByteForPc(int pc);
		bool RewindIfAdjustP(int p);
		void IncrementPForPc(int pc);
		void ReadByteWithOffset(int32_t offset);
		void ReadAlternateByteWithOffset(int32_t offset);
		void CompareByte(uint8_t value);
		void CompareByteWithOffset(uint8_t, int32_t offset);
		void CompareLongWithOffset(uint32_t, int32_t offset);
		void CompareCurrentByte(uint8_t value);
		void CompareValue(uint32_t value);
		void OrByte(uint8_t value);
		void OrLong(uint32_t value);
		void SubtractValue(uint32_t value);
		void AdjustP(int32_t value);
		void AdjustBitDataPointer(int32_t offset);
		void ClearProgressCheckTable(uint32_t numberOfProgressChecks);
		
		// Destroys EAX!!
		void LoadValueIntoXmm(uint32_t value, uint8_t xmmIndex, bool useAvx);
		void LoadXmmValueFromPointer(uint32_t index);
		void CallPointer();
		
		void DoBitStateCheck(int bitOffset);

		void Return();
		void BeginStackGuard();
		void EndStackGuard();
		void DoStackGuardCheck();
		void WriteStackGuardList();
		
		void Dispatch();
		
		struct PatchEntry
		{
			uint32_t	location;
			uint32_t	pc;
		};
		
		Table<PatchEntry> 		patchList;
		Table<uint32_t>			stackGuardList;
		PatternProcessorType 	type;
		int						checkStartPc = 0;
		int						checkPEndPc  = 0;
		int						bytesForBitState;
	};
}

//============================================================================

class Amd64ProcessorBase
{
protected:
	Amd64ProcessorBase(const void* data, size_t length, PatternProcessorType type, int bytesForBitState);
	Amd64ProcessorBase(DataBlock&& dataBlock, PatternProcessorType type, int bytesForBitState);
	~Amd64ProcessorBase();

	bool			allowPartialMatch;
	uint8_t			numberOfCaptures;
	DataBlock		dataStore;
	void*			fullMatchProgram;
	void*			partialMatchProgram;
	

private:
	struct ExpandedJumpTables
	{
	public:
		void Allocate(const PatternData& patternData, size_t numberOfInstructions);
		void Populate(const PatternData& patternData, size_t numberOfInstructions, const void* codeBase, const Table<uint32_t>& instructionOffsets, PatternProcessorType type);
		const void* GetJumpTable(size_t pc) const 					{ return jumpTableForPcList[pc]; }
		const void* GetData(size_t pc) const	 					{ return dataForPcList[pc]; }
		
	private:
		Table<const void*>	jumpTable;
		Table<const void**>	jumpTableForPcList;
		Table<const void*>	dataForPcList;
	};

	void*				mapRegion;
	size_t				mapRegionLength;
	ExpandedJumpTables	expandedJumpTables;
	
	void GroupPEndCheck(uint32_t pc, const PatternData& patternData, Program& program, bool skipCheck);
	void Set(const void* data, size_t length, PatternProcessorType type, int bytesForBitState);
	void MakeProgramExecutable(const Program& program);
};

void Amd64ProcessorBase::ExpandedJumpTables::Allocate(const PatternData& patternData, size_t numberOfInstructions)
{
	// Allocate all buffers
	jumpTableForPcList.SetCount(numberOfInstructions);
	jumpTableForPcList.SetAll(nullptr);
	dataForPcList.SetCount(numberOfInstructions);
	dataForPcList.SetAll((nullptr));
	
	OpenHashSet<uint32_t> bitMaskOffsets;
	
	size_t jumpTableSize = 0;
	for(size_t pc = 0; pc < numberOfInstructions; ++pc)
	{
		switch(patternData[pc].type)
		{
		case InstructionType::ByteBitMask:
		case InstructionType::ByteJumpMask:
		case InstructionType::DispatchMask:
			if(bitMaskOffsets.Put(patternData[pc].data))
			{
				jumpTableSize += 256 / sizeof(void*);
			}
			break;
			
		case InstructionType::ByteJumpTable:
		case InstructionType::DispatchTable:
			jumpTableSize += 256;
			break;
			
		case InstructionType::SearchByteEitherOf4:
		case InstructionType::SearchByteEitherOf5:
		case InstructionType::SearchByteEitherOf6:
		case InstructionType::SearchByteEitherOf7:
		case InstructionType::SearchByteEitherOf8:
			if(Machine::SupportsAvx())
			{
				jumpTableSize += 32 / sizeof(void*);
			}
			break;

		case InstructionType::SearchBytePair3:
		case InstructionType::SearchBytePair4:
			if(Machine::SupportsAvx())
			{
				jumpTableSize += 64 / sizeof(void*);
			}
			break;
			
		case InstructionType::SearchBoyerMoore:
			{
				const ByteCodeSearchData* searchData = patternData.GetData<ByteCodeSearchData>(patternData[pc].data);
				if(searchData->HasSingleByteData())
				{
					jumpTableSize += 256 / sizeof(void*);
				}
			}
			break;

		case InstructionType::SearchShiftOr:
			if(Machine::SupportsAvx())
			{
				const ByteCodeSearchShiftOrData* searchData = patternData.GetData<ByteCodeSearchShiftOrData>(patternData[pc].data);
				if(searchData->numberOfNibbleMasks > 0 && searchData->numberOfNibbleMasks <= 3)
				{
					jumpTableSize += (32 / sizeof(void*)) * searchData->numberOfNibbleMasks;
				}
			}
			break;

		default:
			break;
		}
	}
	if(jumpTableSize) jumpTableSize += 4;
	jumpTable.SetCount(jumpTableSize);
	
	// Record the offsets
	Map<uint32_t, const void**> bitMaskMap;
	
	const void** p = jumpTable.GetData();
	if(((size_t) p & 31) != 0) p = (const void**) ((size_t) p + (-(size_t) p & 31));
	for(size_t pc = 0; pc < numberOfInstructions; ++pc)
	{
		switch(patternData[pc].type)
		{
		case InstructionType::ByteBitMask:
		case InstructionType::ByteJumpMask:
		case InstructionType::DispatchMask:
			if(bitMaskMap.Contains(patternData[pc].data))
			{
				jumpTableForPcList[pc] = bitMaskMap[patternData[pc].data];
			}
			else
			{
				jumpTableForPcList[pc] = p;
				bitMaskMap.Insert(patternData[pc].data, p);
				p += 256 / sizeof(void*);
			}
			break;
				
		case InstructionType::ByteJumpTable:
		case InstructionType::DispatchTable:
			jumpTableForPcList[pc] = p;
			p += 256;
			break;
			
		case InstructionType::SearchByteEitherOf4:
		case InstructionType::SearchByteEitherOf5:
		case InstructionType::SearchByteEitherOf6:
		case InstructionType::SearchByteEitherOf7:
		case InstructionType::SearchByteEitherOf8:
			if(Machine::SupportsAvx())
			{
				dataForPcList[pc] = p;
				p += 32 / sizeof(void*);
			}
			break;
				
		case InstructionType::SearchBytePair3:
		case InstructionType::SearchBytePair4:
			if(Machine::SupportsAvx())
			{
				dataForPcList[pc] = p;
				p += 64 / sizeof(void*);
			}
			break;

		case InstructionType::SearchBoyerMoore:
			{
				const ByteCodeSearchData* searchData = patternData.GetData<ByteCodeSearchData>(patternData[pc].data);
				if(searchData->HasSingleByteData())
				{
					dataForPcList[pc] = p;
					p += 256 / sizeof(void*);
				}
			}
			break;
				
		case InstructionType::SearchShiftOr:
			if(Machine::SupportsAvx())
			{
				const ByteCodeSearchShiftOrData* searchData = patternData.GetData<ByteCodeSearchShiftOrData>(patternData[pc].data);
				if(searchData->numberOfNibbleMasks > 0 && searchData->numberOfNibbleMasks <= 3)
				{
					dataForPcList[pc] = p;
					p += (32 / sizeof(void*)) * searchData->numberOfNibbleMasks;
				}
			}
			break;

		default:
			break;
		}
	}
}

void Amd64ProcessorBase::ExpandedJumpTables::Populate(const PatternData& patternData, size_t numberOfInstructions, const void* codeBase, const Table<uint32_t>& instructionOffsets, PatternProcessorType type)
{
	const char* pCodeBase = (const char*) codeBase;
	
	// Populate the data
	for(size_t pc = 0; pc < numberOfInstructions; ++pc)
	{
		ByteCodeInstruction instruction = patternData[pc];
		switch(instruction.type)
		{
		case InstructionType::ByteBitMask:
		case InstructionType::ByteJumpMask:
		case InstructionType::DispatchMask:
			{
				const StaticBitTable<256>& data = *patternData.GetData<StaticBitTable<256>>(instruction.data);
				unsigned char* p = (unsigned char*) jumpTableForPcList[pc];
				JASSERT(p != nullptr);
				for(int c = 0; c < 256; ++c)
				{
					p[c] = data[c];
				}
			}
			break;

		case InstructionType::ByteJumpTable:
		case InstructionType::DispatchTable:
			{
				const ByteCodeJumpTableData* jumpTableData = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
				const void** p = jumpTableForPcList[pc];
				JASSERT(p != nullptr);
				for(int c = 0; c < 256; ++c)
				{
					uint32_t targetPc = jumpTableData->pcData[jumpTableData->jumpTable[c]];
					if(targetPc == TypeData<uint32_t>::Maximum())
					{
						p[c] = pCodeBase + 1;
					}
					else if(instruction.type == InstructionType::ByteJumpTable
							&& type != PatternProcessorType::BitState
							&& patternData[targetPc].type == InstructionType::ByteJumpTable)
					{
						p[c] = jumpTableForPcList[targetPc];
					}
					else
					{
						p[c] = pCodeBase + instructionOffsets[targetPc];
					}
				}
			}
			break;
				
		case InstructionType::SearchByteEitherOf2:
		case InstructionType::SearchByteEitherOf3:
		case InstructionType::SearchByteEitherOf4:
		case InstructionType::SearchByteEitherOf5:
		case InstructionType::SearchByteEitherOf6:
		case InstructionType::SearchByteEitherOf7:
		case InstructionType::SearchByteEitherOf8:
			{
				unsigned char* p = (unsigned char*) dataForPcList[pc];
				if(p)
				{
					const ByteCodeSearchByteData* searchData = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
					int numberOfBytes = int(instruction.type) - int(InstructionType::SearchByteEitherOf2) + 2;
					
					NibbleMask nibbleMask;
					for(int i = 0; i < numberOfBytes; ++i)
					{
						nibbleMask.AddByte(searchData->bytes[i]);
					}
					nibbleMask.CopyMaskToTarget(p);
				}
			}
			break;

		case InstructionType::SearchBytePair:
		case InstructionType::SearchBytePair2:
		case InstructionType::SearchBytePair3:
		case InstructionType::SearchBytePair4:
			{
				unsigned char* p = (unsigned char*) dataForPcList[pc];
				if(p)
				{
					const ByteCodeSearchByteData* searchData = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
					const ByteCodeSearchMultiByteData* multiByteData = (ByteCodeSearchMultiByteData*) &searchData->bytes[instruction.type >= InstructionType::SearchBytePair3 ? 8 : 4];
					if(multiByteData->numberOfNibbleMasks == 2)
					{
						memcpy(p, multiByteData->nibbleMask, 64);
					}
					else
					{
						int numberOfBytePairs = int(instruction.type) - int(InstructionType::SearchBytePair) + 1;
						
						NibbleMask nibbleMask[2];
						for(int i = 0; i < numberOfBytePairs; ++i)
						{
							nibbleMask[0].AddByte(searchData->bytes[i]);
							nibbleMask[1].AddByte(searchData->bytes[i+numberOfBytePairs]);
						}
						nibbleMask[0].CopyMaskToTarget(p);
						nibbleMask[1].CopyMaskToTarget(p+32);
					}
				}
			}
			break;
			
		case InstructionType::SearchBoyerMoore:
			if(dataForPcList[pc] != nullptr)
			{
				const ByteCodeSearchData* searchData = patternData.GetData<ByteCodeSearchData>(instruction.data);
				unsigned char* p = (unsigned char*) dataForPcList[pc];
				
				for(int c = 0; c < 256; ++c)
				{
					p[c] = searchData->data[c];
				}
			}
			break;

		case InstructionType::SearchShiftOr:
			if(dataForPcList[pc] != nullptr)
			{
				unsigned char* p = (unsigned char*) dataForPcList[pc];
				const ByteCodeSearchShiftOrData* searchData = patternData.GetData<ByteCodeSearchShiftOrData>(instruction.data);
				memcpy(p, searchData->nibbleMask, searchData->numberOfNibbleMasks*32);
			}
			break;

		default:
			break;
		}
	}
}

//============================================================================

class OnePassAmd64PatternProcessor final : public PatternProcessor, public Amd64ProcessorBase
{
private:
	typedef Amd64ProcessorBase Inherited;

public:
	OnePassAmd64PatternProcessor(const void* data, size_t length) : Inherited(data, length, PatternProcessorType::OnePass, 0) { }
	OnePassAmd64PatternProcessor(DataBlock&& dataBlock) : Inherited((DataBlock&&) dataBlock, PatternProcessorType::OnePass, 0) { }
	
	virtual const void* FullMatch(const void* data, size_t length) const;
	virtual const void* FullMatch(const void* data, size_t length, const char **captures) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const;
	virtual Interval<const void*> LocatePartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const;
	
private:
	struct ProcessData
	{
		bool					isPartialMatch;
		int32_t					recurseValue;
		const unsigned char* 	pStart;
		const unsigned char* 	pSearchStart;
		
		ProcessData(bool aIsPartialMatch, const void* data, size_t length, size_t offset)
		{
			isPartialMatch	= aIsPartialMatch;
			pStart 			= (const unsigned char*) data;
			pSearchStart	= pStart + offset;
			recurseValue	= -1;
		}
	};
	typedef const void*	(*ProgramType)(ProcessData& processData, const void* p, const unsigned char* pEnd, const char** captures);
};

class BitStateAmd64PatternProcessor final : public PatternProcessor, public Amd64ProcessorBase
{
private:
	typedef Amd64ProcessorBase Inherited;
	static constexpr size_t MAXIMUM_BITS = 2048;	// 256 bytes
	
public:
	BitStateAmd64PatternProcessor(const void* data, size_t length) : BitStateAmd64PatternProcessor(data, length, GetBytesForBitState(data)) { }
	BitStateAmd64PatternProcessor(DataBlock&& dataBlock) : BitStateAmd64PatternProcessor((DataBlock&&) dataBlock, GetBytesForBitState(dataBlock.GetData())) 	{ }
	
	virtual bool CanUseFullMatchProgram(size_t inputLength) const 					{ return fullMatchAlwaysFits || CanUsePartialMatchProgram(inputLength); }
	virtual bool CanUsePartialMatchProgram(size_t inputLength) const final 			{ return (inputLength+1) * numberOfBitStateBytes <= MAXIMUM_BITS/8; }

	virtual const void* FullMatch(const void* data, size_t length) const;
	virtual const void* FullMatch(const void* data, size_t length, const char **captures) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const;
	virtual Interval<const void*> LocatePartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const;
	
private:
	bool			fullMatchAlwaysFits;
	uint32_t 		numberOfBitStateBytes;
	
	BitStateAmd64PatternProcessor(const void* data, size_t length, int bytesForBitState) : Inherited(data, length, PatternProcessorType::BitState, bytesForBitState) { Set(data, bytesForBitState); }
	BitStateAmd64PatternProcessor(DataBlock&& dataBlock, int bytesForBitState) : Inherited((DataBlock&&) dataBlock, PatternProcessorType::BitState, bytesForBitState) { Set(dataStore.GetData(), bytesForBitState); }
	
	static int GetBytesForBitState(const void* data);
	void Set(const void* data, int bytesForBitState);
	
	struct ProcessData
	{
		bool							isPartialMatch;
		const unsigned char* 			pStart;
		const unsigned char* 			pSearchStart;
		const void*						pStackCheck;
		void*							pLastStackAlloc;
		StaticBitTable<MAXIMUM_BITS>	stateTracker;
		
		ProcessData(bool aIsPartialMatch, uint32_t numberOfBitStateBytes, const void* data, size_t length, size_t offset)
		: stateTracker(NO_INITIALIZE)
		{
			isPartialMatch	= aIsPartialMatch;
			pStart 			= (const unsigned char*) data;
			pSearchStart	= pStart + offset;
			stateTracker.ClearFirstNBits(numberOfBitStateBytes*(length+1)*8);
		}
	};
	typedef const void* (*ProgramType)(ProcessData& processData, const void* p, const unsigned char* pEnd, const char** captures, void* bitStateTable, uint32_t numberOfBitStateBytes);
};

class BackTrackingAmd64PatternProcessor final : public PatternProcessor, public Amd64ProcessorBase
{
private:
	typedef Amd64ProcessorBase Inherited;
	
public:
	BackTrackingAmd64PatternProcessor(const void* data, size_t length) : Inherited(data, length, PatternProcessorType::BackTracking, 0)  { Set(data); }
	BackTrackingAmd64PatternProcessor(DataBlock&& dataBlock) : Inherited((DataBlock&&) dataBlock, PatternProcessorType::BackTracking, 0) { Set(dataStore.GetData()); }

	virtual const void* FullMatch(const void* data, size_t length) const;
	virtual const void* FullMatch(const void* data, size_t length, const char **captures) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const;
	virtual Interval<const void*> LocatePartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const;
	
private:
	uint32_t				numberOfProgressChecks;

	void Set(const void* data);
	
	struct ProcessData
	{
		bool					isPartialMatch;
		int32_t					recurseValue;
		const unsigned char* 	pStart;
		const unsigned char* 	pSearchStart;
		const void*				pStackCheck;
		void*					pLastStackAlloc;
		const unsigned char*	progressCheck[1];
		
		ProcessData(bool aIsPartialMatch, const void* data, size_t length, size_t offset, uint32_t numberOfProgressChecks)
		{
			isPartialMatch	= aIsPartialMatch;
			recurseValue	= -1;
			pStart 			= (const unsigned char*) data;
			pSearchStart	= pStart + offset;
		}
	};	
	typedef const void*	(*ProgramType)(ProcessData& processData, const void* p, const unsigned char* pEnd, const char** captures);
};

//============================================================================

Amd64ProcessorBase::Amd64ProcessorBase(const void* data, size_t length, PatternProcessorType type, int bytesForBitState)
{
	Set(data, length, type, bytesForBitState);
}

Amd64ProcessorBase::Amd64ProcessorBase(DataBlock&& dataBlock, PatternProcessorType type, int bytesForBitState)
: dataStore((DataBlock&&) dataBlock)
{
	Set(dataStore.GetData(), dataStore.GetCount(), type, bytesForBitState);
}

Amd64ProcessorBase::~Amd64ProcessorBase()
{
	munmap(mapRegion, mapRegionLength);
}

//============================================================================

void Program::Call(int location)
{
	int32_t offset = location-((int) GetCount()+5);
	PrepareAppend(5);
	AppendNoExpand(0xe8);
	AppendDataNoExpand(&offset, 4);
}

void Program::Jump(int location)
{
	int32_t offset = location-((int) GetCount()+5);
	PrepareAppend(5);
	AppendNoExpand(0xe9);
	AppendDataNoExpand(&offset, 4);
}

void Program::PushP()
{
	PrepareAppend(3);
	AppendNoExpand(0x56); // pushq %rsi

	if(type == PatternProcessorType::BitState)
	{
		AppendNoExpand(0x41, 0x50);			// pushq %r8
	}
}

void Program::PopP()
{
	PrepareAppend(3);
	if(type == PatternProcessorType::BitState)
	{
		AppendNoExpand(0x41, 0x58);			// popq %r8
	}
	
	AppendNoExpand(0x5e); 					// popq %rsi
}

void Program::IncrementP()
{
//	Append(0x0f, 0x0d, 0x46, 0x40);	// prefetch 0x40(%rsi)
//	Append(0x48, 0xff, 0xc6); 		// incq %rsi
	Append(0x48, 0x83, 0xc6, 0x01);	// addq   $0x1, %rsi
	
	if(type == PatternProcessorType::BitState)
	{
		Append(0x4d, 0x01, 0xc8);	// addq %r9, %r8
	}
}

void Program::IncrementPForPc(int pc)
{
	if(pc >= checkPEndPc)
	{
		IncrementP();
	}
	else if(pc == checkPEndPc-1)
	{
		size_t step = checkPEndPc-checkStartPc;
		AdjustP(step);
		
		if(type == PatternProcessorType::BitState)
		{
			AdjustBitDataPointer(step * bytesForBitState);
		}
	}
}

void Program::AdjustBitDataPointer(int32_t offset)
{
	if(offset == 0) return;
	if(-128 <= offset && offset <= 127)
	{
		Append(0x49, 0x83, 0xc0, offset);
	}
	else
	{
		PrepareAppend(7);
		AppendNoExpand(0x49, 0x81, 0xc0);
		AppendDataNoExpand(&offset, 4);
	}
}


void Program::TestValue()
{
	Append(0x48, 0x85, 0xc0);		// test %rax, %rax
}

void Program::TestCaptures()
{
	Append(0x48, 0x85, 0xc9);		// testq  %rcx, %rcx
}

void Program::ExitIfNot0()
{
	TestValue();
	JumpIf(NotZero, 3);
}

void Program::JumpIf(int condition, int location)
{
	int32_t offset = location - (int32_t) GetCount();
	if(-128 <= offset-2 && offset-2 <= 127)
	{
		int32_t endOffset = offset-2;
		Append(0x70+condition, endOffset);
	}
	else
	{
		int32_t endOffset = offset-6;
		PrepareAppend(6);
		AppendNoExpand(0x0f, 0x80+condition);
		AppendDataNoExpand(&endOffset, 4);
	}
}

uint32_t Program::JumpIfLong(int condition)
{
	int32_t endOffset = (int32_t) -(GetCount()+6);
	PrepareAppend(6);
	AppendNoExpand(0x0f, 0x80+condition);
	AppendDataNoExpand(&endOffset, 4);
	return (uint32_t) GetCount();
}

uint32_t Program::JumpIfShort(int condition)
{
	Append(0x70+condition, 0);
	return (uint32_t) GetCount();
}

uint32_t Program::JumpShort()
{
	Append(0xeb, 0);
	return (uint32_t) GetCount();
}

void Program::PatchShort(uint32_t location)
{
	JASSERT(GetCount() - location < 128);
	(*this)[location-1] = GetCount() - location;
}

void Program::JumpIfAndMarkPatch(int condition, int pc)
{
	int32_t endOffset = -int32_t(GetCount()+6);
	Append(0x0f, 0x80+condition);
	AppendData(&endOffset, 4);
	MarkPatch(pc);
}

void Program::ComparePToPStart()
{
	Append(0x48, 0x3b, 0x77, 0x08);		// cmpq   0x8(%rdi), %rsi
}

void Program::ComparePToPSearchStart()
{
	Append(0x48, 0x3b, 0x77, 0x10);		// cmpq   0x10(%rdi), %rsi
}

void Program::ComparePToPStop()
{
	Append(0x48, 0x39, 0xd6);			// cmpq %rdx, %rsi
}

void Program::CheckPToPStop(int pc)
{
	// if(p == pEnd) return nullptr;
	if(pc >= checkPEndPc)
	{
		ComparePToPStop();
		FailIf(Equal);
	}
}

void Program::LoadConstantPointer(const void* p)
{
	Append(0x49, 0xba); AppendData(&p, 8);					// movabsq p, %r10
}

void Program::CompareIsPartialMatch()
{
	Append(0x80, 0x3f, 0x00);								// cmpb   $0x0, (%rdi)
}

void Program::Nop(int count)
{
	switch(count)
	{
	case 7: Append(0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00);	break;
	case 6: Append(0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00);			break;
	case 5: Append(0x0f, 0x1f, 0x44, 0x00, 0x00);				break;
	case 4: Append(0x0f, 0x1f, 0x40, 0x00);						break;
	case 3: Append(0x0f, 0x1f, 0x00);							break;
	case 2:	Append(0x66, 0x90);									break;
	case 1:	Append(0x90);										break;
	}
}

void Program::ReadByte()
{
	Append(0x0f, 0xb6, 0x06); 								// movzbl (%rsi), %eax
}

void Program::ReadLongWithOffset(int32_t offset)
{
    if(offset == 0)
    {
        Append(0x8b, 0x06);                           // movl (%rsi), %eax
    }
    else if(offset < 128)
    {
        Append(0x8b, 0x46, offset);                   // // movl $offset(%rsi), %eax        
    }
    else
    {
        PrepareAppend(6);
        AppendNoExpand(0x8b, 0x86);
        AppendDataNoExpand(&offset, 4);
    }
}

void Program::ReadByteWithOffset(int32_t offset)
{
	if(offset == 0)
	{
		ReadByte();
	}
	else if(-128 <= offset && offset <= 127)
	{
		Append(0x0f, 0xb6, 0x46, offset);
	}
	else
	{
		PrepareAppend(7);
		AppendNoExpand(0x0f, 0xb6, 0x86);
		AppendDataNoExpand(&offset, 4);
	}
}

void Program::ReadByteForPc(int pc)
{
	if(pc < checkPEndPc)
	{
		ReadByteWithOffset(pc-checkStartPc);
	}
	else
	{
		ReadByte();
	}
}

void Program::ReadAlternateByteWithOffset(int32_t offset)
{
	if(offset == 0)
	{
		Append(0x44, 0x0f, 0xb6, 0x1e);				// movzbl (%rsi), %r11d
	}
	else if(-128 <= offset && offset <= 127)
	{
		Append(0x44, 0x0f, 0xb6, 0x5e, offset);		// movzbl {offset}(%rsi), %r11d
	}
	else
	{
		PrepareAppend(8);
		AppendNoExpand(0x44, 0x0f, 0xb6, 0x9e);		// movzbl {offset}(%rsi), %r11d
		AppendDataNoExpand(&offset, 4);
	}
}

void Program::ReadAlternateByteForPc(int pc)
{
	if(pc < checkPEndPc)
	{
		ReadAlternateByteWithOffset(pc-checkStartPc);
	}
	else
	{
		ReadAlternateByteWithOffset(0);
	}
}

void Program::ReadPreviousByte()
{
	Append(0x0f, 0xb6, 0x46, 0xff);							// movzbl -0x1(%rsi), %eax
}

void Program::CompareByte(uint8_t value)
{
	Append(0x3c, value);									// cmpb $value, %al
}

void Program::CompareByteWithOffset(uint8_t value, int32_t offset)
{
	// All of these are    cmpb   {value}, {offset}(%esi)
	if(offset == 0)
	{
		Append(0x80, 0x3e, value);								// cmpb $value, (%rsi)
	}
	else if(-128 <= offset && offset <= 127)
	{
		Append(0x80, 0x7e, offset, value);				// cmpb   $0xc, 0x12(%rsi)
	}
	else
	{
		PrepareAppend(7);
		AppendNoExpand(0x80, 0xbe);
		AppendDataNoExpand(&offset, 4);
		AppendNoExpand(value);
	}
}

void Program::CompareLongWithOffset(uint32_t value, int32_t offset)
{
	// All of these are cmpl   {value}, {offset}(%rsi)
	if(offset == 0)
	{
		Append(0x81, 0x3e);
	}
	else if(-128 <= offset && offset <= 127)
	{
		Append(0x81, 0x7e, offset);
	}
	else
	{
		PrepareAppend(6);
		AppendNoExpand(0x81, 0xbe);
		AppendDataNoExpand(&offset, 4);
	}
	AppendData(&value, 4);
}

void Program::CompareCurrentByte(uint8_t value)
{
	Append(0x80, 0x3e, value);								// cmpb $value, (%rsi)
}

void Program::CompareValue(uint32_t value)
{
	if(value < 128)
	{
		Append(0x83, 0xf8, value);							// cmpl $value, %eax
	}
	else
	{
		PrepareAppend(5);
		AppendNoExpand(0x3d);
		AppendDataNoExpand(&value, 4);						// cmpl $value, %eax
	}
}

void Program::OrByte(uint8_t value)
{
    Append(0x0c, value);                                    // orb $value, %al
}

void Program::OrLong(uint32_t value)
{
    PrepareAppend(5);
    AppendNoExpand(0x0d);                                   // orl $value, %eax
    AppendDataNoExpand(&value, 4);							// cmpl $value, %eax
}

void Program::SubtractValue(uint32_t value)
{
	if(value < 128)
	{
		Append(0x83, 0xe8, value);							// subl $value, %eax
	}
	else
	{
		PrepareAppend(5);
		AppendNoExpand(0x2d);
		AppendDataNoExpand(&value, 4);						// subl $value, %eax
	}
}

void Program::BitTest()
{
	Append(0x41, 0x0f, 0xa3, 0x02);							// btl    %eax, (%r10)
}

void Program::CalculateCapture(int32_t offset)
{
	if(offset == 0)
	{
		Append(0x49, 0x89, 0xf2);							// movq   %rsi, %r10
	}
	else if(-128 <= offset && offset < 128)
	{
		Append(0x4c, 0x8d, 0x56, offset);					// leaq   -0x4(%rsi), %r10
	}
	else
	{
		PrepareAppend(7);
		AppendNoExpand(0x4c, 0x8d, 0x96);
		AppendDataNoExpand(&offset, 4); 					// leaq   -0x100(%rsi), %r10
	}
}

void Program::PushCapture(uint32_t captureOffset)
{
	// All of these are pushq captureOffset(%ecx)
	if(captureOffset == 0)
	{
		Append(0xff, 0x31);
	}
	else if(captureOffset < 128)
	{
		Append(0xff, 0x71, captureOffset);
	}
	else
	{
		PrepareAppend(6);
		AppendNoExpand(0xff, 0xb1);
		AppendDataNoExpand(&captureOffset, 4);
	}
}

void Program::PopCapture(uint32_t captureOffset)
{
	// All of these are popq captureOffset(%ecx)
	if(captureOffset == 0)
	{
		Append(0x8f, 0x01);
	}
	else if(captureOffset < 128)
	{
		Append(0x8f, 0x41, captureOffset);
	}
	else
	{
		PrepareAppend(6);
		AppendNoExpand(0x8f, 0x81);
		AppendDataNoExpand(&captureOffset, 4);
	}
}

void Program::WriteCapture(uint32_t captureOffset)
{
	if(captureOffset == 0)
	{
		Append(0x4c, 0x89, 0x11);									// movq   %r10, (%rcx)
	}
	else if(captureOffset < 128)
	{
		Append(0x4c, 0x89, 0x51, captureOffset);					// movq   %r10, saveIndex(%rcx)
	}
	else
	{
		PrepareAppend(7);
		AppendNoExpand(0x4c, 0x89, 0x91);
		AppendDataNoExpand(&captureOffset, 4);						// movq   %r10, saveIndex(%rcx)
	}
}

void Program::WriteZeroOffsetCapture(uint32_t captureIndex)
{
	if(captureIndex == 0)
	{
		Append(0x48, 0x89, 0x31);									// movq   %rsi, (%rcx)
	}
	else if(captureIndex < 128)
	{
		Append(0x48, 0x89, 0x71, captureIndex);						// movq   %rsi, saveIndex(%rcx)
	}
	else
	{
		PrepareAppend(7);
		AppendNoExpand(0x48, 0x89, 0xb1);
		AppendDataNoExpand(&captureIndex, 4);						// movq   %r10, saveIndex(%rcx)
	}
}

void Program::Dispatch()
{
	Append(0x41, 0xff, 0x24, 0xc2);									// jmpq   *(%r10,%rax,8)
}

void Program::DoBitStateCheck(int bitOffset)
{
	int byteMask = 1 << (bitOffset & 7);
	int byteOffset = bitOffset >> 3;
	if(byteOffset == 0)
	{
		Append(0x41, 0xf6, 0x00, byteMask);		// testb  {byteMask}, (%r8)
		FailIf(NotZero);
		Append(0x41, 0x80, 0x08, byteMask);		// orb    {byteMask}, (%r8)
	}
	else if(byteOffset < 0x80)
	{
		Append(0x41, 0xf6, 0x40, byteOffset, byteMask);		// testb  {byteMask}, {byteOffset}(%r8)
		FailIf(NotZero);
		Append(0x41, 0x80, 0x48, byteOffset, byteMask);		// orb    {byteMask}, {byteOffset}(%r8)
	}
	else
	{
		Append(0x41, 0xf6, 0x80); AppendData(&byteOffset, 4); Append(byteMask);		// testb  {byteMask}, {byteOffset}(%r8)
		FailIf(NotZero);
		Append(0x41, 0x80, 0x88); AppendData(&byteOffset, 4); Append(byteMask);		// orb    {byteMask}, {byteOffset}(%r8)
	}
}

void Program::PatchOffset(uint32_t afterInstruction, uint32_t destination)
{
	((UnalignedPointer<int32_t>) (GetData() + afterInstruction))[-1] += destination;
}

bool Program::RewindIfAdjustP(int p)
{
	if(p == 0) return false;
	
	if(-128 <= p && p < 128)
	{
		if(GetCount() < 4) return false;
		const unsigned char *data = &((*this)[GetCount()-4]);
		const unsigned char requiredData[] = { 0x48, 0x83, 0xc6, (unsigned char) p };
		if(memcmp(data, requiredData, 4) != 0) return false;
		SetCount(GetCount()-4);
		return true;
	}
	else
	{
		if(GetCount() < 7) return false;
		const unsigned char *data = &(*this)[GetCount()-7];
		const unsigned char requiredData[] = { 0x48, 0x81, 0xc6 };
		if(memcmp(data, requiredData, 3) != 0) return false;
		if(*(int *)(data+3) != p) return false;
		SetCount(GetCount()-7);
		return true;
	}
}

void Program::AdjustP(int32_t value)
{
	if(value == 0) return;
	
	if(-128 <= value && value < 128)
	{
		Append(0x48, 0x83, 0xc6, value);					// addq $value, %rsi
	}
	else
	{
		PrepareAppend(7);
		AppendNoExpand(0x48, 0x81, 0xc6);
		AppendDataNoExpand(&value, 4);						// addq $value, %rsi
	}
}

void Program::LoadValueIntoXmm(uint32_t value, uint8_t xmmIndex, bool useAvx)
{
    JASSERT(xmmIndex < 8);  // The encodings below only work for xmm0-xmm7
    
	Append(0xb8); AppendData(&value, 4);					// movl  {value}, %eax
	
	if(useAvx)
	{
		Append(0xc5, 0xf9, 0x6e, 0xc0 + 8*xmmIndex);	// vmovd  %eax, %xmm#
	}
	else
	{
		Append(0x66, 0x48, 0x0f, 0x6e, 0xc0 + 8*xmmIndex);	// movd   %rax, %xmm#
	}
}

void Program::LoadXmmValueFromPointer(uint32_t index)
{
	if(index == 0)
	{
		Append(0xc4, 0xc1, 0x79, 0x6f, 0x02);			// vmovdqa (%r10), %xmm0
	
	}
	else
	{
		Append(0xc4, 0xc1, 0x79, 0x6f, 0x42 + 8*index, index * 0x10);  	// vmovdqa 0x10(%r10), %xmm1
	}
}

void Program::CallPointer()
{
	Append(0x41, 0xff, 0xd2);								// callq  *%r10
}

void Program::ClearProgressCheckTable(uint32_t numberOfProgressChecks)
{
	Append(0x31, 0xc0);											// xor %eax, %eax
	for(int i = 0; i < numberOfProgressChecks; ++i)
	{
		uint32_t offset = 0x28 + 8*i;
		if(offset < 0x80)
		{
			Append(0x48, 0x89, 0x47, offset);					// movq   %rax, 0x28(%rdi)
		}
		else
		{
			PrepareAppend(7);
			AppendNoExpand(0x48, 0x89, 0x87);
			AppendDataNoExpand(&offset, 4);						// movq   %rax, 0x1234(%rdi)
		}
	}
}

void Program::Return()
{
	Append(0xc3);				// ret
}

void Program::BeginStackGuard()
{
	int32_t stackGuardOffset = -INITIAL_STACK_USAGE_LIMIT;
	PrepareAppend(20);
	AppendNoExpand(0x48, 0x8d, 0x84, 0x24); AppendDataNoExpand(&stackGuardOffset, 4);	// leaq   -INITIAL_STACK_USAGE_LIMIT(%rsp), %rax
	AppendNoExpand(0x48, 0xc7, 0x47, 0x20, 0, 0, 0, 0);									// movq   $0x0, 0x20(%rdi)
	AppendNoExpand(0x48, 0x89, 0x47, 0x18);												// movq   %rax, 0x18(%rdi)
}

void Program::EndStackGuard()
{
	const Pattern::StackGrowthHandler* stackGrowthHandler = Pattern::GetStackGrowthHandler();
	if(stackGrowthHandler == &NoStackGrowthHandler::instance)
	{
		Return();
		return;
	}
	
	Append(0x48, 0x83, 0x7f, 0x20, 0x00);						// cmpq   $0x0, 0x20(%rdi)
	uint32_t cleanupPatch = JumpIfShort(NotEqual);
	Return();
	PatchShort(cleanupPatch);

	Append(0x50);												// pushq	%rax
	LoadConstantPointer(((void***) stackGrowthHandler)[0][1]);
	Append(0x48, 0x8b, 0x77, 0x20);								// movq   	0x20(%rdi), %rsi
	Append(0x48, 0xbf); AppendData(&stackGrowthHandler, 8); 	// movabsq	constant, %rdi
	CallPointer();
	Append(0x58);												// popq 	%rax
	Return();
}

void Program::DoStackGuardCheck()
{
	Append(0x48, 0x39, 0x67, 0x18);								// cmpq   %rsp, 0x18(%rdi)
	uint32_t stackGuardCheck = JumpIfLong(AboveOrEqual);
	stackGuardList.Append(stackGuardCheck);
}

void Program::WriteStackGuardList()
{
	for(uint32_t offset : stackGuardList)
	{
		const Pattern::StackGrowthHandler* stackGrowthHandler = Pattern::GetStackGrowthHandler();

		PatchLong(offset);
		
		Append(0xff, 0x77, 0x18);									// pushq  0x18(%rdi)

		Append(0x48, 0x83, 0x7f, 0x20, 0x00);						// cmpq   $0x0, 0x20(%rdi)
		uint32_t reuseAllocationPatch = JumpIfShort(NotEqual);
		
		Append(0x57, 0x56, 0x52, 0x51);								// pushq %rdi, %rsi, %rdx, %rcx
		if(type == PatternProcessorType::BitState)
		{
			Append(0x41, 0x50, 0x41, 0x51);							// pushq %r8, %r9
		}

		Append(0x55);												// pushq  %rbp
		Append(0x48, 0x89, 0xe5);									// movq   %rsp, %rbp
		Append(0x48, 0x83, 0xe4, 0xf0);								// andq   $-0x10, %rsp

		Append(0x48, 0xbf); AppendData(&stackGrowthHandler, 8); 	// movabsq	constant, %rdi
		LoadConstantPointer(((void***) stackGrowthHandler)[0][0]);
		uint32_t stackGrowthSize = STACK_GROWTH_SIZE;
		Append(0xbe); AppendData(&stackGrowthSize, 4);				// movl   STACK_GROWTH_SIZE, %esi
		CallPointer();

		Append(0x48, 0x89, 0xec);									// movq   %rbp, %rsp
		Append(0x5d);												// popq   %rbp

		if(type == PatternProcessorType::BitState)
		{
			Append(0x41, 0x59, 0x41, 0x58);							// popq %r9, %r8
		}
		Append(0x59, 0x5a, 0x5e, 0x5f);								// popq %rcx, %rdx, %rsi, %rdi

		TestValue();
		uint32_t swapStackPatch = JumpIfShort(Program::NotZero);
		Append(0x8f, 0x47, 0x18);									// popq   0x18(%rdi)
		Fail();
		
		PatchShort(reuseAllocationPatch);
		Append(0x48, 0x8b, 0x47, 0x20);								// movq   0x20(%rdi), %rax
		Append(0x48, 0xc7, 0x47, 0x20, 0, 0, 0, 0)			;		// movq   $0x0, 0x20(%rdi)
		
		PatchShort(swapStackPatch);
		
		uint32_t guardBand = STACK_GUARD_BAND;
		Append(0x4c, 0x8d, 0x90); AppendData(&guardBand, 4);		// leaq   0x1234(%rax), %r10
		Append(0x4c, 0x89, 0x57, 0x18);								// movq   %r10, 0x18(%rdi)
		Append(0x48, 0xc7, 0x47, 0x20, 0, 0, 0, 0);					// movq   $0x0, 0x20(%rdi)
		
		uint32_t topOfStack = STACK_GROWTH_SIZE - sizeof(void*);
		Append(0x48, 0x89, 0xa0); AppendData(&topOfStack, 4);		// movq   %rsp, TOP_OF_STACK(%rax)
		Append(0x48, 0x8d, 0xa0); AppendData(&topOfStack, 4);		// leaq   TOP_OF_STACK(%rax), %rsp
		// We're now on a new stack
		
		Call(offset);

		// Returning from the call..
		Append(0x48, 0x83, 0x7f, 0x20, 0x00);						// cmpq   $0x0, 0x20(%rdi)
		uint32_t noFreePatch = JumpIfShort(Equal);
		
		Append(0x50, 0x57, 0x56, 0x52, 0x51);						// pushq %rax, %rdi, %rsi, %rdx, %rcx
		if(type == PatternProcessorType::BitState)
		{
			Append(0x41, 0x50, 0x41, 0x51);							// pushq %r8, %r9
		}
		
		Append(0x55);												// pushq  %rbp
		Append(0x48, 0x89, 0xe5);									// movq   %rsp, %rbp
		Append(0x48, 0x83, 0xe4, 0xf0);								// andq   $-0x10, %rsp

		LoadConstantPointer(((void***) stackGrowthHandler)[0][1]);
		Append(0x48, 0x8b, 0x77, 0x20);								// movq   	0x20(%rdi), %rsi
		Append(0x48, 0xbf); AppendData(&stackGrowthHandler, 8); 	// movabsq	constant, %rdi
		CallPointer();

		Append(0x48, 0x89, 0xec);									// movq   %rbp, %rsp
		Append(0x5d);												// popq   %rbp

		if(type == PatternProcessorType::BitState)
		{
			Append(0x41, 0x59, 0x41, 0x58);							// popq %r9, %r8
		}
		Append(0x59, 0x5a, 0x5e, 0x5f, 0x58);						// popq %rcx, %rdx, %rsi, %rdi, %rax
		
		PatchShort(noFreePatch);
		
		int32_t baseOfStack = -topOfStack;
		Append(0x4c, 0x8d, 0x94, 0x24); AppendData(&baseOfStack, 4);	// leaq   -0x12345678(%rsp), %r10
		Append(0x48, 0x8b, 0x24, 0x24);       							// movq   (%rsp), %rsp
		Append(0x4c, 0x89, 0x57, 0x20);									// movq   %r10, 0x20(%rdi)
		Append(0x8f, 0x47, 0x18);										// popq   0x18(%rdi)
		Return();
	}
}

//============================================================================

void Amd64ProcessorBase::GroupPEndCheck(uint32_t pc, const PatternData& patternData, Program& program, bool skipCheck)
{
	// We don't need a numberOfInstructions check, because a byte consumer will always have
	// a subsequent instruction, eg. fail, success, match
	
	static constexpr EnumSet<InstructionType, uint64_t> COMBINE_PEND_CHECK_INSTRUCTIONS
	{
		InstructionType::AdvanceByte,
		InstructionType::AnyByte,
		InstructionType::Byte,
		InstructionType::ByteEitherOf2,
		InstructionType::ByteEitherOf3,
		InstructionType::ByteRange,
		InstructionType::ByteBitMask,
		InstructionType::ByteNot,
		InstructionType::ByteNotEitherOf2,
		InstructionType::ByteNotEitherOf3,
		InstructionType::ByteNotRange
	};
	
	if(pc < program.checkPEndPc
	   || !COMBINE_PEND_CHECK_INSTRUCTIONS.Contains(patternData[pc].type)) return;
	
	if(patternData[pc].type == InstructionType::AdvanceByte) return;
	
	uint32_t pcEnd = pc+1;
	while(patternData[pcEnd].isSingleReference
		  && COMBINE_PEND_CHECK_INSTRUCTIONS.Contains(patternData[pcEnd].type)) ++pcEnd;
	
	if(pcEnd - pc < 2) return;
	
	program.checkStartPc = pc;
	program.checkPEndPc = pcEnd;
	if(!skipCheck)
	{
		program.CalculateCapture(pcEnd-pc);
		program.Append(0x49, 0x39, 0xd2);			//     cmpq   %rdx, %r10
		program.FailIf(Program::Above);
	}
}

// AMD64 ABI:
// Registers passed in RDI, RSI, RDX, RCX, R8, and R9
// Callee save registers: RBP, RBX, and R12–R15
// Return value RAX

// We'll use RDI = ProcessData&
//           RSI = const char* p
//           RDX = const char* pEnd
//           RCX = const char** captures
//
// For bitstate only:
//           R8  = bitData*
//           R9  = numberOfBitTrackingBytes
void Amd64ProcessorBase::Set(const void* data, size_t length, PatternProcessorType type, int bytesForBitState)
{
	const ByteCodeHeader* header = (const ByteCodeHeader*) data;
	const uint32_t numberOfInstructions = header->numberOfInstructions;
	const PatternData patternData(header->GetForwardProgram());
	const bool useStackGuard = header->flags.useStackGuard && type == PatternProcessorType::BackTracking;

	allowPartialMatch = !header->flags.matchRequiresEndOfInput;
	numberOfCaptures = header->numberOfCaptures;
	expandedJumpTables.Allocate(patternData, numberOfInstructions);
	
	Table<uint32_t> instructionOffsets;
	instructionOffsets.SetCount(numberOfInstructions);
	
	// Start off by appending a fail.
	Program program(type);
	program.bytesForBitState = bytesForBitState;
	program.Reserve(numberOfInstructions*32);
	program.Nop(1);						// Valid jump targets cannot be 32-bit aligned due to the
										// implementation of ByteJumpTable
	program.Append(0x31, 0xc0);			// xor %eax, %eax
	program.Return();

	int bitStateOffset = 0;
	
	// Now step through each instruction
	for(uint32_t pc = 0; pc < numberOfInstructions; ++pc)
	{
		// Force instructions not to be 32 byte aligned to differentiate between jump table address and code address
		if(type != PatternProcessorType::BitState && (program.GetCount() & 31) == 0) program.Nop(1);
		
		ByteCodeInstruction instruction = patternData[pc];

		instructionOffsets[pc] = uint32_t(program.GetCount());
		
		if(type == PatternProcessorType::BitState
		   && !instruction.isSingleReference)
		{
			program.DoBitStateCheck(bitStateOffset++);
		}

		GroupPEndCheck(pc, patternData, program, header->IsFixedLength() && header->IsAnchored());
		
		switch(instruction.type)
		{
		case InstructionType::AdvanceByte:
			//	++p;
			program.IncrementPForPc(pc);
			break;
			
		case InstructionType::AnyByte:
			//	if(p == pEnd) return nullptr;
			//	++p;
			if(type != PatternProcessorType::BitState
			   && patternData.GetMaximalJumpTargetForPc(pc) != TypeData<uint32_t>::Maximum())
			{
				// We have a loop from the end instead.
				program.Append(0x53);										// pushq  %rbx
				program.Append(0x48, 0x89, 0xf3);							// movq   %rsi, %rbx
				program.Append(0x48, 0x89, 0xd6);							// movq   %rdx, %rsi
				
				uint32_t loopPoint = program.GetCount();
				program.Append(0x48, 0x39, 0xf3);							// cmpq   %rsi, %rbx
				uint32_t fail = program.JumpIfShort(Program::Above);
				
				program.PushP();
				program.CallAndMarkPatch(patternData.GetMaximalJumpTargetForPc(pc));
				program.PopP();
				program.AdjustP(-1);
				program.TestValue();
				program.JumpIf(Program::Zero, loopPoint);
				
				program.Append(0x5b);              							// popq   %rbx
				program.Return();
				
				program.PatchShort(fail);
				program.Append(0x5b);              							// popq   %rbx
				program.Fail();
			}
			else
			{
				program.CheckPToPStop(pc);
				program.IncrementPForPc(pc);
			}
			break;

		case InstructionType::AssertStartOfInput:
			//	if(p != pStart) return nullptr;
			program.ComparePToPStart();
			program.FailIfNotEqual();
			break;

		case InstructionType::AssertEndOfInput:
			//	if(p != pEnd) return nullptr;
			program.ComparePToPStop();
			program.FailIfNotEqual();
			break;
				
		case InstructionType::AssertStartOfLine:
		//	if(p != pStart && p[-1] != '\n') return nullptr;
			{
				program.ComparePToPStart();
				uint32_t next = program.JumpIfShort(Program::Equal);
				
				program.CompareByteWithOffset('\n', -1);
				program.FailIf(Program::NotEqual);
				
				program.PatchShort(next);
			}
			break;

		case InstructionType::AssertEndOfLine:
		//	if(p != pEnd && *p != '\n') return nullptr;
			{
				program.ComparePToPStop();
				uint32_t next = program.JumpIfShort(Program::Equal);
				
				program.CompareCurrentByte('\n');
				program.FailIf(Program::NotEqual);
				
				program.PatchShort(next);
			}
			break;

		case InstructionType::AssertWordBoundary:
		//  if(p != pEnd)
		//  {
		//     if(p != pStart)
		//     {
		//       if(WORD_MASK[p[-1]] == WORD_MASK[p[0]]) return nullptr;
		//     }
		//     else
		//     {
		//       if(WORD_MASK[p[0]] == 0) return nullptr;
		//     }
		//  }
		//  else
		//  {
		//     if(p == pStart) return nullptr;
		//     if(WORD_MASK[p[-1]] == 0) return nullptr;
		//  }
			switch(instruction.data)
			{
			case ByteCodeWordBoundaryHint::PreviousIsWordOnly:
				{
					program.LoadConstantPointer(PatternProcessorBase::WORD_MASK);
					program.ComparePToPStop();
					uint32_t atEnd = program.JumpIfShort(Program::Equal);
					
					program.ReadByte();
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					program.FailIfEqual();
					
					program.PatchShort(atEnd);
				}
				break;

			case ByteCodeWordBoundaryHint::PreviousIsNotWordOnly:
				{
					program.LoadConstantPointer(PatternProcessorBase::WORD_MASK);
					program.ComparePToPStop();
					program.FailIf(Program::Equal);
					
					program.ReadByte();
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					program.FailIfNotEqual();
				}
				break;
					
			case ByteCodeWordBoundaryHint::NextIsWordOnly:
				{
					program.LoadConstantPointer(PatternProcessorBase::WORD_MASK);
					program.ComparePToPStart();
					uint32_t atStart = program.JumpIfShort(Program::Equal);

					program.ReadByteWithOffset(-1);
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					program.FailIfEqual();

					program.PatchShort(atStart);
				}
				break;
					
			case ByteCodeWordBoundaryHint::NextIsNotWordOnly:
				{
					program.LoadConstantPointer(PatternProcessorBase::WORD_MASK);
					program.ComparePToPStart();
					program.FailIf(Program::Equal);
					
					program.ReadByteWithOffset(-1);
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					program.FailIfNotEqual();
				}
				break;

			default:
				{
					program.LoadConstantPointer(PatternProcessorBase::WORD_MASK);
					program.ComparePToPStop();
					uint32_t atEnd = program.JumpIfShort(Program::Equal);
					program.ComparePToPStart();
					uint32_t atStart = program.JumpIfShort(Program::Equal);
					
					program.Append(0x0f, 0xb6, 0x06);				// movzbl (%rsi), %eax
					program.Append(0x44, 0x0f, 0xb6, 0x5e, 0xff);	// movzbl -0x1(%rsi), %r11d
					program.Append(0x41, 0x8a, 0x04, 0x02);			// movb   (%r10,%rax), %al
					program.Append(0x47, 0x8a, 0x1c, 0x1a);			// movb   (%r10,%r11), %r11b
					program.Append(0x41, 0x38, 0xc3);        		// cmpb   %al, %r11b
					program.FailIfEqual();
					uint32_t next = program.JumpShort();
					
					program.PatchShort(atStart);					// atStart:
					program.ReadByte();
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					program.FailIfNotEqual();
					uint32_t next2 = program.JumpShort();
					
					program.PatchShort(atEnd);						// atEnd:
					program.ComparePToPStart();
					program.FailIfEqual();
					program.ReadPreviousByte();
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					program.FailIfNotEqual();

					program.PatchShort(next);
					program.PatchShort(next2);
				}
				break;
			}
			break;

		case InstructionType::AssertNotWordBoundary:
			//	if(p != pEnd && IsWordCharacter(p[0]))
			//	{
			//		if(p == pStart || !IsWordCharacter(p[-1])) return nullptr;
			//	}
			//	else
			//	{
			//		if(p != pStart && IsWordCharacter(p[-1])) return nullptr;
			//	}
			switch(instruction.data)
			{
			case ByteCodeWordBoundaryHint::PreviousIsWordOnly:
				{
					program.LoadConstantPointer(PatternProcessorBase::WORD_MASK);
					program.ComparePToPStop();
					program.FailIf(Program::Equal);
					
					program.ReadByte();
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					program.FailIfNotEqual();
				}
				break;
					
			case ByteCodeWordBoundaryHint::PreviousIsNotWordOnly:
				{
					program.LoadConstantPointer(PatternProcessorBase::WORD_MASK);
					program.ComparePToPStop();
					uint32_t atEnd = program.JumpIfShort(Program::Equal);
					
					program.ReadByte();
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					program.FailIfEqual();
					
					program.PatchShort(atEnd);
				}
				break;
					
			case ByteCodeWordBoundaryHint::NextIsWordOnly:
				{
					program.LoadConstantPointer(PatternProcessorBase::WORD_MASK);
					program.ComparePToPStart();
					program.FailIf(Program::Equal);
					
					program.ReadByteWithOffset(-1);
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					program.FailIfNotEqual();
				}
				break;
					
			case ByteCodeWordBoundaryHint::NextIsNotWordOnly:
				{
					program.LoadConstantPointer(PatternProcessorBase::WORD_MASK);
					program.ComparePToPStart();
					uint32_t atStart = program.JumpIfShort(Program::Equal);
					
					program.ReadByteWithOffset(-1);
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					program.FailIfEqual();
					
					program.PatchShort(atStart);
				}
				break;
					
			default:
				{
					program.LoadConstantPointer(PatternProcessorBase::WORD_MASK);
					program.ComparePToPStop();
					uint32_t atEnd = program.JumpIfShort(Program::Equal);
					program.ReadByte();
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					uint32_t endIsWordCharacter = program.JumpIfShort(Program::Equal);
					
					// DoesNotHaveWordCharacterAtEnd:
					program.PatchShort(atEnd);
					program.ComparePToPStart();
					uint32_t atStart = program.JumpIfShort(Program::Equal);
					program.ReadPreviousByte();
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					program.FailIf(Program::NotEqual);
					uint32_t success = program.JumpShort();
					
					// HasWordCharacterAtEnd:
					program.PatchShort(endIsWordCharacter);
					program.ComparePToPStart();
					program.FailIf(Program::Equal);
					program.ReadPreviousByte();
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					program.FailIf(Program::Equal);
					
					program.PatchShort(atStart);
					program.PatchShort(success);
				}
				break;
			}
			break;
				
		case InstructionType::AssertStartOfSearch:
			program.ComparePToPSearchStart();
			program.FailIfNotEqual();
			break;
				
		case InstructionType::AssertRecurseValue:
		// if(processData.recurseValue != instruction.data) return nullptr;
			{
				uint32_t recurseValue = instruction.data;
				program.Append(0x81, 0x7f, 0x10); program.AppendData(&recurseValue, 4);		// cmpl   {value}, 0x04(%rdi)
				program.FailIfNotEqual();
			}
			break;
				
		case InstructionType::BackReference:
		//	{
		//		uint32_t index = instruction.data*2;
		//		ssize_t length = captures[index+1] - captures[index];
		//		if(length >= 0
		//		   && p+length <= pEnd
		//		   && memcmp(captures[index], p, length) == 0)
		//		{
		//			p += length;
		//			++pc;
		//			goto Loop;
		//		}
		//	}
		//	return nullptr;
			{
				uint32_t captureOffset1 = instruction.data*16;
				uint32_t captureOffset2 = instruction.data*16+8;
				
				program.Append(0x57, 0x51);													// pushq  %rdi, %rcx
				program.Append(0x48, 0x8b, 0xb9); program.AppendData(&captureOffset1, 4);	// movq   offset(%rcx), %rdi
				program.Append(0x48, 0x8b, 0x89); program.AppendData(&captureOffset2, 4);	// movq   offset(%rcx), %rcx
				program.Append(0x48, 0x29, 0xf9);											// subq	  %rdi, %rcx
				uint32_t patch1 = program.JumpIfShort(Program::Below);
				program.Append(0x48, 0x8d, 0x04, 0x0e);										// leaq   (%rsi,%rcx), %rax
				program.Append(0x48, 0x39, 0xc2);											// cmpq   %rax, %rdx
				uint32_t patch2 = program.JumpIfShort(Program::Below);
				program.Append(0xf3, 0xa6);													// rep cmpsb
				
				program.PatchShort(patch1);
				program.PatchShort(patch2);
				program.Append(0x59, 0x5f);													// popq   %rcx, %rdi
				program.FailIfNotEqual();													// This will catch both of the Below cases above.
			}
			break;
				
		case InstructionType::Byte:
        case InstructionType::ByteEitherOf2:
			{
				enum class RepeatType
				{
					Byte,
					ByteEitherOf2,
					ByteRange
				};
				
				int repeat = 1;
				int numberOfConsecutiveBytes = 1;
				RepeatType repeatType = RepeatType::Byte;
				while(pc+repeat < numberOfInstructions && patternData[pc+repeat].isSingleReference)
				{
					switch(patternData[pc+repeat].type)
					{
					case InstructionType::Byte:
						++repeat;
						if (repeatType == RepeatType::Byte || repeatType == RepeatType::ByteEitherOf2)
						{
							++numberOfConsecutiveBytes;
						}
						continue;
						
					case InstructionType::ByteEitherOf2:
						if(repeatType == RepeatType::Byte) repeatType = RepeatType::ByteEitherOf2;
						if(repeatType == RepeatType::ByteEitherOf2
						   && patternData[pc+repeat].IsEitherOf2AndBitMaskIdentifiable())
						{
							++repeat;
							++numberOfConsecutiveBytes;
							continue;
						}
						break;
						
					case InstructionType::ByteRange:
						if(repeatType == RepeatType::Byte) repeatType = RepeatType::ByteRange;
						if(repeatType == RepeatType::ByteRange
						   && patternData[pc+repeat].GetRange().GetSize() < 128)
						{
							++repeat;
							continue;
						}
						break;
						
					default:
						break;
					}
					break;
				}

				if(repeat >= 4 && repeatType == RepeatType::ByteRange) goto InstructionTypeByteRange;
				
				int currentOffset = 0;
				int extraOffset = 0;
				if(pc < program.checkPEndPc)
				{
					extraOffset += pc - program.checkStartPc;
				}
				
				for(;currentOffset+4 <= numberOfConsecutiveBytes; currentOffset += 4)
				{
					uint32_t data = 0;
					uint32_t mask = 0;
					for(int i = 0; i < 4; ++i)
					{
						const ByteCodeInstruction &instr = patternData[pc+currentOffset+i];
						switch(instr.type)
						{
						case InstructionType::Byte:
							data |= instr.data << (8*i);
							break;
							
						case InstructionType::ByteEitherOf2:
							{
								int valueA = instr.value      & 0xff;
								int valueB = instr.value >> 8 & 0xff;
								int xorValue = valueA ^ valueB;
								data |= (valueA | xorValue) << (8*i);
								mask |= xorValue << (8*i);
							}
							break;
							
						default:
							JERROR("Unexpected");
							JUNREACHABLE;
						}
					}
					
					if(mask == 0)
					{
						program.CompareLongWithOffset(data, currentOffset+extraOffset);
						program.FailIf(Program::NotEqual);
					}
					else
					{
						program.ReadLongWithOffset(currentOffset+extraOffset);
						program.OrLong(mask);
						program.CompareValue(data);
						program.FailIf(Program::NotEqual);
					}
				}
				for(; currentOffset < numberOfConsecutiveBytes; ++currentOffset)
				{
                    const ByteCodeInstruction &instr = patternData[pc+currentOffset];
                    switch(instr.type)
                    {
                    case InstructionType::Byte:
                        program.CompareByteWithOffset(instr.data, currentOffset+extraOffset);
                        program.FailIf(Program::NotEqual);
                        break;
                        
                    case InstructionType::ByteEitherOf2:
                        {
                            uint8_t valueA = instr.data    & 0xff;
                            uint8_t valueB = instr.data>>8 & 0xff;
                            uint8_t xorResult = valueA ^ valueB;
                            program.ReadByteWithOffset(currentOffset+extraOffset);
                            if (BitUtility::IsPowerOf2(xorResult))
                            {
                                program.OrByte(xorResult);
                                program.CompareByte(valueA | xorResult);
                                program.FailIf(Program::NotEqual);
                            }
                            else
                            {
                                program.CompareByte(valueA);
                                uint32_t next = program.JumpIfShort(Program::Equal);
                                program.CompareByte(valueB);
                                program.FailIf(Program::NotEqual);
                                program.PatchShort(next);
                            }
                        }
                        break;
                        
                    default:
                        JERROR("Unexpected");
                        JUNREACHABLE;
                            
                    }
				}
				pc += numberOfConsecutiveBytes-1;
				program.IncrementPForPc(pc);
			}
			break;
				
		case InstructionType::ByteBitMask:
			//	if(p == pEnd) return nullptr;
			//	else
			//	{
			//		const StaticBitTable<256>& data = *patternData.GetData<StaticBitTable<256>>(instruction.data);
			//		if(!data[*p]) return nullptr;
			//	}
			//	++p;

			{
				const void* bitMaskData = expandedJumpTables.GetJumpTable(pc);
				JASSERT(bitMaskData != nullptr);

				program.LoadConstantPointer(bitMaskData);
				program.CheckPToPStop(pc);
				
				int repeat = 1;
				while(pc+repeat < numberOfInstructions
					  && patternData[pc+repeat].type == InstructionType::ByteBitMask
					  && patternData[pc+repeat].isSingleReference
					  && patternData[pc+repeat].data == instruction.data)
				{
					++repeat;
				}
				
				int i = 0;
				for(; i+2 <= repeat; i += 2)
				{
					program.ReadByteForPc(pc+i);
					program.IncrementPForPc(pc+i);
					program.ReadAlternateByteForPc(pc+i+1);
					program.Append(0x41, 0x8a, 0x04, 0x02);		// movb   (%r10,%rax), %al
					program.Append(0x43, 0x84, 0x04, 0x1a);		// testb  (%r10,%r11), %al
					program.FailIf(Program::Zero);
					program.IncrementPForPc(pc+i+1);
				}
				
				for(; i < repeat; ++i)
				{
					program.ReadByteForPc(pc+i);
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					program.FailIf(Program::NotEqual);
					program.IncrementPForPc(pc+i);
				}
				
				pc += repeat-1;
			}
			break;
				
		case InstructionType::ByteEitherOf3:
			//	if(p == pEnd) return nullptr;
			//	if(*p != (instruction.data & 0xff)
			//	   && *p != ((instruction.data>>8) & 0xff)
			//	   && *p != ((instruction.data>>16) & 0xff)) return nullptr;
			//	++p;
			{
				program.CheckPToPStop(pc);
				program.ReadByteForPc(pc);
				program.IncrementPForPc(pc);
				program.CompareByte(instruction.data     & 0xff);
				uint32_t next1 = program.JumpIfShort(Program::Equal);
				program.CompareByte(instruction.data>>8  & 0xff);
				uint32_t next2 = program.JumpIfShort(Program::Equal);
				program.CompareByte(instruction.data>>16 & 0xff);
				program.FailIf(Program::NotEqual);
				program.PatchShort(next1);
				program.PatchShort(next2);
			}
			break;

		InstructionTypeByteRange:
		case InstructionType::ByteRange:
			//	if(p == pEnd) return nullptr;
			//	else
			//	{
			//		unsigned char low = instruction.data & 0xff;
			//		unsigned char high = (instruction.data >> 8) & 0xff;
			//		if(*p < low || *p > high) return nullptr;
			//	}
			{
				program.CheckPToPStop(pc);

				int repeat = 1;
				while(pc+repeat < numberOfInstructions
					  && patternData[pc+repeat].isSingleReference
					  && ((patternData[pc+repeat].type == InstructionType::ByteRange
					      && patternData[pc+repeat].GetRange().GetSize() < 128)
					     || patternData[pc+repeat].type == InstructionType::Byte))
				{
					++repeat;
				}

				int extraOffset = 0;
				if(pc < program.checkPEndPc)
				{
					extraOffset += pc - program.checkStartPc;
				}

				int i = 0;
				for(; i+4 <= repeat; i += 4)
				{
					program.ReadLongWithOffset(extraOffset+i);
					
					uint32_t minValues = 0;
					uint32_t maxValues = 0;
					for(int j = 0; j < 4; ++j)
					{
						Interval<uint8_t> range = patternData[pc+i+j].GetRange();
						minValues |= range.min << 8*j;
						maxValues |= range.max << 8*j;
					}

					program.Append(0x41, 0xba);
					program.AppendData(&maxValues, 4);				// movl {maxValues}, %r10d
					
					program.Append(0x41, 0x29, 0xc2);				// subl %eax, %r10d
					
					program.SubtractValue(minValues);
					
					program.Append(0x44, 0x09, 0xd0);				// orl %r10d, %eax
					program.Append(0xa9, 0x80, 0x80, 0x80, 0x80);	// testl $0x80808080, %eax
					program.FailIf(Program::NotZero);
				}
				
				for(; i < repeat; ++i)
				{
					Interval<uint8_t> range = patternData[pc+i].GetRange();
					program.ReadByteWithOffset(extraOffset+i);
					program.SubtractValue(range.min);
					program.CompareValue(range.GetSize());
					program.FailIfAbove();
				}
				
				pc += repeat-1;
				program.IncrementPForPc(pc);
			}
			break;
			
		case InstructionType::ByteNot:
			//	if(p == pEnd) return nullptr;
			//	if(*p == instruction.data) return nullptr;
			{
				int offset = 0;
				if(pc < program.checkPEndPc) offset = pc-program.checkStartPc;
				
				program.CheckPToPStop(pc);
				program.CompareByteWithOffset(instruction.data, offset);
				program.FailIfEqual();
				program.IncrementP();
			}
			break;
			
		case InstructionType::ByteNotEitherOf2:
			//	if(p == pEnd) return nullptr;
			//	if(*p == (instruction.data & 0xff)
			//	   || *p == ((instruction.data>>8) & 0xff)) return nullptr;
			program.CheckPToPStop(pc);
			program.ReadByteForPc(pc);
			program.IncrementPForPc(pc);
			program.CompareByte(instruction.data & 0xff);
			program.FailIfEqual();
			program.CompareByte(instruction.data>>8 & 0xff);
			program.FailIfEqual();
			break;
				
		case InstructionType::ByteNotEitherOf3:
			//	if(p == pEnd) return nullptr;
			//	if(*p == (instruction.data & 0xff)
			//	   || *p == ((instruction.data>>8) & 0xff)
			//	   || *p == ((instruction.data>>16) & 0xff)) return nullptr;
			program.CheckPToPStop(pc);
			program.ReadByteForPc(pc);
			program.IncrementPForPc(pc);
			program.CompareByte(instruction.data & 0xff);
			program.FailIfEqual();
			program.CompareByte(instruction.data>>8 & 0xff);
			program.FailIfEqual();
			program.CompareByte(instruction.data>>16 & 0xff);
			program.FailIfEqual();
			break;
			
		case InstructionType::ByteNotRange:
			//	if(p == pEnd) return nullptr;
			//	else
			//	{
			//		unsigned char low = instruction.data & 0xff;
			//		unsigned char high = (instruction.data >> 8) & 0xff;
			//		if(low <= *p && *p <= high) return nullptr;
			//	}
			//	++p;
			//
			{
				uint32_t low = instruction.data & 0xff;
				uint32_t high = (instruction.data >> 8) & 0xff;

				program.CheckPToPStop(pc);
				program.ReadByteForPc(pc);
				program.IncrementPForPc(pc);
				program.SubtractValue(low);
				program.CompareValue(high-low);
				program.FailIf(Program::BelowOrEqual);
			}
			break;
				
		case InstructionType::ByteJumpTable:
			//	if(p == pEnd) return nullptr;
			//	else
			//	{
			//		const ByteCodeJumpTableData* data = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
			//		pc = data->pcData[data->jumpTable[*p]];
			//		if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
			//		++p;
			//		goto Loop;
			//	}
			{
				const void* jumpTableData = expandedJumpTables.GetJumpTable(pc);
				JASSERT(jumpTableData != nullptr);

				const ByteCodeJumpTableData* data = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
				bool useJumpDispatch = (data->numberOfTargets <= 3);
				
				if(useJumpDispatch)
				{
					program.LoadConstantPointer(data->jumpTable);
				}
				else
				{
					program.LoadConstantPointer(jumpTableData);
				}

				uint32_t checkedRepeat = program.GetCount();
				program.CheckPToPStop(pc);
				uint32_t repeat = program.GetCount();
				program.ReadByte();															// %rax now contains *p
				program.IncrementP();

				if(useJumpDispatch)
				{
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					
					if(data->pcData[0] == TypeData<uint32_t>::Maximum())
					{
						program.FailIf(Program::Below);
					}
					else
					{
						if(data->pcData[0] == pc) program.JumpIf(Program::Below, checkedRepeat);
						else program.JumpIfAndMarkPatch(Program::Below, data->pcData[0]);
					}
					
					if(data->pcData[1] == pc+1)
					{
						if(data->pcData[2] == pc) program.JumpIf(Program::Above, checkedRepeat);
						else program.JumpIfAndMarkPatch(Program::Above, data->pcData[2]);
					}
					else
					{
						if(data->pcData[1] == pc) program.JumpIf(Program::Equal, checkedRepeat);
						else program.JumpIfAndMarkPatch(Program::Equal, data->pcData[1]);
						if(data->pcData[2] != pc+1)
						{
							if(data->pcData[2] == pc) program.Jump(checkedRepeat);
							else program.JumpAndMarkPatch(data->pcData[2]);
						}
					}
				}
				else if(type == PatternProcessorType::BitState
				   || !patternData.HasLookupTableTarget(*data))
				{
					program.Dispatch();
				}
				else
				{
					program.Append(0xc1, 0xe0, 0x03);			// shll   $0x3, %eax
					program.Append(0x4d, 0x8b, 0x14, 0x02);		// movq   (%r10,%rax), %r10
					program.Append(0x41, 0xf6, 0xc2, 0x1f);		// testb  $0x1f, %r10b
					uint32_t dispatch = program.JumpIfShort(Program::NotZero);
					program.ComparePToPStop();
					program.JumpIf(Program::Below, repeat);
					program.Fail();
					program.PatchShort(dispatch);
					program.Append(0x41, 0xff, 0xe2);			// jmpq   *%r10
				}
			}
			break;
				
		case InstructionType::ByteJumpMask:
			//	if(p == processData.pEnd) return nullptr;
			//	else
			//	{
			//		const ByteCodeJumpMaskData* data = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
			//		pc = data->pcData[data->bitMask[*p]];
			//		if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
			//		++p;
			//		goto Loop;
			//	}				
			{
				const ByteCodeJumpMaskData* data = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
				const void* jumpTableData = expandedJumpTables.GetJumpTable(pc);
				JASSERT(jumpTableData != nullptr);

				program.LoadConstantPointer(jumpTableData);
				program.CheckPToPStop(pc);
				uint32_t repeat = program.GetCount();
				program.ReadByte();															// %rax now contains *p
				program.IncrementP();
				program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)

				if(data->pcData[0] == pc)
				{
					program.JumpIfAndMarkPatch(Program::Equal, data->pcData[1]);
					program.ComparePToPStop();
					program.JumpIf(Program::Below, repeat);
					program.Fail();
				}
				else if(data->pcData[1] == pc)
				{
					if(data->pcData[0] == TypeData<uint32_t>::Maximum())
					{
						program.FailIf(Program::NotEqual);
					}
					else
					{
						program.JumpIfAndMarkPatch(Program::NotEqual, data->pcData[0]);
					}
					program.ComparePToPStop();
					program.JumpIf(Program::Below, repeat);
					program.Fail();
				}
				else if(data->pcData[0] == pc+1)
				{
					program.JumpIfAndMarkPatch(Program::Zero, data->pcData[1]);
				}
				else if(data->pcData[1] == pc+1)
				{
					if(data->pcData[0] == TypeData<uint32_t>::Maximum())
					{
						program.FailIf(Program::NotZero);
					}
					else
					{
						program.JumpIfAndMarkPatch(Program::NotZero, data->pcData[0]);
					}
				}
				else
				{
					if(data->pcData[0] == TypeData<uint32_t>::Maximum())
					{
						program.FailIf(Program::NotZero);
					}
					else
					{
						program.JumpIfAndMarkPatch(Program::NotZero, data->pcData[0]);
					}
					
					program.JumpAndMarkPatch(data->pcData[1]);
				}
			}
			break;
				
		case InstructionType::ByteJumpRange:
			//	if(p == processData.pEnd) return nullptr;
			//	else
			//	{
			//		const ByteCodeJumpRangeData* jumpRangeData = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
			//		pc = jumpRangeData->pcData[jumpRangeData->range.Contains(*p)];
			//		if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
			//		++p;
			//		goto Loop;
			//	}
			//	}
			{
				const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
				
				program.CheckPToPStop(pc);
				uint32_t repeat = program.GetCount();
				program.ReadByte();
				program.IncrementP();
				
				program.SubtractValue(data->range.min);
				program.CompareValue(data->range.GetSize());
				// Flags are:
				// Above if outside the range
				// BelowOrEqual if inside the range
				
				if(data->pcData[0] == pc)
				{
					program.JumpIfAndMarkPatch(Program::BelowOrEqual, data->pcData[1]);

					program.ComparePToPStop();
					program.JumpIf(Program::Below, repeat);
					program.Fail();
				}
				else if(data->pcData[1] == pc)
				{
					if(data->pcData[0] == TypeData<uint32_t>::Maximum())
					{
						program.FailIf(Program::Above);
					}
					else
					{
						program.JumpIfAndMarkPatch(Program::Above, data->pcData[0]);
					}
					program.ComparePToPStop();
					program.JumpIf(Program::Below, repeat);
					program.Fail();
				}
				else if(data->pcData[0] == pc+1)
				{
					program.JumpIfAndMarkPatch(Program::BelowOrEqual, data->pcData[1]);
				}
				else if(data->pcData[1] == pc+1)
				{
					if(data->pcData[0] == TypeData<uint32_t>::Maximum())
					{
						program.FailIf(Program::Above);
					}
					else
					{
						program.JumpIfAndMarkPatch(Program::Above, data->pcData[0]);
					}
				}
				else
				{
					if(data->pcData[0] == TypeData<uint32_t>::Maximum())
					{
						program.FailIf(Program::Above);
					}
					else
					{
						program.JumpIfAndMarkPatch(Program::Above, data->pcData[0]);
					}
					
					program.JumpAndMarkPatch(data->pcData[1]);
				}
			}
			break;
				
		case InstructionType::Call:
			//	const ByteCodeCallData* callData = patternData.GetData<ByteCodeCallData>(instruction.data);
			//	bool result = Process(callData->callIndex, p, processData);
			//	pc = (&callData->falseIndex)[result];
			//	if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
			{
				const ByteCodeCallData* callData = patternData.GetData<ByteCodeCallData>(instruction.data);
				program.PushP();
				program.CallAndMarkPatch(callData->callIndex);
				program.PopP();
				program.TestValue();

				if(callData->falseIndex == TypeData<uint32_t>::Maximum())
				{
					program.FailIf(Program::Zero);
				}
				else
				{
					program.JumpIfAndMarkPatch(Program::Zero, callData->falseIndex);
				}
				
				if(callData->trueIndex == TypeData<uint32_t>::Maximum())
				{
					program.Fail();
				}
				else
				{
					program.JumpAndMarkPatch(callData->trueIndex);
				}
			}
			break;
				
		case InstructionType::DispatchTable:
			//	{
			//		const ByteCodeJumpTableData* data = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
			//		pc = data->pcData[p == pEnd ? 0 : data->jumpTable[*p]];
			//		if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
			//	}
			{
				const void* jumpTableData = expandedJumpTables.GetJumpTable(pc);
				JASSERT(jumpTableData != nullptr);
				const ByteCodeJumpTableData* data = patternData.GetData<ByteCodeJumpTableData>(instruction.data);
				bool useJumpDispatch = data->numberOfTargets <= 3;
				
				if(useJumpDispatch)
				{
					program.LoadConstantPointer(data->jumpTable);
				}
				else
				{
					program.LoadConstantPointer(jumpTableData);
				}
				if(data->pcData[0] == TypeData<uint32_t>::Maximum())
				{
					program.CheckPToPStop(pc);
				}
				else
				{
					program.ComparePToPStop();
					program.JumpIfAndMarkPatch(Program::Equal, data->pcData[0]);
				}
				
				program.ReadByte();
				if(useJumpDispatch)
				{
					program.Append(0x41, 0x80, 0x3c, 0x02, 0x01);	// cmpb   $0x1, (%r10,%rax)
					
					if(data->pcData[0] == TypeData<uint32_t>::Maximum())
					{
						if(data->HasJumpTableTarget(0))
						{
							program.FailIf(Program::Below);
						}
					}
					else
					{
						program.JumpIfAndMarkPatch(Program::Below, data->pcData[0]);
					}
					
					if(data->numberOfTargets <= 2)
					{
						if(data->pcData[1] != pc+1)
						{
							program.JumpIfAndMarkPatch(Program::Equal, data->pcData[1]);
						}
					}
					else
					{
						if(data->pcData[1] == pc+1)
						{
							program.JumpIfAndMarkPatch(Program::Above, data->pcData[2]);
						}
						else
						{
							program.JumpIfAndMarkPatch(Program::Equal, data->pcData[1]);
							if(data->pcData[2] != pc+1)
							{
								program.JumpAndMarkPatch(data->pcData[2]);
							}
						}
					}
				}
				else
				{
					program.Dispatch();
				}
			}
			break;

		case InstructionType::DispatchMask:
			//	{
			//		const ByteCodeJumpMaskData* data = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
			//		pc = data->pcData[p == processData.pEnd ? 0 : data->bitMask[*p]];
			//		if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
			//		goto Loop;
			//	}
			{
				const void* jumpTableData = expandedJumpTables.GetJumpTable(pc);
				JASSERT(jumpTableData != nullptr);
				const ByteCodeJumpMaskData* data = patternData.GetData<ByteCodeJumpMaskData>(instruction.data);
				program.LoadConstantPointer(jumpTableData);
				if(data->pcData[0] == TypeData<uint32_t>::Maximum())
				{
					program.CheckPToPStop(pc);
				}
				else
				{
					program.ComparePToPStop();
					program.JumpIfAndMarkPatch(Program::Equal, data->pcData[0]);
				}
				program.ReadByte();								// %rax now contains *p
				program.Append(0x41, 0x80, 0x3c, 0x02, 0x00);	// cmpb   $0x0, (%r10,%rax)
				
				if(data->pcData[0] == pc+1)
				{
					program.JumpIfAndMarkPatch(Program::NotZero, data->pcData[1]);
				}
				else if(data->pcData[1] == pc+1)
				{
					if(data->pcData[0] == TypeData<uint32_t>::Maximum())
					{
						program.FailIf(Program::Zero);
					}
					else
					{
						program.JumpIfAndMarkPatch(Program::Zero, data->pcData[0]);
					}
				}
				else
				{
					if(data->pcData[0] == TypeData<uint32_t>::Maximum())
					{
						program.FailIf(Program::Zero);
					}
					else
					{
						program.JumpIfAndMarkPatch(Program::Zero, data->pcData[0]);
					}
					
					program.JumpAndMarkPatch(data->pcData[1]);
				}
			}
			break;
				
		case InstructionType::DispatchRange:
			{
				const ByteCodeJumpRangeData* data = patternData.GetData<ByteCodeJumpRangeData>(instruction.data);
				
				if(data->pcData[0] == TypeData<uint32_t>::Maximum())
				{
					program.CheckPToPStop(pc);
				}
				else
				{
					program.ComparePToPStop();
					program.JumpIfAndMarkPatch(Program::Equal, data->pcData[0]);
				}

				program.ReadByte();
				
				program.SubtractValue(data->range.min);
				program.CompareValue(data->range.GetSize());
				// Flags are:
				// Above if outside the range (index 0)
				// BelowOrEqual if inside the range (index 1)
				
				if(data->pcData[0] == pc+1)
				{
					program.JumpIfAndMarkPatch(Program::BelowOrEqual, data->pcData[1]);
				}
				else if(data->pcData[1] == pc+1)
				{
					if(data->pcData[0] == TypeData<uint32_t>::Maximum())
					{
						program.FailIf(Program::Above);
					}
					else
					{
						program.JumpIfAndMarkPatch(Program::Above, data->pcData[0]);
					}
				}
				else
				{
					if(data->pcData[0] == TypeData<uint32_t>::Maximum())
					{
						program.FailIf(Program::Above);
					}
					else
					{
						program.JumpIfAndMarkPatch(Program::Above, data->pcData[0]);
					}
					
					program.JumpAndMarkPatch(data->pcData[1]);
				}
			}
			break;
				
		case InstructionType::Fail:
			//	return nullptr;
			program.Fail();
			break;

		case InstructionType::Success:
			program.Append(0x48, 0x89, 0xf0);				// movq   %rsi, %rax
			program.Return();
			break;
				
		case InstructionType::FindByte:
			{
				//	unsigned char c = instruction.data & 0xff;
				//	size_t remaining = processData.pEnd - p;
				//	p = (const unsigned char*) memchr(p, c, remaining);
				//	if(!p) return nullptr;
				//	++p;
				//	pc = instruction.data >> 8;
				if(type != PatternProcessorType::BitState
				   && patternData.GetMaximalJumpTargetForPc(pc) != TypeData<uint32_t>::Maximum())
				{
					// We have a loop from the end instead.
					program.Append(0x53);										// pushq  %rbx
					program.Append(0x48, 0x89, 0xf3);							// movq   %rsi, %rbx
					program.Append(0x48, 0x89, 0xd6);							// movq   %rdx, %rsi
					program.AdjustP(-1);
					
					uint32_t loopPoint = program.GetCount();
					program.Append(0x48, 0x39, 0xf3);							// cmpq   %rsi, %rbx
					uint32_t fail1 = program.JumpIfShort(Program::Above);

					program.Append(0x52);										// pushq %rdx
					program.Append(0x48, 0x89, 0xda);							// movq   %rbx, %rdx
					
					uint32_t c = instruction.data & 0xff;
					program.LoadConstantPointer((void*) InternalFindByteReverse);
					program.LoadValueIntoXmm(c*0x1010101, 0, false);
					program.CallPointer();

					program.Append(0x5a);										// popq %rdx

					program.Append(0x48, 0x8d, 0x70, 0x01);						// leaq   0x1(%rax), %rsi
					program.TestValue();
					uint32_t fail2 = program.JumpIfShort(Program::Zero);

					program.PushP();
					program.CallAndMarkPatch(patternData.GetMaximalJumpTargetForPc(pc));
					program.PopP();
					program.AdjustP(-2);
					program.TestValue();
					program.JumpIf(Program::Zero, loopPoint);

					program.Append(0x5b);              							// popq   %rbx
					program.Return();
					
					program.PatchShort(fail1);
					program.PatchShort(fail2);
					program.Append(0x5b);              							// popq   %rbx
					program.Fail();
				}
				else
				{
					uint32_t c = instruction.data & 0xff;
                    program.LoadConstantPointer(Machine::SupportsAvx2() ? (void*) InternalAvx2FindByte : (void*) InternalFindByte);
                    program.LoadValueIntoXmm(c*0x1010101, 0, Machine::SupportsAvx2());
					if(type == PatternProcessorType::BitState) program.PushP();
					program.CallPointer();
					
					if(type == PatternProcessorType::BitState)
					{
						program.PopP();
						program.Append(0x4c, 0x8d, 0x50, 0x01);			// leaq   0x1(%rax), %r10
						program.Append(0x49, 0x29, 0xf2);				// subq   %rsi, %r10
						program.Append(0x4d, 0x0f, 0xaf, 0xd1);			// imulq  %r9, %r10
						program.Append(0x4d, 0x01, 0xd0);				// addq   %r10, %r8
					}
					
					program.Append(0x48, 0x8d, 0x70, 0x01);						// leaq   0x1(%rax), %rsi
					program.TestValue();
					program.FailIf(Program::Zero);
					
					uint32_t nextPc = instruction.data>>8;
					if(nextPc != pc+1)
					{
						program.JumpAndMarkPatch(nextPc);
					}
				}
			}
			break;
				
		case InstructionType::Possess:
			//	const ByteCodeCallData* callData = patternData.GetData<ByteCodeCallData>(instruction.data);
			//	const void* result = Process(callData->callIndex, p, processData);
			//	if(result)
			//	{
			//		p = (const unsigned char*) result;
			//		pc = callData->trueIndex;
			//	}
			//	else
			//	{
			//		pc = callData->falseIndex;
			//	}
			//	if(pc == TypeData<uint32_t>::Maximum()) return nullptr;
			{
				const ByteCodeCallData* callData = patternData.GetData<ByteCodeCallData>(instruction.data);
				program.PushP();
				program.CallAndMarkPatch(callData->callIndex);
				program.PopP();
				program.TestValue();
				
				if(callData->falseIndex == TypeData<uint32_t>::Maximum())
				{
					program.FailIf(Program::Zero);
				}
				else
				{
					program.JumpIfAndMarkPatch(Program::Zero, callData->falseIndex);
				}
				
				if(callData->trueIndex == TypeData<uint32_t>::Maximum())
				{
					program.Fail();
				}
				else
				{
					program.Append(0x48, 0x89, 0xc6);					//     movq   %rax, %rsi
					program.JumpAndMarkPatch(callData->trueIndex);
				}
			}
			break;
				
		case InstructionType::PropagateBackwards:
			//	if(processData.captures != nullptr)
			//	{
			//		const StaticBitTable<256>& bitTable = *patternData.GetData<StaticBitTable<256>>(instruction.data);
			//		while(bitTable[p[-1]]) --p;
			//	}
			{
				uint32_t nextPatch;
				if(!header->flags.alwaysRequiresCaptures)
				{
					program.TestCaptures();
					nextPatch = program.JumpIfShort(Program::Zero);
				}
				
				program.LoadConstantPointer(patternData.GetData<StaticBitTable<256>>(instruction.data));
				
				uint32_t loopPoint = program.GetCount();
				program.ReadPreviousByte();
				program.AdjustP(-1);
				if(type == PatternProcessorType::BitState)
				{
					program.Append(0x4d, 0x29, 0xc8);	// subq %r9, %r8
				}
				
				program.BitTest();
				program.JumpIf(Program::BitSet, loopPoint);
				program.IncrementP();
				
				if(!header->flags.alwaysRequiresCaptures)
				{
					program.PatchShort(nextPatch);
				}
			}
			break;

		case InstructionType::Recurse:
		//	int recurseValue = instruction.data & 0xff;
		//	int recursePc = instruction.data;
		//
		//	int oldRecurseValue = processData.recurseValue;
		//	processData.recurseValue = recurseValue;
		//
		//	const char* oldCaptures[numberOfCaptures*2];
		//	memcpy(oldCaptures, processData.captures, numberOfCaptures*2*sizeof(const char*));
		//
		//	void* result = Process(recursePc, p, processData);
		//	memcpy(processData.captures, oldCaptures, numberOfCaptures*2*sizeof(const char*));
		//	processData.recurseValue = oldRecurseValue;
		//	if(result) p = result;
		//	else return nullptr;
		//	++pc;
		//	goto Loop;
			{
				if(useStackGuard) program.DoStackGuardCheck();

				uint32_t recurseValue = instruction.data & 0xff;
				int recursePc = instruction.data >> 8;
				
				program.Append(0xff, 0x37);				// pushq (%rdi) -- push processData.recurseValue
				program.Append(0x48, 0xc7, 0x47, 0x04); program.AppendData(&recurseValue, 4);	//  movq   {value}, 0x04(%rdi)

				
				uint32_t noCapturesPatch;
				if(!header->flags.alwaysRequiresCaptures)
				{
					program.TestCaptures();
					noCapturesPatch = program.JumpIfLong(Program::Zero);
				}

				for(int i = 0; i < 2*header->numberOfCaptures; ++i)
				{
					program.PushCapture(i*8);
				}
				
				program.CallAndMarkPatch(recursePc);
				program.TestValue();
				
				program.Append(0x48, 0x89, 0xc6);					//     movq   %rax, %rsi

				for(int i = 2*header->numberOfCaptures; i > 0;)
				{
					--i;
					program.PopCapture(i*8);
				}

				program.Append(0x8f, 0x07);							// popq   (%rdi)
				program.FailIf(Program::Zero);

				if(!header->flags.alwaysRequiresCaptures)
				{
					program.Jump(0);
					uint32_t nextPatch = program.GetCount();
					program.PatchLong(noCapturesPatch);

					program.CallAndMarkPatch(recursePc);
					program.TestValue();
					program.Append(0x48, 0x89, 0xc6);				//     movq   %rax, %rsi
					program.Append(0x8f, 0x07);						// popq   (%rdi)
					program.FailIf(Program::Zero);
					
					program.PatchLong(nextPatch);
				}
			}

			break;
				
		case InstructionType::ReturnIfRecurseValue:
			// if(processData.recurseValue == instruction.data) return true;
			{
				uint32_t recurseValue = instruction.data;
				program.Append(0x48, 0x89, 0xf0);										// movq   %rsi, %rax
				program.Append(0x81, 0x7f, 0x04); program.AppendData(&recurseValue, 4);	// cmpl  {value}, 0x04(%rdi)
				program.JumpIf(Program::Equal, 3);
			}
			break;
				
		case InstructionType::SearchByte:
			{
				uint32_t c 		= instruction.data & 0xff;
				uint32_t offset = instruction.data >> 8;

				//	unsigned char c = instruction.data & 0xff;
				//	unsigned offset = instruction.data >> 8;
				//
				//	const unsigned char* pSearch = p+offset;
				//	if(pSearch >= processData.pEnd) return nullptr;
				//
				//	size_t remaining = processData.pEnd - pSearch;
				//	pSearch = (const unsigned char*) memchr(pSearch, c, remaining);
				//	if(!pSearch) return nullptr;
				//	p = pSearch-offset;
				//	++pc;
				
                program.LoadConstantPointer(Machine::SupportsAvx2() ? (void*) InternalAvx2FindByte : (void*) InternalFindByte);
				program.AdjustP(offset);
				program.ComparePToPStop();
				program.FailIf(Program::AboveOrEqual);
				program.CheckPToPStop(pc);
				program.LoadValueIntoXmm(c*0x1010101, 0, Machine::SupportsAvx2());
				if(type == PatternProcessorType::BitState) program.PushP();
				program.CallPointer();
				
				uint32_t stepBack = -offset;
				if(type == PatternProcessorType::BitState)
				{
					program.PopP();
					program.Append(0x4c, 0x8d, 0x90); program.AppendData(&stepBack, 4);	// leaq   0x1234(%rax), %r10
					program.Append(0x49, 0x29, 0xf2);									// subq   %rsi, %r10
					program.Append(0x4d, 0x0f, 0xaf, 0xd1);								// imulq  %r9, %r10
					program.Append(0x4d, 0x01, 0xd0);									// addq   %r10, %r8
				}
				
				program.Append(0x48, 0x8d, 0xb0); program.AppendData(&stepBack, 4);	// leaq   {value}(%rax), %rsi
				program.TestValue();
				program.FailIf(Program::Zero);
			}
			break;
				
		case InstructionType::SearchByteEitherOf2:
		case InstructionType::SearchByteEitherOf3:
		case InstructionType::SearchByteEitherOf4:
		case InstructionType::SearchByteEitherOf5:
		case InstructionType::SearchByteEitherOf6:
		case InstructionType::SearchByteEitherOf7:
		case InstructionType::SearchByteEitherOf8:
			{
				int numberOfBytes = int(instruction.type) - int(InstructionType::SearchByteEitherOf2) + 2;
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				const void* nibbleMask = expandedJumpTables.GetData(pc);
				if(nibbleMask)
				{
					program.LoadConstantPointer(nibbleMask);
					program.LoadXmmValueFromPointer(0);
					program.LoadXmmValueFromPointer(1);
					program.LoadConstantPointer(Machine::SupportsAvx2() ?  (void*) InternalAvx2FindNibbleMask : (void*) InternalAvxFindNibbleMask);
					program.AdjustP(data->offset);
				}
				else
				{
					static constexpr void* SSE_FUNCTION_TABLE[8] = {
						nullptr,
						(void*) InternalFindEitherOf2,
						(void*) InternalFindEitherOf3,
						(void*) InternalFindEitherOf4,
						(void*) InternalFindEitherOf5,
						(void*) InternalFindEitherOf6,
						(void*) InternalFindEitherOf7,
						(void*) InternalFindEitherOf8,
					};
					
					static constexpr void* AVX_FUNCTION_TABLE[8] = {
						nullptr,
						(void*) InternalAvxFindEitherOf2,
						(void*) InternalAvxFindEitherOf3,
						(void*) InternalFindEitherOf4,
						(void*) InternalFindEitherOf5,
						(void*) InternalFindEitherOf6,
						(void*) InternalFindEitherOf7,
						(void*) InternalFindEitherOf8,
					};

					static constexpr void* AVX2_FUNCTION_TABLE[8] = {
						nullptr,
						(void*) InternalAvx2FindEitherOf2,
						(void*) InternalAvx2FindEitherOf3,
						(void*) InternalFindEitherOf4,
						(void*) InternalFindEitherOf5,
						(void*) InternalFindEitherOf6,
						(void*) InternalFindEitherOf7,
						(void*) InternalFindEitherOf8,
					};

					const void* const *const FUNCTION_TABLE = Machine::SupportsAvx2() ?
																	AVX2_FUNCTION_TABLE :
																	Machine::SupportsAvx() ?
																		AVX_FUNCTION_TABLE :
																		SSE_FUNCTION_TABLE;

					JASSERT(FUNCTION_TABLE[numberOfBytes-1] != nullptr);
					program.LoadConstantPointer(FUNCTION_TABLE[numberOfBytes-1]);
					program.AdjustP(data->offset);
					for(int i = 0; i < numberOfBytes; ++i)
					{
						program.LoadValueIntoXmm(data->bytes[i]*0x1010101, i, FUNCTION_TABLE[numberOfBytes-1] != SSE_FUNCTION_TABLE[numberOfBytes-1]);
					}
					
				}
				program.ComparePToPStop();
				program.FailIf(Program::AboveOrEqual);
				if(type == PatternProcessorType::BitState) program.PushP();
				program.CallPointer();
				uint32_t stepBack = -data->offset;
				if(patternData[pc+1].type == InstructionType::AdvanceByte
				   && patternData[pc+1].isSingleReference)
				{
					++stepBack;
					++pc;
				}
				if(type == PatternProcessorType::BitState)
				{
					program.PopP();
					program.Append(0x4c, 0x8d, 0x90); program.AppendData(&stepBack, 4);	// leaq   0x1234(%rax), %r10
					program.Append(0x49, 0x29, 0xf2);									// subq   %rsi, %r10
					program.Append(0x4d, 0x0f, 0xaf, 0xd1);								// imulq  %r9, %r10
					program.Append(0x4d, 0x01, 0xd0);									// addq   %r10, %r8
				}
				
				program.Append(0x48, 0x8d, 0xb0); program.AppendData(&stepBack, 4);	// leaq   {value}(%rax), %rsi
				program.TestValue();
				program.FailIf(Program::Zero);
			}
			break;

		case InstructionType::SearchBytePair:
		case InstructionType::SearchBytePair2:
		case InstructionType::SearchBytePair3:
		case InstructionType::SearchBytePair4:
			{
				int numberOfBytePairs = int(instruction.type) - int(InstructionType::SearchBytePair) + 1;
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				const void* nibbleMask = expandedJumpTables.GetData(pc);
				if(nibbleMask)
				{
					const ByteCodeSearchMultiByteData* multiByteData = (ByteCodeSearchMultiByteData*) &data->bytes[instruction.type >= InstructionType::SearchBytePair3 ? 8 : 4];

					void* searchFunction = multiByteData->isPath ?
											Machine::SupportsAvx2() ?
												(void*) InternalAvx2FindPairNibbleMaskPath :
												(void*) InternalAvxFindPairNibbleMaskPath :
											Machine::SupportsAvx2() ?
												(void*) InternalAvx2FindPairNibbleMask :
												(void*) InternalAvxFindPairNibbleMask;
					
					program.LoadConstantPointer(nibbleMask);
					program.LoadXmmValueFromPointer(0);
					program.LoadXmmValueFromPointer(1);
					program.LoadXmmValueFromPointer(2);
					program.LoadXmmValueFromPointer(3);
					program.LoadConstantPointer(searchFunction);
					program.AdjustP(data->offset);
					program.ComparePToPStop();
					program.FailIf(Program::AboveOrEqual);

					if(type == PatternProcessorType::BitState)
					{
						program.PushP();
						program.Append(0x41, 0x51);		// pushq  %r9
					}
					
					program.CallPointer();
					
					uint32_t stepBack = -data->offset-1;
					if(patternData[pc+1].type == InstructionType::AdvanceByte
					   && patternData[pc+1].isSingleReference)
					{
						++stepBack;
						++pc;
					}
					
					if(type == PatternProcessorType::BitState)
					{
						program.Append(0x41, 0x59);											// popq  %r9
						program.PopP();
						program.Append(0x4c, 0x8d, 0x90); program.AppendData(&stepBack, 4);	// leaq   0x1234(%rax), %r10
						program.Append(0x49, 0x29, 0xf2);									// subq   %rsi, %r10
						program.Append(0x4d, 0x0f, 0xaf, 0xd1);								// imulq  %r9, %r10
						program.Append(0x4d, 0x01, 0xd0);									// addq   %r10, %r8
					}
					
					program.Append(0x48, 0x8d, 0xb0); program.AppendData(&stepBack, 4);		// leaq   {value}(%rax), %rsi
					program.TestValue();
					program.FailIf(Program::Zero);
				}
				else
				{
					static constexpr const void *const SSE_FUNCTION_TABLE[4][2] = {
						{ (void*) InternalFindPair,  (void*) InternalFindPair  },
						{ (void*) InternalFindPair2, (void*) InternalFindPair2 },
						{ (void*) InternalFindPair3, (void*) InternalFindPair3 },
						{ (void*) InternalFindPair4, (void*) InternalFindPair4 },
					};
					static constexpr const void *const AVX_FUNCTION_TABLE[4][2] = {
						{ (void*) InternalAvxFindPair, (void*) InternalAvxFindPair },
						{ (void*) InternalAvxFindPair2, (void*) InternalAvxFindPairPath2 },
						{ (void*) InternalFindPair3, (void*) InternalFindPair3 },
						{ (void*) InternalFindPair4, (void*) InternalFindPair4 },
					};
					static constexpr const void *const AVX2_FUNCTION_TABLE[4][2] = {
						{ (void*) InternalAvx2FindPair, (void*) InternalAvx2FindPair },
						{ (void*) InternalAvx2FindPair2, (void*) InternalAvx2FindPairPath2 },
						{ (void*) InternalFindPair3, (void*) InternalFindPair3 },
						{ (void*) InternalFindPair4, (void*) InternalFindPair4 },
					};
					
					const void* const (*const FUNCTION_TABLE)[2] = Machine::SupportsAvx2() ?
																	AVX2_FUNCTION_TABLE :
																	Machine::SupportsAvx() ?
																		AVX_FUNCTION_TABLE :
																		SSE_FUNCTION_TABLE;
					
					const ByteCodeSearchMultiByteData* multiByteData = (ByteCodeSearchMultiByteData*) &data->bytes[instruction.type >= InstructionType::SearchBytePair3 ? 8 : 4];

					JASSERT(FUNCTION_TABLE[numberOfBytePairs-1] != nullptr);
					program.LoadConstantPointer((void*) FUNCTION_TABLE[numberOfBytePairs-1][multiByteData->isPath]);
					program.AdjustP(data->offset);
					program.ComparePToPStop();
					program.FailIf(Program::AboveOrEqual);
					for(int i = 0; i < numberOfBytePairs*2; ++i)
					{
						program.LoadValueIntoXmm(data->bytes[i]*0x1010101, i, FUNCTION_TABLE[numberOfBytePairs-1] != SSE_FUNCTION_TABLE[numberOfBytePairs-1]);
					}
					if(type == PatternProcessorType::BitState)
					{
						program.PushP();
						program.Append(0x41, 0x51);		// pushq  %r9
					}
					program.CallPointer();
					
					uint32_t stepBack = -data->offset-1;
					if(patternData[pc+1].type == InstructionType::AdvanceByte
					   && patternData[pc+1].isSingleReference)
					{
						++stepBack;
						++pc;
					}
					
					if(type == PatternProcessorType::BitState)
					{
						program.Append(0x41, 0x59);											// popq  %r9
						program.PopP();
						program.Append(0x4c, 0x8d, 0x90); program.AppendData(&stepBack, 4);	// leaq   0x1234(%rax), %r10
						program.Append(0x49, 0x29, 0xf2);									// subq   %rsi, %r10
						program.Append(0x4d, 0x0f, 0xaf, 0xd1);								// imulq  %r9, %r10
						program.Append(0x4d, 0x01, 0xd0);									// addq   %r10, %r8
					}
					
					program.Append(0x48, 0x8d, 0xb0); program.AppendData(&stepBack, 4);		// leaq   {value}(%rax), %rsi
					program.TestValue();
					program.FailIf(Program::Zero);
				}
			}
			break;
				
		case InstructionType::SearchByteTriplet:
		case InstructionType::SearchByteTriplet2:
			{
				int numberOfByteTriplets = int(instruction.type) - int(InstructionType::SearchByteTriplet) + 1;
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);

				static constexpr void* SSE_FUNCTION_TABLE[] = {
					(void*) InternalFindTriplet,
					(void*) InternalFindTriplet2,
				};
				
				static constexpr void *const AVX_FUNCTION_TABLE[4] = {
					(void*) InternalAvxFindTriplet,
					(void*) InternalFindTriplet2,
				};

				static constexpr void *const AVX2_FUNCTION_TABLE[4] = {
					(void*) InternalAvx2FindTriplet,
					(void*) InternalFindTriplet2,
				};

				const void* const *const FUNCTION_TABLE = Machine::SupportsAvx2() ?
															AVX2_FUNCTION_TABLE :
															Machine::SupportsAvx() ?
																AVX_FUNCTION_TABLE :
																SSE_FUNCTION_TABLE;

				JASSERT(FUNCTION_TABLE[numberOfByteTriplets-1] != nullptr);
				program.LoadConstantPointer((void*) FUNCTION_TABLE[numberOfByteTriplets-1]);
				program.AdjustP(data->offset);
				program.ComparePToPStop();
				program.FailIf(Program::AboveOrEqual);
				for(int i = 0; i < numberOfByteTriplets*3; ++i)
				{
					program.LoadValueIntoXmm(data->bytes[i]*0x1010101, i, FUNCTION_TABLE[numberOfByteTriplets-1] != SSE_FUNCTION_TABLE[numberOfByteTriplets-1]);
				}
				if(type == PatternProcessorType::BitState)
				{
					program.PushP();
					program.Append(0x41, 0x51);		// pushq  %r9
				}
				program.CallPointer();
				
				uint32_t stepBack = -data->offset-2;
				if(patternData[pc+1].type == InstructionType::AdvanceByte
				   && patternData[pc+1].isSingleReference)
				{
					++stepBack;
					++pc;
				}

				if(type == PatternProcessorType::BitState)
				{
					program.Append(0x41, 0x59);											// popq  %r9
					program.PopP();
					program.Append(0x4c, 0x8d, 0x90); program.AppendData(&stepBack, 4);	// leaq   0x1234(%rax), %r10
					program.Append(0x49, 0x29, 0xf2);									// subq   %rsi, %r10
					program.Append(0x4d, 0x0f, 0xaf, 0xd1);								// imulq  %r9, %r10
					program.Append(0x4d, 0x01, 0xd0);									// addq   %r10, %r8
				}
				
				program.Append(0x48, 0x8d, 0xb0); program.AppendData(&stepBack, 4);	// leaq   {value}(%rax), %rsi
				program.TestValue();
				program.FailIf(Program::Zero);
			}
			break;
				
		case InstructionType::SearchByteRange:
		case InstructionType::SearchByteRangePair:
			{
				int numberOfBytePairs = int(instruction.type) - int(InstructionType::SearchByteRange) + 1;
				const ByteCodeSearchByteData* data = patternData.GetData<ByteCodeSearchByteData>(instruction.data);
				
				static constexpr void* SSE_FUNCTION_TABLE[2] = {
					(void*) InternalFindByteRange,
					(void*) InternalFindByteRangePair,
				};
				
				static constexpr void *const AVX_FUNCTION_TABLE[2] = {
					(void*) InternalAvxFindByteRange,
					(void*) InternalAvxFindByteRangePair,
				};
				
				static constexpr void *const AVX2_FUNCTION_TABLE[2] = {
					(void*) InternalAvx2FindByteRange,
					(void*) InternalAvxFindByteRangePair,
				};
				
				const void* const *const FUNCTION_TABLE = Machine::SupportsAvx2() ?
															AVX2_FUNCTION_TABLE :
															Machine::SupportsAvx() ?
																AVX_FUNCTION_TABLE :
																SSE_FUNCTION_TABLE;
															
				JASSERT(FUNCTION_TABLE[numberOfBytePairs-1] != nullptr);
				program.LoadConstantPointer((void*) FUNCTION_TABLE[numberOfBytePairs-1]);
				program.AdjustP(data->offset);
				program.ComparePToPStop();
				program.FailIf(Program::AboveOrEqual);
				for(int i = 0; i < numberOfBytePairs; ++i)
				{
					unsigned char low = data->bytes[i*2];
					unsigned char high = data->bytes[i*2+1];
					
					// Example: low = '0', high = '9'.
					// SSE only has greater-than signed compare
					// Need to pass values (127-high)
					// and 127-(high-low)-1
					// So that GT compare can be done
					program.LoadValueIntoXmm(((127-high)&0xff)*0x1010101, i*2, FUNCTION_TABLE[numberOfBytePairs-1] != SSE_FUNCTION_TABLE[numberOfBytePairs-1]);
					program.LoadValueIntoXmm(((126-(high-low))&0xff)*0x1010101, i*2+1, FUNCTION_TABLE[numberOfBytePairs-1] != SSE_FUNCTION_TABLE[numberOfBytePairs-1]);
				}
				if(type == PatternProcessorType::BitState)
				{
					program.PushP();
					if(numberOfBytePairs > 1) program.Append(0x41, 0x51);		// pushq  %r9
				}
				program.CallPointer();
				
				uint32_t stepBack = -data->offset-numberOfBytePairs+1;
				if(patternData[pc+1].type == InstructionType::AdvanceByte
				   && patternData[pc+1].isSingleReference)
				{
					++stepBack;
					++pc;
				}

				if(type == PatternProcessorType::BitState)
				{
					if(numberOfBytePairs > 1) program.Append(0x41, 0x59);				// popq  %r9
					program.PopP();
					program.Append(0x4c, 0x8d, 0x90); program.AppendData(&stepBack, 4);	// leaq   0x1234(%rax), %r10
					program.Append(0x49, 0x29, 0xf2);									// subq   %rsi, %r10
					program.Append(0x4d, 0x0f, 0xaf, 0xd1);								// imulq  %r9, %r10
					program.Append(0x4d, 0x01, 0xd0);									// addq   %r10, %r8
				}
				
				program.Append(0x48, 0x8d, 0xb0); program.AppendData(&stepBack, 4);	// leaq   {value}(%rax), %rsi
				program.TestValue();
				program.FailIf(Program::Zero);
			}
			break;
				
		case InstructionType::SearchBoyerMoore:
			{
				const ByteCodeSearchData* searchData = patternData.GetData<ByteCodeSearchData>(instruction.data);
				//	const ByteCodeSearchData* searchData = patternData.GetData<ByteCodeSearchData>(instruction.data);
				//	
				//	const unsigned char* pSearch = p+searchData->offset;
				//	for(;;)
				//	{
				//		if(pSearch >= processData.pEnd) return nullptr;
				//		uint32_t adjust = searchData->data[*pSearch];
				//		if(adjust == 0) break;
				//		pSearch += adjust;
				//	}
				//	
				//	p = pSearch-searchData->offset;
				//	++pc;
				const void* p = nullptr; // expandedJumpTables.GetData(pc);
				
				program.LoadConstantPointer(p ? p : searchData->data);
				program.AdjustP(searchData->length);
				if(type == PatternProcessorType::BitState)
				{
					program.Append(0x49, 0x89, 0xf3);				// movq   %rsi, %r11
				}
				
				uint32_t loopPoint = program.GetCount();
				program.ComparePToPStop();
				program.FailIf(Program::AboveOrEqual);
				program.ReadByte();
				if(p)
				{
					program.Append(0x41, 0x0f, 0xb6, 0x04, 0x02);	//  movzbl (%r10,%rax), %eax
				}
				else
				{
					program.Append(0x41, 0x8b, 0x04, 0x82);			// movl   (%r10,%rax,4), %eax
				}
				program.Append(0x48, 0x01, 0xc6);					// addq   %rax, %rsi
				program.TestValue();
				program.JumpIf(Program::NotZero, loopPoint);
				program.AdjustP(-searchData->length);

				if(type == PatternProcessorType::BitState)
				{
					program.Append(0x49, 0x29, 0xf3);				// subq   %rsi, %r11
					program.Append(0x4d, 0x0f, 0xaf, 0xd9);			// imulq  %r9, %r11
					program.Append(0x4d, 0x01, 0xd8);				// addq   %r11, %r8
				}
			}
			break;
			
		case InstructionType::SearchShiftOr:
			{
				const ByteCodeSearchShiftOrData* searchData = patternData.GetData<ByteCodeSearchShiftOrData>(instruction.data);
				const void* maskData = expandedJumpTables.GetData(pc);
				if(maskData != nullptr)
				{
					// maskData is only created for shift or if AVX is supported.
					JASSERT(Machine::SupportsAvx());
					
					static constexpr const void *const AVX_FUNCTION_TABLE[3][2] = {
						{ (void*) InternalAvxFindNibbleMask, (void*) InternalAvxFindNibbleMask },
						{ (void*) InternalAvxFindPairNibbleMask, (void*) InternalAvxFindPairNibbleMaskPath },
						{ (void*) InternalAvxFindTripletNibbleMask, (void*) InternalAvxFindTripletNibbleMaskPath },
					};

					static constexpr const void *const AVX2_FUNCTION_TABLE[3][2] = {
						{ (void*) InternalAvx2FindNibbleMask, (void*) InternalAvx2FindNibbleMask },
						{ (void*) InternalAvx2FindPairNibbleMask, (void*) InternalAvx2FindPairNibbleMaskPath },
						{ (void*) InternalAvxFindTripletNibbleMask, (void*) InternalAvx2FindTripletNibbleMaskPath },
					};
					
					const void* const (*const FUNCTION_TABLE)[2] = Machine::SupportsAvx2() ?
																	AVX2_FUNCTION_TABLE :
																	AVX_FUNCTION_TABLE;

					program.LoadConstantPointer(maskData);
					for(int x = 0; x < searchData->numberOfNibbleMasks; ++x)
					{
						program.LoadXmmValueFromPointer(x*2);
						program.LoadXmmValueFromPointer(x*2+1);
					}
					
					program.LoadConstantPointer(FUNCTION_TABLE[searchData->numberOfNibbleMasks-1][searchData->isPath]);
					program.ComparePToPStop();
					program.FailIf(Program::AboveOrEqual);

					if(type == PatternProcessorType::BitState)
					{
						program.PushP();
					}
					program.CallPointer();
					
					uint32_t stepBack = -searchData->numberOfNibbleMasks+1;
					if(patternData[pc+1].type == InstructionType::AdvanceByte
					   && patternData[pc+1].isSingleReference)
					{
						++stepBack;
						++pc;
					}
					
					if(type == PatternProcessorType::BitState)
					{
						program.PopP();
						program.Append(0x4c, 0x8d, 0x90); program.AppendData(&stepBack, 4);	// leaq   0x1234(%rax), %r10
						program.Append(0x49, 0x29, 0xf2);									// subq   %rsi, %r10
						program.Append(0x4d, 0x0f, 0xaf, 0xd1);								// imulq  %r9, %r10
						program.Append(0x4d, 0x01, 0xd0);									// addq   %r10, %r8
					}
					
					program.Append(0x48, 0x8d, 0xb0); program.AppendData(&stepBack, 4);		// leaq   {value}(%rax), %rsi
					program.TestValue();
					program.FailIf(Program::Zero);
				}
				else
				{
					const void* searchDataPointer = &searchData->data;
					program.LoadConstantPointer((void*) &InternalFindShiftOr);
					
					program.Append(0x51, 0x57);										// pushq %rcx, %rdi
					
					program.Append(0x48, 0xbf); program.AppendData(&searchDataPointer, 8);	// movabsq $searchData, %rdi
					if(type == PatternProcessorType::BitState)
					{
						program.Append(0x56, 0x41, 0x50, 0x41, 0x51);					// pushq %rsi, %r8, %r9
					}
					program.Append(0xb9); program.AppendData(&searchData->length, 4);	// movl $offset, %ecx
					program.CallPointer();
					
					if(type == PatternProcessorType::BitState)
					{
						program.Append(0x41, 0x59, 0x41, 0x58, 0x41, 0x5a);			// popq %r9, %r8, %r10
					}
					program.Append(0x5f, 0x59);										// popq %rdi, %rcx
					program.FailIf(Program::Zero);
					if(type == PatternProcessorType::BitState)
					{
						program.Append(0x49, 0x29, 0xf2);							// subq   %rsi, %r10
						program.Append(0x4d, 0x0f, 0xaf, 0xd1);						// imulq  %r9, %r10
						program.Append(0x4d, 0x01, 0xd0);							// addq   %r10, %r8
					}
				}
			}
			break;
				
		case InstructionType::Jump:
			// pc = instruction.data;
			program.JumpAndMarkPatch(instruction.data);
			break;

		case InstructionType::Match:
			//	if(isPartialMatch) return true;
			//  return p == pEnd;
			program.Append(0x48, 0x89, 0xf0);							// movq   %rsi, %rax
            if(!header->IsFixedLength())
            {
                program.CompareIsPartialMatch();
                program.JumpIf(Program::NotZero, 3);
                program.ComparePToPStop();
                program.FailIfNotEqual();
            }
			program.Return();
			break;
				
		case InstructionType::ProgressCheck:
			if(type == PatternProcessorType::BackTracking)
			{
				//		if(progressCheck[instruction.data] >= p) return nullptr;
				//		else
				//		{
				//			const unsigned char* old = progressCheck[instruction.data];
				//			progressCheck[instruction.data] = p;
				//			if(Process(pc+1, p)) return true;
				//			progressCheck[instruction.data] = old;
				//			return nullptr;
				//		}
				uint32_t offset = 0x28 + 8*instruction.data;
				if(offset < 0x80)
				{
					program.Append(0x48, 0x8b, 0x47, offset); 							// movq   0x10(%rdi), %rax
				}
				else
				{
					program.Append(0x48, 0x8b, 0x87); program.AppendData(&offset, 4); 	// movq 0x1234(%rdi), %rax
				}
				
				program.Append(0x48, 0x39, 0xc6);										// cmpq   %rax, %rsi
				program.FailIf(Program::BelowOrEqual);
				program.Append(0x50);													// push %rax

				if(offset < 0x80)
				{
					program.Append(0x48, 0x89, 0x77, offset);							// movq   %rsi, 0x10(%rdi)
				}
				else
				{
					program.Append(0x48, 0x89, 0xb7); program.AppendData(&offset, 4); 	// movq   %rsi, 0x1234(%rdi)
				}

				program.PushP();
				program.Call(0);
				uint32_t callLocation = uint32_t(program.GetCount());
				program.PopP();
				program.Append(0x41, 0x5a);												// popq   %r10
				program.ExitIfNot0();
				
				if(offset < 0x80)
				{
					program.Append(0x4c, 0x89, 0x57, offset);							// movq   %r10, 0x10(%rdi)
				}
				else
				{
					program.Append(0x4c, 0x89, 0x97); program.AppendData(&offset, 4);	// movq   %r10, 0x1234(%rdi)
				}
				program.Fail();
				program.PatchLong(callLocation);
			}
			break;
				
		case InstructionType::Save:
		case InstructionType::SaveNoRecurse:
			//	if(captures != nullptr)
			//  {
			//		uint32_t saveIndex = instruction.data & 0xff;
			//		uint32_t saveOffset = instruction.data >> 8;
			//		const char* backup = captures[saveIndex];
			//		captures[saveIndex] = (const char*) p - saveOffset;
			//		if(Process(pc+1, p)) return true;
			//		else captures[saveIndex] = backup;
			//		return nullptr;
			//	}
			//
				
			// Special case code for one-pass
			if(type == PatternProcessorType::OnePass
			   || instruction.type == InstructionType::SaveNoRecurse)
			{
				int32_t saveOffset = instruction.data >> 8;
				int32_t saveIndex = (instruction.data & 0xff) * 8;
				bool rewind = instruction.isSingleReference && program.RewindIfAdjustP(saveOffset);
				if (rewind)
				{
					program.CalculateCapture(0);						// %r10 now contains char* to store
					program.AdjustP(saveOffset);
					instructionOffsets[pc] = uint32_t(program.GetCount());
				}

				uint32_t nextPatch;
				if(!header->flags.alwaysRequiresCaptures)
				{
					program.TestCaptures();
					nextPatch = program.JumpIfLong(Program::Zero);
				}
				
				if(rewind)
				{
					program.WriteCapture(saveIndex);
				}
				else if(saveOffset != 0)
				{
					program.CalculateCapture(-saveOffset);				// %r10 now contains char* to store
					program.WriteCapture(saveIndex);
				}
				else
				{
					program.WriteZeroOffsetCapture(saveIndex);
				}
				while(pc+1 < numberOfInstructions
					  && (patternData[pc+1].type == InstructionType::Save || patternData[pc+1].type == InstructionType::SaveNoRecurse)
					  && (type == PatternProcessorType::OnePass || patternData[pc+1].type == InstructionType::SaveNoRecurse)
					  && patternData[pc+1].isSingleReference)
				{
					++pc;
					instruction = patternData[pc];
					int32_t newSaveOffset = instruction.data >> 8;
					saveIndex = (instruction.data & 0xff) * 8;
					if(newSaveOffset != saveOffset)
					{
						saveOffset = newSaveOffset;
						if(saveOffset != 0) program.CalculateCapture(-saveOffset);		// %r10 now contains char* to store
					}
					if(saveOffset == 0) program.WriteZeroOffsetCapture(saveIndex);
					else program.WriteCapture(saveIndex);
				}
				
				if(!header->flags.alwaysRequiresCaptures)
				{
					program.PatchLong(nextPatch);
				}
			}
			else
			{
				int32_t saveOffset = instruction.data >> 8;
				int32_t saveIndex = (instruction.data & 0xff) * 8;
				bool rewind = instruction.isSingleReference && program.RewindIfAdjustP(saveOffset);
				if (rewind)
				{
					program.CalculateCapture(0);									// %r10 now contains char* to store
					program.AdjustP(saveOffset);
					instructionOffsets[pc] = uint32_t(program.GetCount());
				}

				uint32_t nextPatch;
				if(!header->flags.alwaysRequiresCaptures)
				{
					program.TestCaptures();
					nextPatch = program.JumpIfShort(Program::Zero);
				}

				program.PushCapture(saveIndex);
				if(rewind)
				{
					program.WriteCapture(saveIndex);
				}
				else if(saveOffset != 0)
				{
					program.CalculateCapture(-saveOffset);				// %r10 now contains char* to store
					program.WriteCapture(saveIndex);
				}
				else
				{
					program.WriteZeroOffsetCapture(saveIndex);
				}

				program.PushP();
				program.Call(0);
				uint32_t callLocation = uint32_t(program.GetCount());
				program.PopP();
				program.Append(0x41, 0x5a);												// popq   %r10
				program.ExitIfNot0();
				program.WriteCapture(saveIndex);
				program.Jump(0);
				
				program.WriteCapture(saveIndex);

				if(!header->flags.alwaysRequiresCaptures)
				{
					program.PatchShort(nextPatch);										// next:
				}
				program.PatchLong(callLocation);
			}
			break;

		case InstructionType::Split:
			//	{
			//		const ByteCodeSplitData* split = patternData.GetData<ByteCodeSplitData>(instruction.data);
			//		uint32_t numberOfTargets = split->numberOfTargets;
			//		for(size_t i = 0; i < numberOfTargets-1; ++i)
			//		{
			//			if(Process(split->targetList[i], p)) return true;
			//		}
			//		pc = split->targetList[numberOfTargets-1];
			//	}
			if(type != PatternProcessorType::OnePass)
			{
				if(useStackGuard) program.DoStackGuardCheck();
				
				const ByteCodeSplitData* split = patternData.GetData<ByteCodeSplitData>(instruction.data);
				uint32_t numberOfTargets = split->numberOfTargets;
				for(size_t i = 0; i < numberOfTargets-1; ++i)
				{
					program.PushP();
					program.CallAndMarkPatch(split->targetList[i]);
					program.PopP();
					program.ExitIfNot0();
				}
				program.JumpAndMarkPatch(split->targetList[numberOfTargets-1]);
			}
			break;

		case InstructionType::SplitMatch:
			//	{
			//		const uint32_t* splitData = patternData.GetData<uint32_t>(instruction.data);
			//		pc = (!processData.isFullMatch || p == processData.pEnd) ? splitData[0] : splitData[1];
			//		goto Loop;
			//	}
			{
				const uint32_t* splitData = patternData.GetData<uint32_t>(instruction.data);
				program.ComparePToPStop();
				if(splitData[0] < pc) program.JumpIf(Program::Equal, instructionOffsets[splitData[0]]);
				else program.JumpIfAndMarkPatch(Program::Equal, splitData[0]);
				program.CompareIsPartialMatch();
				if(splitData[0] < pc) program.JumpIf(Program::NotZero, instructionOffsets[splitData[0]]);
				else program.JumpIfAndMarkPatch(Program::NotZero, splitData[0]);
				program.JumpAndMarkPatch(splitData[1]);
			}
			break;

		case InstructionType::SplitNextN:
			//	if(Process(pc+1, p)) return true;
			//	pc = instruction.data;
			if(type != PatternProcessorType::OnePass)
			{
				if(useStackGuard) program.DoStackGuardCheck();

				program.PushP();
				program.CallAndMarkPatch(pc+1);
				program.PopP();
				program.ExitIfNot0();
				program.JumpAndMarkPatch(instruction.data);
			}
			break;
				
		case InstructionType::SplitNNext:
			//	if(Process(instruction.data, p)) return true;
			//	++pc;
			if(type != PatternProcessorType::OnePass)
			{
				if(useStackGuard) program.DoStackGuardCheck();

				program.PushP();
				program.CallAndMarkPatch(instruction.data);
				program.PopP();
				program.ExitIfNot0();
			}
			break;
				
		case InstructionType::SplitNextMatchN:
		//	pc = (isPartialMatch || p == pEnd) ? pc+1 : instruction.data;
			{
				program.ComparePToPStop();
				uint32_t nextOffset = program.JumpIfShort(Program::Equal);
				program.CompareIsPartialMatch();		// Sets non-zero if partial match
				if(instruction.data < pc) program.JumpIf(Program::Zero, instructionOffsets[instruction.data]);
				else program.JumpIfAndMarkPatch(Program::Zero, instruction.data);
				program.PatchShort(nextOffset);
			}
				
			break;
				
		case InstructionType::SplitNMatchNext:
		//	pc = (isPartialMatch || p == pEnd) ? instruction.data : pc+1;
			program.ComparePToPStop();
			if(instruction.data < pc) program.JumpIf(Program::Equal, instructionOffsets[instruction.data]);
			else program.JumpIfAndMarkPatch(Program::Equal, instruction.data);
			program.CompareIsPartialMatch();
			if(instruction.data < pc) program.JumpIf(Program::NotZero, instructionOffsets[instruction.data]);
			else program.JumpIfAndMarkPatch(Program::NotZero, instruction.data);
			break;

		case InstructionType::StepBack:
			//	p -= instruction.data;
			//	if(p < processData.pStart) return nullptr;
			//	++pc;
			//	goto Loop;
			program.AdjustP(-instruction.data);
			program.ComparePToPStart();
			program.FailIfBelow();
			break;
		}
	}
	
	
	uint32_t fullMatchProgramOffset = instructionOffsets[header->fullMatchStartingInstruction];
	uint32_t partialMatchProgramOffset = instructionOffsets[header->partialMatchStartingInstruction];
	
	if(type == PatternProcessorType::BackTracking && (header->numberOfProgressChecks > 0 || header->flags.useStackGuard))
	{
		uint32_t newFullMatchProgramOffset = program.GetCount();
		if(header->numberOfProgressChecks > 0) program.ClearProgressCheckTable(header->numberOfProgressChecks);
		if(header->flags.useStackGuard)
		{
			program.BeginStackGuard();
			program.Call(fullMatchProgramOffset);
			program.EndStackGuard();
		}
		else
		{
			program.Jump(fullMatchProgramOffset);
		}
		fullMatchProgramOffset = newFullMatchProgramOffset;
		
		uint32_t newPartialMatchProgramOffset = program.GetCount();
		if(header->numberOfProgressChecks > 0) program.ClearProgressCheckTable(header->numberOfProgressChecks);
		if(header->flags.useStackGuard)
		{
			program.BeginStackGuard();
			program.Call(partialMatchProgramOffset);
			program.EndStackGuard();
		}
		else
		{
			program.Jump(partialMatchProgramOffset);
		}
		partialMatchProgramOffset = newPartialMatchProgramOffset;
		
		program.WriteStackGuardList();
	}
	
	for(const Program::PatchEntry& entry : program.patchList)
	{
		program.PatchOffset(entry.location, uint32_t(instructionOffsets[entry.pc]));
	}
	
	
	MakeProgramExecutable(program);
	
	fullMatchProgram = ((char*) mapRegion + fullMatchProgramOffset);
	partialMatchProgram = ((char*) mapRegion + partialMatchProgramOffset);
	expandedJumpTables.Populate(patternData, numberOfInstructions, mapRegion, instructionOffsets, type);
}

void Amd64ProcessorBase::MakeProgramExecutable(const Program& program)
{
	size_t pageSize = Machine::GetPageSize();
	mapRegionLength = (program.GetCount() + pageSize-1) & -pageSize;
	mapRegion = mmap(0, mapRegionLength, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	JVERIFY(mapRegion != MAP_FAILED);
	
	memcpy(mapRegion, program.GetData(), program.GetCount());
}

//============================================================================

const void* OnePassAmd64PatternProcessor::FullMatch(const void* data, size_t length) const
{
	ProcessData processData(false, data, length, 0);
	return (*(ProgramType) fullMatchProgram)(processData, data, (const unsigned char*) data + length, nullptr);
}

const void* OnePassAmd64PatternProcessor::FullMatch(const void* data, size_t length, const char **captures) const
{
	ProcessData processData(false, data, length, 0);
	return (*(ProgramType) fullMatchProgram)(processData, data, (const unsigned char*) data + length, captures);
}

const void* OnePassAmd64PatternProcessor::PartialMatch(const void* data, size_t length, size_t offset) const
{
	ProcessData processData(allowPartialMatch, data, length, offset);
	return (*(ProgramType) partialMatchProgram)(processData, processData.pSearchStart, (const unsigned char*) data + length, nullptr);
}

const void* OnePassAmd64PatternProcessor::PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const
{
	ProcessData processData(allowPartialMatch, data, length, offset);
	return (*(ProgramType) partialMatchProgram)(processData, processData.pSearchStart, (const unsigned char*) data + length, captures);
}

Interval<const void*> OnePassAmd64PatternProcessor::LocatePartialMatch(const void* data, size_t length, size_t offset) const
{
	const char* captures[numberOfCaptures*2];
	captures[0] = nullptr;
	captures[1] = nullptr;
	ProcessData processData(allowPartialMatch, data, length, offset);
	(*(ProgramType) partialMatchProgram)(processData, processData.pSearchStart, (const unsigned char*) data + length, captures);
	return {captures[0], captures[1]};
}

const void* OnePassAmd64PatternProcessor::PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const
{
	ProcessData processData(allowPartialMatch, data, length, offset);
	return (*(ProgramType) fullMatchProgram)(processData, processData.pSearchStart, (const unsigned char*) data + length, captures);
}


PatternProcessor* PatternProcessor::CreateOnePassProcessor(DataBlock&& dataBlock)
{
	return new OnePassAmd64PatternProcessor((DataBlock&&) dataBlock);
}

PatternProcessor* PatternProcessor::CreateOnePassProcessor(const void* data, size_t length, bool makeCopy)
{
	if(makeCopy)
	{
		DataBlock dataBlock(data, length);
		return new OnePassAmd64PatternProcessor((DataBlock&&) dataBlock);
	}
	else
	{
		return new OnePassAmd64PatternProcessor(data, length);
	}
}

//============================================================================

int BitStateAmd64PatternProcessor::GetBytesForBitState(const void* data)
{
	uint32_t numberOfBitStateInstructions = 0;
	const ByteCodeHeader* header = (const ByteCodeHeader*) data;
	
	const PatternData patternData(header->GetForwardProgram());
	for(uint32_t i = 0; i < header->numberOfInstructions; ++i)
	{
		if(!patternData[i].isSingleReference) numberOfBitStateInstructions++;
	}
	
	return (numberOfBitStateInstructions+7) >> 3;
}

void BitStateAmd64PatternProcessor::Set(const void* data, int bytesForBackTracking)
{
	const ByteCodeHeader* header = (const ByteCodeHeader*) data;
	numberOfBitStateBytes = bytesForBackTracking;
	fullMatchAlwaysFits = (header->maximumMatchLength+1) * numberOfBitStateBytes <= MAXIMUM_BITS/8;
}

const void* BitStateAmd64PatternProcessor::FullMatch(const void* data, size_t length) const
{
	ProcessData processData(false, numberOfBitStateBytes, data, length, 0);
	return (*(ProgramType) fullMatchProgram)(processData, data, (const unsigned char*) data + length, nullptr, &processData.stateTracker, numberOfBitStateBytes);
}

const void* BitStateAmd64PatternProcessor::FullMatch(const void* data, size_t length, const char **captures) const
{
	ProcessData processData(false, numberOfBitStateBytes, data, length, 0);
	return (*(ProgramType) fullMatchProgram)(processData, data, (const unsigned char*) data + length, captures, &processData.stateTracker, numberOfBitStateBytes);
}

const void* BitStateAmd64PatternProcessor::PartialMatch(const void* data, size_t length, size_t offset) const
{
	ProcessData processData(allowPartialMatch, numberOfBitStateBytes, data, length-offset, offset);
	return (*(ProgramType) partialMatchProgram)(processData, processData.pSearchStart, (const unsigned char*) data + length, nullptr, &processData.stateTracker, numberOfBitStateBytes);
}

const void* BitStateAmd64PatternProcessor::PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const
{
	ProcessData processData(allowPartialMatch, numberOfBitStateBytes, data, length-offset, offset);
	return (*(ProgramType) partialMatchProgram)(processData, processData.pSearchStart, (const unsigned char*) data + length, captures, &processData.stateTracker, numberOfBitStateBytes);
}

Interval<const void*> BitStateAmd64PatternProcessor::LocatePartialMatch(const void* data, size_t length, size_t offset) const
{
	const char* captures[numberOfCaptures*2];
	captures[0] = nullptr;
	captures[1] = nullptr;
	ProcessData processData(allowPartialMatch, numberOfBitStateBytes, data, length-offset, offset);
	(*(ProgramType) partialMatchProgram)(processData, processData.pSearchStart, (const unsigned char*) data + length, captures, &processData.stateTracker, numberOfBitStateBytes);
	return {captures[0], captures[1]};
}

const void* BitStateAmd64PatternProcessor::PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const
{
	ProcessData processData(allowPartialMatch, numberOfBitStateBytes, data, length-offset, offset);
	return (*(ProgramType) fullMatchProgram)(processData, processData.pSearchStart, (const unsigned char*) data + length, captures, &processData.stateTracker, numberOfBitStateBytes);
}

PatternProcessor* PatternProcessor::CreateBitStateProcessor(const void* data, size_t length)
{
	return new BitStateAmd64PatternProcessor(data, length);
}

//============================================================================

void BackTrackingAmd64PatternProcessor::Set(const void* data)
{
	const ByteCodeHeader* header = (const ByteCodeHeader*) data;
	numberOfProgressChecks = header->numberOfProgressChecks;
}

const void* BackTrackingAmd64PatternProcessor::FullMatch(const void* data, size_t length) const
{
	void* stackBuffer = alloca(sizeof(ProcessData) + numberOfProgressChecks*sizeof(const unsigned char*));
	ProcessData* processData = new(Placement(stackBuffer)) ProcessData(false, data, length, 0, numberOfProgressChecks);
	return (*(ProgramType) fullMatchProgram)(*processData, data, (const unsigned char*) data + length, nullptr);
}

const void* BackTrackingAmd64PatternProcessor::FullMatch(const void* data, size_t length, const char **captures) const
{
	void* stackBuffer = alloca(sizeof(ProcessData) + numberOfProgressChecks*sizeof(const unsigned char*));
	ProcessData* processData = new(Placement(stackBuffer)) ProcessData(false, data, length, 0, numberOfProgressChecks);
	return (*(ProgramType) fullMatchProgram)(*processData, data, (const unsigned char*) data + length, captures);
}

const void* BackTrackingAmd64PatternProcessor::PartialMatch(const void* data, size_t length, size_t offset) const
{
	void* stackBuffer = alloca(sizeof(ProcessData) + numberOfProgressChecks*sizeof(const unsigned char*));
	ProcessData* processData = new(Placement(stackBuffer)) ProcessData(allowPartialMatch, data, length, offset, numberOfProgressChecks);
	return (*(ProgramType) partialMatchProgram)(*processData, processData->pSearchStart, (const unsigned char*) data + length, nullptr);
}

const void* BackTrackingAmd64PatternProcessor::PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const
{
	void* stackBuffer = alloca(sizeof(ProcessData) + numberOfProgressChecks*sizeof(const unsigned char*));
	ProcessData* processData = new(Placement(stackBuffer)) ProcessData(allowPartialMatch, data, length, offset, numberOfProgressChecks);
	return (*(ProgramType) partialMatchProgram)(*processData, processData->pSearchStart, (const unsigned char*) data + length, captures);
}

Interval<const void*> BackTrackingAmd64PatternProcessor::LocatePartialMatch(const void* data, size_t length, size_t offset) const
{
	const char* captures[numberOfCaptures*2];
	captures[0] = nullptr;
	captures[1] = nullptr;
	void* stackBuffer = alloca(sizeof(ProcessData) + numberOfProgressChecks*sizeof(const unsigned char*));
	ProcessData* processData = new(Placement(stackBuffer)) ProcessData(allowPartialMatch, data, length, offset, numberOfProgressChecks);
	(*(ProgramType) partialMatchProgram)(*processData, processData->pSearchStart, (const unsigned char*) data + length, captures);
	return {captures[0], captures[1]};
}

const void* BackTrackingAmd64PatternProcessor::PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const
{
	void* stackBuffer = alloca(sizeof(ProcessData) + numberOfProgressChecks*sizeof(const unsigned char*));
	ProcessData* processData = new(Placement(stackBuffer)) ProcessData(allowPartialMatch, data, length, offset, numberOfProgressChecks);
	return (*(ProgramType) fullMatchProgram)(*processData, processData->pSearchStart, (const unsigned char*) data + length, captures);
}

PatternProcessor* PatternProcessor::CreateBackTrackingProcessor(DataBlock&& dataBlock)
{
	return new BackTrackingAmd64PatternProcessor((DataBlock&&) dataBlock);
}

PatternProcessor* PatternProcessor::CreateBackTrackingProcessor(const void* data, size_t length, bool makeCopy)
{
	if(makeCopy)
	{
		DataBlock dataBlock(data, length);
		return new BackTrackingAmd64PatternProcessor((DataBlock&&) dataBlock);
	}
	else
	{
		return new BackTrackingAmd64PatternProcessor(data, length);
	}
}

//============================================================================
#endif
//============================================================================



