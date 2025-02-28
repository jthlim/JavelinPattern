//============================================================================

#include "Javelin/Template/Swap.h"
#include <stdint.h>

//============================================================================

using namespace Javelin;

//============================================================================
	
void Javelin::SwapBlock(void* a, void* b, size_t length)
{
	unsigned char* p1 = (unsigned char*) a;
	unsigned char* p2 = (unsigned char*) b;
	
	// Only do 32-bit transfers if both a and b are aligned!
	if((reinterpret_cast<size_t>(a) & 3) == 0 && (reinterpret_cast<size_t>(b) & 3) == 0)
	{
		uint32_t* w1 = (uint32_t*) a;
		uint32_t* w2 = (uint32_t*) b;
		size_t wordCount = length >> 2;
		length -= wordCount << 2;
		
		for(size_t i = 0; i < wordCount; ++i)
		{
			uint32_t w = *w1;
			*w1 = *w2;
			*w2 = w;
			++w1; ++w2;
		}
		
		p1 = (unsigned char*) w1;
		p2 = (unsigned char*) w2;
	}
	
	for(size_t i = 0; i < length; ++i)
	{
		unsigned char c = *p1;
		*p1 = *p2;
		*p2 = c;
		++p1; ++p2;
	}
}
	
//============================================================================
