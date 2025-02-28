//============================================================================

#include "Javelin/Pattern/Internal/PatternByteCode.h"
#include "Javelin/Container/EnumSet.h"
#include "Javelin/Cryptography/Crc64.h"
#include "Javelin/Math/BitUtility.h"
#include "Javelin/Pattern/Pattern.h"
#include "Javelin/Pattern/Internal/PatternInstruction.h"
#include "Javelin/Pattern/Internal/PatternNibbleMask.h"
#include "Javelin/Stream/IWriter.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

bool ByteCodeInstruction::IsSimpleByteConsumer() const
{
	static constexpr EnumSet<InstructionType, uint32_t> SIMPLE_BYTE_CONSUMER_SET
	{
		InstructionType::AdvanceByte,
		InstructionType::AnyByte,
		InstructionType::Byte,
		InstructionType::ByteEitherOf2,
		InstructionType::ByteEitherOf3,
		InstructionType::ByteRange,
		InstructionType::ByteBitMask,
		InstructionType::ByteJumpTable,
		InstructionType::ByteJumpMask,
		InstructionType::ByteNot,
		InstructionType::ByteNotEitherOf2,
		InstructionType::ByteNotEitherOf3,
		InstructionType::ByteNotRange,
	};

	return SIMPLE_BYTE_CONSUMER_SET.Contains(type);
}

bool ByteCodeInstruction::IsZeroWidth() const
{
	static constexpr EnumSet<InstructionType, uint64_t> ZERO_WIDTH_SET
	{
		InstructionType::AssertStartOfInput,
		InstructionType::AssertEndOfInput,
		InstructionType::AssertStartOfLine,
		InstructionType::AssertEndOfLine,
		InstructionType::AssertWordBoundary,
		InstructionType::AssertNotWordBoundary,
		InstructionType::AssertStartOfSearch,
		InstructionType::AssertRecurseValue,
		InstructionType::Save,
		InstructionType::SaveNoRecurse,
		InstructionType::ReturnIfRecurseValue,
		InstructionType::ProgressCheck,
	};
	
	return ZERO_WIDTH_SET.Contains(type);
}

bool ByteCodeInstruction::IsEitherOf2AndBitMaskIdentifiable() const
{
    if(type != InstructionType::ByteEitherOf2) return false;
    uint8_t xorResult = (value ^ (value >> 8)) & 0xff;
    return BitUtility::IsPowerOf2(xorResult);
}

bool ByteCodeInstruction::UsesJumpTargets() const
{
	switch(type)
	{
	case InstructionType::ByteJumpMask:
	case InstructionType::ByteJumpTable:
	case InstructionType::ByteJumpRange:
		return true;
		
	default:
		return false;
	}
}

Interval<uint8_t> ByteCodeInstruction::GetRange() const	{
	switch(type)
	{
	case InstructionType::Byte:
	case InstructionType::ByteNot:
		return {data, data};
			
	case InstructionType::ByteRange:
	case InstructionType::ByteNotRange:
		return { data & 0xff, data>>8 & 0xff};
			
	default:
		JERROR("Invalid instruction type for GetRange call");
		JUNREACHABLE;
	}
}

//============================================================================

String ByteCodeHeader::GetPatternString() const
{
	const char* start = (const char*) this + patternStringOffset;
	return String(start, totalPatternSize-patternStringOffset);
}

void ByteCodeHeader::SetMagic()
{
	magic[0] = 'J';
	magic[1] = 'P';
	magic[2] = '1';
	magic[3] = '0';
}

bool ByteCodeHeader::IsValid() const
{
	return magic[0] == 'J'
			&& magic[1] == 'P'
			&& magic[2] == '1'
			&& magic[3] == '0'
			&& flags.partialMatchProcessorType <= PatternProcessorType::ForwardMax
			&& flags.fullMatchProcessorType <= PatternProcessorType::ForwardMax
			&& flags.reverseProcessorType <= PatternProcessorType::ReverseMax;
}

