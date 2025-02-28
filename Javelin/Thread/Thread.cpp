//============================================================================

#include "Javelin/Thread/Thread.h"

//============================================================================

#if defined(JPLATFORM_WIN32) || defined(JPLATFORM_WIN64)
	#include "Javelin/Thread/Windows/Thread.cpp"
#else
	#include "Javelin/Thread/Unix/Thread.cpp"
#endif

//============================================================================

using namespace Javelin;

//============================================================================

void Thread::ExecuteRunnable(void* parameter)
{
	Runnable<void(void)>& task = *reinterpret_cast<Runnable<void(void)>*>(parameter);
	task();
}

//============================================================================

void* IThread::EntryPoint(IThread* p)
{
	p->Run();
	return nullptr;
}

//============================================================================
