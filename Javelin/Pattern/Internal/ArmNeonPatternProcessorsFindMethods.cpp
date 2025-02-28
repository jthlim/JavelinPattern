//============================================================================

#include "Javelin/Pattern/Internal/ArmNeonPatternProcessorsFindMethods.h"
#if defined(JASM_GNUC_ARM_NEON) || defined(JASM_GNUC_ARM64)

//============================================================================

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

JINLINE static uint8x16_t LoadNibbleMask(const void* data, size_t index)
{
	return vld1q_u8((const unsigned char*) data + index*16);
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

//============================================================================

__attribute__((no_sanitize("address")))
const void* Javelin::PatternInternal::ArmNeonFindMethods::InternalFindBytePairPath2(const void* pIn, uint32_t v, const void* pEnd)
{
	int shift = int(size_t(pIn)) & 0x1f;
	const uint8x16_t* p = (const uint8x16_t*) (intptr_t(pIn) & -32);

	uint8x8_t c = vcreate_u8(v);
	const uint8x16_t c00 = vdupq_lane_u8(c, 0);
	const uint8x16_t c10 = vdupq_lane_u8(c, 1);
	const uint8x16_t c01 = vdupq_lane_u8(c, 2);
	const uint8x16_t c11 = vdupq_lane_u8(c, 3);
	uint8x16_t last = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	uint8x16_t b01 = *p++;
	uint8x16_t b11 = *p++;
	__builtin_prefetch(p+8);

	uint8x16_t b00 = vextq_u8(last, b01, 15);
	uint8x16_t b10 = vextq_u8(b01, b11, 15);

	uint8x16_t x01 = (b01 == c01);
	uint8x16_t y01 = (b01 == c11);
	uint8x16_t x11 = (b11 == c01);
	uint8x16_t y11 = (b11 == c11);
	uint8x16_t x00 = (b00 == c00);
	uint8x16_t y00 = (b00 == c10);
	uint8x16_t x10 = (b10 == c00);
	uint8x16_t y10 = (b10 == c10);

	last = b11;

	uint8x16_t m00 = x00 & x01;
	uint8x16_t m10 = y00 & y01;
	uint8x16_t m01 = x10 & x11;
	uint8x16_t m11 = y10 & y11;

	uint32_t mask = Create32BitMask(m00|m10, m01|m11) >> shift;
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

		x01 = (b01 == c01);
		y01 = (b01 == c11);
		x11 = (b11 == c01);
		y11 = (b11 == c11);
		x00 = (b00 == c00);
		y00 = (b00 == c10);
		x10 = (b10 == c00);
		y10 = (b10 == c10);

		last = b11;

		m00 = x00 & x01;
		m10 = y00 & y01;
		m01 = x10 & x11;
		m11 = y10 & y11;

		uint8x16_t m0 = m00 | m10;
		uint8x16_t m1 = m01 | m11;

		if(JUNLIKELY(IsNotZero(m0|m1)))
		{
			mask = Create32BitMask(m0, m1);
			const uint8_t* result = (const uint8_t*) p - 33 + __builtin_ctz(mask);
			return result < pEnd ? result : nullptr;
		}
	}

	return nullptr;
}

