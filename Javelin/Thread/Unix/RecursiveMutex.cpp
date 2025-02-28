//===========================================================================

#include "Javelin/Thread/Unix/RecursiveMutex.h"

//===========================================================================

using namespace Javelin;

//===========================================================================

struct RecursiveMutexAttributes
{
	pthread_mutexattr_t attr;

	RecursiveMutexAttributes()
	{ 
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	}
	~RecursiveMutexAttributes()	{ pthread_mutexattr_destroy(&attr);		}
};

//===========================================================================

RecursiveMutex::RecursiveMutex()
{
	static const RecursiveMutexAttributes mutexAttributes;
	JVERIFY(pthread_mutex_init(&data, &mutexAttributes.attr) == 0);
}

//===========================================================================