//============================================================================

bool ByteCodeJumpTableData::HasJumpTableTarget(uint8_t index) const
{
	for(uint32_t j = 0; j < 256; ++j)
	{
		if(jumpTable[j] == index) return true;
	}
	return false;
}

bool ByteCodeSearchData::HasSingleByteData() const
{
	for(uint32_t i = 0; i < 256; ++i)
	{
		if(data[i] >= 256) return false;
	}
	return true;
}

//============================================================================

void ByteCodeSearchMultiByteData::Set(const Table<NibbleMask>& nibbleMaskList)
{
	numberOfNibbleMasks = nibbleMaskList.GetCount();
	isPath = (numberOfNibbleMasks != 0);
	_filler2 = 0;
	_filler3 = 0;
	for(size_t i = 0; i < nibbleMaskList.GetCount(); ++i)
	{
		nibbleMaskList[i].CopyMaskToTarget(nibbleMask[i*2]);
	}
}

void ByteCodeSearchMultiByteData::Set(const uint32_t* data, uint32_t length)
{
	isPath = false;
	_filler2 = 0;
	_filler3 = 0;
	for(int i = 0; i < length; ++i)
	{
		StaticBitTable<256> bits;
		int mask = 1 << i;
		for(int c = 0; c < 256; ++c)
		{
			if((data[c] & mask) == 0)
			{
				bits.SetBit(c);
			}
		}
		NibbleMask nm(bits);
		if(!nm.IsValid())
		{
			numberOfNibbleMasks = 0;
			return;
		}
		nm.CopyMaskToTarget(nibbleMask[i*2]);
	}
	numberOfNibbleMasks = length;
}

void ByteCodeSearchMultiByteData::SetUnused()
{
	numberOfNibbleMasks = 0;
	isPath = false;
	_filler2 = 0;
	_filler3 = 0;
}

//============================================================================

ByteCodeBuilder::ByteCodeBuilder(const String&		  aPattern,
								 uint32_t             aPatternStringOptions,
								 uint32_t             aTotalInstructionCount,
								 uint32_t             aPartialMatchStartingInstruction,
								 uint32_t             aFullMatchStartingInstruction,
								 uint32_t             aNumberOfCaptures,
								 uint32_t             aMinimumLength,
								 uint32_t             aMaximumLength,
								 PatternProcessorType aProcessorType,
								 bool                 aHasStartAnchor,
								 bool                 aMatchRequiresEndOfInput,
								 bool                 aIsFixedLength,
								 bool				  aIsForwards,
								 bool 				  aHasResetCapture)
: pattern(aPattern),
  patternStringOptions(aPatternStringOptions),
  totalInstructionCount(aTotalInstructionCount),
  partialMatchStartingInstruction(aPartialMatchStartingInstruction),
  fullMatchStartingInstruction(aFullMatchStartingInstruction),
  numberOfCaptures(aNumberOfCaptures),
  minimumLength(aMinimumLength),
  maximumLength(aMaximumLength),
  processorType(aProcessorType),
  isForwards(aIsForwards),
  hasResetCapture(aHasResetCapture),
  hasStartAnchor(aHasStartAnchor),
  isFixedLength(aIsFixedLength),
  matchRequiresEndOfInput(aMatchRequiresEndOfInput),
  instructionList(aTotalInstructionCount),
  patternData(4096)
{
}

//============================================================================

void ByteCodeBuilder::AddInstruction(const Instruction* instruction, InstructionType opcode, uint32_t opcodeData)
{
	ByteCodeInstruction b;
	
	JASSERT((uint32_t) opcode < 128);
	JASSERT(opcodeData < (1 << 26));
	
	b.data = opcodeData;
	b.isSingleReference = instruction->IsSingleReference();
	b.type = opcode;
	
	instructionList.Append(b.value);
}

