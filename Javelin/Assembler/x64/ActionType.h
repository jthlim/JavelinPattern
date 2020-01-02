//============================================================================

#pragma once
#include <stdint.h>

//============================================================================

namespace Javelin::x64Assembler
{
//============================================================================
	
	enum class ActionType : uint8_t
	{
		#define TAG(x) x,
		#include "ActionTypeTags.h"
		#undef TAG
	};

	union DynamicOpcodeRControlByte
	{
		struct
		{
			uint8_t opcodeLengthM1 : 2;
			uint8_t hasRegExpression : 1;
		};
		uint8_t value;
	};
	static_assert(sizeof(DynamicOpcodeRControlByte) == 1, "ControlByte needs to be byte sized");

	union DynamicOpcodeRRControlByte
	{
		struct
		{
			uint8_t opcodeLengthM1 : 2;
			uint8_t hasRegExpression : 1;
			uint8_t hasRmExpression : 1;
		};
		uint8_t value;
	};
	static_assert(sizeof(DynamicOpcodeRRControlByte) == 1, "ControlByte needs to be byte sized");

	union DynamicOpcodeRMControlByte
	{
		struct
		{
			uint8_t opcodeLengthM1 : 2;
			uint8_t hasRegExpression : 1;
			uint8_t hasScaleExpression : 1;
			uint8_t hasIndexExpression : 1;
			uint8_t hasBaseExpression : 1;
			uint8_t hasDisplacementExpression : 1;
		};
		uint8_t value;
	};
	static_assert(sizeof(DynamicOpcodeRMControlByte) == 1, "ControlByte needs to be byte sized");

	union DynamicOpcodeRVControlByte
	{
		struct
		{
			uint8_t opcodeLengthM1 : 2;
			uint8_t hasReg0Expression : 1;
			uint8_t hasReg1Expression : 1;
		};
		uint8_t value;
	};
	static_assert(sizeof(DynamicOpcodeRVControlByte) == 1, "ControlByte needs to be byte sized");

	union DynamicOpcodeRVRControlByte
	{
		struct
		{
			uint8_t opcodeLengthM1 : 2;
			uint8_t hasReg0Expression : 1;
			uint8_t hasReg1Expression : 1;
			uint8_t hasReg2Expression : 1;
		};
		uint8_t value;
	};
	static_assert(sizeof(DynamicOpcodeRVRControlByte) == 1, "ControlByte needs to be byte sized");

	union DynamicOpcodeRVMControlByte
	{
		struct
		{
			uint8_t opcodeLengthM1 : 2;
			uint8_t hasReg0Expression : 1;
			uint8_t hasReg1Expression : 1;
			uint8_t hasScaleExpression : 1;
			uint8_t hasIndexExpression : 1;
			uint8_t hasBaseExpression : 1;
			uint8_t hasDisplacementExpression : 1;
		};
		uint8_t value;
	};
	static_assert(sizeof(DynamicOpcodeRVMControlByte) == 1, "ControlByte needs to be byte sized");

//============================================================================
} // namespace Javelin::x64Assembler
//============================================================================
