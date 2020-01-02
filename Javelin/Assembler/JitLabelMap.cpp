//============================================================================

#include "Javelin/Assembler/JitLabelMap.h"
#include <assert.h>
#include <string.h>

//============================================================================

using namespace Javelin;

//===========================================================================

void JitLabelMap::Reserve(uint32_t size)
{
	delete [] data;

	if(size == 0)
	{
		data = nullptr;
		return;
	}

	// For a load factor of ~0.75, increase by 3/8
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

	data = new Data[nextPowerOf2];
	memset(data, 0xff, nextPowerOf2*sizeof(Data));
}

void* JitLabelMap::Get(uint32_t label) const
{
	for(uint32_t key = label & lengthMask;; key = (key+1) & lengthMask)
	{
		assert(data[key].label != NO_LABEL);
		if(data[key].label == label)
		{
			return data[key].p;
		}
	}
}

void* JitLabelMap::GetIfExists(uint32_t label) const
{
	for(uint32_t key = label & lengthMask;; key = (key+1) & lengthMask)
	{
		if(data[key].label == label) return data[key].p;
		if(data[key].label == NO_LABEL) return nullptr;
	}
}

void JitLabelMap::Set(uint32_t label, void *p)
{
	assert(label != NO_LABEL);
	for(uint32_t key = label & lengthMask;; key = (key+1) & lengthMask)
	{
		if(data[key].label == NO_LABEL)
		{
			data[key].label = label;
			data[key].p = p;
			return;
		}
		if(data[key].label == label)
		{
			data[key].p = p;
			return;
		}
	}
}

void JitLabelMap::Clear()
{
	if(data)
	{
		memset(data, 0xff, (lengthMask+1) * sizeof(Data));
	}
}
	
//============================================================================
