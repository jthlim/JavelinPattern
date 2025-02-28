//============================================================================
// Included from PatternProcessor.cpp to provide ARM search methods
//============================================================================

#include <arm_neon.h>

#if defined(JASM_GNUC_ARM64)
  #define USE_UNALIGNED_LOADS 1
#else
  #define USE_UNALIGNED_LOADS 0
#endif

JINLINE static uint8x16_t vtblq(uint8x16_t a, uint8x16_t b)
{
#if defined(JASM_GNUC_ARM64)
	return vqtbl1q_u8(a, b);
#else
    uint8x8x2_t lookup = { vget_low_u8(a), vget_high_u8(a) };
    uint8x8_t m0 = vtbl2_u8(lookup, vget_low_u8(b));
    uint8x8_t m1 = vtbl2_u8(lookup, vget_high_u8(b));
    return vcombine_u8(m0, m1);
#endif
}

JINLINE static bool IsNotZero(uint8x16_t v)
{
    uint64x2_t v64 = vreinterpretq_u64_u8(v);
    uint32x2_t v32 = vqmovn_u64(v64);
#if defined(JASM_GNUC_ARM64)
    uint64x1_t result = vreinterpret_u64_u32(v32);
    return result[0] != 0;
#else
    uint32x2_t v32max = vpmax_u32(v32, v32);
    return v32max[0] != 0;
#endif
}

JINLINE static uint32_t Create32BitMask(uint8x16_t b0, uint8x16_t b1)
{
	static const uint8x16_t COLLAPSE_MASK = {1,2,4,8,16,32,64,128,1,2,4,8,16,32,64,128};
	
	b0 &= COLLAPSE_MASK;
	b1 &= COLLAPSE_MASK;
	
#if defined(JASM_GNUC_ARM64)
	uint8x16_t c0 = vpaddq_u8(b0, b1);
	uint8x16_t c1 = vpaddq_u8(c0, c0);
	uint8x8_t c2 = vpadd_u8(vget_low_u8(c1), vget_low_u8(c1));
	return vreinterpret_u32_u8(c2)[0];
#else
	uint8x8_t c0 = vpadd_u8(vget_low_u8(b0), vget_high_u8(b0));
	uint8x8_t c1 = vpadd_u8(vget_low_u8(b1), vget_high_u8(b1));
	uint8x8_t d = vpadd_u8(c0, c1);
	uint8x8_t e = vpadd_u8(d, d);
	return vreinterpret_u32_u8(e)[0];
#endif
}

JINLINE static uint64_t Create64BitMask(uint8x16_t b0, uint8x16_t b1, uint8x16_t b2, uint8x16_t b3)
{
    static const uint8x16_t COLLAPSE_MASK = {1,2,4,8,16,32,64,128,1,2,4,8,16,32,64,128};
    
    b0 &= COLLAPSE_MASK;
    b1 &= COLLAPSE_MASK;
    b2 &= COLLAPSE_MASK;
    b3 &= COLLAPSE_MASK;

#if defined(JASM_GNUC_ARM64)
    uint8x16_t c0 = vpaddq_u8(b0, b1);
    uint8x16_t c1 = vpaddq_u8(b2, b3);
    
    uint8x16_t d = vpaddq_u8(c0, c1);
    uint8x16_t e = vpaddq_u8(d, d);
    return vreinterpretq_u64_u8(e)[0];
#else
    #error Not implemented
#endif
}

