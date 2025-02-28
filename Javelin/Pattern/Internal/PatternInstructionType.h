//==========================================================================

#pragma once
#include "Javelin/Type/String.h"

//==========================================================================

namespace Javelin
{
	namespace PatternInternal
	{
//==========================================================================

		enum class InstructionType : unsigned char
		{
			#define TAG(x, y, c) x,
				#include "PatternInstructionTypeTags.h"
			#undef TAG
		};
		
//==========================================================================
	} // namespace Javelin
	
	String ToString(PatternInternal::InstructionType type);
	
//==========================================================================
} // namespace Javelin::PatternInternal
//==========================================================================
