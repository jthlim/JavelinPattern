//============================================================================

#pragma once
#include <string>

//============================================================================

namespace Javelin::Assembler::arm64
{
//============================================================================

	struct RegisterOperand;
	struct Instruction;

//============================================================================

	struct Token
	{
		enum class Type : uint8_t
		{
			#define TAG(x) x,
			#include "TokenTags.h"
			#undef TAG
		};

		Type 		type;
		std::string	sValue;
		union
		{
			struct
			{
				int64_t 			iValue;
				int32_t				labelScopeId;
			};
			long double				rValue;
			const RegisterOperand*	reg;
			const Instruction*		instruction;
		};
		
		Token() { }
		Token(const Type aType) : type(aType) { }
		Token(int64_t aValue, int32_t aLabelScopeId) : type(Type::IntegerValue), iValue(aValue), labelScopeId(aLabelScopeId) { }
		Token(double aValue) : type(Type::RealValue), rValue(aValue) { }

		std::string GetDescription() const;
		
		int64_t GetLabelValue() const { return iValue + (int64_t(labelScopeId) << 32); }		
	};
	
//============================================================================
}
//============================================================================
