//============================================================================

#include "Javelin/Pattern/Pattern.h"
#include "Javelin/Container/StackBuffer.h"
#include "Javelin/Pattern/Internal/PatternByteCode.h"
#include "Javelin/Pattern/Internal/PatternCompiler.h"
#include "Javelin/Pattern/Internal/PatternDfaMemoryManager.h"
#include "Javelin/Pattern/Internal/PatternNoStackGrowthHandler.h"
#include "Javelin/Pattern/Internal/PatternProcessor.h"
#include "Javelin/Stream/IWriter.h"
#include "Javelin/Type/VariableName.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class DefaultStackGrowthHandler : public Pattern::StackGrowthHandler
{
public:
	constexpr DefaultStackGrowthHandler() { }
	
	virtual void* Allocate(size_t size) const	{ return malloc(size); 	}
	virtual void Free(void* p) const			{ free(p);				}

	static const DefaultStackGrowthHandler instance;
};

constexpr NoStackGrowthHandler		NoStackGrowthHandler::instance;
constexpr DefaultStackGrowthHandler	DefaultStackGrowthHandler::instance;
const Pattern::StackGrowthHandler*	Pattern::stackGrowthHandler = &DefaultStackGrowthHandler::instance;

//============================================================================
	
MatchResult::MatchResult(const char** captures, uint32_t numberOfCaptures)
{
	captureList.SetCount(numberOfCaptures);
	for(size_t i = 0; i < numberOfCaptures; ++i)
	{
		captureList[i].capture = { captures[i*2], captures[i*2+1] };
	}
}

const String MatchResult::MatchData::GetString() const
{
	if(capture.IsValid()) return String{capture};
	else return String::EMPTY_STRING;
}

const char* MatchResult::GetCaptureBegin(size_t captureIndex) const
{
	if(!HasResult(captureIndex)) return nullptr;
	return captureList[captureIndex].capture.min;
}

const char* MatchResult::GetCaptureEnd(size_t captureIndex) const
{
	if(!HasResult(captureIndex)) return nullptr;
	return captureList[captureIndex].capture.max;
}

String Javelin::ToString(const MatchResult& a)
{
	if(a) return JS("Match");
	else return JS("No Match");
}

//============================================================================

PatternInternal::PatternProcessor* Pattern::CreateProcessor(DataBlock&& dataBlock, PatternInternal::PatternProcessorType type)
{
	switch(type)
	{
	case PatternProcessorType::BackTracking:
		return PatternProcessor::CreateBackTrackingProcessor((DataBlock&&) dataBlock);
		
	case PatternProcessorType::ConsistencyCheck:
		return PatternProcessor::CreateConsistencyCheckProcessor((DataBlock&&) dataBlock);
		
	case PatternProcessorType::Nfa:
		return PatternProcessor::CreateNfaProcessor((DataBlock&&) dataBlock);

	case PatternProcessorType::NfaOrBitStateBackTracking:
		return PatternProcessor::CreateNfaOrBitStateProcessor((DataBlock&&) dataBlock);
			
	case PatternProcessorType::ScanAndCapture:
		return PatternProcessor::CreateScanAndCaptureProcessor((DataBlock&&) dataBlock);
			
	case PatternProcessorType::OnePass:
		return PatternProcessor::CreateOnePassProcessor((DataBlock&&) dataBlock);
		
	default:
		JERROR("Invalid processor type!");
		return nullptr;
	}
}

Pattern::Pattern(const String& pattern, int options) // Can throw PatternException
{
	Compiler compiler(pattern.Begin(), pattern.End(), options);
	compiler.Compile(options);

	numberOfCaptures = compiler.GetNumberOfCaptures();
	const DataBlock& dataBlock = compiler.GetByteCode();
	
	const ByteCodeHeader* header = (ByteCodeHeader*) dataBlock.GetData();
	flags = header->flags.value;
	minimumMatchLength = header->minimumMatchLength;
	maximumMatchLength = header->maximumMatchLength == TypeData<uint16_t>::Maximum() ? TypeData<uint32_t>::Maximum() : size_t(header->maximumMatchLength);
	matchLengthCheck = GetMaximumLength() - GetMinimumLength();
	JASSERT(GetMatchLengthCheck() == GetMaximumLength() - GetMinimumLength());
	
	const void* data = dataBlock.GetData();
	size_t length = dataBlock.GetNumberOfBytes();
	
	partialMatchProcessor = CreateProcessor((DataBlock&&) dataBlock, header->flags.partialMatchProcessorType);
	if(header->flags.partialMatchProcessorType == header->flags.fullMatchProcessorType)
	{
		fullMatchProcessor = partialMatchProcessor;
	}
	else
	{
		fullMatchProcessor = CreateProcessor(data, length, false, header->flags.fullMatchProcessorType);
	}
}