__attribute__((no_sanitize("address")))
const void* Javelin::PatternInternal::ArmNeonFindMethods::InternalFindNibbleMask(const void* pIn, const uint8x16_t* nibbleMasks, const void* pEnd)
{
    int shift = int(size_t(pIn) & 0x1f);
    const uint8x16_t* p = (const uint8x16_t*) (intptr_t(pIn) & -32);
    
	const uint8x16_t nm0 = LoadNibbleMask(nibbleMasks, 0);
	const uint8x16_t nm1 = LoadNibbleMask(nibbleMasks, 1);
    const uint8x16_t AND_MASK = {0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf};
    
    uint8x16_t b0 = *p++;
    uint8x16_t b1 = *p++;
    __builtin_prefetch(p+8);
    uint8x16_t b0h = vshrq_n_u8(b0, 4);
    uint8x16_t b0l = b0 & AND_MASK;
    uint8x16_t b1h = vshrq_n_u8(b1, 4);
    uint8x16_t b1l = b1 & AND_MASK;
    
    b0 = vtstq_u8(vtblq(nm0, b0l), vtblq(nm1, b0h));
    b1 = vtstq_u8(vtblq(nm0, b1l), vtblq(nm1, b1h));
	
    uint32_t mask = Create32BitMask(b0, b1) >> shift;
    if(JUNLIKELY(mask != 0))
    {
        const uint8_t* result = (const uint8_t*) pIn + __builtin_ctz(mask);
        return result < pEnd ? result : nullptr;
    }
    
    while(p < pEnd)
    {
        b0 = *p++;
        b1 = *p++;
        __builtin_prefetch(p+8);
        b0h = vshrq_n_u8(b0, 4);
        b0l = b0 & AND_MASK;
        b1h = vshrq_n_u8(b1, 4);
        b1l = b1 & AND_MASK;
        
        b0 = vtstq_u8(vtblq(nm0, b0l), vtblq(nm1, b0h));
        b1 = vtstq_u8(vtblq(nm0, b1l), vtblq(nm1, b1h));
        
        if(JUNLIKELY(IsNotZero(b0|b1)))
        {
            mask = Create32BitMask(b0, b1);
            const uint8_t* result = (const uint8_t*) p - 32 + __builtin_ctz(mask);
            return result < pEnd ? result : nullptr;
        }
    }
    
    return nullptr;
}

__attribute__((no_sanitize("address")))
const void* Javelin::PatternInternal::ArmNeonFindMethods::InternalFindPairNibbleMask(const void* pIn, const uint8x16_t* nibbleMasks, const void* pEnd)
{
    int shift = int(size_t(pIn) & 0x1f);
	const uint8x16_t* p = (const uint8x16_t*) (intptr_t(pIn) & -32);
	
	const uint8x16_t nm0 = LoadNibbleMask(nibbleMasks, 0);
	const uint8x16_t nm1 = LoadNibbleMask(nibbleMasks, 1);
	const uint8x16_t nm2 = LoadNibbleMask(nibbleMasks, 2);
	const uint8x16_t nm3 = LoadNibbleMask(nibbleMasks, 3);
	const uint8x16_t AND_MASK = {0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf};
	uint8x16_t last = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	
	uint8x16_t b0 = *p++;
	uint8x16_t b1 = *p++;
	__builtin_prefetch(p+8);
	uint8x16_t b0h = vshrq_n_u8(b0, 4);
	uint8x16_t b0l = b0 & AND_MASK;
	uint8x16_t b1h = vshrq_n_u8(b1, 4);
	uint8x16_t b1l = b1 & AND_MASK;

    uint8x16_t x0 = vtstq_u8(vtblq(nm0, b0l), vtblq(nm1, b0h));
	uint8x16_t x1 = vtstq_u8(vtblq(nm0, b1l), vtblq(nm1, b1h));

	uint8x16_t y0 = vtstq_u8(vtblq(nm2, b0l), vtblq(nm3, b0h));
	uint8x16_t y1 = vtstq_u8(vtblq(nm2, b1l), vtblq(nm3, b1h));

	uint8x16_t m0 = vextq_u8(last, x0, 15) & y0;
	uint8x16_t m1 = vextq_u8(x0, x1, 15) & y1;
	last = x1;
	
	uint32_t mask = Create32BitMask(m0, m1) >> shift;
	mask >>= 1;
	if(JUNLIKELY(mask != 0))
	{
		const uint8_t* result = (const uint8_t*) pIn + __builtin_ctz(mask);
		return result < pEnd ? result : nullptr;
	}
	
	while(p < pEnd)
	{
		b0 = *p++;
		b1 = *p++;
		__builtin_prefetch(p+8);
		b0h = vshrq_n_u8(b0, 4);
		b0l = b0 & AND_MASK;
		b1h = vshrq_n_u8(b1, 4);
		b1l = b1 & AND_MASK;
		
        x0 = vtstq_u8(vtblq(nm0, b0l), vtblq(nm1, b0h));
        x1 = vtstq_u8(vtblq(nm0, b1l), vtblq(nm1, b1h));
        
        y0 = vtstq_u8(vtblq(nm2, b0l), vtblq(nm3, b0h));
        y1 = vtstq_u8(vtblq(nm2, b1l), vtblq(nm3, b1h));
		
		m0 = vextq_u8(last, x0, 15) & y0;
		m1 = vextq_u8(x0, x1, 15) & y1;
		last = x1;
		
		if(JUNLIKELY(IsNotZero(m0|m1)))
		{
			mask = Create32BitMask(m0, m1);
			const uint8_t* result = (const uint8_t*) p - 33 + __builtin_ctz(mask);
			return result < pEnd ? result : nullptr;
		}
	}
	
	return nullptr;
}

