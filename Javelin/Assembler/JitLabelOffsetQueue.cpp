//============================================================================

#include "Javelin/Assembler/JitLabelOffsetQueue.h"
#include <assert.h>
#include <string.h>

//============================================================================

using namespace Javelin;

//===========================================================================

void JitLabelOffsetQueue::Reserve(uint32_t numberOfLabels)
{
	free(hashData);
	
	if(numberOfLabels == 0)
	{
		hashData = nullptr;
		entries = nullptr;
		return;
	}

	// For a load factor of ~0.75, increase by 3/8
	uint32_t size = numberOfLabels;
	size += (size >> 2) + (size >> 3);
	
	// Round up to power of 2.
	uint32_t t = size-1;
	t |= (t >> 1);
	t |= (t >> 2);
	t |= (t >> 4);
	t |= (t >> 8);
	t |= (t >> 16);
	lengthMask = t;
	uint32_t nextPowerOf2 = t+1;

	hashData = (HashData*) malloc(nextPowerOf2 * sizeof(HashData) + numberOfLabels * sizeof(EntryData));
	entries = (EntryData*) (hashData + nextPowerOf2);
	memset(hashData, 0xff, nextPowerOf2*sizeof(HashData));
}

uint32_t JitLabelOffsetQueue::GetFirstLabelOffset(uint32_t label) const
{
	for(uint32_t key = label & lengthMask;; key = (key+1) & lengthMask)
	{
		HashData &hashEntry = hashData[key];
		assert(hashEntry.label != NO_LABEL);
		if(hashEntry.label == label)
		{
			EntryData& entry = entries[hashEntry.headIndex];
			return entry.offset;
		}
	}
}

uint32_t JitLabelOffsetQueue::GetLastLabelOffset(uint32_t label) const
{
	assert(label != NO_LABEL);
	
	for(uint32_t key = label & lengthMask;; key = (key+1) & lengthMask)
	{
		HashData &hashEntry = hashData[key];
		assert(hashEntry.label != NO_LABEL);
		if(hashEntry.label == label)
		{
			EntryData& lastEntry = entries[hashEntry.tailIndex];
			assert(lastEntry.nextIndex == NO_LABEL);
			return lastEntry.offset;
		}
	}
}

uint32_t JitLabelOffsetQueue::GetLastLabelOffsetIfExists(uint32_t label) const
{
	assert(label != NO_LABEL);
	
	for(uint32_t key = label & lengthMask;; key = (key+1) & lengthMask)
	{
		HashData &hashEntry = hashData[key];
		if(hashEntry.label == NO_LABEL) return NO_LABEL;
		if(hashEntry.label == label)
		{
			EntryData& lastEntry = entries[hashEntry.tailIndex];
			assert(lastEntry.nextIndex == NO_LABEL);
			return lastEntry.offset;
		}
	}
}

void JitLabelOffsetQueue::DequeueLabel(uint32_t label)
{
	for(uint32_t key = label & lengthMask;; key = (key+1) & lengthMask)
	{
		HashData &hashEntry = hashData[key];
		assert(hashEntry.label != NO_LABEL);
		if(hashEntry.label == label)
		{
			EntryData& entry = entries[hashEntry.headIndex];
			hashEntry.headIndex = entry.nextIndex;
			return;
		}
	}
}

void JitLabelOffsetQueue::QueueLabelOffset(uint32_t label, uint32_t offset)
{
	assert(label != NO_LABEL);
	
	uint32_t entryIndex = numberOfEntries++;
	EntryData& entry = entries[entryIndex];
	entry.offset = offset;
	entry.nextIndex = NO_LABEL;
	
	for(uint32_t key = label & lengthMask;; key = (key+1) & lengthMask)
	{
		HashData &hashEntry = hashData[key];
		if(hashEntry.label == NO_LABEL)
		{
			hashEntry.label = label;
			hashEntry.headIndex = entryIndex;
			hashEntry.tailIndex = entryIndex;
			return;
		}
		if(hashEntry.label == label)
		{
			EntryData& lastEntry = entries[hashEntry.tailIndex];
			assert(lastEntry.nextIndex == NO_LABEL);
			lastEntry.nextIndex = entryIndex;
			hashEntry.tailIndex = entryIndex;
			return;
		}
	}
}

//============================================================================
