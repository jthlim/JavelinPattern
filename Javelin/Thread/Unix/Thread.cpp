//============================================================================

#include "Javelin/Thread/Thread.h"
#include "Javelin/Thread/Barrier.h"
#include "Javelin/Type/Atomic.h"
#include <pthread.h>
#include <unistd.h>

//============================================================================

using namespace Javelin;

//============================================================================

// On ARM64, It is possible for this Start() to be called before td->thread is
// populated. This wait makes sure that any calls made in ThreadData::Start
// have a valid thread variable. This was causing a problem when a thread
// deleted itself, caught by DynamicThread unit test in UnitTest/Thread/Thread.cpp
#if defined(JASM_GNUC_ARM64)
  #define WAIT_AFTER_THREAD_START 1
#else
  #define WAIT_AFTER_THREAD_START 0
#endif

struct ThreadHandle::ThreadData
{
	pthread_t					thread;
	Atomic<unsigned>			counter;
#if WAIT_AFTER_THREAD_START
	Barrier						barrier;
#endif
	Function<void*(void*)> 		threadFunction;
	void*						threadParameter;
	
	ThreadData() : thread(0), counter(1) { }
	void RemoveReference()
	{
		if(--counter == 0) delete this;
	}
	
	static void* Start(void* data)
	{
		ThreadData* td = (ThreadData*) data;
		
#if WAIT_AFTER_THREAD_START
		td->barrier.Wait();
#endif
		void* result = td->threadFunction(td->threadParameter);
		td->RemoveReference();
		return result;
	}
};

//============================================================================

ThreadHandle::ThreadHandle()
{
	threadData = new ThreadData;
}

//============================================================================

ThreadHandle::~ThreadHandle()
{
	if(threadData->thread != 0)
	{
		JVERIFY(pthread_detach(threadData->thread) == 0);
	}
	threadData->RemoveReference();
}

//============================================================================

void ThreadHandle::Start(const Function<void*(void*)>& threadFunction, void* threadParameter)
{
	threadData->threadFunction = threadFunction;
	threadData->threadParameter = threadParameter;
	threadData->counter++;
	JVERIFY(pthread_create(&threadData->thread, nullptr, &ThreadData::Start, threadData) == 0);
#if WAIT_AFTER_THREAD_START
	threadData->barrier.Signal();
#endif
}

bool ThreadHandle::IsActive() const
{
	return pthread_self() == threadData->thread;
}

void ThreadHandle::Run(void (*threadFunction)())
{
	pthread_t thread;
	JVERIFY(pthread_create(&thread, nullptr, (void*(*)(void*)) threadFunction, nullptr) == 0);
	JVERIFY(pthread_detach(thread) == 0);
}

void ThreadHandle::Run(void (*threadFunction)(void*), void* threadParameter)
{
	pthread_t thread;
	JVERIFY(pthread_create(&thread, nullptr, (void*(*)(void*)) threadFunction, threadParameter) == 0);
	JVERIFY(pthread_detach(thread) == 0);
}

//============================================================================

void* ThreadHandle::Join()
{
	void* exitCode;
	JVERIFY(pthread_join(threadData->thread, &exitCode) == 0);
	threadData->thread = 0;
	return exitCode;
}

//============================================================================

void ThreadHandle::Sleep(unsigned milliseconds)
{
	usleep(milliseconds * 1000);
}

//============================================================================

#if defined(JPLATFORM_APPLEOS)

void Thread::Yield()
{
	sched_yield();
}

#endif

//============================================================================