Pattern::Pattern(const void* data, size_t length, bool makeCopy)
{
	Set(data, length, makeCopy);
}

JINLINE size_t Pattern::GetMatchLengthCheck() const
{
	return size_t(ssize_t(matchLengthCheck));
}

PatternProcessor* Pattern::CreateProcessor(const void* data, size_t length, bool makeCopy, PatternProcessorType type)
{
	switch(type)
	{
	case PatternProcessorType::BackTracking:
		return PatternProcessor::CreateBackTrackingProcessor(data, length, makeCopy);
		
	case PatternProcessorType::ConsistencyCheck:
		return PatternProcessor::CreateConsistencyCheckProcessor(data, length, makeCopy);
		
	case PatternProcessorType::Nfa:
		return PatternProcessor::CreateNfaProcessor(data, length, makeCopy);
		
	case PatternProcessorType::NfaOrBitStateBackTracking:
		return PatternProcessor::CreateNfaOrBitStateProcessor(data, length, makeCopy);
			
	case PatternProcessorType::ScanAndCapture:
		return PatternProcessor::CreateScanAndCaptureProcessor(data, length, makeCopy);
			
	case PatternProcessorType::OnePass:
		return PatternProcessor::CreateOnePassProcessor(data, length, makeCopy);
		
	default:
		JERROR("Invalid processor type!");
		return nullptr;
	}
}

void Pattern::Set(const void* data, size_t length, bool makeCopy)
{
	const ByteCodeHeader* header = (ByteCodeHeader*) data;
	JVERIFY(header->IsValid());
	
	numberOfCaptures = header->numberOfCaptures;
	flags = header->flags.value;
	minimumMatchLength = header->minimumMatchLength;
	maximumMatchLength = header->maximumMatchLength == TypeData<uint16_t>::Maximum() ? TypeData<uint32_t>::Maximum() : header->maximumMatchLength;
	matchLengthCheck = GetMaximumLength() - GetMinimumLength();
	JASSERT(GetMatchLengthCheck() == GetMaximumLength() - GetMinimumLength());

	partialMatchProcessor = CreateProcessor(data, length, makeCopy, header->flags.partialMatchProcessorType);
	
	if(header->flags.partialMatchProcessorType == header->flags.fullMatchProcessorType)
	{
		fullMatchProcessor = partialMatchProcessor;
	}
	else
	{
		fullMatchProcessor = CreateProcessor(data, length, makeCopy, header->flags.fullMatchProcessorType);
	}
}

Pattern::~Pattern()
{
	if(partialMatchProcessor != fullMatchProcessor)
	{
		delete fullMatchProcessor;
	}
	delete partialMatchProcessor;
}

//============================================================================

JINLINE bool Pattern::HasStartAnchor() const
{
	ByteCodeFlags bcf;
	bcf.value = flags;
	return bcf.hasStartAnchor;
}

JINLINE bool Pattern::HasEndAnchor() const
{
	ByteCodeFlags bcf = { .value = flags };
	return bcf.matchRequiresEndOfInput;
}

JINLINE bool Pattern::AlwaysRequiresCaptures() const
{
	ByteCodeFlags bcf = { .value = flags };
	return bcf.alwaysRequiresCaptures;
}

//============================================================================

DataBlock Pattern::CreateByteCode(const String& pattern, int options) // Can throw PatternException
{
	Compiler compiler(pattern.Begin(), pattern.End(), options);
	compiler.Compile(options);
	return compiler.GetByteCode();
}

