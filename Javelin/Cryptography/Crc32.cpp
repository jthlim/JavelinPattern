//============================================================================

#include "Javelin/Cryptography/Crc32.h"

//============================================================================

unsigned Javelin::Crc32(const void* v, size_t count)
{
	const unsigned char* p = (const unsigned char*) v;
	unsigned hash = 0xffffffff;
	for(size_t i = 0; i < count; i++)
	{
		hash = Private::CRC32_TABLE[(hash ^ *p++) & 0xff] ^ (hash >> 8);
	}
	return ~hash;
}

//============================================================================

