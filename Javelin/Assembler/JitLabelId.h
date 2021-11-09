//============================================================================

#pragma once
#include "JitHash.h"
#include <stdint.h>

//============================================================================

namespace Javelin
{
//===========================================================================

	// Labels are 32 bit identifiers.
	//  Upper 30 bits:Lower 2 bits
	//   Expression: label:0
	//   Indexed:    index:1		(non-global names and non-global numerics)
	//   Named:      hash(name):2
	//   Numeric:    label:3
	enum LabelType
	{
		Expression = 0,
		Indexed = 1,
		Named = 2,
		Numeric = 3,
	};
	
	inline bool IsLabelIdNamed(uint32_t labelId)			{ return (labelId & 3) == LabelType::Named; }
	inline uint32_t GetLabelIdForIndexed(uint32_t label)    { return (label << 2) | LabelType::Indexed; }
	uint32_t GetLabelIdForNamed(const char *label);
	inline uint32_t GetLabelIdForNumeric(uint32_t label)    { return (label << 2) | LabelType::Numeric; }
	inline uint32_t GetLabelIdForExpression(uint32_t label) { return (label << 2) | LabelType::Expression; }
	
	// For use by build time assembler
	inline int64_t  GetLabelIdForNumeric(int64_t label) 	    { return (label << 2) | LabelType::Numeric; }
	inline uint32_t GetLabelIdForGlobalNumeric(int64_t label) 	{ return uint32_t((label << 2) | LabelType::Numeric); }

    #define JASM_LABEL_ID(x) ((~JIT_HASH(x) << 2) | Javelin::LabelType::Named)

//============================================================================
} // namespace Javelin
//============================================================================
