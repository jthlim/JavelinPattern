/*==========================================================================
 
 Pattern instruction encoding
 Goals
 - Minimize branch mispredicts
 - Fixed size encoding to allow indexing into states + data block at end
 
 Header:
 uint8_t	magic[4]						// 'JP10' -- Javelin Pattern 1.0
 uint32_t	numberOfInstructions;
 uint32_t	totalPatternSize				// (Instructions + data) excluding header
 uint32_t	fullMatchStartingInstruction	// Partial match always starts at 0
 uint32_t	numberOfCaptures				// Number of strings captured. Number of pointers saved is 2x this value
 
 32 bits per instruction
 
 31   26   25                                              0
 +----+----+-----------------------------------------------+
 | Op | SR | OpData                                        |
 +----+----+-----------------------------------------------+
 
 Op     - InstructionType - 6 bits
 SR     - SingleReference - 1 bit
 OpData - 26 bits
 
 OpCode Data Notes
 
 Instructions that require extra data instead contain a relative-pointer
 into the data section, which will immediately follow the end of instructions
 
 ==========================================================================*/

#pragma once
#include "Javelin/Container/BitTable.h"
#include "Javelin/Container/Table.h"
#include "Javelin/Container/Map.h"
#include "Javelin/Stream/DataBlockWriter.h"
#include "Javelin/Pattern/Internal/PatternInstructionType.h"
#include "Javelin/Pattern/Internal/PatternProcessorType.h"

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================
	
	class Instruction;
	class NibbleMask;

	enum ByteCodeWordBoundaryHint : uint32_t
	{
		None                  = 0,
		PreviousIsNotWordOnly = 1,
		PreviousIsWordOnly    = 2,
		NextIsNotWordOnly     = 4,
		NextIsWordOnly        = 8,
	};
	
//============================================================================
	
	union ByteCodeInstruction
	{
		// The ordering of type:isSingleReference is exploited by
		// the loop in BitStateBackTrackingPatternProcessor::Process to
		// avoid having to check the bit state if there is only a
		// single reference.
		struct
		{
			uint32_t 		data 				: 24;
			uint32_t		isSingleReference	: 1;
			InstructionType type				: 7;
		};
		uint32_t value;
		
		ByteCodeInstruction() { }
		ByteCodeInstruction(uint32_t v) : value(v) { }
		
		// Returns true for all instructions marked Consumer in PatternInstructionTypeTags
		bool IsSimpleByteConsumer() const;
		
		// Returns true for Save, SaveNoRecurse, ProgressCheck and Asserts.
		bool IsZeroWidth() const;
		
		// Returns true for ByteJumpTable, ByteJumpMask, ByteJumpRange
		bool UsesJumpTargets() const;
		
		bool IsEitherOf2AndBitMaskIdentifiable() const;
		Interval<uint8_t> GetRange() const;
		
		bool IsSave() const { return type == InstructionType::Save || type == InstructionType::SaveNoRecurse; }
	};
	static_assert(sizeof(ByteCodeInstruction) == 4, "Unexpected sizeof PatternInternal::ByteCodeInstruction");
	
	union ByteCodeFlags
	{
		struct
		{
			PatternProcessorType 	partialMatchProcessorType	: 3;
			PatternProcessorType 	fullMatchProcessorType 	 	: 3;
			PatternProcessorType 	reverseProcessorType 	 	: 4;	// Reverse processor type needs extra bit for Anchored/FixedLength enum values
			bool	 				alwaysRequiresCaptures	 	: 1;
			bool					hasStartAnchor			 	: 1;
			bool					matchRequiresEndOfInput	 	: 1;
			bool					hasPotentialSlowProcessing	: 1;
			bool					useStackGuard				: 1;
			bool					hasResetCapture				: 1;
		};
		uint16_t	value;
	};
	static_assert(sizeof(ByteCodeFlags) == 2, "Expected ByteCodeFlags to fit in 2 bytes");
	
	struct ByteCodeHeader
	{
		uint8_t			magic[4];
		ByteCodeFlags	flags;
		uint8_t			numberOfCaptures;
		uint8_t			numberOfProgressChecks;
		uint16_t 		numberOfInstructions;
		uint16_t		numberOfReverseInstructions;
		uint16_t		minimumMatchLength;
		uint16_t		partialMatchStartingInstruction;
		uint16_t		fullMatchStartingInstruction;
		uint16_t		reverseMatchStartingInstruction;
		uint16_t		maximumMatchLength;
		uint16_t		patternStringOptions;
		uint32_t		reverseProgramOffset;
		uint32_t		patternStringOffset;
		uint32_t		totalPatternSize;			// This is inclusive of the header.
		
		String			GetPatternString() const;
		
		void SetMagic();
		bool IsValid() const;
		
		bool IsFixedLength() const          { return minimumMatchLength == maximumMatchLength; }
		bool IsAnchored() const             { return flags.hasStartAnchor && flags.matchRequiresEndOfInput; }
		bool RequiresReverseProgram() const	{ return flags.partialMatchProcessorType == PatternProcessorType::ScanAndCapture || flags.fullMatchProcessorType == PatternProcessorType::ScanAndCapture || flags.partialMatchProcessorType == PatternProcessorType::ConsistencyCheck; }
		
		const ByteCodeInstruction* GetForwardProgram() const { return (const ByteCodeInstruction*) (this + 1); }
		const ByteCodeInstruction* GetReverseProgram() const { return (const ByteCodeInstruction*) ((const unsigned char*) this + reverseProgramOffset); }
	};
	static_assert(sizeof(ByteCodeHeader) == 36, "Unexpected sizeof PatternInternal::ByteCodeHeader");
	
	struct ByteCodeSplitData
	{
		uint32_t	numberOfTargets;
		uint32_t	targetList[1];
	};
	
	struct ByteCodeJumpTableData
	{
		uint8_t		jumpTable[256];
		uint32_t	numberOfTargets;
		uint32_t	pcData[256];			// Dispatch uses pcData[0] as the EOF target.
		
		bool HasJumpTableTarget(uint8_t index) const;
	};
	
	struct ByteCodeJumpMaskData
	{
		StaticBitTable<256>	bitMask;
		uint32_t			pcData[2];
	};
	
	struct ByteCodeJumpRangeData
	{
		Interval<unsigned char, true>	range;
		uint8_t							_filler2 = 0;
		uint8_t							_filler3 = 0;
		uint32_t						pcData[2];
	};
	
	struct ByteCodeSearchByteData
	{
		uint32_t	offset;
		uint8_t		bytes[8];
		uint32_t 	GetFourBytes() const	{ return *(uint32_t*) bytes; }
		uint64_t 	GetAllBytes() const		{ return *(uint64_t*) bytes; }
	};
	
	// Immediately follows ByteCodeSearchByteData if pair or triplet.
	struct ByteCodeSearchMultiByteData
	{
		uint8_t		numberOfNibbleMasks;
		bool		isPath;
		uint8_t		_filler2;
		uint8_t		_filler3;
		uint8_t		nibbleMask[8][16];
		
		void Set(const Table<NibbleMask>& nibbleMaskList);
		void Set(const uint32_t* data, uint32_t length);
		void SetUnused();
	};
	
	struct ByteCodeSearchData
	{
		uint32_t	length;					// Both BoyerMoore and ShiftOr store one less than the acutal length to reduce run time instructions
		uint32_t	data[256];
		
		bool HasSingleByteData() const;
	};
	
	struct ByteCodeSearchShiftOrData : public ByteCodeSearchData, public ByteCodeSearchMultiByteData
	{
		
	};
	
	struct ByteCodeCallData
	{
		uint32_t	callIndex;
		uint32_t	falseIndex;
		uint32_t	trueIndex;
	};
	