__attribute__((no_sanitize("address")))
static const void* FindNibbleMask(const void* pIn, const uint8x16_t* nibbleMasks, const void* pEnd)
{
#if defined(JASM_GNUC_ARM64)
    int shift = int(size_t(pIn) & 0x3f);
    uint8x16_t* p = (uint8x16_t*) (intptr_t(pIn) & -64);
    
    const uint8x16_t nm0 = nibbleMasks[0];
    const uint8x16_t nm1 = nibbleMasks[1];
    const uint8x16_t AND_MASK = {0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf};
    
    uint8x16_t b0 = *p++;
    uint8x16_t b1 = *p++;
    uint8x16_t b2 = *p++;
    uint8x16_t b3 = *p++;

    uint8x16_t b0h = vshrq_n_u8(b0, 4);
    uint8x16_t b0l = b0 & AND_MASK;
    uint8x16_t b1h = vshrq_n_u8(b1, 4);
    uint8x16_t b1l = b1 & AND_MASK;
    uint8x16_t b2h = vshrq_n_u8(b2, 4);
    uint8x16_t b2l = b2 & AND_MASK;
    uint8x16_t b3h = vshrq_n_u8(b3, 4);
    uint8x16_t b3l = b3 & AND_MASK;

    uint8x16_t m0 = vtstq_u8(vtblq(nm0, b0l), vtblq(nm1, b0h));
    uint8x16_t m1 = vtstq_u8(vtblq(nm0, b1l), vtblq(nm1, b1h));
    uint8x16_t m2 = vtstq_u8(vtblq(nm0, b2l), vtblq(nm1, b2h));
    uint8x16_t m3 = vtstq_u8(vtblq(nm0, b3l), vtblq(nm1, b3h));

    const uint64_t mask = Create64BitMask(m0, m1, m2, m3) >> shift;
    if(JUNLIKELY(mask != 0))
    {
        uint8_t* result = (uint8_t*) pIn + __builtin_ctzll(mask);
        return result < pEnd ? result : nullptr;
    }
    
    while(p < pEnd)
    {
        b0 = *p++;
        b1 = *p++;
        b2 = *p++;
        b3 = *p++;

        b0h = vshrq_n_u8(b0, 4);
        b0l = b0 & AND_MASK;
        b1h = vshrq_n_u8(b1, 4);
        b1l = b1 & AND_MASK;
        b2h = vshrq_n_u8(b2, 4);
        b2l = b2 & AND_MASK;
        b3h = vshrq_n_u8(b3, 4);
        b3l = b3 & AND_MASK;

        m0 = vtstq_u8(vtblq(nm0, b0l), vtblq(nm1, b0h));
        m1 = vtstq_u8(vtblq(nm0, b1l), vtblq(nm1, b1h));
        m2 = vtstq_u8(vtblq(nm0, b2l), vtblq(nm1, b2h));
        m3 = vtstq_u8(vtblq(nm0, b3l), vtblq(nm1, b3h));

        m1 |= m0;
        m3 |= m2;
        
        if(JUNLIKELY(IsNotZero(m1|m3)))
        {
            uint64_t mask = Create64BitMask(m0, m1, m2, m3);
            uint8_t* result = (uint8_t*) p - 64 + __builtin_ctzll(mask);
            return result < pEnd ? result : nullptr;
        }
    }
    
    return nullptr;
#else
    int shift = int(size_t(pIn) & 0x1f);
    uint8x16_t* p = (uint8x16_t*) (intptr_t(pIn) & -32);
    
    const uint8x16_t nm0 = nibbleMasks[0];
    const uint8x16_t nm1 = nibbleMasks[1];
    const uint8x16_t AND_MASK = {0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf};
    
    uint8x16_t b0 = *p++;
    uint8x16_t b1 = *p++;

    uint8x16_t b0h = vshrq_n_u8(b0, 4);
    uint8x16_t b0l = b0 & AND_MASK;
    uint8x16_t b1h = vshrq_n_u8(b1, 4);
    uint8x16_t b1l = b1 & AND_MASK;
    
    b0 = vtstq_u8(vtblq(nm0, b0l), vtblq(nm1, b0h));
    b1 = vtstq_u8(vtblq(nm0, b1l), vtblq(nm1, b1h));
	
    uint32_t mask = Create32BitMask(b0, b1) >> shift;
    if(JUNLIKELY(mask != 0))
    {
        uint8_t* result = (uint8_t*) pIn + __builtin_ctz(mask);
        return result < pEnd ? result : nullptr;
    }
    
    while(p < pEnd)
    {
        b0 = *p++;
        b1 = *p++;

        b0h = vshrq_n_u8(b0, 4);
        b0l = b0 & AND_MASK;
        b1h = vshrq_n_u8(b1, 4);
        b1l = b1 & AND_MASK;
        
        b0 = vtstq_u8(vtblq(nm0, b0l), vtblq(nm1, b0h));
        b1 = vtstq_u8(vtblq(nm0, b1l), vtblq(nm1, b1h));
        
        if(JUNLIKELY(IsNotZero(b0|b1)))
        {
            mask = Create32BitMask(b0, b1);
            uint8_t* result = (uint8_t*) p - 32 + __builtin_ctz(mask);
            return result < pEnd ? result : nullptr;
        }
    }
    
    return nullptr;
#endif
}

