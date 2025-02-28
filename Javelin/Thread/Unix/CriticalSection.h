//===========================================================================

#pragma once
#include "Javelin/System/Assert.h"
#include <pthread.h>

//===========================================================================

namespace Javelin
{
//===========================================================================

	// Use the same implementation for Critical Sections and Mutexes, except we don't allow recursion.
	class CriticalSection
	{
	public:
		JINLINE constexpr CriticalSection() : data(PTHREAD_MUTEX_INITIALIZER) { }
		JINLINE ~CriticalSection()		{ JVERIFY(pthread_mutex_destroy(&data) == 0);	}
		
		JINLINE void BeginLock() const	{ JVERIFY(pthread_mutex_lock(&data) == 0);		}
		JINLINE bool TryLock() const	{ return pthread_mutex_trylock(&data) == 0;		}
		JINLINE void EndLock() const	{ JVERIFY(pthread_mutex_unlock(&data) == 0);	}
		
		template<typename T>
		JINLINE void Synchronize(T&& callback) const
		{
			BeginLock();
			callback();
			EndLock();
		}
	
	private:
		mutable pthread_mutex_t data;
	};

//===========================================================================
} // namespace Javelin
//===========================================================================
