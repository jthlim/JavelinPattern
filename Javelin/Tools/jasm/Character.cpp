//============================================================================

#include "Javelin/Tools/jasm/Character.h"

//============================================================================

using namespace Javelin::Assembler;

//============================================================================
	
// This does not skip newlines,
bool Character::IsIgnorableWhitespace(int c)
{
	static constexpr bool LUT[64] =
	{
		false, false, false, false, false, false, false, false,
		false,  true, false,  true,  true,  true, false, false,	// tab, \v, \r, \f
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		 true, false, false, false, false, false, false, false,	// space
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
	};
	
	return (0 <= c && c < 64) ? LUT[c] : false;
}

int Character::HexValueForCharacter(int c)
{
	static constexpr signed char LUT[128] =
	{
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
		-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	};
	
	return (0 <= c && c < 128) ? LUT[c] : -1;
}

bool Character::IsWordCharacter(int c)
{
	static const bool LUT[128] =
	{
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
		 true,  true,  true,  true,  true,  true,  true,  true,  true,  true, false, false, false, false, false, false,
		false,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
		 true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true, false, false, false, false,  true,
		false,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
		 true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true, false, false, false, false, false,
	};
	
	return (0 <= c && c < 128) ? LUT[c] : false;
}

bool Character::IsWhitespace(uint32_t c)
{
	static const bool LUT[64] =
	{
		false, false, false, false, false, false, false, false,
		false,  true,  true,  true,  true,  true, false, false,	// tab, \n, \v, \r, \f
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		true, false, false, false, false, false, false, false,	// space
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
	};
	
	return c < 64 && LUT[c];
}

//============================================================================