//============================================================================

#define FUNCTION_FIND_BYTE_DEFINED
__attribute__((no_sanitize("address")))
const void* PatternProcessor::FindByte(const void* pIn, uint64_t v, const void* pEnd)
{
#if defined(JASM_GNUC_ARM64)

    int shift = int(size_t(pIn) & 0x3f);
	const uint8x16_t* p = (const uint8x16_t*) (intptr_t(pIn) & -64);

    uint8x8_t c = vcreate_u8(v);
    const uint8x16_t c0 = vdupq_lane_u8(c, 0);

    uint8x16_t b0 = *p++;
    uint8x16_t b1 = *p++;
    uint8x16_t b2 = *p++;
    uint8x16_t b3 = *p++;
    
    uint8x16_t m0 = (b0 == c0);
    uint8x16_t m1 = (b1 == c0);
    uint8x16_t m2 = (b2 == c0);
    uint8x16_t m3 = (b3 == c0);

    uint64_t mask = Create64BitMask(m0, m1, m2, m3) >> shift;
    if(JUNLIKELY(mask != 0))
    {
        uint8_t* result = (uint8_t*) pIn + __builtin_ctzll(mask);
        return result < pEnd ? result : nullptr;
    }
    
    while(p < pEnd)
    {
        uint8x16_t b0 = *p++;
        uint8x16_t b1 = *p++;
        uint8x16_t b2 = *p++;
        uint8x16_t b3 = *p++;

        uint8x16_t m0 = (b0 == c0);
        uint8x16_t m1 = (b1 == c0);
        uint8x16_t m2 = (b2 == c0);
        uint8x16_t m3 = (b3 == c0);

        m1 |= m0;
        m3 |= m2;
        
        if(JUNLIKELY(IsNotZero(m1|m3)))
        {
            mask = Create64BitMask(m0, m1, m2, m3);
            uint8_t* result = (uint8_t*) p - 64 + __builtin_ctzll(mask);
            return result < pEnd ? result : nullptr;
        }
    }
    
    return nullptr;
#else
    int shift = int(size_t(pIn) & 0x1f);
    const uint8x16_t* p = (const uint8x16_t*) (intptr_t(pIn) & -32);

    uint8x8_t c = vcreate_u8(v);
    const uint8x16_t c0 = vdupq_lane_u8(c, 0);

    uint8x16_t b0 = *p++;
    uint8x16_t b1 = *p++;
    
    uint8x16_t m0 = (b0 == c0);
    uint8x16_t m1 = (b1 == c0);
    
    uint32_t mask = Create32BitMask(m0, m1) >> shift;
    if(JUNLIKELY(mask != 0))
    {
        uint8_t* result = (uint8_t*) pIn + __builtin_ctz(mask);;
        return result < pEnd ? result : nullptr;
    }
    
    while(p < pEnd)
    {
        b0 = *p++;
        b1 = *p++;
        
        m0 = (b0 == c0);
        m1 = (b1 == c0);
        
        if(JUNLIKELY(IsNotZero(m0|m1)))
        {
            mask = Create32BitMask(m0, m1);
            uint8_t* result = (uint8_t*) p - 32 + __builtin_ctz(mask);
            return result < pEnd ? result : nullptr;
        }
    }
    
    return nullptr;
#endif
}

