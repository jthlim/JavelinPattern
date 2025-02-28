//==========================================================================

#include "Javelin/Pattern/Internal/PatternInstructionType.h"

//==========================================================================

using namespace Javelin;

//==========================================================================

String Javelin::ToString(PatternInternal::InstructionType type)
{
	static const String INSTRUCTION_NAMES[] =
	{
		#define TAG(x, y, c) JS(#x),
		#include "PatternInstructionTypeTags.h"
		#undef TAG
	};
	
	return INSTRUCTION_NAMES[(int)type];
}

//==========================================================================
