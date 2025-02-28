//===========================================================================

#pragma once
#include "Javelin/System/Assert.h"
#include <pthread.h>

//===========================================================================

namespace Javelin
{
//===========================================================================

	class ReadWriteLock
	{
	public:
		ReadWriteLock()						{ JVERIFY(pthread_rwlock_init(&data, nullptr) == 0); }
		~ReadWriteLock()					{ JVERIFY(pthread_rwlock_destroy(&data) == 0);	}
		
		JINLINE void BeginReadLock() const	{ JVERIFY(pthread_rwlock_rdlock(&data) == 0);	}
		JINLINE void EndReadLock() const	{ JVERIFY(pthread_rwlock_unlock(&data) == 0);	}
		JINLINE void BeginWriteLock() const	{ JVERIFY(pthread_rwlock_wrlock(&data) == 0);	}
		JINLINE void EndWriteLock() const	{ JVERIFY(pthread_rwlock_unlock(&data) == 0);	}
		
	private:
		mutable pthread_rwlock_t data;
	};

//===========================================================================
} // namespace Javelin
//===========================================================================
