//============================================================================

#include "Javelin/System/Unix/Machine.h"
#include "Javelin/JavelinBase.h"
#include <unistd.h>

//============================================================================

using namespace Javelin;

//============================================================================

size_t Machine::GetPageSize()
{
	return getpagesize();
}

size_t Machine::GetNumberOfProcessors()
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}

#if defined(JASM_GNUC_X86_64)
#include <cpuid.h>

bool Machine::SupportsAvx()
{
	const uint32_t OS_XSAVE_BIT = (1<<27);
	const uint32_t AVX_BIT = (1<<28);
	const uint32_t AVX_MASK = (AVX_BIT | OS_XSAVE_BIT);
	
	unsigned int eax, ebx, ecx, edx;
	__get_cpuid(1, &eax, &ebx, &ecx, &edx);
	return (ecx & AVX_MASK) == AVX_MASK;
}

// Inlined since the compiler is having problems finding <intrin.h>
static JINLINE uint64_t _xgetbv(unsigned int __xcr_no) {
	unsigned int __eax, __edx;
	__asm__ ("xgetbv" : "=a" (__eax), "=d" (__edx) : "c" (__xcr_no));
	return ((uint64_t) __edx << 32) | __eax;
}

bool Machine::SupportsAvx2()
{
	if(!SupportsAvx()) return false;
	
	const uint32_t EXTENDED_FEATURES_LEVEL = 7;
	if(__get_cpuid_max(0, nullptr) < EXTENDED_FEATURES_LEVEL) return false;
	
	unsigned int eax, ebx, ecx, edx;
	__cpuid_count(EXTENDED_FEATURES_LEVEL, 0, eax, ebx, ecx, edx);
	
	const uint32_t AVX2_BIT = (1<<5);
	if((ebx & AVX2_BIT) == 0) return false;
	
	const uint32_t XMM_AND_YMMSAVE_MASK = 6;
	uint64_t xgetbv = _xgetbv(0);
	if((xgetbv & XMM_AND_YMMSAVE_MASK) != XMM_AND_YMMSAVE_MASK) return false;
	
	return true;
}

bool Machine::SupportsAvx512BW()
{
    if(!SupportsAvx()) return false;
    
    const uint32_t EXTENDED_FEATURES_LEVEL = 7;
    if(__get_cpuid_max(0, nullptr) < EXTENDED_FEATURES_LEVEL) return false;
    
    unsigned int eax, ebx, ecx, edx;
    __cpuid_count(EXTENDED_FEATURES_LEVEL, 0, eax, ebx, ecx, edx);
    
    const uint32_t AVX512F_BW_BIT = (1<<16) | (1<<30);
    if((ebx & AVX512F_BW_BIT) != AVX512F_BW_BIT) return false;
    
#if defined(JPLATFORM_MACOS)
    // macOS has implemented on-demand support for avx512, so the xsetbv call will not return the upper bits set.
    const uint32_t OPMASK_XMM_YMM_ZMM_SAVE_MASK = 6;
#else
    const uint32_t OPMASK_XMM_YMM_ZMM_SAVE_MASK = 0xe6;
#endif
    uint64_t xgetbv = _xgetbv(0);
    if((xgetbv & OPMASK_XMM_YMM_ZMM_SAVE_MASK) != OPMASK_XMM_YMM_ZMM_SAVE_MASK) return false;
    
    return true;
}

#elif defined(JASM_GNUC_ARM)
  #if defined(JPLATFORM_IOS)

    bool Machine::SupportsNeon()
    {
        return true;
    }

  #else
    #include <sys/auxv.h>
    #include <asm/hwcap.h>

    bool Machine::SupportsNeon()
    {
        return (getauxval(AT_HWCAP) & HWCAP_NEON) != 0;
    }

  #endif
#endif

//============================================================================