//============================================================================

	class ByteCodeBuilder
	{
	public:
		ByteCodeBuilder(const String& pattern,
						uint32_t patternStringOptions,
						uint32_t totalInstructionCount,
						uint32_t partialMatchStartingInstruction,
						uint32_t fullMatchStartingInstruction,
						uint32_t numberOfCaptures,
						uint32_t minimumLength,
						uint32_t maximumLength,
						PatternProcessorType processorType,
						bool hasStartAnchor,
						bool matchRequiresEndOfInput,
						bool isFixedLength,
						bool isForwards,
						bool hasResetCapture);
		
		void AddInstruction(const Instruction* instruction, InstructionType opcode, uint32_t opcodeData = 0);
		uint32_t GetOffsetForData(const void* data, size_t length);
		void RegisterExistingData(uint32_t offset, size_t length);
		
		void WriteInstructionByteCode(IWriter& output) const;

		// Once instructions and data are added, this returns the full set.
		void WriteByteCode(DataBlockWriter& output) const;
		void AppendByteCode(DataBlockWriter& output) const;
		
		bool IsForwards() const { return isForwards; }
		bool IsReverse()  const { return !isForwards; }

	private:
		const String&			pattern;
		uint32_t				patternStringOptions;
		bool					isForwards;
		bool					hasStartAnchor;
		bool					isFixedLength;
		bool 					matchRequiresEndOfInput;
		bool					hasResetCapture;
		PatternProcessorType	processorType;
		uint32_t				totalInstructionCount;
		uint32_t				partialMatchStartingInstruction;
		uint32_t 				fullMatchStartingInstruction;
		uint32_t 				numberOfCaptures;
		uint32_t				minimumLength;
		uint32_t				maximumLength;
		Table<uint32_t>			instructionList;
		DataBlock				patternData;
		Map<uint64_t, uint32_t>	hashToDataMap;		// Map of crc64 -> offset in data.
		
		uint32_t	GetMaximumOfProgressCheckInstructions() const;
		bool		ContainsInstruction(InstructionType type) const;
		
		uint32_t 	GetMaximumInstructionForSinglePass(uint32_t startingInstruction, bool includeSplitMatch) const;
	};

//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
