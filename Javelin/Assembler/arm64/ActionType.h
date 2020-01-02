//============================================================================

#pragma once
#include <stdint.h>

//============================================================================

namespace Javelin::arm64Assembler
{
//============================================================================
	
	enum class ActionType : uint8_t
	{
		#define TAG(x) x,
		#include "ActionTypeTags.h"
		#undef TAG
	};

//============================================================================

	struct BitMaskEncodeResult
	{
		int size;			// 0 means invalid input value.
		int length;
		int rotate;
	};
	
	BitMaskEncodeResult EncodeBitMask(uint64_t value);

//============================================================================
} // namespace Javelin::arm64Assembler
//============================================================================