__attribute__((no_sanitize("address")))
const void* Javelin::PatternInternal::ArmNeonFindMethods::InternalFindPairNibbleMaskPath(const void* pIn, const uint8x16_t* nibbleMasks, const void* pEnd)
{
    int shift = int(size_t(pIn) & 0x1f);
	const uint8x16_t* p = (const uint8x16_t*) (intptr_t(pIn) & -32);
	
	const uint8x16_t nm0 = LoadNibbleMask(nibbleMasks, 0);
	const uint8x16_t nm1 = LoadNibbleMask(nibbleMasks, 1);
	const uint8x16_t nm2 = LoadNibbleMask(nibbleMasks, 2);
	const uint8x16_t nm3 = LoadNibbleMask(nibbleMasks, 3);
	const uint8x16_t AND_MASK = {0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf};
	uint8x16_t last = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	
	uint8x16_t b0 = *p++;
	uint8x16_t b1 = *p++;
	__builtin_prefetch(p+8);
	uint8x16_t b0h = vshrq_n_u8(b0, 4);
	uint8x16_t b0l = b0 & AND_MASK;
	uint8x16_t b1h = vshrq_n_u8(b1, 4);
	uint8x16_t b1l = b1 & AND_MASK;
	
    uint8x16_t x0 = vtblq(nm0, b0l) & vtblq(nm1, b0h);
	uint8x16_t x1 = vtblq(nm0, b1l) & vtblq(nm1, b1h);
	
	uint8x16_t y0 = vtblq(nm2, b0l) & vtblq(nm3, b0h);
    uint8x16_t y1 = vtblq(nm2, b1l) & vtblq(nm3, b1h);
	
	uint8x16_t mm0 = vtstq_u8(vextq_u8(last, x0, 15), y0);
	uint8x16_t mm1 = vtstq_u8(vextq_u8(x0, x1, 15), y1);
	last = x1;
	
	uint32_t mask = Create32BitMask(mm0, mm1) >> shift;
	mask >>= 1;
	if(JUNLIKELY(mask != 0))
	{
		const uint8_t* result = (const uint8_t*) pIn + __builtin_ctz(mask);
		return result < pEnd ? result : nullptr;
	}
	
	while(p < pEnd)
	{
		b0 = *p++;
		b1 = *p++;
		__builtin_prefetch(p+8);
		b0h = vshrq_n_u8(b0, 4);
		b0l = b0 & AND_MASK;
		b1h = vshrq_n_u8(b1, 4);
		b1l = b1 & AND_MASK;
		
        x0 = vtblq(nm0, b0l) & vtblq(nm1, b0h);
        x1 = vtblq(nm0, b1l) & vtblq(nm1, b1h);
        
        y0 = vtblq(nm2, b0l) & vtblq(nm3, b0h);
        y1 = vtblq(nm2, b1l) & vtblq(nm3, b1h);
		
		mm0 = vtstq_u8(vextq_u8(last, x0, 15), y0);
		mm1 = vtstq_u8(vextq_u8(x0, x1, 15), y1);
		last = x1;
		
		if(JUNLIKELY(IsNotZero(mm0|mm1)))
		{
			mask = Create32BitMask(mm0, mm1);
			const uint8_t* result = (const uint8_t*) p - 33 + __builtin_ctz(mask);
			return result < pEnd ? result : nullptr;
		}
	}
	
	return nullptr;
}

