//============================================================================

#include "Javelin/Assembler/JitForwardReferenceMap.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

//============================================================================

using namespace Javelin;

//===========================================================================

void JitForwardReferenceMap::Reserve(uint32_t size)
{
	free(pool);

	if(size == 0)
	{
		poolIndices = nullptr;
		pool = nullptr;
		return;
	}

	// For a load factor of ~0.75, increase by 3/8
	size += (size >> 2) + (size >> 3) + 1;
	
	// Round up to power of 2.
	uint32_t t = size-1;
	t |= (t >> 1);
	t |= (t >> 2);
	t |= (t >> 4);
	t |= (t >> 8);
	t |= (t >> 16);
	lengthMask = t;
	uint32_t nextPowerOf2 = t+1;

	pool = (JitForwardReferenceData*) malloc(nextPowerOf2 * (sizeof(JitForwardReferenceData) + sizeof(uint32_t)));
	poolIndices = (uint32_t*) (pool + nextPowerOf2);
	memset(poolIndices, 0xff, nextPowerOf2 * sizeof(uint32_t));

	headFree = nullptr;
	poolFreeIndex = 0;
}

JitForwardReferenceData* JitForwardReferenceMap::Add(uint32_t label)
{
	assert(label != NO_LABEL);
	
	JitForwardReferenceData* newData;
	if(headFree)
	{
		newData = headFree;
		headFree = headFree->next;
	}
	else
	{
		newData = &pool[poolFreeIndex++];
	}
	newData->label = label;
	
	uint32_t key;
	for(key = label & lengthMask;; key = (key+1) & lengthMask)
	{
		uint32_t index = poolIndices[key];
		if(index == NO_LABEL)
		{
			newData->next = nullptr;
			break;
		}
		if(pool[index].label == label)
		{
			newData->next = &pool[index];
			break;
		}
	}
	poolIndices[key] = uint32_t(newData - pool);
	return newData;
}

void JitForwardReferenceMap::Add(uint32_t label, uint8_t *p, uint32_t data)
{
	assert(label != NO_LABEL);
	
	JitForwardReferenceData* newData;
	if(headFree)
	{
		newData = headFree;
		headFree = headFree->next;
	}
	else
	{
		newData = &pool[poolFreeIndex++];
	}
	newData->label = label;
	newData->data = data;
	newData->p = p;

	uint32_t key;
	for(key = label & lengthMask;; key = (key+1) & lengthMask)
	{
		uint32_t index = poolIndices[key];
		if(index == NO_LABEL)
		{
			newData->next = nullptr;
			break;
		}
		if(pool[index].label == label)
		{
			newData->next = &pool[index];
			break;
		}
	}
	poolIndices[key] = uint32_t(newData - pool);
}

void JitForwardReferenceMap::Remove(JitForwardReferenceMapLookupResult result, JitForwardReferenceData* tail)
{
	tail->next = headFree;
	headFree = result.reference;
	
	int holeIndex = result.keyIndex;

	uint32_t offset = 1;
	uint32_t scanToEnd = (holeIndex+1) & lengthMask;
	for(;;)
	{
		uint32_t index = poolIndices[scanToEnd];
		if(index == NO_LABEL) break;
		
		uint32_t hashIndex = pool[index].label & lengthMask;
		if(((scanToEnd - hashIndex) & lengthMask) >= offset)
		{
			poolIndices[holeIndex] = poolIndices[scanToEnd];
			holeIndex = scanToEnd;
			offset = 0;
		}
		++offset;
		scanToEnd = (scanToEnd + 1) & lengthMask;
	}
	poolIndices[holeIndex] = NO_LABEL;
}

JitForwardReferenceMapLookupResult JitForwardReferenceMap::Find(uint32_t label)
{
	JitForwardReferenceMapLookupResult result;
	if(!poolIndices)
	{
		result.reference = nullptr;
		return result;
	}
	for(uint32_t key = label & lengthMask;; key = (key+1) & lengthMask)
	{
		uint32_t index = poolIndices[key];
		if(index == NO_LABEL)
		{
			result.reference = nullptr;
			return result;
		}
		if(pool[index].label == label)
		{
			result.reference = &pool[index];
			result.keyIndex = key;
			return result;
		}
	}
}

//===========================================================================

bool JitForwardReferenceMap::HasData() const
{
	if(poolIndices == nullptr) return false;
	for(uint32_t key = 0; key <= lengthMask; ++key)
	{
		uint32_t index = poolIndices[key];
		if(index != NO_LABEL)
		{
			printf("key: %x, label %x, data %x, p: %p\n",
				   key,
				   pool[index].label,
				   pool[index].data,
				   pool[index].p);
			return true;
		}
	}
	return false;
}

//===========================================================================
