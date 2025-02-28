//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

namespace Javelin
{
//============================================================================

	namespace BitUtility
	{
		JINLINE bool				IsPowerOf2(unsigned int i)	{ return (i & (i-1) ) == 0; }
		JEXPORT unsigned int		CountBits(unsigned int i);
		JEXPORT unsigned int		CountBits(unsigned int* data, size_t numberOfWords);
		JEXPORT unsigned int		RoundUpToPowerOf2(unsigned int u);
		JEXPORT unsigned long long	RoundUpToPowerOf2(unsigned long long u);

		namespace Private
		{
			template<int n> struct RoundUpHelper;
			template<> struct RoundUpHelper<4> { JINLINE static unsigned Run(unsigned u) { return RoundUpToPowerOf2(u); } };
			template<> struct RoundUpHelper<8> { JINLINE static unsigned long long Run(unsigned long long u) { return RoundUpToPowerOf2(u); } };
		}
		JINLINE unsigned long RoundUpToPowerOf2(unsigned long u) { return Private::RoundUpHelper<sizeof(unsigned long)>::Run(u); }

		JINLINE int DetermineHighestNonZeroBit32(unsigned int value);
		JINLINE int DetermineHighestNonZeroBit64(unsigned long long value);

		JINLINE uint32_t RotateLeft(uint32_t a, int s);
		JINLINE uint32_t RotateRight(uint32_t a, int s);
		JINLINE uint64_t RotateLeft(uint64_t a, int s);
		JINLINE uint64_t RotateRight(uint64_t a, int s);

		JINLINE void ClearBit(unsigned* data, size_t bitIndex) 			{ size_t wordIndex = bitIndex >> 5; int bitOffset = bitIndex & 31; data[wordIndex] &= ~(1<<bitOffset); }
		JINLINE void SetBit(unsigned* data, size_t bitIndex)	 		{ size_t wordIndex = bitIndex >> 5; int bitOffset = bitIndex & 31; data[wordIndex] |= (1<<bitOffset); }
		JINLINE bool IsBitSet(const unsigned* data, size_t bitIndex) 	{ size_t wordIndex = bitIndex >> 5; int bitOffset = bitIndex & 31; return (data[wordIndex] & (1<<bitOffset)) != 0; }

		class BitProxy
		{
		public:
			BitProxy(unsigned *aData, size_t aBitIndex) : data(aData), bitIndex(aBitIndex) { }
			JINLINE void operator=(bool b) 	{ if(b) SetBit(data, bitIndex); else ClearBit(data, bitIndex); }
			
			JINLINE operator bool() const 	{ return IsBitSet(data, bitIndex); }
			
		private:
			unsigned*		data;
			size_t			bitIndex;
		};
	};

//============================================================================
	
#if defined(JCOMPILER_MSVC)
	#define FUNCTION_BITUTILITY_DETERMINEHIGHESTNONZEROBIT32_DEFINED
	JINLINE int BitUtility::DetermineHighestNonZeroBit32(unsigned value)
	{
		unsigned long result;
		if(!_BitScanReverse(&result, value)) return -1;
		return result;
	}
	
#elif defined(JASM_GNUC_X86) || defined(JASM_GNUC_X86_64)
	// This is platform specific, as x86 has undefined return values for value == 0
	#define FUNCTION_BITUTILITY_DETERMINEHIGHESTNONZEROBIT32_DEFINED
	JINLINE int BitUtility::DetermineHighestNonZeroBit32(unsigned value)
	{
		int result;
		asm("bsrl\t%1, %0\n\t"
			"cmovel\t%2, %0"
			: "=r" (result)
			: "rm" (value), "rm"(-1)
			);
		return result;
	}
	
