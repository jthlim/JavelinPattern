//============================================================================

#include "Javelin/Container/BitTable.h"
#include "Javelin/Template/Memory.h"
#include "Javelin/Template/Range.h"

//============================================================================

using namespace Javelin;

//============================================================================

void Private::BitTableBase::ClearAllBitsInTable(unsigned* data, size_t numberOfWords)
{
	ClearMemory(data, numberOfWords);
}

void Private::BitTableBase::SetAllBitsInTable(unsigned* data, size_t numberOfWords)
{
	SetMemory(data, (unsigned) -1, numberOfWords);
}

void Private::BitTableBase::AndAllBitsInTable(unsigned* data, const unsigned* other, size_t numberOfWords)
{
	for(size_t i = 0; i < numberOfWords; ++i)
	{
		data[i] &= other[i];
	}
}

void Private::BitTableBase::OrAllBitsInTable(unsigned* data, const unsigned* other, size_t numberOfWords)
{
	for(size_t i = 0; i < numberOfWords; ++i)
	{
		data[i] |= other[i];
	}
}

bool Private::BitTableBase::HasAnyBitSetInTable(const unsigned* data, size_t numberOfWords)
{
	for(size_t i = 0; i < numberOfWords; ++i)
	{
		if(data[i] != 0) return true;
	}
	return false;
}

void Private::BitTableBase::NotAllBitsInTable(unsigned* out, const unsigned* in, size_t numberOfWords)
{
	for(size_t i = 0; i < numberOfWords; ++i)
	{
		out[i] = ~in[i];
	}
}

size_t Private::BitTableBase::CountBitsInTable(const unsigned* data, size_t numberOfWords)
{
	size_t result = 0;
	for(size_t i = 0; i < numberOfWords; ++i)
	{
		result += BitUtility::CountBits(data[i]);
	}
	return result;
}

size_t Private::BitTableBase::CountTrailingZeros(const unsigned* data, size_t numberOfWords)
{
	size_t result = 0;
	for(size_t i = 0; i < numberOfWords; ++i)
	{
		unsigned value = data[i];
		if(value == 0)
		{
			result += 32;
			continue;
		}
		
		if((value & 0xffff) == 0)
		{
			value >>= 16;
			result += 16;
		}
		if((value & 0xff) == 0)
		{
			value >>= 8;
			result += 8;
		}
		if((value & 0xf) == 0)
		{
			value >>= 4;
			result += 4;
		}
		static const unsigned char LUT[16] = { 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0 };
		return result + LUT[value&0xf];
	}
	return result;
}

Optional<Interval<size_t>> Private::BitTableBase::GetContiguousRange(const unsigned* data, size_t numberOfWords)
{
	size_t minimum = CountTrailingZeros(data, numberOfWords);
	if(minimum == numberOfWords*32) return {};
	
	size_t currentWord = minimum / 32;;
	int testBit = minimum & 31;
	
	unsigned expectedRemainder = (unsigned) -1 << (minimum & 31);
	if(data[currentWord] == expectedRemainder)
	{
		++currentWord;
		for(;;)
		{
			if(currentWord == numberOfWords) return Interval<size_t>(minimum, numberOfWords*32);
			if(data[currentWord] != (unsigned) -1) break;
			++currentWord;
		}
		testBit = 0;
	}
	while(testBit < 32 && ((data[currentWord] >> testBit) & 1)) ++testBit;
	size_t maximum = currentWord*32 + testBit;
	if((data[currentWord] >> testBit) != 0
	   || HasAnyBitSetInTable(data+currentWord+1, numberOfWords-currentWord-1)) return {};
	return Interval<size_t>(minimum, maximum);
}

bool Private::BitTableBase::HasAllBitSetInTable(const unsigned* data, size_t numberOfBits)
{
	size_t numberOfWords = numberOfBits / 32;
	for(size_t i = 0; i < numberOfWords; ++i)
	{
		if(data[i] != (unsigned) -1) return false;
	}
	size_t remainder = numberOfBits & 31;
	if(remainder)
	{
		unsigned lastValue = ~(~0U << remainder);
		if(data[numberOfWords] != lastValue) return false;
	}
	return true;
}

//============================================================================

BitTable::BitTable()
{
	numberOfBits = 0;
	capacityInWords = 0;
	data = nullptr;
}

BitTable::BitTable(size_t aNumberOfBits)
{
	numberOfBits = aNumberOfBits;
	capacityInWords = GetNumberOfWords(aNumberOfBits);
	data = new unsigned[capacityInWords];
	ClearMemory(data, capacityInWords);
}

BitTable::BitTable(const BitTable &a)
{
	numberOfBits = a.numberOfBits;
	capacityInWords = a.capacityInWords;
	data = new unsigned[capacityInWords];
	CopyMemory(data, a.data, capacityInWords);
}

BitTable::BitTable(BitTable&& a)
{
	numberOfBits = a.numberOfBits;
	capacityInWords = a.capacityInWords;
	data = a.data;
	
	a.numberOfBits = 0;
	a.capacityInWords = 0;
	a.data = nullptr;
}

BitTable::~BitTable()
{
	delete [] data;
}

//============================================================================

void BitTable::SetCount(size_t newNumberOfBits)
{
	if(newNumberOfBits == numberOfBits) return;
	
	if(newNumberOfBits < numberOfBits)
	{
		size_t oldNumberOfWords = GetNumberOfWords(numberOfBits);
		size_t newNumberOfWords = GetNumberOfWords(newNumberOfBits);
		
		// Zero out the excess
		if(newNumberOfWords > 0)
		{
			// Clear the remainder of the bits in the current word
			int mask = unsigned(-1) << (newNumberOfBits & 31);
			data[newNumberOfWords] &= ~mask;
			
			// And clear the remainder of the words.
			for(size_t i : Range(oldNumberOfWords, newNumberOfWords)) data[i] = 0;
		}
		
		numberOfBits = newNumberOfBits;
	}
	else
	{
		// newNumberOfBits > numberOfBits
		size_t newNumberOfWords = GetNumberOfWords(newNumberOfBits);
		
		if(newNumberOfWords > capacityInWords)
		{
			// Reallocation time.
			unsigned* newData = new unsigned[newNumberOfWords];
			CopyMemory(newData, data, capacityInWords);
			memset(newData+capacityInWords, 0, (newNumberOfWords-capacityInWords)*sizeof(unsigned));
			delete [] data;
			data = newData;
			capacityInWords = newNumberOfWords;
		}
		
		numberOfBits = newNumberOfBits;
	}
}

//============================================================================