#define FUNCTION_FIND_BYTE_EITHER_OF_2_DEFINED
__attribute__((no_sanitize("address")))
const void* PatternProcessor::FindByteEitherOf2(const void* pIn, uint64_t v, const void* pEnd)
{
    int shift = int(size_t(pIn) & 0x1f);
	const uint8x16_t* p = (const uint8x16_t*) (intptr_t(pIn) & -32);

    uint8x8_t c = vcreate_u8(v);
    const uint8x16_t c0 = vdupq_lane_u8(c, 0);
    const uint8x16_t c1 = vdupq_lane_u8(c, 1);
    
    uint8x16_t b0 = *p++;
    uint8x16_t b1 = *p++;
    __builtin_prefetch(p+8);
    
    uint8x16_t m0 = (b0 == c0) | (b0 == c1);
    uint8x16_t m1 = (b1 == c0) | (b1 == c1);
    
    uint32_t mask = Create32BitMask(m0, m1) >> shift;
    if(JUNLIKELY(mask != 0))
    {
        const uint8_t* result = (const uint8_t*) pIn + __builtin_ctz(mask);;
        return result < pEnd ? result : nullptr;
    }
    
    while(p < pEnd)
    {
        b0 = *p++;
        b1 = *p++;
        __builtin_prefetch(p+8);
        
        m0 = (b0 == c0) | (b0 == c1);
        m1 = (b1 == c0) | (b1 == c1);
        
        if(JUNLIKELY(IsNotZero(m0|m1)))
        {
            mask = Create32BitMask(m0, m1);
            const uint8_t* result = (const uint8_t*) p - 32 + __builtin_ctz(mask);
            return result < pEnd ? result : nullptr;
        }
    }
    
    return nullptr;
}

#define FUNCTION_FIND_BYTE_EITHER_OF_3_DEFINED
const void* PatternProcessor::FindByteEitherOf3(const void* pIn, uint64_t v, const void* pEnd)
{
    uint8_t nm[2][16] = { 0 };
    for(int i = 0; i < 3; ++i)
    {
        nm[0][v & 15] |= (1<<i);
        v >>= 4;
        nm[1][v & 15] |= (1<<i);
        v >>= 4;
    }
    
    return FindNibbleMask(pIn, (uint8x16_t*) nm, pEnd);
}

#define FUNCTION_FIND_BYTE_EITHER_OF_4_DEFINED
const void* PatternProcessor::FindByteEitherOf4(const void* pIn, uint64_t v, const void* pEnd)
{
    uint8x16_t nm[2] = { 0 };
    for(int i = 0; i < 4; ++i)
    {
        nm[0][v & 15] |= (1<<i);
        v >>= 4;
        nm[1][v & 15] |= (1<<i);
        v >>= 4;
    }
    
    return FindNibbleMask(pIn, nm, pEnd);
}

#define FUNCTION_FIND_BYTE_EITHER_OF_5_DEFINED
const void* PatternProcessor::FindByteEitherOf5(const void* pIn, uint64_t v, const void* pEnd)
{
    uint8x16_t nm[2] = { 0 };
    for(int i = 0; i < 5; ++i)
    {
        nm[0][v & 15] |= (1<<i);
        v >>= 4;
        nm[1][v & 15] |= (1<<i);
        v >>= 4;
    }
    
    return FindNibbleMask(pIn, nm, pEnd);
}

#define FUNCTION_FIND_BYTE_EITHER_OF_6_DEFINED
const void* PatternProcessor::FindByteEitherOf6(const void* pIn, uint64_t v, const void* pEnd)
{
    uint8x16_t nm[2] = { 0 };
    for(int i = 0; i < 6; ++i)
    {
        nm[0][v & 15] |= (1<<i);
        v >>= 4;
        nm[1][v & 15] |= (1<<i);
        v >>= 4;
    }
    
    return FindNibbleMask(pIn, nm, pEnd);
}

#define FUNCTION_FIND_BYTE_EITHER_OF_7_DEFINED
const void* PatternProcessor::FindByteEitherOf7(const void* pIn, uint64_t v, const void* pEnd)
{
    uint8x16_t nm[2] = { 0 };
    for(int i = 0; i < 7; ++i)
    {
        nm[0][v & 15] |= (1<<i);
        v >>= 4;
        nm[1][v & 15] |= (1<<i);
        v >>= 4;
    }
    
    return FindNibbleMask(pIn, nm, pEnd);
}

#define FUNCTION_FIND_BYTE_EITHER_OF_8_DEFINED
const void* PatternProcessor::FindByteEitherOf8(const void* pIn, uint64_t v, const void* pEnd)
{
    uint8x16_t nm[2] = { 0 };
    for(int i = 0; i < 8; ++i)
    {
        nm[0][v & 15] |= (1<<i);
        v >>= 4;
        nm[1][v & 15] |= (1<<i);
        v >>= 4;
    }
    
    return FindNibbleMask(pIn, nm, pEnd);
}

