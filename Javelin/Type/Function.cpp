//============================================================================

#include "Javelin/Type/Function.h"

//============================================================================

using namespace Javelin;

//============================================================================

void FunctionBase::Copy(const FunctionBase& a)
{
	if(a.runnable == (Runnable<void()>*) &a.storage)
	{
		storage = a.storage;
		runnable = (Runnable<void()>*) &storage;
	}
	else if(a.runnable != nullptr)
	{
		runnable = a.runnable;
		reinterpret_cast<ReferenceCountedRunnable<void()>*>(runnable)->AddReference();
	}
	else
	{
		runnable = nullptr;
	}
}

void FunctionBase::Move(FunctionBase&& a)
{
	if(a.runnable == (Runnable<void()>*) &a.storage)
	{
		storage = a.storage;
		runnable = (Runnable<void()>*) &storage;
	}
	else if(a.runnable != nullptr)
	{
		runnable = a.runnable;
		a.runnable = nullptr;
	}
	else
	{
		runnable = nullptr;
	}
}

void FunctionBase::Release()
{
	if(runnable == (Runnable<void()>*) &storage)
	{
		runnable->~Runnable();
	}
	else if(runnable != nullptr)
	{
		ReferenceCountedRunnable<void()>* p = reinterpret_cast<ReferenceCountedRunnable<void()>*>(runnable);
		if(p->RemoveReference()) delete runnable;
	}
}

//============================================================================
