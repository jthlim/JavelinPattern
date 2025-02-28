//===========================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/System/Assert.h"
#include <semaphore.h>

//===========================================================================

namespace Javelin
{
//===========================================================================

	struct Semaphore
	{
	public:
		JEXPORT Semaphore(int n=0)		{ JVERIFY(sem_init(&data, 0, n) != -1);	}
		~Semaphore()					{ JVERIFY(sem_destroy(&data) != -1);	}

		void JCALL BeginLock()	const	{ JVERIFY(sem_wait(&data) != -1);		}
		bool JCALL TryLock()	const	{ return sem_trywait(&data) == 0;		}
		void JCALL EndLock()	const	{ JVERIFY(sem_post(&data) != -1);		}

		void JCALL Wait()		const	{ JVERIFY(sem_wait(&data) != -1);		}
		void JCALL Signal()		const	{ JVERIFY(sem_post(&data) != -1);		}
		
		JEXPORT void JCALL Wait(size_t n) const;
		JEXPORT void JCALL Signal(size_t n) const;

	private:
		mutable sem_t data;
	};

//===========================================================================
} // namespace Javelin
//===========================================================================
