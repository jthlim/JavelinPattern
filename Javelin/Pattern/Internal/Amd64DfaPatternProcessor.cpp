//============================================================================

#include "Javelin/Pattern/Internal/Amd64DfaPatternProcessor.h"
#include "Javelin/Pattern/Internal/Amd64PatternProcessorsFindMethods.h"
#include "Javelin/Pattern/Internal/PatternDfaState.h"
#include "Javelin/Pattern/Internal/PatternNibbleMask.h"
#include "Javelin/System/Machine.h"
#if defined(JABI_AMD64_SYSTEM_V)

#include <immintrin.h>

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;
using namespace Javelin::PatternInternal::Amd64FindMethods;

//============================================================================

template<Amd64DfaPatternProcessor::SearchHandler SEARCH_FUNCTION>
const unsigned char* Amd64DfaPatternProcessor::FindAssertWrapper(const unsigned char* p, const void* data, const unsigned char* pEnd)
{
	const unsigned char* pSearch = (*SEARCH_FUNCTION)(p, data, pEnd);
	if(!pSearch) return nullptr;
	return pSearch > p ? pSearch-1 : p;
}

//============================================================================

static const unsigned char* AvxFindByteNibbleMask(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vmovdqu %0, %%xmm0" : : "m"(sbd->bytes[0]) : "%xmm0");
	asm volatile("vmovdqu %0, %%xmm1" : : "m"(sbd->bytes[16]) : "%xmm1");
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvxFindNibbleMask)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset;
}

static const unsigned char* Avx2FindByteNibbleMask(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vmovdqu %0, %%xmm0" : : "m"(sbd->bytes[0]) : "%xmm0");
	asm volatile("vmovdqu %0, %%xmm1" : : "m"(sbd->bytes[16]) : "%xmm1");
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvx2FindNibbleMask)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset;
}

static const unsigned char* AvxFindPairNibbleMask(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	const ByteCodeSearchMultiByteData* mbd = (const ByteCodeSearchMultiByteData*) &sbd->bytes[8];
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;

	asm volatile("vmovdqu %0, %%xmm0" : : "m"(mbd->nibbleMask[0]) : "%xmm0");
	asm volatile("vmovdqu %0, %%xmm1" : : "m"(mbd->nibbleMask[1]) : "%xmm1");
	asm volatile("vmovdqu %0, %%xmm2" : : "m"(mbd->nibbleMask[2]) : "%xmm2");
	asm volatile("vmovdqu %0, %%xmm3" : : "m"(mbd->nibbleMask[3]) : "%xmm3");

	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvxFindPairNibbleMask)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset - 1;
}

static const unsigned char* Avx2FindPairNibbleMask(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	const ByteCodeSearchMultiByteData* mbd = (const ByteCodeSearchMultiByteData*) &sbd->bytes[8];
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vmovdqu %0, %%xmm0" : : "m"(mbd->nibbleMask[0]) : "%xmm0");
	asm volatile("vmovdqu %0, %%xmm1" : : "m"(mbd->nibbleMask[1]) : "%xmm1");
	asm volatile("vmovdqu %0, %%xmm2" : : "m"(mbd->nibbleMask[2]) : "%xmm2");
	asm volatile("vmovdqu %0, %%xmm3" : : "m"(mbd->nibbleMask[3]) : "%xmm3");
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvx2FindPairNibbleMask)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset - 1;
}

static const unsigned char* AvxFindPairNibbleMaskPath(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	const ByteCodeSearchMultiByteData* mbd = (const ByteCodeSearchMultiByteData*) &sbd->bytes[8];
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vmovdqu %0, %%xmm0" : : "m"(mbd->nibbleMask[0]) : "%xmm0");
	asm volatile("vmovdqu %0, %%xmm1" : : "m"(mbd->nibbleMask[1]) : "%xmm1");
	asm volatile("vmovdqu %0, %%xmm2" : : "m"(mbd->nibbleMask[2]) : "%xmm2");
	asm volatile("vmovdqu %0, %%xmm3" : : "m"(mbd->nibbleMask[3]) : "%xmm3");
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvxFindPairNibbleMaskPath)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset - 1;
}

static const unsigned char* Avx2FindPairNibbleMaskPath(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	const ByteCodeSearchMultiByteData* mbd = (const ByteCodeSearchMultiByteData*) &sbd->bytes[8];
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vmovdqu %0, %%xmm0" : : "m"(mbd->nibbleMask[0]) : "%xmm0");
	asm volatile("vmovdqu %0, %%xmm1" : : "m"(mbd->nibbleMask[1]) : "%xmm1");
	asm volatile("vmovdqu %0, %%xmm2" : : "m"(mbd->nibbleMask[2]) : "%xmm2");
	asm volatile("vmovdqu %0, %%xmm3" : : "m"(mbd->nibbleMask[3]) : "%xmm3");
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvx2FindPairNibbleMaskPath)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset - 1;
}

