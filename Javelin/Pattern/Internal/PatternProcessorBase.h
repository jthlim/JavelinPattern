//============================================================================

#pragma once
#include "Javelin/Pattern/Internal/PatternTypes.h"
#include "Javelin/Pattern/Internal/PatternData.h"

//============================================================================

#if defined(JABI_AMD64_SYSTEM_V)
  #define PATTERN_USE_JIT			1
#elif defined(JABI_ARM64_PCS)
  #define PATTERN_USE_JIT			1
#else
  #define PATTERN_USE_JIT           0
#endif

#define INITIAL_STACK_USAGE_LIMIT	(256*1024)
#define STACK_GROWTH_SIZE			(4*1024*1024)

// Space at the bottom of a STACK_GROWTH_SIZE alloc that won't be touched
// This needs to be large enough to call any functions inside StackGrowthHandler
#define STACK_GUARD_BAND			(64*1024)

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

	struct ByteCodeSearchData;
	class InstructionListBuilder;
	
	class PatternProcessorBase
	{
	public:
		virtual ~PatternProcessorBase() { }

		static const bool WORD_MASK[];

	protected:
		struct ExpandedJumpTables
		{
		public:
			void Set(const PatternData& patternData, size_t numberOfInstructions);
			uint32_t* GetJumpTable(size_t pc) const 					{ return jumpTableForPcList[pc]; }

		private:
			Table<uint32_t>		jumpTable;
			Table<uint32_t*>	jumpTableForPcList;
		};

		static const uintptr_t DFA_FAILED = 1;
	};

//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
