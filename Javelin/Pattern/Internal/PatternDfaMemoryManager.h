//============================================================================

#pragma once
#include "Javelin/Container/IntrusiveList.h"
#include "Javelin/Pattern/Internal/PatternByteCode.h"
#include "Javelin/Pattern/Internal/DfaProcessorBase.h"
#include "Javelin/Thread/Lock.h"
#include "Javelin/Thread/Mutex.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	enum class DfaMemoryManagerMode : uint8_t;
	
	namespace PatternInternal
	{
//============================================================================

		class DfaMemoryManager
		{
		public:
			static void AddProcessor(DfaProcessorBase& processor);
			static void RemoveProcessor(DfaProcessorBase& processor);
			
			static void SetConfiguration(DfaMemoryManagerMode mode, size_t limit=1024*1024);
			static void* Allocate(size_t numberOfBytes, DfaProcessorBase& dfaProcessor);
			static void  Release(void* data, size_t numberOfBytes, DfaProcessorBase& dfaProcessor);
			static void  OnReleaseAll(size_t numberOfBytes);
			
		private:
			DfaMemoryManager();

			static DfaMemoryManager& GetInstance();
			
			DfaMemoryManagerMode 	mode;
			size_t 					limit				= 1024*1024;
			size_t					totalAllocation		= 0;

			typedef IntrusiveList<DfaProcessorBase, &DfaProcessorBase::listNode> ProcessorList;
			Lock<ProcessorList> processorList;
			
			void ResetAllProcessors(DfaProcessorBase& originator);
			
			void PreAllocate(size_t numberOfBytes, DfaProcessorBase& dfaProcessor);
			void PreRelease(size_t numberOfBytes, DfaProcessorBase& dfaProcessor);
		};
	
//============================================================================
	} // namespace PatternInternal
} // namespace Javelin
//============================================================================