uint32_t ByteCodeBuilder::GetOffsetForData(const void* data, size_t length)
{
	uint64_t hash = Crc64(data, length);
	
	Map<uint64_t, uint32_t>::Iterator it = hashToDataMap.Find(hash);
	if(it != hashToDataMap.End())
	{
		uint32_t offset = it->value;
		if(offset + length <= patternData.GetCount())
		{
			if(memcmp(patternData.GetData()+offset, data, length) == 0)
			{
				return offset + totalInstructionCount*4;
			}
		}
	}
	else
	{
		hashToDataMap.Insert(hash, (uint32_t) patternData.GetCount());
	}
	
	uint32_t offset = (uint32_t) patternData.GetCount();
	patternData.AppendData(data, length);
	return offset + totalInstructionCount*4;
}

void ByteCodeBuilder::RegisterExistingData(uint32_t offset, size_t length)
{
	uint32_t adjustedOffset = offset - totalInstructionCount*4;
	uint64_t hash = Crc64(&patternData[adjustedOffset], length);
	hashToDataMap.Put(Map<uint64_t, uint32_t>::Data{hash, adjustedOffset});
}

void ByteCodeBuilder::WriteInstructionByteCode(IWriter& output) const
{
	output.WriteData(instructionList);
}

uint32_t ByteCodeBuilder::GetMaximumOfProgressCheckInstructions() const
{
	uint32_t count = 0;
	for(uint32_t u : instructionList)
	{
		ByteCodeInstruction instruction{u};
		if(instruction.type == InstructionType::ProgressCheck)
		{
			if(instruction.data >= count) count = instruction.data+1;
		}
	}
	return count;
}

bool ByteCodeBuilder::ContainsInstruction(InstructionType type) const
{
	for(uint32_t value : instructionList)
	{
		ByteCodeInstruction instruction{value};
		if(instruction.type == type) return true;
	}
	return false;
}

uint32_t ByteCodeBuilder::GetMaximumInstructionForSinglePass(uint32_t startingInstruction, bool includeSplitMatch) const
{
	uint32_t maximum = (uint32_t) instructionList.GetCount();
	for(uint32_t i = startingInstruction; i < instructionList.GetCount(); ++i)
	{
		ByteCodeInstruction instruction = instructionList[i];
		
		switch(instruction.type)
		{
		case InstructionType::Split:
			if(i < maximum) maximum = i;
			{
				const ByteCodeSplitData* splitData = (const ByteCodeSplitData*) (patternData.GetData() + instruction.data - instructionList.GetNumberOfBytes());
				for(uint32_t j = 0; j < splitData->numberOfTargets; ++j)
				{
					if(splitData->targetList[j] < maximum) maximum = splitData->targetList[j];
				}
			}
			break;
				
		case InstructionType::SplitNextN:
		case InstructionType::SplitNNext:
			if(i < maximum) maximum = i;
			if(instruction.data < maximum) maximum = instruction.data;
			break;

		case InstructionType::SplitMatch:
			if(includeSplitMatch)
			{
				if(i < maximum) i = maximum;
				const uint32_t* data = (const uint32_t*) (patternData.GetData() + instruction.data - instructionList.GetNumberOfBytes());
				for(int i = 0; i < 2; ++i)
				{
					if(data[i] < maximum) maximum = data[i];
				}
			}
			break;
				
		case InstructionType::SplitNextMatchN:
		case InstructionType::SplitNMatchNext:
			if(includeSplitMatch)
			{
				if(i < maximum) maximum = i;
				if(instruction.data < maximum) maximum = instruction.data;
			}
			break;
			
		default:
			break;
		}
	}
	return maximum;
}