static const unsigned char* AvxShiftOrFindTripletNibbleMask(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchShiftOrData* sbd = (const ByteCodeSearchShiftOrData*) data;
	asm volatile("vmovdqu %0, %%xmm0" : : "m"(sbd->nibbleMask[0]) : "%xmm0");
	asm volatile("vmovdqu %0, %%xmm1" : : "m"(sbd->nibbleMask[1]) : "%xmm1");
	asm volatile("vmovdqu %0, %%xmm2" : : "m"(sbd->nibbleMask[2]) : "%xmm2");
	asm volatile("vmovdqu %0, %%xmm3" : : "m"(sbd->nibbleMask[3]) : "%xmm3");
	asm volatile("vmovdqu %0, %%xmm4" : : "m"(sbd->nibbleMask[4]) : "%xmm4");
	asm volatile("vmovdqu %0, %%xmm5" : : "m"(sbd->nibbleMask[5]) : "%xmm5");
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvxFindTripletNibbleMask)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - 2;
}

static const unsigned char* AvxShiftOrFindTripletNibbleMaskPath(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchShiftOrData* sbd = (const ByteCodeSearchShiftOrData*) data;
	asm volatile("vmovdqu %0, %%xmm0" : : "m"(sbd->nibbleMask[0]) : "%xmm0");
	asm volatile("vmovdqu %0, %%xmm1" : : "m"(sbd->nibbleMask[1]) : "%xmm1");
	asm volatile("vmovdqu %0, %%xmm2" : : "m"(sbd->nibbleMask[2]) : "%xmm2");
	asm volatile("vmovdqu %0, %%xmm3" : : "m"(sbd->nibbleMask[3]) : "%xmm3");
	asm volatile("vmovdqu %0, %%xmm4" : : "m"(sbd->nibbleMask[4]) : "%xmm4");
	asm volatile("vmovdqu %0, %%xmm5" : : "m"(sbd->nibbleMask[5]) : "%xmm5");

	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvxFindTripletNibbleMaskPath)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - 2;
}

static const unsigned char* Avx2ShiftOrFindTripletNibbleMaskPath(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchShiftOrData* sbd = (const ByteCodeSearchShiftOrData*) data;
	asm volatile("vmovdqu %0, %%xmm0" : : "m"(sbd->nibbleMask[0]) : "%xmm0");
	asm volatile("vmovdqu %0, %%xmm1" : : "m"(sbd->nibbleMask[1]) : "%xmm1");
	asm volatile("vmovdqu %0, %%xmm2" : : "m"(sbd->nibbleMask[2]) : "%xmm2");
	asm volatile("vmovdqu %0, %%xmm3" : : "m"(sbd->nibbleMask[3]) : "%xmm3");
	asm volatile("vmovdqu %0, %%xmm4" : : "m"(sbd->nibbleMask[4]) : "%xmm4");
	asm volatile("vmovdqu %0, %%xmm5" : : "m"(sbd->nibbleMask[5]) : "%xmm5");
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvx2FindTripletNibbleMaskPath)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - 2;
}

static const unsigned char* AvxFindEitherOf2(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;

	asm volatile("vmovd %0, %%xmm0\n"
				 "vpunpcklbw %%xmm0, %%xmm0, %%xmm0\n"
				 "vpunpcklwd %%xmm0, %%xmm0, %%xmm0\n"
	
				 "vpshufd $0x55, %%xmm0, %%xmm1\n"
				 : : "r"((uint32_t) sbd->GetFourBytes()) : "%xmm0", "%xmm1");
	
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvxFindEitherOf2)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset;
}

static const unsigned char* Avx2FindEitherOf2(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vpbroadcastb %0, %%xmm0" : : "m"(sbd->bytes[0]) : "%xmm0");
	asm volatile("vpbroadcastb %0, %%xmm1" : : "m"(sbd->bytes[1]) : "%xmm1");
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvx2FindEitherOf2)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset;
}

static const unsigned char* AvxFindEitherOf3(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vmovd %0, %%xmm0\n"
				 "vpunpcklbw %%xmm0, %%xmm0, %%xmm0\n"
				 "vpunpcklwd %%xmm0, %%xmm0, %%xmm0\n"
				 
				 "vpshufd $0xaa, %%xmm0, %%xmm2\n"
				 "vpshufd $0x55, %%xmm0, %%xmm1\n"
				 : : "r"((uint32_t) sbd->GetFourBytes()) : "%xmm0", "%xmm1", "%xmm2");
	
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvxFindEitherOf3)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset;
}

