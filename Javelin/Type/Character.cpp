//============================================================================

#include "Javelin/Type/Character.h"

//============================================================================

using namespace Javelin;

//============================================================================

constexpr signed char Character::HEX_LUT[128] =
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

constexpr bool Character::WHITESPACE_LUT[64] =
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

constexpr bool Character::VOWEL_LUT[128] =
{
	false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false,

	false,  true, false, false, false,  true, false, false,
	false,  true, false, false, false, false, false,  true,
	false, false, false, false, false,  true, false, false,
	false, false, false, false, false, false, false, false,
	false,  true, false, false, false,  true, false, false,
	false,  true, false, false, false, false, false,  true,
	false, false, false, false, false,  true, false, false,
	false, false, false, false, false, false, false, false,
};

//============================================================================

Character Character::ToLower(unsigned c)
{
	// TODO: Lots of improvements here - better Unicode support
	static constexpr unsigned char LUT[256] =
	{
		  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
	 	 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
	 	 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
	 	 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
	 	 64,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,  91,  92,  93,  94,  95,
		 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
		128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
		144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
		160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
		176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
		192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
		208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
		224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
		240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
	};
	
	if(c < 256) return LUT[c];
	return c;
}

bool Character::IsLower(unsigned int c)
{
	return 'a' <= c && c <= 'z';
}

Character Character::ToUpper(unsigned c)
{
	// TODO: Lots of improvements here - better Unicode support
	static constexpr unsigned char LUT[256] =
	{
		  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
		 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
		 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
		 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
		 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
		 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
		 96,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
		 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 123, 124, 125, 126, 127,
		128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
		144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
		160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
		176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
		192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
		208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
		224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
		240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
	};
	
	if(c < 256) return LUT[c];
	return c;
}

bool Character::IsUpper(unsigned int c)
{
	return 'A' <= c && c <= 'Z';
}

//============================================================================

bool Character::IsDigit(unsigned c)
{
	return '0' <= c && c <= '9';
}

bool Character::IsWordCharacter(unsigned c)
{
#if 0 // defined(JASM_GNUC_X86_64)
	constexpr __uint128_t lut = (__uint128_t(0x07FFFFFE07FFFFFE) << 64) | 0x03FF000000000000;
	return ((lut >> (c < 128 ? c : 0)) & 1) != 0;
#else
	static const bool lut[128] =
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

	return lut[c < 128 ? c : 0];
#endif
}

//============================================================================

size_t Character::GetLength(const Character* string)
{
	const Character* p = string;
	while(*p) ++p;
	return p - string;
}

//============================================================================