__attribute__((no_sanitize("address")))
const void* Javelin::PatternInternal::ArmNeonFindMethods::InternalFindTripletNibbleMask(const void* pIn, const uint8x16_t* nibbleMasks, const void* pEnd)
{
    int shift = int(size_t(pIn) & 0x1f);
    const uint8x16_t* p = (const uint8x16_t*) (intptr_t(pIn) & -32);
    
    const uint8x16_t nm0 = LoadNibbleMask(nibbleMasks, 0);
    const uint8x16_t nm1 = LoadNibbleMask(nibbleMasks, 1);
    const uint8x16_t nm2 = LoadNibbleMask(nibbleMasks, 2);
    const uint8x16_t nm3 = LoadNibbleMask(nibbleMasks, 3);
    const uint8x16_t nm4 = LoadNibbleMask(nibbleMasks, 4);
    const uint8x16_t nm5 = LoadNibbleMask(nibbleMasks, 5);
    const uint8x16_t AND_MASK = {0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf};
    uint8x16_t lastX = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    uint8x16_t lastY = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    
    uint8x16_t b0 = *p++;
    uint8x16_t b1 = *p++;
    __builtin_prefetch(p+8);
    
    uint8x16_t b0h = vshrq_n_u8(b0, 4);
    uint8x16_t b0l = b0 & AND_MASK;
    uint8x16_t b1h = vshrq_n_u8(b1, 4);
    uint8x16_t b1l = b1 & AND_MASK;

    uint8x16_t x0 = vtstq_u8(vtblq(nm0, b0l), vtblq(nm1, b0h));
    uint8x16_t x1 = vtstq_u8(vtblq(nm0, b1l), vtblq(nm1, b1h));

    uint8x16_t y0 = vtstq_u8(vtblq(nm2, b0l), vtblq(nm3, b0h));
    uint8x16_t y1 = vtstq_u8(vtblq(nm2, b1l), vtblq(nm3, b1h));

    uint8x16_t z0 = vtstq_u8(vtblq(nm4, b0l), vtblq(nm5, b0h));
    uint8x16_t z1 = vtstq_u8(vtblq(nm4, b1l), vtblq(nm5, b1h));

    uint8x16_t m0 = vextq_u8(lastX, x0, 14) & vextq_u8(lastY, y0, 15) & z0;
    uint8x16_t m1 = vextq_u8(x0, x1, 14) & vextq_u8(y0, y1, 15) & z1;

    lastX = x1;
    lastY = y1;

    uint32_t mask = Create32BitMask(m0, m1) >> shift;
    mask >>= 2;
    if(JUNLIKELY(mask != 0))
    {
        const uint8_t* result = (const uint8_t*) pIn + __builtin_ctz(mask);
        return result < pEnd ? result : nullptr;
    }
    
    while(p < pEnd)
    {
        b0 = *p++;
        b1 = *p++;
        __builtin_prefetch(p+8);
        b0h = vshrq_n_u8(b0, 4);
        b0l = b0 & AND_MASK;
        b1h = vshrq_n_u8(b1, 4);
        b1l = b1 & AND_MASK;
        
        x0 = vtstq_u8(vtblq(nm0, b0l), vtblq(nm1, b0h));
        x1 = vtstq_u8(vtblq(nm0, b1l), vtblq(nm1, b1h));
        
        y0 = vtstq_u8(vtblq(nm2, b0l), vtblq(nm3, b0h));
        y1 = vtstq_u8(vtblq(nm2, b1l), vtblq(nm3, b1h));

        z0 = vtstq_u8(vtblq(nm4, b0l), vtblq(nm5, b0h));
        z1 = vtstq_u8(vtblq(nm4, b1l), vtblq(nm5, b1h));

        m0 = vextq_u8(lastX, x0, 14) & vextq_u8(lastY, y0, 15) & y0;
        m1 = vextq_u8(x0, x1, 14) & vextq_u8(y0, y1, 15) & z1;
        
        lastX = x1;
        lastY = y1;

        if(JUNLIKELY(IsNotZero(m0|m1)))
        {
            mask = Create32BitMask(m0, m1);
            const uint8_t* result = (const uint8_t*) p - 34 + __builtin_ctz(mask);
            return result < pEnd ? result : nullptr;
        }
    }
    
    return nullptr;
}