void Pattern::SetNoStackGrowth()
{
	stackGrowthHandler = &NoStackGrowthHandler::instance;
}

void Pattern::DumpInstructions(IWriter& output, const DataBlock& data)
{
	const ByteCodeHeader* header = (const ByteCodeHeader*) data.GetData();
	if(!header->IsValid())
	{
		output.PrintF("Header not valid\n");
		return;
	}
	
	static const char *const PROCESSOR_TYPES[] = { "none", "nfa", "bit-state", "one-pass", "back-tracking", "scan-and-capture", "consistency-check", "nfa-or-bitstate-back-tracking", "fixed-length", "anchored" };

	output.PrintF("Pattern: \"%A\"\n", &header->GetPatternString());
	output.PrintF("Options:");
	if(header->patternStringOptions == 0) output.PrintF(" {none}\n");
	else
	{
		if(header->patternStringOptions & IGNORE_CASE) output.PrintF(" IGNORE_CASE");
		if(header->patternStringOptions & MULTILINE) output.PrintF(" MULTILINE");
		if(header->patternStringOptions & DOTALL) output.PrintF(" DOTALL");
		if(header->patternStringOptions & UNICODE_CASE) output.PrintF(" UNICODE_CASE");
		if(header->patternStringOptions & UNGREEDY) output.PrintF(" UNGREEDY");
		if(header->patternStringOptions & UTF8) output.PrintF(" UTF8");
		if(header->patternStringOptions & AUTO_CLUSTER) output.PrintF(" AUTO_CLUSTER");
		if(header->patternStringOptions & GLOB_SYNTAX) output.PrintF(" GLOB_SYNTAX");
		if(header->patternStringOptions & ANCHORED) output.PrintF(" ANCHORED");
		if(header->patternStringOptions & NO_OPTIMIZE) output.PrintF(" NO_OPTIMIZE");
		switch(header->patternStringOptions & PREFER_MASK)
		{
		case PREFER_NFA:				output.PrintF(" PREFER_NFA"); 				break;
		case PREFER_SCAN_AND_CAPTURE:	output.PrintF(" PREFER_SCAN_AND_CAPTURE");	break;
		case PREFER_BACK_TRACKING: 		output.PrintF(" PREFER_BACK_TRACKING"); 	break;
		case PREFER_NO_SCAN:			output.PrintF(" PREFER_NO_SCAN"); 			break;
		}
		if(header->patternStringOptions == 0) output.PrintF("NONE");
		output.PrintF("\n");
	}
	
	static constexpr const char* ANCHOR_TEXT[] =
	{
		"none",
		"start",
		"end",
		"both"
	};
	
	bool hasReverseProgram = header->flags.reverseProcessorType != PatternProcessorType::None
								&& header->flags.reverseProcessorType != PatternProcessorType::FixedLength
								&& header->flags.reverseProcessorType != PatternProcessorType::Anchored;
	
	output.PrintF("PartialMatch: %s -> %u\n", PROCESSOR_TYPES[(int) header->flags.partialMatchProcessorType], header->partialMatchStartingInstruction);
	output.PrintF("FullMatch: %s -> %u\n", PROCESSOR_TYPES[(int) header->flags.fullMatchProcessorType], header->fullMatchStartingInstruction);
	output.PrintF(hasReverseProgram ? "ReverseMatch: %s -> %u\n" : "ReverseMatch: %s\n", PROCESSOR_TYPES[(int) header->flags.reverseProcessorType], header->reverseMatchStartingInstruction);
	output.PrintF("NumberOfCaptures: %u\n", header->numberOfCaptures);
	output.PrintF("NumberOfProgressChecks: %u\n", header->numberOfProgressChecks);
    if(header->IsFixedLength())
    {
        output.PrintF("MatchLength: %u\n", header->minimumMatchLength);
    }
    else if(header->maximumMatchLength == TypeData<uint16_t>::Maximum())
    {
        output.PrintF("MatchLength: [%u, ∞]\n", header->minimumMatchLength);
    }
    else
    {
        output.PrintF("MatchLength: [%u, %u]\n", header->minimumMatchLength, header->maximumMatchLength);
    }
	output.PrintF("Anchoring: %s\n", ANCHOR_TEXT[header->flags.hasStartAnchor + header->flags.matchRequiresEndOfInput*2]);

	const ByteCodeInstruction* instructions = (const ByteCodeInstruction*) (header+1);
	DumpInstructionList(output, instructions, header->numberOfInstructions, sizeof(ByteCodeHeader));
	
	if(hasReverseProgram)
	{
		output.PrintF("\nReverse program:\n");
		const ByteCodeInstruction* instructions = header->GetReverseProgram();
		DumpInstructionList(output, instructions, header->numberOfReverseInstructions, header->reverseProgramOffset);		
	}
}

