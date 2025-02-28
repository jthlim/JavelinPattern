//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/System/Assert.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class Mutex
	{
	public:
		Mutex()							{ handle = CreateMutex(NULL, FALSE, NULL); JASSERT(handle);	}
		~Mutex()						{ CloseHandle(handle);							}

		void JCALL BeginLock()	const	{ WaitForSingleObject(handle, INFINITE); 		}
		bool JCALL TryLock()	const	{ return WaitForSingleObject(handle, 0) == 0;	}
		void JCALL EndLock()	const	{ ReleaseMutex(handle);							}
		
	private:
		mutable HANDLE	handle;
	};

//============================================================================
} // namespace Javelin
//============================================================================