#define FUNCTION_FIND_BYTE_PAIR_DEFINED
__attribute__((no_sanitize("address")))
const void* PatternProcessor::FindBytePair(const void* pIn, uint64_t v, const void* pEnd)
{
	int shift = int(size_t(pIn)) & 0x1f;
	uint8x16_t* p = (uint8x16_t*) (intptr_t(pIn) & -32);
#if USE_UNALIGNED_LOADS
    uint8x16_t* pm1 = (uint8x16_t*) (intptr_t(p) + 15);
#endif
    
	uint8x8_t c = vcreate_u8(v);
	const uint8x16_t c0 = vdupq_lane_u8(c, 0);
	const uint8x16_t c1 = vdupq_lane_u8(c, 1);
	uint8x16_t last = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	uint8x16_t b01 = *p++;
	uint8x16_t b11 = *p++;
#if USE_UNALIGNED_LOADS
    uint8x16_t b10 = *pm1++;
    __builtin_prefetch(p+8);
    uint8x16_t b00 = vextq_u8(last, b01, 15);
#else
    __builtin_prefetch(p+8);

    uint8x16_t b00 = vextq_u8(last, b01, 15);
    uint8x16_t b10 = vextq_u8(b01, b11, 15);
#endif

	uint8x16_t y0 = (b01 == c1);
	uint8x16_t y1 = (b11 == c1);
	uint8x16_t x0 = (b00 == c0);
	uint8x16_t x1 = (b10 == c0);

#if !USE_UNALIGNED_LOADS
	last = b11;
#endif
    
	uint8x16_t m0 = x0 & y0;
	uint8x16_t m1 = x1 & y1;

	uint32_t mask = Create32BitMask(m0, m1) >> shift;
	mask >>= 1;
	if(JUNLIKELY(mask != 0))
	{
		uint8_t* result = (uint8_t*) pIn + __builtin_ctz(mask);
		return result < pEnd ? result : nullptr;
	}

	while(p < pEnd)
	{
		b01 = *p++;
		b11 = *p++;
#if USE_UNALIGNED_LOADS
        b00 = *pm1++;
        b10 = *pm1++;
#else
		__builtin_prefetch(p+8);

		b00 = vextq_u8(last, b01, 15);
		b10 = vextq_u8(b01, b11, 15);
#endif
        
        y0 = (b01 == c1);
        y1 = (b11 == c1);
        x0 = (b00 == c0);
        x1 = (b10 == c0);
        
#if !USE_UNALIGNED_LOADS
        last = b11;
#endif
        
        m0 = x0 & y0;
        m1 = x1 & y1;

		if(JUNLIKELY(IsNotZero(m0|m1)))
		{
			mask = Create32BitMask(m0, m1);
			uint8_t* result = (uint8_t*) p - 33 + __builtin_ctz(mask);
			return result < pEnd ? result : nullptr;
		}
	}

	return nullptr;
}

#define FUNCTION_FIND_BYTE_PAIR_2_DEFINED
__attribute__((no_sanitize("address")))
const void* PatternProcessor::FindBytePair2(const void* pIn, uint64_t v, const void* pEnd)
{
    int shift = int(size_t(pIn)) & 0x1f;
    const uint8x16_t* p = (const uint8x16_t*) (intptr_t(pIn) & -32);
    
    uint8x8_t c = vcreate_u8(v);
    const uint8x16_t c0 = vdupq_lane_u8(c, 0);
    const uint8x16_t c1 = vdupq_lane_u8(c, 1);
    const uint8x16_t c2 = vdupq_lane_u8(c, 2);
    const uint8x16_t c3 = vdupq_lane_u8(c, 3);
    uint8x16_t last = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    
    uint8x16_t b01 = *p++;
    uint8x16_t b11 = *p++;
    __builtin_prefetch(p+8);
    
    uint8x16_t b00 = vextq_u8(last, b01, 15);
    uint8x16_t b10 = vextq_u8(b01, b11, 15);
    
    uint8x16_t y0 = (b01 == c2) | (b01 == c3);
    uint8x16_t y1 = (b11 == c2) | (b11 == c3);
    uint8x16_t x0 = (b00 == c0) | (b00 == c1);
    uint8x16_t x1 = (b10 == c0) | (b10 == c1);
    
    last = b11;
    
    uint8x16_t m0 = x0 & y0;
    uint8x16_t m1 = x1 & y1;
    
    uint32_t mask = Create32BitMask(m0, m1) >> shift;
    mask >>= 1;
    if(JUNLIKELY(mask != 0))
    {
        const uint8_t* result = (const uint8_t*) pIn + __builtin_ctz(mask);;
        return result < pEnd ? result : nullptr;
    }
    
    while(p < pEnd)
    {
        b01 = *p++;
        b11 = *p++;
        __builtin_prefetch(p+8);
        
        b00 = vextq_u8(last, b01, 15);
        b10 = vextq_u8(b01, b11, 15);
        
        y0 = (b01 == c2) | (b01 == c3);
        y1 = (b11 == c2) | (b11 == c3);
        x0 = (b00 == c0) | (b00 == c1);
        x1 = (b10 == c0) | (b10 == c1);
        
        last = b11;
        
        m0 = x0 & y0;
        m1 = x1 & y1;
        
        if(JUNLIKELY(IsNotZero(m0|m1)))
        {
            mask = Create32BitMask(m0, m1);
            const uint8_t* result = (const uint8_t*) p - 33 + __builtin_ctz(mask);
            return result < pEnd ? result : nullptr;
        }
    }
    
    return nullptr;
}