void Pattern::DumpInstructionList(IWriter& output, const ByteCodeInstruction* instructions, size_t numberOfInstructions, size_t byteCodeDataOffset)
{
	enum class InstructionParameterType
	{
		None,
		ByteValueAndInstruction,
		Call,
		Value,
		SingleByte,
		DoubleByte,
		TripleByte,
		BitMask,
		ByteRange,
		JumpMask,
		JumpRange,
		JumpTable,
		Save,
		SearchByte,
		SearchByte2,
		SearchByte3,
		SearchByte4,
		SearchByte5,
		SearchByte6,
		SearchByte7,
		SearchByte8,
		SearchData,
		Split,
		SplitMatch,
		ValueAndInstruction,
		WordBoundaryHint,
	};
	
	struct InstructionData
	{
		String						name;
		InstructionParameterType	parameterType;
	};
	
	static const InstructionData INSTRUCTION_DATA[] =
	{
		#define TAG(x, y, c) { VariableName{JS(#x), VariableName::CAMEL_CASE}.AsLowerWithSeparator('-'), InstructionParameterType::y },
		#include "Internal/PatternInstructionTypeTags.h"
		#undef TAG
	};

	for(uint32_t i = 0; i < numberOfInstructions; ++i)
	{
		ByteCodeInstruction instruction = instructions[i];
		const InstructionData& data = INSTRUCTION_DATA[(int) instruction.type];
		output.PrintF("%3z%c %A", i, instruction.isSingleReference ? '.' : ':', &data.name);
		switch(data.parameterType)
		{
		case InstructionParameterType::None:
			output.PrintF("\n");
			break;

		case InstructionParameterType::Call:
			{
				const unsigned char* byteCodeData = (const unsigned char*) instructions;
				const ByteCodeCallData* callData = (const ByteCodeCallData*) (byteCodeData+instruction.data);
				output.PrintF(" %d -> ", callData->callIndex);
				if(callData->falseIndex != TypeData<uint32_t>::Maximum()) output.PrintF("%d : ", callData->falseIndex);
				else output.PrintF("fail : ");
				if(callData->trueIndex != TypeData<uint32_t>::Maximum()) output.PrintF("%d\n", callData->trueIndex);
				else output.PrintF("fail\n");
			}
			break;
				
		case InstructionParameterType::Save:
			{
				uint32_t saveIndex = instruction.data & 0xff;
				uint32_t saveOffset = instruction.data >> 8;
				output.PrintF(saveOffset != 0 ? " %d @ %d\n" : " %d\n", saveIndex, -saveOffset);
			}
			break;

		case InstructionParameterType::Value:
			output.PrintF(" %d\n", instruction.data);
			break;
				
		case InstructionParameterType::SingleByte:
		case InstructionParameterType::DoubleByte:
		case InstructionParameterType::TripleByte:
			{
				int numberOfBytes = (int) (data.parameterType) - (int) InstructionParameterType::SingleByte + 1;
				for(int i = 0; i < numberOfBytes; ++i)
				{
					int byte = instruction.data >> (8*i) & 0xff;
					output.PrintF(" '%C'", byte);
				}
				output.PrintF("\n");
			}
			break;
				
		case InstructionParameterType::ByteRange:
			output.PrintF(" {'%C','%C'}\n", instruction.data & 0xff, instruction.data>>8 & 0xff);
			break;

		case InstructionParameterType::JumpTable:
			{
				const unsigned char* data = (const unsigned char*) instructions;
				const ByteCodeJumpTableData* jumpTable = (const ByteCodeJumpTableData*) (data+instruction.data);
				uint32_t numberOfTargets = jumpTable->numberOfTargets;
				for(uint32_t j = 0; j < numberOfTargets; ++j)
				{
					if(jumpTable->pcData[j] == TypeData<uint32_t>::Maximum())
					{
						output.PrintF(" fail");
					}
					else
					{
						uint32_t pc = jumpTable->pcData[j];
						output.PrintF(" %u", pc);
					}
				}
			}
			output.PrintF(" [0x%x]\n", instruction.data+ byteCodeDataOffset);
			break;
				
		case InstructionParameterType::JumpMask:
			{
				const unsigned char* data = (const unsigned char*) instructions;
				const ByteCodeJumpMaskData* jumpMaskData = (const ByteCodeJumpMaskData*) (data+instruction.data);
				for(uint32_t j = 0; j < 2; ++j)
				{
					if(jumpMaskData->pcData[j] == TypeData<uint32_t>::Maximum())
					{
						output.PrintF(" fail");
					}
					else
					{
						uint32_t pc = jumpMaskData->pcData[j];
						output.PrintF(" %u", pc);
					}
				}
				output.PrintF(" %A", &String::CreateHexStringFromBytes(&jumpMaskData->bitMask, 32));
			}
			output.PrintF(" [0x%x]\n", instruction.data+ byteCodeDataOffset);
			break;
				
		case InstructionParameterType::JumpRange:
			{
				const unsigned char* data = (const unsigned char*) instructions;
				const ByteCodeJumpRangeData* jumpRangeData = (const ByteCodeJumpRangeData*) (data+instruction.data);
				for(uint32_t j = 0; j < 2; ++j)
				{
					if(jumpRangeData->pcData[j] == TypeData<uint32_t>::Maximum())
					{
						output.PrintF(" fail");
					}
					else
					{
						uint32_t pc = jumpRangeData->pcData[j];
						output.PrintF(" %u", pc);
					}
				}
				output.PrintF(" {'%C','%C'} [0x%x]\n", jumpRangeData->range.min, jumpRangeData->range.max, instruction.data+ byteCodeDataOffset);
			}
			break;
				
		case InstructionParameterType::BitMask:
			{
				const unsigned char* data = (const unsigned char*) instructions;
				output.PrintF(" %A", &String::CreateHexStringFromBytes(data+instruction.data, 32));
			}
			output.PrintF(" [0x%x]\n", instruction.data+ byteCodeDataOffset);
			break;
				
		case InstructionParameterType::Split:
			{
				const unsigned char* data = (const unsigned char*) instructions;
				const ByteCodeSplitData* splitData = (const ByteCodeSplitData*) (data+instruction.data);
				for(uint32_t i = 0; i < splitData->numberOfTargets; ++i)
				{
					output.PrintF(" %u", splitData->targetList[i]);
				}
				output.PrintF(" [0x%x]\n", instruction.data + byteCodeDataOffset);
			}
			break;

		case InstructionParameterType::SplitMatch:
			{
				const uint32_t* data = (const uint32_t*) ((const unsigned char*) instructions + instruction.data);
				output.PrintF(" %u %u [0x%x]\n", data[0], data[1], instruction.data+ byteCodeDataOffset);
			}
			break;
				
		case InstructionParameterType::ByteValueAndInstruction:
			output.PrintF(" '%C' -> %u\n", instruction.data & 0xff, instruction.data >> 8);
			break;

		case InstructionParameterType::ValueAndInstruction:
			output.PrintF(" %u -> %u\n", instruction.data & 0xff, instruction.data >> 8);
			break;
				
		case InstructionParameterType::SearchByte:
			output.PrintF((instruction.data >> 8) != 0 ? " '%C' @ %u\n" : " '%C'\n", instruction.data & 0xff, instruction.data >> 8);
			break;

		case InstructionParameterType::SearchByte2:
			{
				const unsigned char* data = (const unsigned char*) instructions;
				const ByteCodeSearchByteData* searchData = (const ByteCodeSearchByteData*) (data+instruction.data);
				output.PrintF(searchData->offset != 0 ? " '%C' '%C' @ %u" : " '%C' '%C'", searchData->bytes[0], searchData->bytes[1], searchData->offset);
				output.PrintF(" [0x%x]\n", instruction.data + byteCodeDataOffset);
			}
			break;

		case InstructionParameterType::SearchByte3:
			{
				const unsigned char* data = (const unsigned char*) instructions;
				const ByteCodeSearchByteData* searchData = (const ByteCodeSearchByteData*) (data+instruction.data);
				output.PrintF(searchData->offset != 0 ? " '%C' '%C' '%C' @ \n" : " '%C' '%C' '%C'", searchData->bytes[0], searchData->bytes[1], searchData->bytes[2], searchData->offset);
				output.PrintF(" [0x%x]\n", instruction.data + byteCodeDataOffset);
			}
			break;

		case InstructionParameterType::SearchByte4:
			{
				const unsigned char* data = (const unsigned char*) instructions;
				const ByteCodeSearchByteData* searchData = (const ByteCodeSearchByteData*) (data+instruction.data);
				output.PrintF(searchData->offset != 0 ? " '%C' '%C' '%C' '%C' @ %u" : " '%C' '%C' '%C' '%C'", searchData->bytes[0], searchData->bytes[1], searchData->bytes[2], searchData->bytes[3], searchData->offset);
				output.PrintF(" [0x%x]\n", instruction.data + byteCodeDataOffset);
			}
			break;
				
		case InstructionParameterType::SearchByte5:
			{
				const unsigned char* data = (const unsigned char*) instructions;
				const ByteCodeSearchByteData* searchData = (const ByteCodeSearchByteData*) (data+instruction.data);
				output.PrintF(searchData->offset != 0 ? " '%C' '%C' '%C' '%C' '%C' @ %u" : " '%C' '%C' '%C' '%C' '%C'", searchData->bytes[0], searchData->bytes[1], searchData->bytes[2], searchData->bytes[3], searchData->bytes[4], searchData->offset);
				output.PrintF(" [0x%x]\n", instruction.data + byteCodeDataOffset);
			}
			break;
				
		case InstructionParameterType::SearchByte6:
			{
				const unsigned char* data = (const unsigned char*) instructions;
				const ByteCodeSearchByteData* searchData = (const ByteCodeSearchByteData*) (data+instruction.data);
				output.PrintF(searchData->offset != 0 ? " '%C' '%C' '%C' '%C' '%C' '%C' @ %u" : " '%C' '%C' '%C' '%C' '%C' '%C'", searchData->bytes[0], searchData->bytes[1], searchData->bytes[2], searchData->bytes[3], searchData->bytes[4], searchData->bytes[5], searchData->offset);
				output.PrintF(" [0x%x]\n", instruction.data + byteCodeDataOffset);
			}
			break;
				
		case InstructionParameterType::SearchByte7:
			{
				const unsigned char* data = (const unsigned char*) instructions;
				const ByteCodeSearchByteData* searchData = (const ByteCodeSearchByteData*) (data+instruction.data);
				output.PrintF(searchData->offset != 0 ? " '%C' '%C' '%C' '%C' '%C' '%C' '%C' @ %u" : " '%C' '%C' '%C' '%C' '%C' '%C' '%C'", searchData->bytes[0], searchData->bytes[1], searchData->bytes[2], searchData->bytes[3], searchData->bytes[4], searchData->bytes[5], searchData->bytes[6], searchData->offset);
				output.PrintF(" [0x%x]\n", instruction.data + byteCodeDataOffset);
			}
			break;
				
		case InstructionParameterType::SearchByte8:
			{
				const unsigned char* data = (const unsigned char*) instructions;
				const ByteCodeSearchByteData* searchData = (const ByteCodeSearchByteData*) (data+instruction.data);
				output.PrintF(searchData->offset != 0 ? " '%C' '%C' '%C' '%C' '%C' '%C' '%C' '%C' @ %u" : " '%C' '%C' '%C' '%C' '%C' '%C' '%C' '%C'", searchData->bytes[0], searchData->bytes[1], searchData->bytes[2], searchData->bytes[3], searchData->bytes[4], searchData->bytes[5], searchData->bytes[6], searchData->bytes[7], searchData->offset);
				output.PrintF(" [0x%x]\n", instruction.data + byteCodeDataOffset);
			}
			break;
				
		case InstructionParameterType::SearchData:
			{
				const unsigned char* data = (const unsigned char*) instructions;
				const ByteCodeSearchData* searchData = (const ByteCodeSearchData*) (data+instruction.data);
				output.PrintF(" %u [0x%x]\n", searchData->length, instruction.data+ byteCodeDataOffset);
			}
			break;
				
		case InstructionParameterType::WordBoundaryHint:
			if(instruction.data & ByteCodeWordBoundaryHint::PreviousIsWordOnly) output.PrintF(" previous-is-word-only");
			if(instruction.data & ByteCodeWordBoundaryHint::PreviousIsNotWordOnly) output.PrintF(" previous-is-not-word-only");
			if(instruction.data & ByteCodeWordBoundaryHint::NextIsWordOnly) output.PrintF(" next-is-word-only");
			if(instruction.data & ByteCodeWordBoundaryHint::NextIsNotWordOnly) output.PrintF(" next-is-not-word-only");
			output.PrintF("\n");
			break;
		}
	}
}

