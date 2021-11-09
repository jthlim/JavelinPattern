//============================================================================

#include "Javelin/Assembler/JitLabelId.h"
#include <string.h>

//============================================================================

using namespace Javelin;

//===========================================================================

#if defined(__amd64__)
	#include <nmmintrin.h>
	__attribute__((target("sse4.2")))
	uint32_t Javelin::GetLabelIdForNamed(const char *label)
	{
		uint32_t result = 0xffffffff;
		while(*label) result = _mm_crc32_u8(result, *label++);
		return (result << 2) | LabelType::Named;
	}
#elif defined(__arm64__)
	__attribute__((target("crc")))
	uint32_t Javelin::GetLabelIdForNamed(const char *label)
	{
		uint32_t result = 0xffffffff;
		while(*label)
		{
			asm volatile("crc32cb %w0, %w0, %w1"
						 : "+r"(result)
						 : "r"(*label++));
		}
		return (result << 2) | LabelType::Named;
	}
#else
    #include "JitHash.h"

    uint32_t Javelin::GetLabelIdForNamed(const char *label)
    {
        uint32_t value = JitHash(label, strlen(label));
        return (~value << 2) | LabelType::Named;
    }
#endif

//============================================================================