#define FUNCTION_FIND_BYTE_PAIR_3_DEFINED
__attribute__((no_sanitize("address")))
const void* PatternProcessor::FindBytePair3(const void* pIn, uint64_t v, const void* pEnd)
{
    int shift = int(size_t(pIn)) & 0x1f;
    const uint8x16_t* p = (const uint8x16_t*) (intptr_t(pIn) & -32);
    
    uint8x8_t c = vcreate_u8(v);
    const uint8x16_t c0 = vdupq_lane_u8(c, 0);
    const uint8x16_t c1 = vdupq_lane_u8(c, 1);
    const uint8x16_t c2 = vdupq_lane_u8(c, 2);
    const uint8x16_t c3 = vdupq_lane_u8(c, 3);
    const uint8x16_t c4 = vdupq_lane_u8(c, 4);
    const uint8x16_t c5 = vdupq_lane_u8(c, 5);
    uint8x16_t last = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    
    uint8x16_t b01 = *p++;
    uint8x16_t b11 = *p++;
    __builtin_prefetch(p+8);
    
    uint8x16_t b00 = vextq_u8(last, b01, 15);
    uint8x16_t b10 = vextq_u8(b01, b11, 15);
    
    uint8x16_t y0 = (b01 == c3) | (b01 == c4) | (b01 == c5);
    uint8x16_t y1 = (b11 == c3) | (b11 == c4) | (b11 == c5);
    uint8x16_t x0 = (b00 == c0) | (b00 == c1) | (b00 == c2);
    uint8x16_t x1 = (b10 == c0) | (b10 == c1) | (b10 == c2);
    
    last = b11;
    
    uint8x16_t m0 = x0 & y0;
    uint8x16_t m1 = x1 & y1;
    
    uint32_t mask = Create32BitMask(m0, m1) >> shift;
    mask >>= 1;
    if(JUNLIKELY(mask != 0))
    {
        const uint8_t* result = (const uint8_t*) pIn + __builtin_ctz(mask);;
        return result < pEnd ? result : nullptr;
    }
    
    while(p < pEnd)
    {
        b01 = *p++;
        b11 = *p++;
        __builtin_prefetch(p+8);
        
        b00 = vextq_u8(last, b01, 15);
        b10 = vextq_u8(b01, b11, 15);
        
        y0 = (b01 == c3) | (b01 == c4) | (b01 == c5);
        y1 = (b11 == c3) | (b11 == c4) | (b11 == c5);
        x0 = (b00 == c0) | (b00 == c1) | (b00 == c2);
        x1 = (b10 == c0) | (b10 == c1) | (b10 == c2);
        
        last = b11;
        
        m0 = x0 & y0;
        m1 = x1 & y1;
        
        if(JUNLIKELY(IsNotZero(m0|m1)))
        {
            mask = Create32BitMask(m0, m1);
            const uint8_t* result = (const uint8_t*) p - 33 + __builtin_ctz(mask);
            return result < pEnd ? result : nullptr;
        }
    }
    
    return nullptr;
}