//============================================================================

const void* Pattern::InternalHasFullMatch(const void* data, size_t length) const
{
	size_t testLength = length - GetMinimumLength();
	if(JUNLIKELY(testLength > GetMatchLengthCheck())) return nullptr;
//	if(JUNLIKELY(length < GetMinimumLength() || length > GetMaximumMatchLength())) return false;
	
	if(JUNLIKELY(AlwaysRequiresCaptures()))
	{
		return HasFullMatchWithCaptures(data, length);
	}
	else
	{
		return fullMatchProcessor->FullMatch(data, length);
	}
}

const void* Pattern::HasFullMatchWithCaptures(const void* data, size_t length) const
{
	const char* captures[2*numberOfCaptures];
	memset(captures, 0, sizeof(const char*)*2*numberOfCaptures);
	return fullMatchProcessor->FullMatch(data, length, captures);
}

const void* Pattern::InternalHasPartialMatch(const void* data, size_t length, size_t offset) const
{
	JASSERT(offset <= length);
	
	size_t remainingLength = length - offset;
	if(HasEndAnchor() && remainingLength > GetMaximumLength())
	{
		offset = length - GetMaximumLength();
	}
	else if(remainingLength < GetMinimumLength()) return nullptr;

	if(offset > 0 && HasStartAnchor()) return nullptr;

	if(JUNLIKELY(AlwaysRequiresCaptures()))
	{
		return HasPartialMatchWithCaptures(data, length, offset);
	}
	else
	{
		return partialMatchProcessor->PartialMatch(data, length, offset);
	}
}

