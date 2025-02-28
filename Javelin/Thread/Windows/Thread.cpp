//============================================================================

#include "Javelin/Thread/Windows/Thread.h"

//============================================================================

using namespace Javelin;

//============================================================================

ThreadHandle::ThreadHandle()
{
	thread = nullptr;
}

//============================================================================

ThreadHandle::~ThreadHandle()
{
	CloseHandle(thread);
}

//============================================================================

void ThreadHandle::Run(void (*threadFunction)())
{
	CloseHandle(CreateThread(nullptr, 0, &ThreadHandle::StartThreadNoParameters, (void*) threadFunction, 0, nullptr));
}

void ThreadHandle::Run(void (*threadFunction)(void*), void* threadParameter)
{
}

//============================================================================

void* ThreadHandle::Join()
{
	DWORD exitCode;
	WaitForSingleObject(thread, INFINITE);
	GetExitCodeThread(thread, reinterpret_cast<unsigned long*>(&exitCode));

	CloseHandle(thread);
	thread = nullptr;

	return (void*) exitCode;
}

//============================================================================

DWORD ThreadHandle::StartThreadNoParameters(LPVOID lpParam)
{
	void (*threadFunction)() = (void(*)()) lpParam;
	(*threadFunction)();
	return 0;
}

//============================================================================
