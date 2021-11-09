//============================================================================

#include "JitHash.h"

//============================================================================

unsigned Javelin::JitHash(const void* v, size_t count)
{
	const unsigned char* p = (const unsigned char*) v;
	unsigned hash = 0xffffffff;
	for(size_t i = 0; i < count; i++)
	{
		hash = Private::JIT_HASH_TABLE[(hash ^ *p++) & 0xff] ^ (hash >> 8);
	}
	return ~hash;
}

//============================================================================