const void *Pattern::HasPartialMatchWithCaptures(const void* data, size_t length, size_t offset) const
{
	const char* captures[2*numberOfCaptures];
	memset(captures, 0, sizeof(const char*)*2*numberOfCaptures);
	return partialMatchProcessor->PartialMatch(data, length, offset, captures);
}

bool Pattern::FullMatch(const void* data, size_t length, const void** captures) const
{
	size_t testLength = length - GetMinimumLength();
	if(JUNLIKELY(testLength > GetMatchLengthCheck())) return false;
//	if(JUNLIKELY(length < GetMinimumLength() || length > GetMaximumMatchLength())) return false;

	return fullMatchProcessor->FullMatch(data, length, (const char**) captures) != nullptr;
}

bool Pattern::PartialMatch(const void* data, size_t length, const void** captures, size_t offset) const
{
	JASSERT(offset <= length);

	size_t remainingLength = length - offset;
	if(HasEndAnchor() && remainingLength > GetMaximumLength())
	{
		offset = length - GetMaximumLength();
	}
	else if(remainingLength < GetMinimumLength()) return false;
	
	if(offset > 0 && HasStartAnchor()) return false;
	
	return partialMatchProcessor->PartialMatch(data, length, offset, (const char**) captures) != nullptr;
}

MatchResult Pattern::FullMatch(const String& s) const
{
	const char* captures[2*numberOfCaptures];
	memset(captures, 0, sizeof(const char*)*2*numberOfCaptures);

	if(FullMatch(s.GetData(), s.GetNumberOfBytes(), (const void**) captures))
	{
		return MatchResult(captures, numberOfCaptures);
	}
	else
	{
		return MatchResult(nullptr, 0);
	}
}

