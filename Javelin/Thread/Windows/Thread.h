//============================================================================

#pragma once
#include "Javelin/Type/Function.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class ThreadHandle
	{
	public:
		ThreadHandle();
		~ThreadHandle();

		void Start(const Function<void*(void*)>& threadFunction, void* threadParameter=nullptr);

		static void Run(void (*threadFunction)());
		static void Run(void (*threadFunction)(void*), void* threadParameter);

		static void Sleep(unsigned milliseconds);
		
		void*	Join();

	private:
		static DWORD WINAPI StartThreadNoParameters(LPVOID lpParam);

		HANDLE	thread;
	};

//============================================================================
} // namespace Javelin
//============================================================================