#define FUNCTION_FIND_BYTE_PAIR_4_DEFINED
__attribute__((no_sanitize("address")))
const void* PatternProcessor::FindBytePair4(const void* pIn, uint64_t v, const void* pEnd)
{
    int shift = int(size_t(pIn)) & 0x1f;
    const uint8x16_t* p = (const uint8x16_t*) (intptr_t(pIn) & -32);
    
    uint8x8_t c = vcreate_u8(v);
    const uint8x16_t c0 = vdupq_lane_u8(c, 0);
    const uint8x16_t c1 = vdupq_lane_u8(c, 1);
    const uint8x16_t c2 = vdupq_lane_u8(c, 2);
    const uint8x16_t c3 = vdupq_lane_u8(c, 3);
    const uint8x16_t c4 = vdupq_lane_u8(c, 4);
    const uint8x16_t c5 = vdupq_lane_u8(c, 5);
    const uint8x16_t c6 = vdupq_lane_u8(c, 6);
    const uint8x16_t c7 = vdupq_lane_u8(c, 7);
    uint8x16_t last = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    
    uint8x16_t b01 = *p++;
    uint8x16_t b11 = *p++;
    __builtin_prefetch(p+8);
    
    uint8x16_t b00 = vextq_u8(last, b01, 15);
    uint8x16_t b10 = vextq_u8(b01, b11, 15);
    
    uint8x16_t y0 = (b01 == c4) | (b01 == c5) | (b01 == c6) | (b01 == c7);
    uint8x16_t y1 = (b11 == c4) | (b11 == c5) | (b11 == c6) | (b11 == c7);
    uint8x16_t x0 = (b00 == c0) | (b00 == c1) | (b00 == c2) | (b00 == c3);
    uint8x16_t x1 = (b10 == c0) | (b10 == c1) | (b10 == c2) | (b10 == c3);
    
    last = b11;
    
    uint8x16_t m0 = x0 & y0;
    uint8x16_t m1 = x1 & y1;
    
    uint32_t mask = Create32BitMask(m0, m1) >> shift;
    mask >>= 1;
    if(JUNLIKELY(mask != 0))
    {
        const uint8_t* result = (const uint8_t*) pIn + __builtin_ctz(mask);;
        return result < pEnd ? result : nullptr;
    }
    
    while(p < pEnd)
    {
        b01 = *p++;
        b11 = *p++;
        __builtin_prefetch(p+8);
        
        b00 = vextq_u8(last, b01, 15);
        b10 = vextq_u8(b01, b11, 15);
        
        y0 = (b01 == c4) | (b01 == c5) | (b01 == c6) | (b01 == c7);
        y1 = (b11 == c4) | (b11 == c5) | (b11 == c6) | (b11 == c7);
        x0 = (b00 == c0) | (b00 == c1) | (b00 == c2) | (b00 == c3);
        x1 = (b10 == c0) | (b10 == c1) | (b10 == c2) | (b10 == c3);
        
        last = b11;
        
        m0 = x0 & y0;
        m1 = x1 & y1;
        
        if(JUNLIKELY(IsNotZero(m0|m1)))
        {
            mask = Create32BitMask(m0, m1);
            const uint8_t* result = (const uint8_t*) p - 33 + __builtin_ctz(mask);
            return result < pEnd ? result : nullptr;
        }
    }
    
    return nullptr;
}

#define FUNCTION_FIND_BYTE_RANGE_DEFINED
__attribute__((no_sanitize("address")))
const void* PatternProcessor::FindByteRange(const void* pIn, uint64_t v, const void* pEnd)
{
    int shift = int(size_t(pIn) & 0x1f);
    const uint8x16_t* p = (const uint8x16_t*) (intptr_t(pIn) & -32);
    
    uint8x8_t c = vcreate_u8(v);
    uint8x16_t c0 = vdupq_lane_u8(c, 0);
    uint8x16_t c1 = vdupq_lane_u8(c, 1);
	c1 -= c0;
    
    uint8x16_t b0 = *p++;
    uint8x16_t b1 = *p++;
    
    uint8x16_t m0 = (b0 - c0) <= c1;
    uint8x16_t m1 = (b1 - c0) <= c1;
    
    uint32_t mask = Create32BitMask(m0, m1) >> shift;
    if(JUNLIKELY(mask != 0))
    {
        const uint8_t* result = (const uint8_t*) pIn + __builtin_ctz(mask);
        return result < pEnd ? result : nullptr;
    }
    
    while(p < pEnd)
    {
        b0 = *p++;
        b1 = *p++;
        
        uint8x16_t m0 = (b0 - c0) <= c1;
        uint8x16_t m1 = (b1 - c0) <= c1;
        
        if(JUNLIKELY(IsNotZero(m0|m1)))
        {
            mask = Create32BitMask(m0, m1);
            const uint8_t* result = (const uint8_t*) p - 32 + __builtin_ctz(mask);
            return result < pEnd ? result : nullptr;
        }
    }
    
    return nullptr;
}

