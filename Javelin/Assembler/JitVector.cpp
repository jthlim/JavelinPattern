//============================================================================

#include "Javelin/Assembler/JitVector.h"
#include <assert.h>
#include <string.h>

//============================================================================

using namespace Javelin;

//===========================================================================

void* JitVectorBase::ExpandAndAppend(uint32_t appendSize)
{
	capacity = 2 * (offset + appendSize);
	
	// If realloc fails, this will lead to a crash.
	// This is a conscious choice.
	data = (uint8_t*) realloc(data, capacity);
	uint8_t *result = data + offset;
	offset += appendSize;
	return result;
}

void JitVectorBase::Reserve(uint32_t reserveSize)
{
	if(data) free(data);
	
	data = (uint8_t*) malloc(reserveSize);
	capacity = reserveSize;
}

//============================================================================