static const unsigned char* Avx2FindEitherOf3(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vpbroadcastb %0, %%xmm0" : : "m"(sbd->bytes[0]) : "%xmm0");
	asm volatile("vpbroadcastb %0, %%xmm1" : : "m"(sbd->bytes[1]) : "%xmm1");
	asm volatile("vpbroadcastb %0, %%xmm2" : : "m"(sbd->bytes[2]) : "%xmm2");
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvx2FindEitherOf3)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset;
}

static const unsigned char* AvxFindBytePair(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vmovd %0, %%xmm0\n"
				 "vpunpcklbw %%xmm0, %%xmm0, %%xmm0\n"
				 "vpunpcklwd %%xmm0, %%xmm0, %%xmm0\n"
				 
				 "vpshufd $0x55, %%xmm0, %%xmm1\n"
				 : : "r"((uint32_t) sbd->GetFourBytes()) : "%xmm0", "%xmm1");
	
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvxFindPair)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset - 1;
}

static const unsigned char* Avx2FindBytePair(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vpbroadcastb %0, %%xmm0" : : "m"(sbd->bytes[0]) : "%xmm0");
	asm volatile("vpbroadcastb %0, %%xmm1" : : "m"(sbd->bytes[1]) : "%xmm1");
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvx2FindPair)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset - 1;
}

struct Amd64ByteRangeSearchData
{
	uint32_t offset;
	uint32_t searchValues[4];
};

static const unsigned char* AvxFindByteRange(const unsigned char* p, const void* data, const unsigned char* pStop)
{
    const Amd64ByteRangeSearchData* sbd = (const Amd64ByteRangeSearchData*) data;
    uint32_t offset = sbd->offset;
    p += offset;
    if(p >= pStop) return nullptr;
    
    asm volatile("vmovq %0, %%xmm0\n"
                 "vpshufd $0x55, %%xmm0, %%xmm1\n"
                 : : "m"(sbd->searchValues[0]) : "%xmm0", "%xmm1");
    
    
    const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvxFindByteRange)(p, p, pStop);
    if(!pSearch) return nullptr;
    return pSearch - offset;
}

static const unsigned char* AvxFindByteRangePair(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const Amd64ByteRangeSearchData* sbd = (const Amd64ByteRangeSearchData*) data;
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vmovdqu %0, %%xmm0\n"
				 
				 "vpshufd $0xff, %%xmm0, %%xmm3\n"
				 "vpshufd $0xaa, %%xmm0, %%xmm2\n"
				 "vpshufd $0x55, %%xmm0, %%xmm1\n"
				 : : "m"(sbd->searchValues[0]) : "%xmm0", "%xmm1", "%xmm2", "%xmm3");
	
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvxFindByteRangePair)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset - 1;
}

static const unsigned char* AvxFindBytePair2(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vmovd %0, %%xmm0\n"
				 "vpunpcklbw %%xmm0, %%xmm0, %%xmm0\n"
				 "vpunpcklwd %%xmm0, %%xmm0, %%xmm0\n"
				 
				 "vpshufd $0xff, %%xmm0, %%xmm3\n"
				 "vpshufd $0xaa, %%xmm0, %%xmm2\n"
				 "vpshufd $0x55, %%xmm0, %%xmm1\n"
				 : : "r"((uint32_t) sbd->GetFourBytes()) : "%xmm0", "%xmm1", "%xmm2", "%xmm3");
	
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvxFindPair2)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset - 1;
}

static const unsigned char* Avx2FindBytePair2(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vpbroadcastb %0, %%xmm0" : : "m"(sbd->bytes[0]) : "%xmm0");
	asm volatile("vpbroadcastb %0, %%xmm1" : : "m"(sbd->bytes[1]) : "%xmm1");
	asm volatile("vpbroadcastb %0, %%xmm2" : : "m"(sbd->bytes[2]) : "%xmm2");
	asm volatile("vpbroadcastb %0, %%xmm3" : : "m"(sbd->bytes[3]) : "%xmm3");
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvx2FindPair2)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset - 1;
}

