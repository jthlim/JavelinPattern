//============================================================================

#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <string>

//============================================================================

namespace Javelin::Assembler
{
//============================================================================
	
	/// Simple FILE* wrapper
	class Character
	{
	public:
		static bool IsIgnorableWhitespace(int c);
		static int HexValueForCharacter(int c);
		static bool IsWordCharacter(int c);
		static bool IsWhitespace(uint32_t c);
	};
	
//============================================================================
}
//============================================================================