MatchResult Pattern::PartialMatch(const String& s, size_t offset) const
{
	const char* captures[2*numberOfCaptures];
	memset(captures, 0, sizeof(const char*)*2*numberOfCaptures);
	
	if(PartialMatch(s.GetData(), s.GetNumberOfBytes(), (const void**) captures, offset))
	{
		return MatchResult(captures, numberOfCaptures);
	}
	else
	{
		return MatchResult(nullptr, 0);
	}
}

size_t Pattern::CountPartialMatches(const void* data, size_t length, size_t offset) const
{
	JASSERT(offset <= length);
	
	size_t remainingLength = length - offset;
	if(HasEndAnchor() && remainingLength > GetMaximumLength())
	{
		offset = length - GetMaximumLength();
	}
	else if(remainingLength < GetMinimumLength()) return 0;

	if(JUNLIKELY(AlwaysRequiresCaptures()))
	{
		return CountPartialMatchesWithCaptures(data, length, offset);
	}
	
	if(HasStartAnchor())
	{
		if(offset != 0) return 0;
		
		const void* result = partialMatchProcessor->PartialMatch(data, length, offset);
		return (result != nullptr) ? 1 : 0;
	}
	
	size_t count = 0;
	if(GetMinimumLength() == 0)
	{
		do
		{
			Interval<const void*> result = partialMatchProcessor->LocatePartialMatch(data, length, offset);
			if(result.max == nullptr) return count;
			
			++count;
			offset = uintptr_t(result.max) - uintptr_t(data);
			if(result.min == result.max) ++offset;
		} while(offset < length);
	}
	else
	{
		do
		{
			const void* result = partialMatchProcessor->PartialMatch(data, length, offset);
			if(result == nullptr) return count;
			++count;
			offset = uintptr_t(result) - uintptr_t(data);
		} while(offset < length);
	}
	return count;
}

