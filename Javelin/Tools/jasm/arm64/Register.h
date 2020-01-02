//============================================================================

#pragma once
#include "Javelin/Tools/jasm/arm64/Instruction.h"

#include <string>
#include <unordered_map>

//============================================================================

namespace Javelin::Assembler::arm64
{
//============================================================================

	class RegisterMap : public std::unordered_map<std::string, const RegisterOperand*>
	{
	public:
		static const RegisterMap& GetInstance() { return instance; }
		
	private:
		RegisterMap();
		
		static const RegisterMap instance;
	};

//============================================================================

	struct ParameterizedRegister
	{
		uint32_t		registerData;
		MatchBitfield 	matchBitField;
	};
	
	class ParameterizedRegisterMap : public std::unordered_map<std::string, ParameterizedRegister>
	{
	public:
		ParameterizedRegisterMap();
		
		static const ParameterizedRegisterMap instance;
	};

//============================================================================
} // namespace Javelin::Assembler::arm64
//============================================================================
