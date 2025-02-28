//============================================================================

#include "Javelin/Pattern/Internal/PatternDfaMemoryManager.h"
#include "Javelin/Pattern/Pattern.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

DfaMemoryManager::DfaMemoryManager()
: mode(DfaMemoryManagerMode::GlobalLimit)
{
}

DfaMemoryManager& DfaMemoryManager::GetInstance()
{
	static DfaMemoryManager instance;
	return instance;
}

//============================================================================

void DfaMemoryManager::AddProcessor(DfaProcessorBase& processor)
{
	GetInstance().processorList->PushFront(&processor);
}

void DfaMemoryManager::RemoveProcessor(DfaProcessorBase& processor)
{
	GetInstance().processorList->Remove(&processor);
}

//============================================================================

void DfaMemoryManager::SetConfiguration(DfaMemoryManagerMode aMode, size_t aLimit)
{
	DfaMemoryManager& instance = GetInstance();
	instance.processorList.Synchronize([&] {
		instance.mode = aMode;
		instance.limit = aLimit;
	});
}

void DfaMemoryManager::PreAllocate(size_t numberOfBytes, DfaProcessorBase& dfaProcessor)
{
	processorList.Synchronize([&] {
		processorList.MoveToFront(&dfaProcessor);
		
		totalAllocation += numberOfBytes;
		dfaProcessor.stateMemoryAllocated += numberOfBytes;
		
		switch(mode)
		{
		case DfaMemoryManagerMode::Unlimited:
			break;
			
		case DfaMemoryManagerMode::GlobalLimit:
			if(totalAllocation >= limit)
			{
				ResetAllProcessors(dfaProcessor);
			}
			break;
			
		case DfaMemoryManagerMode::PerPatternLimit:
			if(dfaProcessor.stateMemoryAllocated >= limit)
			{
				dfaProcessor.ResetStates(true);
			}
			break;
		}
	});
}

void* DfaMemoryManager::Allocate(size_t numberOfBytes, DfaProcessorBase& dfaProcessor)
{
	GetInstance().PreAllocate(numberOfBytes, dfaProcessor);
	return ::operator new(numberOfBytes);
}

void DfaMemoryManager::PreRelease(size_t numberOfBytes, DfaProcessorBase& dfaProcessor)
{
	JASSERT(processorList.TryLock() == false);
	totalAllocation -= numberOfBytes;
	dfaProcessor.stateMemoryAllocated -= numberOfBytes;
}

void DfaMemoryManager::Release(void* data, size_t numberOfBytes, DfaProcessorBase& dfaProcessor)
{
	GetInstance().PreRelease(numberOfBytes, dfaProcessor);
	::operator delete(data);
}

void DfaMemoryManager::OnReleaseAll(size_t numberOfBytes)
{
	DfaMemoryManager& instance = GetInstance();
	instance.processorList.Synchronize([&] {
		instance.totalAllocation -= numberOfBytes;
	});
}

//============================================================================

void DfaMemoryManager::ResetAllProcessors(DfaProcessorBase& originator)
{
	ProcessorList::Iterator it = processorList.Begin();
	while(it != processorList.End())
	{
		DfaProcessorBase& processor = *it;
		++it;
		
		if(processor.stateMemoryAllocated == 0) return;
		
		processor.ResetStates(&processor == &originator);
		if(processor.stateMemoryAllocated > 0)
		{
			processorList.MoveToFront(&processor);
		}
	}
}

//============================================================================