void ByteCodeBuilder::WriteByteCode(DataBlockWriter& writer) const
{
	ByteCodeHeader header;
	header.SetMagic();
	header.flags.value                     = 0;
	header.flags.hasStartAnchor            = hasStartAnchor;
	header.flags.matchRequiresEndOfInput   = matchRequiresEndOfInput;
	header.numberOfInstructions            = instructionList.GetCount();
	header.partialMatchStartingInstruction = partialMatchStartingInstruction;
	header.fullMatchStartingInstruction    = fullMatchStartingInstruction;
	header.numberOfReverseInstructions     = 0;
	header.reverseMatchStartingInstruction = 0;
	header.reverseProgramOffset            = 0;
	header.flags.reverseProcessorType      = PatternProcessorType::None;
	header.numberOfProgressChecks          = GetMaximumOfProgressCheckInstructions();
	header.numberOfCaptures                = numberOfCaptures;
	header.minimumMatchLength              = minimumLength;
	header.maximumMatchLength              = maximumLength > TypeData<uint16_t>::Maximum() ? TypeData<uint16_t>::Maximum() : maximumLength;
	header.flags.hasResetCapture		   = hasResetCapture;

	bool hasBackReferences = ContainsInstruction(InstructionType::BackReference);
	header.flags.alwaysRequiresCaptures = hasBackReferences;
	
	uint32_t maximumInstructionForSinglePassFullMatch    = GetMaximumInstructionForSinglePass(fullMatchStartingInstruction, false);
	uint32_t maximumInstructionForSinglePassPartialMatch = GetMaximumInstructionForSinglePass(partialMatchStartingInstruction, false);
	
	if(processorType == PatternProcessorType::Default || processorType == PatternProcessorType::NoScan)
	{
		const PatternProcessorType fallback = processorType == PatternProcessorType::Default ? PatternProcessorType::ScanAndCapture : PatternProcessorType::NfaOrBitStateBackTracking;
		header.flags.fullMatchProcessorType = maximumInstructionForSinglePassFullMatch == header.numberOfInstructions ? PatternProcessorType::OnePass : fallback;
		header.flags.partialMatchProcessorType = maximumInstructionForSinglePassPartialMatch == header.numberOfInstructions ? PatternProcessorType::OnePass : fallback;
		
		if(isFixedLength && (patternStringOptions & Pattern::NO_OPTIMIZE) == 0)
		{
			if(header.flags.fullMatchProcessorType != PatternProcessorType::OnePass)
			{
				header.flags.fullMatchProcessorType = PatternProcessorType::BackTracking;
			}
			if(header.flags.partialMatchProcessorType != PatternProcessorType::OnePass)
			{
				header.flags.partialMatchProcessorType = PatternProcessorType::BackTracking;
			}
		}
	}
	else
	{
		if(processorType == PatternProcessorType::BackTracking && !isFixedLength)
		{
			header.flags.useStackGuard = maximumInstructionForSinglePassPartialMatch != header.numberOfInstructions;
		}
		
		header.flags.fullMatchProcessorType = processorType;
		header.flags.partialMatchProcessorType = processorType;
	}
	
	if(patternStringOptions & Pattern::DO_CONSISTENCY_CHECK)
	{
		header.flags.fullMatchProcessorType = PatternProcessorType::ConsistencyCheck;
		header.flags.partialMatchProcessorType = PatternProcessorType::ConsistencyCheck;
	}

	// Back tracking only patterns
	if(hasBackReferences
	   || ContainsInstruction(InstructionType::Recurse)
	   || ContainsInstruction(InstructionType::Call)
	   || ContainsInstruction(InstructionType::Possess))
	{
		header.flags.hasPotentialSlowProcessing = true;
		
		if(header.flags.fullMatchProcessorType != PatternProcessorType::OnePass
		   && header.flags.fullMatchProcessorType != PatternProcessorType::BackTracking)
		{
			header.flags.useStackGuard = true;
			header.flags.fullMatchProcessorType = PatternProcessorType::BackTracking;
		}
		if(header.flags.partialMatchProcessorType != PatternProcessorType::OnePass
		   && header.flags.partialMatchProcessorType != PatternProcessorType::BackTracking)
		{
			header.flags.useStackGuard = true;
			header.flags.partialMatchProcessorType = PatternProcessorType::BackTracking;
		}
	}
	
	header.patternStringOffset = sizeof(ByteCodeHeader) + uint32_t(instructionList.GetNumberOfBytes() + patternData.GetNumberOfBytes());
	header.patternStringOptions = patternStringOptions;
	header.totalPatternSize = uint32_t(header.patternStringOffset + pattern.GetNumberOfBytes());

	writer.Write(header);

	// Check that narrowing conversions doesn't lose information
	JPATTERN_VERIFY(header.numberOfInstructions == instructionList.GetCount(), TooManyByteCodeInstructions, nullptr);
	JPATTERN_VERIFY(header.numberOfProgressChecks == GetMaximumOfProgressCheckInstructions(), TooManyProgressCheckInstructions, nullptr);
	JPATTERN_VERIFY(header.numberOfCaptures == numberOfCaptures, TooManyCaptures, nullptr);
	JASSERT(header.minimumMatchLength == minimumLength);
	JASSERT(header.partialMatchStartingInstruction == partialMatchStartingInstruction);
	JASSERT(header.fullMatchStartingInstruction == fullMatchStartingInstruction);
	
	writer.WriteData(instructionList);
	writer.WriteData(patternData);
	writer.WriteData(pattern);
}

