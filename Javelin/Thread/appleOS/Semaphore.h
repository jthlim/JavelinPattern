//===========================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/System/Assert.h"
#include <mach/mach.h>

//===========================================================================

namespace Javelin
{
//===========================================================================

	// Mac OSX pthread does NOT support sem_init!
	struct Semaphore
	{
	public:
		JEXPORT Semaphore(int n=0)		{ JVERIFY(semaphore_create(mach_task_self(), &data, SYNC_POLICY_FIFO, n) == KERN_SUCCESS);	}
		~Semaphore()					{ JVERIFY(semaphore_destroy(mach_task_self(), data) == KERN_SUCCESS);	}

		void JCALL BeginLock()	const	{ JVERIFY(semaphore_wait(data) == KERN_SUCCESS);				}
		bool JCALL TryLock()	const	{ return semaphore_timedwait(data, {0,0}) == KERN_SUCCESS;		}
		void JCALL EndLock()	const	{ JVERIFY(semaphore_signal(data) == KERN_SUCCESS);				}

		void JCALL Wait()		const	{ JVERIFY(semaphore_wait(data) == KERN_SUCCESS);				}
		void JCALL Signal()		const	{ JVERIFY(semaphore_signal(data) == KERN_SUCCESS);				}
		
		JEXPORT void JCALL Wait(size_t n) const;
		JEXPORT void JCALL Signal(size_t n) const;
		
	private:
		mutable semaphore_t data;
	};

//===========================================================================
} // namespace Javelin
//===========================================================================
