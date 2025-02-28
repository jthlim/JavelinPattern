//===========================================================================

#include "Javelin/Type/Utf8Character.h"

#include "Javelin/Data/Utf8DecodeTable.h"
#include "Javelin/Type/Exception.h"

//===========================================================================

using namespace Javelin;
using Data::UTF8_DECODE_TABLE;

//===========================================================================

void Utf8Character::Set(unsigned c)
{
	if(c < 0x80)
	{
		buffer[0] = c;
	}
	else if(c < 0x800)
	{
		buffer[0] = 0xc0 | (c >> 6);
		buffer[1] = 0x80 | (c & 0x3f);
	}
	else if(c < 0x10000)
	{
		buffer[0] = 0xe0 | (c >> 12);
		buffer[1] = 0x80 | ((c >> 6) & 0x3f);
		buffer[2] = 0x80 | (c & 0x3f);
	}
	else if(c < 0x200000)
	{
		buffer[0] = 0xf0 | (c >> 18);
		buffer[1] = 0x80 | ((c >> 12) & 0x3f);
		buffer[2] = 0x80 | ((c >> 6) & 0x3f);
		buffer[3] = 0x80 | (c & 0x3f);
	}
	else if(c < 0x4000000)
	{
		buffer[0] = 0xf8 | (c >> 24);
		buffer[1] = 0x80 | ((c >> 18) & 0x3f);
		buffer[2] = 0x80 | ((c >> 12) & 0x3f);
		buffer[3] = 0x80 | ((c >> 6) & 0x3f);
		buffer[4] = 0x80 | (c & 0x3f);
	}
	else
	{
		buffer[0] = 0xfc | (c >> 30);
		buffer[1] = 0x80 | ((c >> 24) & 0x3f);
		buffer[2] = 0x80 | ((c >> 18) & 0x3f);
		buffer[3] = 0x80 | ((c >> 12) & 0x3f);
		buffer[4] = 0x80 | ((c >> 6) & 0x3f);
		buffer[5] = 0x80 | (c & 0x3f);
	}
}

//===========================================================================

static Character Decode0(const unsigned char* p)
{
	JDATA_ERROR();
}

static Character Decode1(const unsigned char* p)
{
	return Character(p[0]);
}

static Character Decode2(const unsigned char* p)
{
	return Character((p[0] << 6) ^ p[1] ^ 0x3080);
}

static Character Decode3(const unsigned char* p)
{
	return Character((p[0] << 12) ^ (p[1] << 6) ^ p[2] ^ 0xe2080);
}

static Character Decode4(const unsigned char* p)
{
	return Character((p[0] << 18) ^ (p[1] << 12) ^ (p[2] << 6) ^ p[3] ^ 0x3c82080);
}

static Character Decode5(const unsigned char* p)
{
	return Character((p[0] << 24) ^ (p[1] << 18) ^ (p[2] << 12) ^ (p[3] << 6) ^ p[4] ^ 0xfa082080);
}

static Character Decode6(const unsigned char* p)
{
	return Character((p[0] << 30) ^ (p[1] << 24) ^ (p[2] << 18) ^ (p[3] << 12) ^ (p[4] << 6) ^ p[5] ^ 0x82082080);
}

static Character (*const DECODE_LUT[])(const unsigned char*) =
{
	Decode0,
	Decode1,
	Decode2,
	Decode3,
	Decode4,
	Decode5,
	Decode6,
};

Character Utf8Character::AsCharacter() const
{
	const unsigned char *p = buffer;
	return (*DECODE_LUT[UTF8_DECODE_TABLE[*p]])(p);
}

//===========================================================================