	#if defined(JASM_GNUC_X86_64)
		#define FUNCTION_BITUTILITY_DETERMINEHIGHESTNONZEROBIT64_DEFINED
		JINLINE int BitUtility::DetermineHighestNonZeroBit64(unsigned long long value)
		{
			long long result;
			asm("bsrq\t%1, %0\n\t"
				"cmoveq\t%2, %0"
				: "=r" (result)
				: "rm" (value), "rm"(-1ll)
				);
			return (int) result;
		}
	#endif
#elif defined(JCOMPILER_GCC) || defined(JCOMPILER_CLANG)
	// This is platform specific, as x86 has undefined return values for value == 0
	#define FUNCTION_BITUTILITY_DETERMINEHIGHESTNONZEROBIT32_DEFINED
	JINLINE int BitUtility::DetermineHighestNonZeroBit32(unsigned value)
	{
		return 31-__builtin_clz(value);
	}
#endif
	
#if !defined(FUNCTION_BITUTILITY_DETERMINEHIGHESTNONZEROBIT32_DEFINED)
	#warning "No assembler specific DetermineHighestNonZeroBit32 defined"
	JINLINE int BitUtility::DetermineHighestNonZeroBit32(unsigned value)
	{
		for(int i = sizeof(unsigned)-1; i >= 0; --i)
		{
			if(value & (1<<i)) return i;
		}
		return -1;
	}
#endif
	
#if !defined(FUNCTION_BITUTILITY_DETERMINEHIGHESTNONZEROBIT64_DEFINED)
	JINLINE int BitUtility::DetermineHighestNonZeroBit64(unsigned long long value)
	{
		if((value >> 32) == 0) return DetermineHighestNonZeroBit32((unsigned) value);
		else return 32 + DetermineHighestNonZeroBit32((unsigned) (value >> 32));
	}
#endif
	
//============================================================================

#if defined(JCOMPILER_MSVC)
	#define FUNCTION_BITWISE_ROTATIONS_DEFINED
	JINLINE uint32_t BitUtility::RotateLeft(uint32_t a, int s)	{ return _rotl(a, s);	}
	JINLINE uint32_t BitUtility::RotateRight(uint32_t a, int s)	{ return _rotr(a, s);	}
	JINLINE uint64_t BitUtility::RotateLeft(uint64_t a, int s)	{ return _rotl64(a, s);	}
	JINLINE uint64_t BitUtility::RotateRight(uint64_t a, int s)	{ return _rotr64(a, s);	}
	
	// GCC is now good enough to detect rotations and automatically encodes it for us!
//#elif defined(JASM_GNUC_X86)
//	#define FUNCTION_BITWISE_ROTATIONS_DEFINED
//	 JINLINE uint32_t BitUtility::RotateLeft(uint32_t value, int s)
//	 {
//		 asm("rol %1, %0" : "+r" (value) : "rI" (s) );
//		 return value;
//	 }
// 
//	 JINLINE uint32_t BitUtility::RotateRight(uint32_t value, int s)
//	 {
//		 asm("ror %1, %0" : "+r" (value) : "rI" (s));
//		 return value;
//	 }
//	
//	JINLINE uint64_t BitUtility::RotateLeft(uint64_t value, int s)
//	{
//		asm("roll %1, %0" : "+r" (value) : "rI" (s) );
//		return value;
//	}
// 
//	JINLINE uint64_t BitUtility::RotateRight(uint64_t value, int s)
//	{
//		asm("rorl %1, %0" : "+r" (value) : "rI" (s));
//		return value;
//	}

//#elif defined(JASM_GNUC_PPC)
//	#define FUNCTION_BITWISE_ROTATIONS_DEFINED
//	 JINLINE uint32_t BitUtility::RotateLeft(uint32_t value, int s)
//	 {
//		 register int returnValue;
//		 asm("rotlw%I2 %0, %1, %2" : "=r" (returnValue) : "r" (value), "rI" (s));
//		 return returnValue;
//	 }
//	 JINLINE uint32_t BitUtility::RotateRight(uint32_t value, int s)
//	 {
//		 register int returnValue;
//		 asm("rotlw%I2 %0, %1, %2" : "=r" (returnValue) : "r" (value), "rI" (32-s));
//		 return returnValue;
//	 }
#endif

#if !defined(FUNCTION_BITWISE_ROTATIONS_DEFINED)
	JINLINE uint32_t BitUtility::RotateLeft(uint32_t a, int s)	{ return a << (s&31) | a >> (32-(s&31));	}
	JINLINE uint32_t BitUtility::RotateRight(uint32_t a, int s)	{ return a >> (s&31) | a << (32-(s&31));	}
	JINLINE uint64_t BitUtility::RotateLeft(uint64_t a, int s)	{ return a << (s&63) | a >> (64-(s&63));	}
	JINLINE uint64_t BitUtility::RotateRight(uint64_t a, int s)	{ return a >> (s&63) | a << (64-(s&63));	}
#endif
	
//============================================================================
} // namespace Javelin
//============================================================================
