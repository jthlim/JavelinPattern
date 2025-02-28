//============================================================================

#include "Javelin/Math/BitUtility.h"

//============================================================================

using namespace Javelin;

//============================================================================

unsigned int BitUtility::CountBits(unsigned int i)
{
	static_assert(sizeof(i) == 4, "Expect unsigned int to be 32 bits!");
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return ((i + (i >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
}

unsigned int BitUtility::CountBits(unsigned int* data, size_t numberOfWords)
{
	unsigned int result = 0;
	for(size_t i = 0; i < numberOfWords; ++i)
	{
		result += CountBits(data[i]);
	}
	return result;
}

//============================================================================

unsigned int BitUtility::RoundUpToPowerOf2(unsigned int u)
{
	unsigned int t = u-1;
	t |= (t >> 1);
	t |= (t >> 2);
	t |= (t >> 4);
	t |= (t >> 8);
	t |= (t >> 16);
	return t + 1;
}

unsigned long long BitUtility::RoundUpToPowerOf2(unsigned long long u)
{
	unsigned long long t = u-1;
	t |= (t >> 1);
	t |= (t >> 2);
	t |= (t >> 4);
	t |= (t >> 8);
	t |= (t >> 16);
	t |= (t >> 32);
	return t + 1;
}

//============================================================================