void ByteCodeBuilder::AppendByteCode(DataBlockWriter& writer) const
{
	ByteCodeHeader* header = (ByteCodeHeader*) writer.GetData();
	
	if(processorType == PatternProcessorType::Default)
	{
		uint32_t maximumInstructionForSinglePassFullMatch = GetMaximumInstructionForSinglePass(0, true);
		header->flags.reverseProcessorType = maximumInstructionForSinglePassFullMatch == instructionList.GetCount() ? PatternProcessorType::OnePass : PatternProcessorType::ScanAndCapture;
	}
	else if(processorType == PatternProcessorType::NotOnePass)
	{
		uint32_t maximumInstructionForSinglePassFullMatch = GetMaximumInstructionForSinglePass(0, false);
		header->flags.reverseProcessorType = maximumInstructionForSinglePassFullMatch == instructionList.GetCount() ? PatternProcessorType::BackTracking : PatternProcessorType::ScanAndCapture;
	}
	else
	{
		header->flags.reverseProcessorType = processorType;
	}
	
	if(isFixedLength && header->flags.hasResetCapture == false)
	{
		if(header->flags.reverseProcessorType != PatternProcessorType::OnePass
		   || header->numberOfCaptures == 1)
		{
			header->flags.reverseProcessorType = PatternProcessorType::FixedLength;
			return;
		}
	}
	
	if(header->partialMatchStartingInstruction == header->fullMatchStartingInstruction)
	{
		if(header->numberOfCaptures == 1 && header->flags.hasResetCapture == false)
		{
			header->flags.reverseProcessorType = PatternProcessorType::Anchored;
			return;
		}
	}

	String patternString = header->GetPatternString();
	size_t oldPatternStringOffset = header->patternStringOffset;
	
	header->numberOfReverseInstructions = instructionList.GetCount();
	header->reverseMatchStartingInstruction = fullMatchStartingInstruction;
	header->reverseProgramOffset = header->patternStringOffset;
	size_t extraDataLength = instructionList.GetNumberOfBytes() + patternData.GetNumberOfBytes();
	header->patternStringOffset = uint32_t(header->reverseProgramOffset + extraDataLength);
	header->totalPatternSize += extraDataLength;
	
	// Strip off the pattern string.
	((DataBlock&) writer.GetBuffer()).SetCount(oldPatternStringOffset);

	writer.WriteData(instructionList);
	writer.WriteData(patternData);
	writer.WriteData(patternString);
}

//============================================================================
