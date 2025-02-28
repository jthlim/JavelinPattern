//===========================================================================

#pragma once
#include "Javelin/Data/Utf8DecodeTable.h"
#include "Javelin/Type/Character.h"

//===========================================================================

namespace Javelin
{
//===========================================================================
	
	class Utf8Character
	{
	public:
		Utf8Character()						{ buffer[0] = 0; }
		explicit Utf8Character(Character c)	{ Set(c); }
		
		operator Character() const			{ return AsCharacter(); }
		void operator=(Character c) 		{ Set(c); }
		
		const char*		GetData() 			const		{ return (const char*) buffer;	}
		size_t			GetNumberOfBytes()	const		{ return Data::UTF8_DECODE_TABLE[buffer[0]]; }

		const char*		AsString()						{ buffer[GetNumberOfBytes()] = '\0'; return (const char*) buffer; }

		JINLINE Character	JCALL ToLower() 		const	{ return AsCharacter().ToLower();			}
		JINLINE Character	JCALL ToUpper() 		const	{ return AsCharacter().ToUpper();			}
		JINLINE bool		JCALL IsHexCharacter() 	const	{ return AsCharacter().IsHexCharacter();	}
		JINLINE int	   		JCALL GetHexValue()		const	{ return AsCharacter().GetHexValue();		}
		JINLINE bool    	JCALL IsDigit()			const	{ return AsCharacter().IsDigit();			}
		JINLINE bool    	JCALL IsWordCharacter()	const	{ return AsCharacter().IsWordCharacter();	}
		JINLINE bool    	JCALL IsWhitespace()	const	{ return AsCharacter().IsWhitespace();		}
		JINLINE bool    	JCALL IsLower()			const	{ return AsCharacter().IsLower();			}
		JINLINE bool    	JCALL IsUpper()			const	{ return AsCharacter().IsUpper();			}

		JINLINE bool		JCALL operator==(char c) const	{ return AsCharacter() == c;				}
		
	protected:
		void		Set(unsigned c);
		
		// Utf8Pointer uses Utf8Character to decode data. This MUST be the first element in the structure for this to work.
		// Utf8 characters can encode to 6 bytes long max.
		unsigned char buffer[7];

		Character AsCharacter() const;
	};
	
//===========================================================================
} // namespace Javelin
//===========================================================================