__attribute__((no_sanitize("address")))
const void* Javelin::PatternInternal::ArmNeonFindMethods::InternalFindTripletNibbleMaskPath(const void* pIn, const uint8x16_t* nibbleMasks, const void* pEnd)
{
    int shift = int(size_t(pIn) & 0x1f);
    const uint8x16_t* p = (const uint8x16_t*) (intptr_t(pIn) & -32);
    
    const uint8x16_t nm0 = LoadNibbleMask(nibbleMasks, 0);
    const uint8x16_t nm1 = LoadNibbleMask(nibbleMasks, 1);
    const uint8x16_t nm2 = LoadNibbleMask(nibbleMasks, 2);
    const uint8x16_t nm3 = LoadNibbleMask(nibbleMasks, 3);
    const uint8x16_t nm4 = LoadNibbleMask(nibbleMasks, 4);
    const uint8x16_t nm5 = LoadNibbleMask(nibbleMasks, 5);
    const uint8x16_t AND_MASK = {0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf};
    uint8x16_t lastX = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    uint8x16_t lastY = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    uint8x16_t b0 = *p++;
    uint8x16_t b1 = *p++;
    __builtin_prefetch(p+8);
    uint8x16_t b0h = vshrq_n_u8(b0, 4);
    uint8x16_t b0l = b0 & AND_MASK;
    uint8x16_t b1h = vshrq_n_u8(b1, 4);
    uint8x16_t b1l = b1 & AND_MASK;
    
    uint8x16_t x0 = vtblq(nm0, b0l) & vtblq(nm1, b0h);
    uint8x16_t x1 = vtblq(nm0, b1l) & vtblq(nm1, b1h);
    
    uint8x16_t y0 = vtblq(nm2, b0l) & vtblq(nm3, b0h);
    uint8x16_t y1 = vtblq(nm2, b1l) & vtblq(nm3, b1h);
    
    uint8x16_t z0 = vtblq(nm4, b0l) & vtblq(nm5, b0h);
    uint8x16_t z1 = vtblq(nm4, b1l) & vtblq(nm5, b1h);
    
    uint8x16_t mm0 = vtstq_u8(vextq_u8(lastX, x0, 14) & vextq_u8(lastY, y0, 15), z0);
    uint8x16_t mm1 = vtstq_u8(vextq_u8(x0, x1, 14) & vextq_u8(y0, y1, 15), z1);
    
    lastX = x1;
    lastY = y1;

    uint32_t mask = Create32BitMask(mm0, mm1) >> shift;
    mask >>= 2;
    if(JUNLIKELY(mask != 0))
    {
        const uint8_t* result = (const uint8_t*) pIn + __builtin_ctz(mask);
        return result < pEnd ? result : nullptr;
    }
    
    while(p < pEnd)
    {
        b0 = *p++;
        b1 = *p++;
        __builtin_prefetch(p+8);
        b0h = vshrq_n_u8(b0, 4);
        b0l = b0 & AND_MASK;
        b1h = vshrq_n_u8(b1, 4);
        b1l = b1 & AND_MASK;
        
        x0 = vtblq(nm0, b0l) & vtblq(nm1, b0h);
        x1 = vtblq(nm0, b1l) & vtblq(nm1, b1h);
        
        y0 = vtblq(nm2, b0l) & vtblq(nm3, b0h);
        y1 = vtblq(nm2, b1l) & vtblq(nm3, b1h);

        z0 = vtblq(nm4, b0l) & vtblq(nm5, b0h);
        z1 = vtblq(nm4, b1l) & vtblq(nm5, b1h);

        mm0 = vtstq_u8(vextq_u8(lastX, x0, 14) & vextq_u8(lastY, y0, 15), z0);
        mm1 = vtstq_u8(vextq_u8(x0, x1, 14) & vextq_u8(y0, y1, 15), z1);

        lastX = x1;
        lastY = y1;

        if(JUNLIKELY(IsNotZero(mm0|mm1)))
        {
            mask = Create32BitMask(mm0, mm1);
            const uint8_t* result = (const uint8_t*) p - 34 + __builtin_ctz(mask);
            return result < pEnd ? result : nullptr;
        }
    }
    
    return nullptr;
}


//============================================================================
#endif
//============================================================================
