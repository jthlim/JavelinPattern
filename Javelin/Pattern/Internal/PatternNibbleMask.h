//============================================================================

#pragma once
#include "Javelin/Container/BitTable.h"
#include "Javelin/Container/Map.h"

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================
	
	struct NibbleMask
	{
	public:
		NibbleMask();
		NibbleMask(const StaticBitTable<256>& bits);
		NibbleMask(const uint32_t* data, uint32_t mask);

		bool 		IsValid() const 						{ return numberOfBitsUsed <= 8; }
		uint32_t 	GetNumberOfBitsUsed() const				{ return numberOfBitsUsed; 		}

		uint32_t	GetMaskForByte(uint8_t byte) const;
		void 		AddByte(uint8_t value);

		void 		CopyMaskToTarget(void* target)	const	{ memcpy(target, lowNibbleMask, 32); }
		const void* GetNibbleMask() const					{ return lowNibbleMask; }
		
		void Merge(const NibbleMask& from, int numberOfChannels);
		void MergeToChannel(const NibbleMask& from, int channel);
		void MergeInternalChannel(int toChannel, int fromChannel);
		
		NibbleMask operator>>(int i) const;

		uint32_t CalculateMergeCost(int thisBit, const NibbleMask& other, int otherBit) const;
		
		bool ContainsVowel(int bitIndex) const;
		
	private:
		uint32_t numberOfBitsUsed = 0;

		uint8_t lowNibbleMask[16];
		uint8_t highNibbleMask[16];
		
		void UpdateNibbleMask(uint32_t lookupValue, Map<uint32_t, uint32_t>& lowNibbleToBitIndexMap, uint8_t& highNibbleMaskValue);
	};
	
//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