#define FUNCTION_FIND_BYTE_RANGE_PAIR_DEFINED
__attribute__((no_sanitize("address")))
const void* PatternProcessor::FindByteRangePair(const void* pIn, uint64_t v, const void* pEnd)
{
    int shift = int(size_t(pIn)) & 0x1f;
    const uint8x16_t* p = (const uint8x16_t*) (intptr_t(pIn) & -32);
    
    uint8x8_t c = vcreate_u8(v);
    uint8x16_t c0 = vdupq_lane_u8(c, 0);
    uint8x16_t c1 = vdupq_lane_u8(c, 1);
    uint8x16_t c2 = vdupq_lane_u8(c, 2);
    uint8x16_t c3 = vdupq_lane_u8(c, 3);
    uint8x16_t last = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	
	c1 -= c0;
	c3 -= c2;
	
    uint8x16_t b01 = *p++;
    uint8x16_t b11 = *p++;
    __builtin_prefetch(p+8);
    
    uint8x16_t b00 = vextq_u8(last, b01, 15);
    uint8x16_t b10 = vextq_u8(b01, b11, 15);
    
    uint8x16_t y0 = (b01 - c2) <= c3;
    uint8x16_t y1 = (b11 - c2) <= c3;
    uint8x16_t x0 = (b00 - c0) <= c1;
    uint8x16_t x1 = (b10 - c0) <= c1;
    
    last = b11;
    
    uint8x16_t m0 = x0 & y0;
    uint8x16_t m1 = x1 & y1;
    
    uint32_t mask = Create32BitMask(m0, m1) >> shift;
    mask >>= 1;
    if(JUNLIKELY(mask != 0))
    {
        const uint8_t* result = (const uint8_t*) pIn + __builtin_ctz(mask);
        return result < pEnd ? result : nullptr;
    }
    
    while(p < pEnd)
    {
        b01 = *p++;
        b11 = *p++;
        __builtin_prefetch(p+8);
        
        b00 = vextq_u8(last, b01, 15);
        b10 = vextq_u8(b01, b11, 15);
        
        y0 = (b01 - c2) <= c3;
        y1 = (b11 - c2) <= c3;
        x0 = (b00 - c0) <= c1;
        x1 = (b10 - c0) <= c1;
        
        last = b11;
        
        m0 = x0 & y0;
        m1 = x1 & y1;
        
        if(JUNLIKELY(IsNotZero(m0|m1)))
        {
            mask = Create32BitMask(m0, m1);
            const uint8_t* result = (const uint8_t*) p - 33 + __builtin_ctz(mask);
            return result < pEnd ? result : nullptr;
        }
    }
    
    return nullptr;
}

//#define FUNCTION_FIND_SHIFT_OR_DEFINED
//__attribute__((naked)) const void* PatternProcessor::FindShiftOr(const void* p, const ByteCodeSearchData* searchData, const void* pEnd)
//{
//}
//
//#define FUNCTION_FIND_BYTE_TRIPLET_DEFINED
//__attribute__((naked)) const void* PatternProcessor::FindByteTriplet(const void* p, uint64_t v, const void* pEnd)
//{
//}
//
//#define FUNCTION_FIND_BYTE_TRIPLET_2_DEFINED
//__attribute__((naked)) const void* PatternProcessor::FindByteTriplet2(const void* p, uint64_t v, const void* pEnd)
//{
//}
//
//============================================================================

#undef USE_UNALIGNED_LOADS
