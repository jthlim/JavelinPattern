//============================================================================

#include "Javelin/JavelinBase.h"

#if defined(JASM_GNUC_ARM) || defined(JASM_GNUC_ARM64)
#include "Javelin/Pattern/Internal/ArmNeonDfaPatternProcessor.h"
#include "Javelin/Pattern/Internal/ArmNeonPatternProcessorsFindMethods.h"
#include "Javelin/Pattern/Internal/PatternDfaState.h"
#include "Javelin/Pattern/Internal/PatternNibbleMask.h"
#include "Javelin/System/Machine.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;
using namespace Javelin::PatternInternal::ArmNeonFindMethods;

//============================================================================

template<size_t offset, const void* (*FUNCTION)(const void*, const uint8x16_t*, const void*)> const unsigned char* ArmNeonDfaPatternProcessor::FindNeonWrapper(const unsigned char* p, const void* data, const unsigned char* pEnd)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	const unsigned char* pSearch = p + sbd->offset;
	if(pSearch >= pEnd) return nullptr;
	pSearch = (const unsigned char*) (*FUNCTION)(pSearch, (uint8x16_t*) (sbd->bytes + offset), pEnd);
	if(!pSearch) return nullptr;
	return pSearch - sbd->offset;
}

template<size_t offset, const void* (*FUNCTION)(const void*, const uint8x16_t*, const void*)> const unsigned char* ArmNeonDfaPatternProcessor::FindNeonAssertWrapper(const unsigned char* p, const void* data, const unsigned char* pEnd)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	const unsigned char* pSearch = p + sbd->offset;
	if(pSearch >= pEnd) return nullptr;
	pSearch = (const unsigned char*) (*FUNCTION)(pSearch, (uint8x16_t*) (sbd->bytes + offset), pEnd);
	if(!pSearch) return nullptr;
	pSearch -= sbd->offset;
	return pSearch > p ? pSearch-1 : p;
}

static const unsigned char* NeonFindBytePairPath2(const unsigned char* p, const void* data, const unsigned char* pEnd)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	const unsigned char* pSearch = p + sbd->offset;
	if(pSearch >= pEnd) return nullptr;
	pSearch = (const unsigned char*) InternalFindBytePairPath2(pSearch, sbd->GetFourBytes(), pEnd);
	if(!pSearch) return nullptr;
	return pSearch - sbd->offset;
}

//============================================================================

void ArmNeonDfaPatternProcessor::CreateNibbleMask(State* state, int numberOfBytes) const
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) state->searchData;
	
	NibbleMask nibbleMask;
	for(int i = 0; i < numberOfBytes; ++i)
	{
		nibbleMask.AddByte(sbd->bytes[i]);
	}
	*((uint32_t*) state->workingArea) = sbd->offset;
	nibbleMask.CopyMaskToTarget(&state->workingArea[4]);
	state->searchData = &state->workingArea;
}

void ArmNeonDfaPatternProcessor::CreateByteRangePairData(State* state) const
{
	struct LocalData
	{
		uint32_t offset;
		uint32_t searchValues[4];
	};
	
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) state->searchData;
	LocalData* p = (LocalData*) state->workingArea;
	p->offset = sbd->offset;
	
	for(int i = 0; i < 2; ++i)
	{
		unsigned char low = sbd->bytes[i*2];
		unsigned char high = sbd->bytes[i*2+1];
		p->searchValues[i*2] = ((127-high)&0xff)*0x1010101;
		p->searchValues[i*2+1] = ((126-(high-low))&0xff)*0x1010101;
	}
	
	state->searchData = p;
}

ArmNeonDfaPatternProcessor::SearchHandler ArmNeonDfaPatternProcessor::GetSearchHandler(SearchHandlerEnum value, State* state) const
{
	if(Machine::SupportsNeon())
	{
		switch(value)
		{
        case SearchHandlerEnum::SearchByteEitherOf3:
		case SearchHandlerEnum::SearchByteEitherOf4:
		case SearchHandlerEnum::SearchByteEitherOf5:
		case SearchHandlerEnum::SearchByteEitherOf6:
		case SearchHandlerEnum::SearchByteEitherOf7:
		case SearchHandlerEnum::SearchByteEitherOf8:
			CreateNibbleMask(state, (int) value - (int) SearchHandlerEnum::SearchByte0);
			return FindNeonWrapper<0, InternalFindNibbleMask>;
				
        case SearchHandlerEnum::SearchByteEitherOf3WithAssert:
		case SearchHandlerEnum::SearchByteEitherOf4WithAssert:
		case SearchHandlerEnum::SearchByteEitherOf5WithAssert:
		case SearchHandlerEnum::SearchByteEitherOf6WithAssert:
		case SearchHandlerEnum::SearchByteEitherOf7WithAssert:
		case SearchHandlerEnum::SearchByteEitherOf8WithAssert:
			CreateNibbleMask(state, (int) value - (int) SearchHandlerEnum::SearchByte0WithAssert);
			return FindNeonAssertWrapper<0, InternalFindNibbleMask>;
				
		case SearchHandlerEnum::SearchBytePair2:
			{
				const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) state->searchData;
				const ByteCodeSearchMultiByteData* smbd = (const ByteCodeSearchMultiByteData*) &sbd->bytes[4];

				if(smbd->numberOfNibbleMasks == 2 && smbd->isPath)
				{
					return NeonFindBytePairPath2;
				}
                // else let parent class return the default FindBytePairPath
			}
			break;

		case SearchHandlerEnum::SearchBytePair3:
		case SearchHandlerEnum::SearchBytePair4:
			{
				const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) state->searchData;
				const ByteCodeSearchMultiByteData* smbd = (const ByteCodeSearchMultiByteData*) &sbd->bytes[8];

				if(smbd->numberOfNibbleMasks == 2)
				{
					return smbd->isPath ?
							FindNeonWrapper<12, InternalFindPairNibbleMaskPath> :
							FindNeonWrapper<12, InternalFindPairNibbleMask>;
				}
			}
			break;

		case SearchHandlerEnum::SearchBytePair2WithAssert:
			{
				const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) state->searchData;
				const ByteCodeSearchMultiByteData* smbd = (const ByteCodeSearchMultiByteData*) &sbd->bytes[4];

				if(smbd->numberOfNibbleMasks == 2 && smbd->isPath)
				{
					return FindNeonAssertWrapper<8, InternalFindPairNibbleMaskPath>;
				}
                // else let parent class return the default FindBytePairPath
			}
			break;

		case SearchHandlerEnum::SearchBytePair3WithAssert:
		case SearchHandlerEnum::SearchBytePair4WithAssert:
			{
				const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) state->searchData;
				const ByteCodeSearchMultiByteData* smbd = (const ByteCodeSearchMultiByteData*) &sbd->bytes[8];
				
				if(smbd->numberOfNibbleMasks == 2)
				{
					return smbd->isPath ?
							FindNeonAssertWrapper<12, InternalFindPairNibbleMaskPath> :
							FindNeonAssertWrapper<12, InternalFindPairNibbleMask>;
				}
                // else let parent class return the default FindBytePairPath
			}
			break;
				
		default:
			break;
		}
	}
			   
	return DfaPatternProcessor::GetSearchHandler(value, state);
}

//============================================================================

PatternProcessor* PatternProcessor::CreateDfaProcessor(const void* data, size_t length)
{
	return new ArmNeonDfaPatternProcessor(data, length);
}

//============================================================================
#endif // defined(JASM_GNUC_ARM) || defined(JASM_GNUC_ARM64)
//============================================================================
