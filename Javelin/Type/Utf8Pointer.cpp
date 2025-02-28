//===========================================================================

#include "Javelin/Type/Utf8Pointer.h"
#include "Javelin/Type/Exception.h"

//===========================================================================

namespace Javelin
{
//===========================================================================

using Data::UTF8_DECODE_TABLE;
	
//===========================================================================

bool Utf8Pointer::IsValid() const
{
	const unsigned char *process = reinterpret_cast<const unsigned char*>(p);
	while(*process)
	{
		int step = UTF8_DECODE_TABLE[*process];
		if(!step) return false;

		process++;
		for(int i = 1; i < step; i++)
		{
			if((*process++ & 0xC0) != 0x80) return false;
		}
	}
	return true;
}

size_t Utf8Pointer::GetNumberOfCharacters() const
{
	size_t count = 0;
	const unsigned char *process = reinterpret_cast<const unsigned char*>(p);

	while(*process)
	{
		int advance = UTF8_DECODE_TABLE[*process];
		JDATA_VERIFY(advance != 0);
		process += advance;
		++count;
	}
	return count;
}

void Utf8Pointer::SeekForwards(size_t n)
{
	const unsigned char *process = reinterpret_cast<const unsigned char*>(p);
	while(n)
	{
		process += UTF8_DECODE_TABLE[*process];
		--n;
	}
	p = reinterpret_cast<const Utf8Character*>(process);
}

void Utf8Pointer::SeekBackwards(size_t n)
{
	const unsigned char *process = reinterpret_cast<const unsigned char*>(p);
	while(n)
	{
		if((*--process & 0x80) != 0)
		{
			while((*--process & 0xC0) == 0x80);
		}
		--n;
	}
	p = reinterpret_cast<const Utf8Character*>(process);
}

size_t operator-(const Utf8Pointer& a, const Utf8Pointer& b)
{
	size_t count = 0;
	const unsigned char* p = reinterpret_cast<const unsigned char*>(b.p);
	while(p < reinterpret_cast<const unsigned char*>(a.p))
	{
		unsigned char advance = UTF8_DECODE_TABLE[*p];
		if(!advance) return count;
		++count;
		p += advance;
	}
	return count;
}

Utf8Pointer& Utf8Pointer::operator--()
{
	const unsigned char *process = reinterpret_cast<const unsigned char*>(p);
	
	if((*--process & 0x80) != 0)
	{
		while((*--process & 0xC0) == 0x80);
	}
	p = reinterpret_cast<const Utf8Character*>(process);

	return *this;
}

//===========================================================================
} // namespace Javelin
//===========================================================================
