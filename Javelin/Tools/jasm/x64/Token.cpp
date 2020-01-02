//============================================================================

#include "Javelin/Tools/jasm/x64/Token.h"

#include <inttypes.h>

//============================================================================

using namespace Javelin::Assembler::x64;

//============================================================================

std::string Token::GetDescription() const
{
	static const char *const TAG_NAMES[] =
	{
		#define TAG(x) #x,
		#include "TokenTags.h"
		#undef TAG
	};
	
	switch(type)
	{
	case Type::Preprocessor:
		return sValue;
	case Type::Identifier:
		return "\"" + sValue + "\"";
	case Type::Instruction:
		return "op:" + sValue;
	case Type::Register:
		return "reg:" + sValue;
	case Type::Expression:
		return "{" + sValue + "}";
	case Type::IntegerValue:
		{
			char buffer[32];
			sprintf(buffer, "%" PRId64, iValue);
			return buffer;
		}
		break;
	case Type::RealValue:
		{
			char buffer[32];
			sprintf(buffer, "%Lg", rValue);
			return buffer;
		}
		break;
	default:
		return TAG_NAMES[(int)type];
	}
}

//============================================================================
