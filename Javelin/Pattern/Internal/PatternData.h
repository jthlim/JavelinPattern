//============================================================================

#pragma once
#include "Javelin/Pattern/Internal/PatternByteCode.h"

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

	struct PatternData
	{
	public:
		PatternData() = default;
		PatternData(const void* aP) : p((const unsigned char*) aP)	{ }
		
		const ByteCodeInstruction& operator[](size_t i) const		{ return ((ByteCodeInstruction*) p)[i];	}
		template<typename T> const T* GetData(size_t offset) const 	{ return (T*) (p+offset); 				}

		bool IsSingleByteConsumerFromPcToPc(int startPc, int endPc) const;
		
		bool HasLookupTableTarget(const ByteCodeJumpTableData& a) const;
		bool HasLookupTableTarget(const ByteCodeJumpMaskData& a) const;
		uint32_t GetMaximalJumpTargetForPc(uint32_t pc) const;

		bool ContainsInstruction(InstructionType type, uint32_t numberOfInstructions) const;
		
		int CountSingleReferenceInstructions(int offset, InstructionType type, uint32_t numberOfInstructions) const;
		int CountSingleReferenceInstructions(int offset, InstructionType type, uint32_t instructionData, uint32_t numberOfInstructions) const;

		const unsigned char*	p;
	};
			
//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
