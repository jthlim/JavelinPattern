//============================================================================

#pragma once
#include "Javelin/Tools/jasm/riscv/Instruction.h"

#include <string>
#include <unordered_map>

//============================================================================

namespace Javelin::Assembler::riscv
{
//============================================================================

	class RoundingModeMap : public std::unordered_map<std::string, const ImmediateOperand*>
	{
	public:
		static const RoundingModeMap& GetInstance() { return instance; }
		
	private:
        RoundingModeMap();
		
		static const RoundingModeMap instance;
	};

//============================================================================
} // namespace Javelin::Assembler::riscv
//============================================================================
