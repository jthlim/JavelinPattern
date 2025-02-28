//===========================================================================

#pragma once
#include "Javelin/System/Assert.h"
#include <pthread.h>

//===========================================================================

namespace Javelin
{
//===========================================================================

	struct Event
	{
	public:
		JINLINE Event() : cond(PTHREAD_COND_INITIALIZER), mutex(PTHREAD_MUTEX_INITIALIZER) { }
		JINLINE ~Event()				{ JVERIFY(pthread_cond_destroy(&cond) == 0); JVERIFY(pthread_mutex_destroy(&mutex) == 0); }
		
		JINLINE void BeginLock()		{ JVERIFY(pthread_mutex_lock(&mutex) == 0); 	}
		JINLINE void EndLock()			{ JVERIFY(pthread_mutex_unlock(&mutex) == 0); 	}
		
		JINLINE void Wait() 			{ BeginLock(); JVERIFY(pthread_cond_wait(&cond, &mutex) == 0); EndLock(); }
		JINLINE void WaitNoLock() 		{ JVERIFY(pthread_cond_wait(&cond, &mutex) == 0); 	}
		JINLINE void Signal() 			{ JVERIFY(pthread_cond_signal(&cond) == 0);			}
		
	private:
		pthread_cond_t cond;
		pthread_mutex_t mutex;
	};

//===========================================================================
} // namespace Javelin
//===========================================================================
