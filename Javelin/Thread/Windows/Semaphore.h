//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

namespace Javelin
{
//============================================================================

	struct Semaphore
	{
	public:
		Semaphore(int n=0)				{ handle = CreateSemaphore(NULL, n, 32767, NULL);	}
		~Semaphore()					{ CloseHandle(handle);								}

		void JCALL BeginLock()	const	{ WaitForSingleObject(handle, INFINITE);	}
		void JCALL EndLock()	const	{ ReleaseSemaphore(handle, 1, NULL);		}
		
	private:
		mutable HANDLE	handle;
	};

//============================================================================
} // namespace Javelin
//============================================================================
