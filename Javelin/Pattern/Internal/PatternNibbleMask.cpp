//============================================================================

#include "Javelin/Pattern/Internal/PatternNibbleMask.h"
#include "Javelin/Container/Map.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

NibbleMask::NibbleMask()
{
	memset(lowNibbleMask, 0, sizeof(lowNibbleMask));
	memset(highNibbleMask, 0, sizeof(highNibbleMask));
}

NibbleMask::NibbleMask(const StaticBitTable<256>& bits)
: NibbleMask()
{
	Map<uint32_t, uint32_t> lowNibbleToBitIndexMap(16);
	
	for(int i = 0; i < 256; i += 16)
	{
		uint32_t lookupValue = 0;
		for(int j = 0; j < 16; ++j)
		{
			if(bits[i+j]) lookupValue |= (1<<j);
		}
		if(lookupValue == 0) continue;
		
		UpdateNibbleMask(lookupValue, lowNibbleToBitIndexMap, highNibbleMask[i/16]);
	}
}

NibbleMask::NibbleMask(const uint32_t* data, uint32_t mask)
: NibbleMask()
{
	Map<uint32_t, uint32_t> lowNibbleToBitIndexMap(16);
	
	for(int i = 0; i < 256; i += 16)
	{
		uint32_t lookupValue = 0;
		for(int j = 0; j < 16; ++j)
		{
			if(data[i+j] & mask) lookupValue |= (1<<j);
		}
		if(lookupValue == 0) continue;
		
		UpdateNibbleMask(lookupValue, lowNibbleToBitIndexMap, highNibbleMask[i/16]);
	}
}

void NibbleMask::UpdateNibbleMask(uint32_t lookupValue, Map<uint32_t, uint32_t>& lowNibbleToBitIndexMap, uint8_t& highNibbleMaskValue)
{
	if(Map<uint32_t, uint32_t>::Iterator it = lowNibbleToBitIndexMap.Find(lookupValue);
	   it != lowNibbleToBitIndexMap.End())
	{
		uint32_t index = it->value;
		highNibbleMaskValue |= (1 << index);
	}
	else
	{
		uint32_t index = numberOfBitsUsed++;
		uint32_t mask = 1 << index;
		
		lowNibbleToBitIndexMap.Insert(lookupValue, index);
		highNibbleMaskValue |= mask;
		for(int j = 0; j < 16; ++j)
		{
			if(lookupValue & 1) lowNibbleMask[j] |= mask;
			lookupValue >>= 1;
		}
	}
}

NibbleMask NibbleMask::operator>>(int shift) const
{
	NibbleMask result;
	result.numberOfBitsUsed = 1;
	for(int i = 0; i < 16; ++i)
	{
		result.lowNibbleMask[i] = (lowNibbleMask[i] >> shift) & 1;
		result.highNibbleMask[i] = (highNibbleMask[i] >> shift) & 1;
	}
	return result;
}

//============================================================================

void NibbleMask::AddByte(uint8_t value)
{
	uint32_t index = numberOfBitsUsed++;
	uint32_t mask = 1 << index;
	highNibbleMask[value >> 4] |= mask;
	lowNibbleMask[value & 0xf] |= mask;
}

uint32_t NibbleMask::GetMaskForByte(uint8_t byte) const
{
	uint32_t highNibble = highNibbleMask[byte >> 4];
	uint32_t lowNibble = lowNibbleMask[byte & 0xf];
	return lowNibble & highNibble;
}

//============================================================================

void NibbleMask::Merge(const NibbleMask& from, int numberOfChannels)
{
	uint32_t multiplier = ((1 << (numberOfChannels - from.numberOfBitsUsed + 1)) - 1) << numberOfBitsUsed;
	for(int i = 0; i < 16; ++i)
	{
		lowNibbleMask[i] |= from.lowNibbleMask[i] * multiplier;
		highNibbleMask[i] |= from.highNibbleMask[i] * multiplier;
	}
	numberOfBitsUsed += numberOfChannels;
}

void NibbleMask::MergeToChannel(const NibbleMask& from, int channel)
{
	for(int i = 0; i < 16; ++i)
	{
		lowNibbleMask[i] |= from.lowNibbleMask[i] << channel;
		highNibbleMask[i] |= from.highNibbleMask[i] << channel;
	}
}

void NibbleMask::MergeInternalChannel(int toChannel, int fromChannel)
{
	JASSERT(toChannel > fromChannel);
	
	int fromMask = ~(1<<fromChannel);
	int toMask = 1 << toChannel;
	
	int shift = toChannel - fromChannel;
	
	for(int i = 0; i < 16; ++i)
	{
		lowNibbleMask[i] = (lowNibbleMask[i] | ((lowNibbleMask[i] << shift) & toMask)) & fromMask;
		highNibbleMask[i] = (highNibbleMask[i] | ((highNibbleMask[i] << shift) & toMask)) & fromMask;
	}
}

//============================================================================

bool NibbleMask::ContainsVowel(int bitIndex) const
{
	int mask = 1 << bitIndex;
	
	// A = 0x41, E = 0x45, I = 0x49, O = 0x4F, U = 0x55
	// a = 0x61, e = 0x65, i = 0x69, o = 0x6F, u = 0x75
	if((highNibbleMask[4] | highNibbleMask[6]) & mask)
	{
		if((lowNibbleMask[1] | lowNibbleMask[5] | lowNibbleMask[9] | lowNibbleMask[15]) & mask) return true;
	}

	if((highNibbleMask[5] | highNibbleMask[7]) & mask)
	{
		if(lowNibbleMask[5] & mask) return true;
	}
	
	return false;
}

uint32_t NibbleMask::CalculateMergeCost(int thisBit, const NibbleMask& other, int otherBit) const
{
	int lowBitXorCount = 0;
	int highBitXorCount = 0;
	int lowBitOrCount = 0;
	int highBitOrCount = 0;
	
	uint32_t firstMask = 0;
	uint32_t secondMask = 0;
	
	for(int i = 0; i < 16; ++i)
	{
		int firstLow = (lowNibbleMask[i] >> thisBit) & 1;
		int secondLow = (other.lowNibbleMask[i] >> otherBit) & 1;
		int firstHigh = (highNibbleMask[i] >> thisBit) & 1;
		int secondHigh = (other.highNibbleMask[i] >> otherBit) & 1;
	
		firstMask = firstMask<<2 | firstHigh<<1 | firstLow;
		secondMask = secondMask<<2 | secondHigh<<1 | secondLow;
		
		lowBitXorCount += firstLow ^ secondLow;
		highBitXorCount += firstHigh ^ secondHigh;
		
		lowBitOrCount |= firstLow | secondLow;
		highBitOrCount |= firstHigh | secondHigh;
	}

	// Check if one mask completely dominates the other
	if((firstMask & secondMask) == firstMask
	   || (firstMask & secondMask) == secondMask) return 0;
	
	int vowelMismatchPenalty = ContainsVowel(thisBit) != other.ContainsVowel(otherBit) ? 2 : 1;
	
	return (lowBitXorCount * highBitOrCount + lowBitOrCount * highBitXorCount) * vowelMismatchPenalty;
}

//============================================================================
