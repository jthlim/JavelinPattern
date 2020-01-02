//============================================================================

#include "Javelin/Assembler/arm64/ActionType.h"

//============================================================================

Javelin::arm64Assembler::BitMaskEncodeResult Javelin::arm64Assembler::EncodeBitMask(uint64_t value)
{
	if(value == 0 || value == -1) return { 0, 0, 0 };
	
	int size = 2;
	uint64_t upper_mask;
	for(;;)
	{
		if(size == 64)
		{
			upper_mask = 0;
			break;
		}
		upper_mask = ~0ULL << size;
		if(((value ^ (value << size)) & upper_mask) == 0)
		{
			value &= ~upper_mask;
			break;
		}
		size *= 2;
	}
	
	// 00111110 -> (A) series of central ones
	// 00001111 -> (B) non-rotated ones
	// 11100011 -> (C) rotated series of ones.
	
	// Convert (C) to (A) or (B)
	bool bitwiseNotUsed = false;
	if(value & (1ULL << (size-1)))
	{
		bitwiseNotUsed = true;
		value = ~(value | upper_mask);
	}
	int trailingZeros = __builtin_ctzll(value);
	value >>= trailingZeros;
	
	// Check if it's 1 less than a power of two
	uint64_t valueP1 = value + 1;
	if((value & valueP1) != 0) return { 0, 0, 0 };
	
	int length = __builtin_ctzll(valueP1);
	
	int rotate = -trailingZeros;
	if(bitwiseNotUsed)
	{
		rotate -= length;
		length = size - length;
	}
	rotate &= size-1;
	
	return { size, length, rotate };
}

//============================================================================
