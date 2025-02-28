//============================================================================

#include "Javelin/Pattern/Internal/PatternProcessor.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

#if defined(JASM_GNUC_X86_64)
	#include "PatternProcessorAsmGnuAmd64.h"
#elif defined(JASM_GNUC_ARM_NEON) || defined(JASM_GNUC_ARM64)
	#include "PatternProcessorArmNeon.h"
#endif

//============================================================================

#if !defined(FUNCTION_FIND_BYTE_DEFINED)
const void* PatternProcessor::FindByte(const void* p, uint64_t v, const void* pEnd)
{
	return memchr(p, v, size_t(pEnd)-size_t(p));
}
#endif

#if !defined(FUNCTION_FIND_BYTE_PAIR_DEFINED)
const void* PatternProcessor::FindBytePair(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c0 = v & 0xff;
	unsigned char c1 = v>>8 & 0xff;
	
	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		process = (const unsigned char*) FindByte(process, c0, pEnd);
		if(!process) return nullptr;
		if(process+1 >= pEnd) return nullptr;
		if(process[1] == c1) return process;
		++process;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BYTE_PAIR_2_DEFINED)
const void* PatternProcessor::FindBytePair2(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c1a = v>>16 & 0xff;
	unsigned char c1b = v>>24 & 0xff;
	
	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		process = (const unsigned char*) FindByteEitherOf2(process, v, pEnd);
		if(!process) return nullptr;
		if(process+1 >= pEnd) return nullptr;
		if(process[1] == c1a || process[1] == c1b) return process;
		++process;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BYTE_PAIR_3_DEFINED)
const void* PatternProcessor::FindBytePair3(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c1a = v>>24 & 0xff;
	unsigned char c1b = v>>32 & 0xff;
	unsigned char c1c = v>>40 & 0xff;
	
	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		process = (const unsigned char*) FindByteEitherOf3(process, v, pEnd);
		if(!process) return nullptr;
		if(process+1 >= pEnd) return nullptr;
		if(process[1] == c1a || process[1] == c1b || process[1] == c1c) return process;
		++process;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BYTE_PAIR_4_DEFINED)
const void* PatternProcessor::FindBytePair4(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c1a = v>>32 & 0xff;
	unsigned char c1b = v>>40 & 0xff;
	unsigned char c1c = v>>48 & 0xff;
	unsigned char c1d = v>>56 & 0xff;
	
	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		process = (const unsigned char*) FindByteEitherOf4(process, v, pEnd);
		if(!process) return nullptr;
		if(process+1 >= pEnd) return nullptr;
		if(process[1] == c1a || process[1] == c1b || process[1] == c1c || process[1] == c1d) return process;
		++process;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BYTE_EITHER_OF_2_DEFINED)
const void* PatternProcessor::FindByteEitherOf2(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c0 = v & 0xff;
	unsigned char c1 = v>>8 & 0xff;

	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		if(*process == c0 || *process == c1) return process;
		++process;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BYTE_EITHER_OF_3_DEFINED)
const void* PatternProcessor::FindByteEitherOf3(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c0 = v & 0xff;
	unsigned char c1 = v>>8 & 0xff;
	unsigned char c2 = v>>16 & 0xff;
	
	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		if(*process == c0 || *process == c1 || *process == c2) return process;
		++process;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BYTE_EITHER_OF_4_DEFINED)
const void* PatternProcessor::FindByteEitherOf4(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c0 = v & 0xff;
	unsigned char c1 = v>>8 & 0xff;
	unsigned char c2 = v>>16 & 0xff;
	unsigned char c3 = v>>24 & 0xff;
	
	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		if(*process == c0 || *process == c1 || *process == c2 || *process == c3) return process;
		++process;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BYTE_EITHER_OF_5_DEFINED)
const void* PatternProcessor::FindByteEitherOf5(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c0 = v & 0xff;
	unsigned char c1 = v>>8 & 0xff;
	unsigned char c2 = v>>16 & 0xff;
	unsigned char c3 = v>>24 & 0xff;
	unsigned char c4 = v>>32 & 0xff;
	
	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		if(*process == c0 || *process == c1 || *process == c2 || *process == c3
		   || *process == c4) return process;
		++process;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BYTE_EITHER_OF_6_DEFINED)
const void* PatternProcessor::FindByteEitherOf6(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c0 = v & 0xff;
	unsigned char c1 = v>>8 & 0xff;
	unsigned char c2 = v>>16 & 0xff;
	unsigned char c3 = v>>24 & 0xff;
	unsigned char c4 = v>>32 & 0xff;
	unsigned char c5 = v>>40 & 0xff;
	
	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		if(*process == c0 || *process == c1 || *process == c2 || *process == c3
		   || *process == c4 || *process == c5) return process;
		++process;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BYTE_EITHER_OF_7_DEFINED)
const void* PatternProcessor::FindByteEitherOf7(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c0 = v & 0xff;
	unsigned char c1 = v>>8 & 0xff;
	unsigned char c2 = v>>16 & 0xff;
	unsigned char c3 = v>>24 & 0xff;
	unsigned char c4 = v>>32 & 0xff;
	unsigned char c5 = v>>40 & 0xff;
	unsigned char c6 = v>>48 & 0xff;
	
	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		if(*process == c0 || *process == c1 || *process == c2 || *process == c3
		   || *process == c4 || *process == c5 || *process == c6) return process;
		++process;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BYTE_EITHER_OF_8_DEFINED)
