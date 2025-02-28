//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class Character
	{
	public:
		Character() = default;
		constexpr Character(unsigned c) : value(c)	{ }
		
		// Setup to take unsigned parameters so that the compiler doesn't require memory accesses
		// inside Character.cpp functions
		static JEXPORT Character	JCALL ToLower(unsigned c);
		static JEXPORT Character	JCALL ToUpper(unsigned c);
		static JEXPORT bool			JCALL IsHexCharacter(unsigned c)		{ return HEX_LUT[c < 128 ? c : 0] != -1;	}
		static JINLINE int			JCALL GetHexValue(unsigned c)			{ return HEX_LUT[c < 128 ? c : 0];			}
		static JEXPORT bool			JCALL IsDigit(unsigned c);
		static JEXPORT bool			JCALL IsWordCharacter(unsigned c);
		static JEXPORT bool    		JCALL IsWhitespace(unsigned c)			{ return WHITESPACE_LUT[c < 64 ? c : 0];	}
		static JEXPORT bool    		JCALL IsLower(unsigned c);
		static JEXPORT bool    		JCALL IsUpper(unsigned c);
		static JEXPORT bool    		JCALL IsVowel(unsigned c)				{ return VOWEL_LUT[c < 128 ? c : 0];	}
		
		static JEXPORT size_t JCALL GetLength(const Character* string);
		
		// Don't use wchar_t because it's different on different compilers!
		uint32_t value;
		operator uint32_t() const 					{ return value; 				}
		Character& operator=(unsigned aValue)		{ value = aValue; return *this; }
		
		Character operator+(int i) const 			{ return Character(value+i); 	}
		Character operator-(int i) const	 		{ return Character(value-i); 	}
		int operator-(const Character& c) const 	{ return (int) value - (int) c.value; }
		
		void operator&=(int i) 						{ value &= i; }
		void operator|=(int i) 						{ value |= i; }
		
		JINLINE Character	JCALL ToLower() 		const	{ return ToLower(value); 			}
		JINLINE Character	JCALL ToUpper() 		const	{ return ToUpper(value); 			}
		JINLINE bool		JCALL IsHexCharacter() 	const	{ return IsHexCharacter(value);		}
		JINLINE int	   		JCALL GetHexValue()		const	{ return GetHexValue(value);		}
		JINLINE bool    	JCALL IsDigit()			const	{ return IsDigit(value);			}
		JINLINE bool    	JCALL IsWordCharacter()	const	{ return IsWordCharacter(value);	}
		JINLINE bool    	JCALL IsWhitespace()	const	{ return IsWhitespace(value);		}
		JINLINE bool    	JCALL IsLower()			const	{ return IsLower(value);			}
		JINLINE bool    	JCALL IsUpper()			const	{ return IsUpper(value);			}
		JINLINE bool    	JCALL IsVowel()			const	{ return IsVowel(value);			}
		
		JINLINE static Character JCALL Minimum()			{ return Character(0); 				}
		JINLINE static Character JCALL Maximum()			{ return Character(0x10FFFF);       }
		
	private:
		static const signed char	HEX_LUT[128];
		static const bool			WHITESPACE_LUT[64];
		static const bool			VOWEL_LUT[128];
	};

//============================================================================
} // namespace Javelin
//============================================================================
