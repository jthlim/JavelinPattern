//============================================================================

#pragma once
#include <stdint.h>

//============================================================================

namespace Javelin::Assembler::arm64
{
//============================================================================

	class Assembler;
	class ListAction;
	struct EncodingVariant;
	struct Instruction;
	struct Operand;

//============================================================================

	typedef void InstructionEncoder(Assembler &assembler,
									ListAction& listAction,
									const Instruction &instruction,
									const EncodingVariant &encodingVariant,
									const Operand *const *operands);
	
	class Encoder
	{
	public:
		enum Value : uint8_t
		{
			#define TAG(x) x,
			#include "EncoderTags.h"
			#undef TAG
		};
		
		constexpr Encoder(Value aValue) : value(aValue) { }
		
		InstructionEncoder* GetFunction() const;
		
	private:
		Value value;
	};

//============================================================================
} // namespace Javelin::Assembler::arm64
//============================================================================
