//============================================================================

#include "Javelin/Cryptography/Crc64.h"

//============================================================================

uint64_t Javelin::Crc64(const void* v, size_t count)
{
	const unsigned char* p = (const unsigned char*) v;
	uint64_t hash = 0xffffffffffffffffull;
	for(size_t i = 0; i < count; i++)
	{
		hash = Private::CRC64_TABLE[(hash >> 56) ^ *p++] ^ (hash << 8);
	}
	return ~hash;
}

//============================================================================

uint64_t Javelin::Crc64Iteration(uint64_t hash, const void* v, size_t count)
{
	const unsigned char* p = (const unsigned char*) v;
	for(size_t i = 0; i < count; i++)
	{
		hash = Private::CRC64_TABLE[(hash >> 56) ^ *p++] ^ (hash << 8);
	}
	return hash;
}

//============================================================================