static const unsigned char* AvxFindBytePairPath2(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vmovd %0, %%xmm0\n"
				 "vpunpcklbw %%xmm0, %%xmm0, %%xmm0\n"
				 "vpunpcklwd %%xmm0, %%xmm0, %%xmm0\n"
				 
				 "vpshufd $0xff, %%xmm0, %%xmm3\n"
				 "vpshufd $0xaa, %%xmm0, %%xmm2\n"
				 "vpshufd $0x55, %%xmm0, %%xmm1\n"
				 : : "r"((uint32_t) sbd->GetFourBytes()) : "%xmm0", "%xmm1", "%xmm2", "%xmm3");
	
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvxFindPairPath2)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset - 1;
}

static const unsigned char* Avx2FindBytePairPath2(const unsigned char* p, const void* data, const unsigned char* pStop)
{
	const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) data;
	uint32_t offset = sbd->offset;
	p += offset;
	if(p >= pStop) return nullptr;
	
	asm volatile("vpbroadcastb %0, %%xmm0" : : "m"(sbd->bytes[0]) : "%xmm0");
	asm volatile("vpbroadcastb %0, %%xmm1" : : "m"(sbd->bytes[1]) : "%xmm1");
	asm volatile("vpbroadcastb %0, %%xmm2" : : "m"(sbd->bytes[2]) : "%xmm2");
	asm volatile("vpbroadcastb %0, %%xmm3" : : "m"(sbd->bytes[3]) : "%xmm3");
	
	const unsigned char* pSearch = ((const unsigned char* (*)(const void* data, const unsigned char* p, const unsigned char* pEnd)) &Amd64FindMethods::InternalAvx2FindPairPath2)(p, p, pStop);
	if(!pSearch) return nullptr;
	return pSearch - offset - 1;
}

//============================================================================

void Amd64DfaPatternProcessor::CreateNibbleMask(State* state, int numberOfBytes) const
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

void Amd64DfaPatternProcessor::CreateByteRangeData(State* state, int count) const
{
    const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) state->searchData;
    Amd64ByteRangeSearchData* p = (Amd64ByteRangeSearchData*) state->workingArea;
    p->offset = sbd->offset;
    
    for(int i = 0; i < count; ++i)
    {
        unsigned char low = sbd->bytes[i*2];
        unsigned char high = sbd->bytes[i*2+1];
        p->searchValues[i*2] = ((127-high)&0xff)*0x1010101;
        p->searchValues[i*2+1] = ((126-(high-low))&0xff)*0x1010101;
    }
    
    state->searchData = p;
}

