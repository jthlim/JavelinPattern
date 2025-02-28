//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class CriticalSection
	{
	public:
		CriticalSection()				{ InitializeCriticalSection(&cs);		}
		~CriticalSection()				{ DeleteCriticalSection(&cs);			}

		void JCALL BeginLock()	const	{ EnterCriticalSection(&cs);			}
		bool JCALL TryLock()	const	{ return TryEnterCriticalSection(&cs); 	}
		void JCALL EndLock()	const	{ LeaveCriticalSection(&cs);			}
		
	private:
		CRITICAL_SECTION cs;
	};

//============================================================================
} // namespace Javelin
//============================================================================