size_t Pattern::CountPartialMatchesWithCaptures(const void* data, size_t length, size_t offset) const
{
	const char* captures[2*numberOfCaptures];

	if(HasStartAnchor())
	{
		if(offset != 0) return 0;

		memset(captures, 0, sizeof(const char*)*2*numberOfCaptures);
		const void* result = partialMatchProcessor->PartialMatch(data, length, offset, captures);
		return (result != nullptr) ? 1 : 0;
	}
		

	const void* lastMatch = nullptr;
	size_t count = 0;
	do
	{
		memset(captures, 0, sizeof(const char*)*2*numberOfCaptures);
		const void* result = partialMatchProcessor->PartialMatch(data, length, offset, captures);
		if(result == nullptr) return count;
		
		if(result == lastMatch) ++offset;
		else
		{
			++count;
			offset = uintptr_t(result) - uintptr_t(data);
			lastMatch = result;
		}
	} while(offset < length);

	return count;
}

String Pattern::EscapeString(const String& literal)
{
	return StackBuffer(literal.GetNumberOfBytes()*2 + 1, [=](char* buffer) -> String
	{
		char* p = buffer;

		for(const unsigned char c : literal.RawBytes())
		{
			switch(c)
			{
			case '.':
			case '^':
			case '$':
			case '\\':
			case '|':
			case '+':
			case '?':
			case '(':
			case ')':
			case '<':
			case '>':
			case '[':
			case ']':
			case '{':
			case '*':
			case '}':
				*p++ = '\\';
				*p++ = c;
				break;

			default:
				*p++ = c;
			}
		}

		return String(buffer, p-buffer);
	});

}

//============================================================================

void Pattern::SetDfaMemoryModeConfiguration(DfaMemoryManagerMode mode, size_t limit)
{
	DfaMemoryManager::SetConfiguration(mode, limit);
}

//============================================================================