Amd64DfaPatternProcessor::SearchHandler Amd64DfaPatternProcessor::GetSearchHandler(SearchHandlerEnum value, State* state) const
{
	if(Machine::SupportsAvx())
	{
		switch(value)
		{
		case SearchHandlerEnum::SearchByteEitherOf2:
			return Machine::SupportsAvx2() ? (SearchHandler) Avx2FindEitherOf2 : (SearchHandler) AvxFindEitherOf2;

		case SearchHandlerEnum::SearchByteEitherOf2WithAssert:
			return Machine::SupportsAvx2() ? FindAssertWrapper<Avx2FindEitherOf2> : FindAssertWrapper<AvxFindEitherOf2>;

		case SearchHandlerEnum::SearchByteEitherOf3:
			return Machine::SupportsAvx2() ? (SearchHandler) Avx2FindEitherOf3 : (SearchHandler) AvxFindEitherOf3;

		case SearchHandlerEnum::SearchByteEitherOf3WithAssert:
			return Machine::SupportsAvx2() ? FindAssertWrapper<Avx2FindEitherOf3> : FindAssertWrapper<AvxFindEitherOf3>;

		case SearchHandlerEnum::SearchByteEitherOf4:
		case SearchHandlerEnum::SearchByteEitherOf5:
		case SearchHandlerEnum::SearchByteEitherOf6:
		case SearchHandlerEnum::SearchByteEitherOf7:
		case SearchHandlerEnum::SearchByteEitherOf8:
			CreateNibbleMask(state, (int) value - (int) SearchHandlerEnum::SearchByte0);
			return Machine::SupportsAvx2() ? Avx2FindByteNibbleMask : AvxFindByteNibbleMask;
				
		case SearchHandlerEnum::SearchByteEitherOf4WithAssert:
		case SearchHandlerEnum::SearchByteEitherOf5WithAssert:
		case SearchHandlerEnum::SearchByteEitherOf6WithAssert:
		case SearchHandlerEnum::SearchByteEitherOf7WithAssert:
		case SearchHandlerEnum::SearchByteEitherOf8WithAssert:
			CreateNibbleMask(state, (int) value - (int) SearchHandlerEnum::SearchByte0WithAssert);
			return Machine::SupportsAvx2() ? FindAssertWrapper<Avx2FindByteNibbleMask> : FindAssertWrapper<AvxFindByteNibbleMask>;
				
		case SearchHandlerEnum::SearchBytePair:
			return Machine::SupportsAvx2() ? (SearchHandler) Avx2FindBytePair : (SearchHandler) AvxFindBytePair;
				
		case SearchHandlerEnum::SearchBytePairWithAssert:
			return Machine::SupportsAvx2() ? FindAssertWrapper<Avx2FindBytePair> : FindAssertWrapper<AvxFindBytePair>;
			
		case SearchHandlerEnum::SearchBytePair2:
			{
				const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) state->searchData;
				const ByteCodeSearchMultiByteData* smbd = (const ByteCodeSearchMultiByteData*) &sbd->bytes[4];

				if(smbd->numberOfNibbleMasks == 2)
				{
					return smbd->isPath ?
								Machine::SupportsAvx2() ?
									(SearchHandler) Avx2FindBytePairPath2 :
									(SearchHandler) AvxFindBytePairPath2 :
								Machine::SupportsAvx2() ?
									(SearchHandler) Avx2FindBytePair2 :
									(SearchHandler) AvxFindBytePair2;
				}
			}
			break;
				
		case SearchHandlerEnum::SearchBytePair2WithAssert:
			{
				const ByteCodeSearchByteData* sbd = (const ByteCodeSearchByteData*) state->searchData;
				const ByteCodeSearchMultiByteData* smbd = (const ByteCodeSearchMultiByteData*) &sbd->bytes[4];
				
				if(smbd->numberOfNibbleMasks == 2)
				{
					return smbd->isPath ?
								Machine::SupportsAvx2() ?
									FindAssertWrapper<Avx2FindBytePairPath2> :
									FindAssertWrapper<AvxFindBytePairPath2> :
								Machine::SupportsAvx2() ?
									FindAssertWrapper<Avx2FindBytePair2> :
									FindAssertWrapper<AvxFindBytePair2>;
				}
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
							(Machine::SupportsAvx2() ?
							 	(SearchHandler) Avx2FindPairNibbleMaskPath :
							 	(SearchHandler) AvxFindPairNibbleMaskPath) :
							(SearchHandler) AvxFindPairNibbleMask;
				}
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
							(Machine::SupportsAvx2() ?
							 	FindAssertWrapper<Avx2FindPairNibbleMaskPath> :
							 	FindAssertWrapper<AvxFindPairNibbleMaskPath>) :
							FindAssertWrapper<AvxFindPairNibbleMask>;
				}
			}
			break;

        case SearchHandlerEnum::SearchByteRange:
            CreateByteRangeData(state, 1);
            return (SearchHandler) AvxFindByteRange;

        case SearchHandlerEnum::SearchByteRangeWithAssert:
            CreateByteRangeData(state, 1);
            return FindAssertWrapper<AvxFindByteRange>;

        case SearchHandlerEnum::SearchByteRangePair:
			CreateByteRangeData(state, 2);
			return (SearchHandler) AvxFindByteRangePair;
				
		case SearchHandlerEnum::SearchByteRangePairWithAssert:
			CreateByteRangeData(state, 2);
			return FindAssertWrapper<AvxFindByteRangePair>;
			
		case SearchHandlerEnum::SearchShiftOr:
			{
				const ByteCodeSearchShiftOrData* searchData = (const ByteCodeSearchShiftOrData*) state->searchData;
				if(searchData->numberOfNibbleMasks > 0)
				{
					switch(searchData->numberOfNibbleMasks)
					{
					case 3:
						return searchData->isPath ?
								(Machine::SupportsAvx2() ?
								 	Avx2ShiftOrFindTripletNibbleMaskPath :
								 	AvxShiftOrFindTripletNibbleMaskPath) :
								AvxShiftOrFindTripletNibbleMask;
					}
				}
			}
			break;
				
		case SearchHandlerEnum::SearchShiftOrWithAssert:
			{
				const ByteCodeSearchShiftOrData* searchData = (const ByteCodeSearchShiftOrData*) state->searchData;
				if(searchData->numberOfNibbleMasks > 0)
				{
					switch(searchData->numberOfNibbleMasks)
					{
					case 3:
						return searchData->isPath ?
								(Machine::SupportsAvx2() ?
									 FindAssertWrapper<Avx2ShiftOrFindTripletNibbleMaskPath> :
									 FindAssertWrapper<AvxShiftOrFindTripletNibbleMaskPath>) :
								FindAssertWrapper<AvxShiftOrFindTripletNibbleMask>;
					}
				}
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
	return new Amd64DfaPatternProcessor(data, length);
}

//============================================================================
#endif // defined(JABI_AMD64_SYSTEM_V)
//============================================================================