const void* PatternProcessor::FindByteEitherOf8(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c0 = v & 0xff;
	unsigned char c1 = v>>8 & 0xff;
	unsigned char c2 = v>>16 & 0xff;
	unsigned char c3 = v>>24 & 0xff;
	unsigned char c4 = v>>32 & 0xff;
	unsigned char c5 = v>>40 & 0xff;
	unsigned char c6 = v>>48 & 0xff;
	unsigned char c7 = v>>56 & 0xff;
	
	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		if(*process == c0 || *process == c1 || *process == c2 || *process == c3
		   || *process == c4 || *process == c5 || *process == c6 || *process == c7) return process;
		++process;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BYTE_RANGE_DEFINED)
const void* PatternProcessor::FindByteRange(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c0l = v & 0xff;
	unsigned char c0h = v>>8 & 0xff;
	uint32_t d0 = c0h - c0l;
	
	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		if(uint32_t(*process - c0l) <= d0) return process;
		++process;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BYTE_RANGE_PAIR_DEFINED)
const void* PatternProcessor::FindByteRangePair(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c1l = v>>16 & 0xff;
	unsigned char c1h = v>>24 & 0xff;
	uint32_t d1 = c1h - c1l;
	
	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		process = (const unsigned char*) FindByteRange(process, v, pEnd);
		if(!process) return nullptr;
		if(process+1 >= pEnd) return nullptr;
		if(uint32_t(process[1] - c1l) <= d1) return process;
		++process;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BOYER_MOORE_DEFINED)
const void* PatternProcessor::FindBoyerMoore(const void* p, const ByteCodeSearchData* data, const void* pEnd)
{
	const unsigned char* pSearch = (const unsigned char*) p + data->length;
	for(;;)
	{
		if(pSearch >= pEnd) return nullptr;
		uint32_t adjust = data->data[*pSearch];
		pSearch += adjust;
		if(adjust == 0) break;
	}
	
	return pSearch - data->length;
}
#endif

#if !defined(FUNCTION_FIND_SHIFT_OR_DEFINED)

#if defined(__arm64__)
  #define FIND_SHIFT_OR_USE_64_BIT_LOAD 1
#else
  #define FIND_SHIFT_OR_USE_64_BIT_LOAD 0
#endif

const void* PatternProcessor::FindShiftOr(const void* p, const ByteCodeSearchData* data, const void* pEnd)
{
	uint32_t state = ~0U;
	const unsigned char* process = (const unsigned char*) p;
	const unsigned char* end = (const unsigned char*) pEnd - 8;

	uint32_t testMask = ~0U << data->length;

	while(process < end)
	{
#if FIND_SHIFT_OR_USE_64_BIT_LOAD
        uint64_t value;
        memcpy(&value, process, sizeof(value));

        state = state << 8
                | data->data[value & 0xff] << 7
                | data->data[(value >> 8) & 0xff] << 6
                | data->data[(value >> 16) & 0xff] << 5
                | data->data[(value >> 24) & 0xff] << 4
                | data->data[(value >> 32) & 0xff] << 3
                | data->data[(value >> 40) & 0xff] << 2
                | data->data[(value >> 48) & 0xff] << 1
                | data->data[(value >> 56) & 0xff];

#else
		state = state << 8
				| data->data[process[0]] << 7
				| data->data[process[1]] << 6
				| data->data[process[2]] << 5
				| data->data[process[3]] << 4
				| data->data[process[4]] << 3
				| data->data[process[5]] << 2
				| data->data[process[6]] << 1
				| data->data[process[7]];
#endif
		process += 8;
		
		if(JUNLIKELY(state & testMask))
		{
			uint32_t test = ~state >> data->length;
			uint32_t clz = __builtin_clz(test);
			return process + clz - 32 - data->length;
		}
	}
	
	while(process < pEnd)
	{
		state = (state << 1) | data->data[*process++];
		if((state & testMask) == 0) return process - data->length-1;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BYTE_TRIPLET_DEFINED)
const void* PatternProcessor::FindByteTriplet(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c2 = v>>16 & 0xff;
	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		process = (const unsigned char*) FindBytePair(process, v, pEnd);
		if(!process) return nullptr;
		if(process+2 >= pEnd) return nullptr;
		if(process[2] == c2) return process;
		++process;
	}
	return nullptr;
}
#endif

#if !defined(FUNCTION_FIND_BYTE_TRIPLET_2_DEFINED)
const void* PatternProcessor::FindByteTriplet2(const void* p, uint64_t v, const void* pEnd)
{
	unsigned char c2a = v>>32 & 0xff;
	unsigned char c2b = v>>40 & 0xff;
	
	const unsigned char* process = (const unsigned char*) p;
	while(process < pEnd)
	{
		process = (const unsigned char*) FindBytePair2(process, v, pEnd);
		if(!process) return nullptr;
		if(process+2 >= pEnd) return nullptr;
		if(process[2] == c2a || process[2] == c2b) return process;
		++process;
	}
	return nullptr;
}
#endif

//============================================================================
