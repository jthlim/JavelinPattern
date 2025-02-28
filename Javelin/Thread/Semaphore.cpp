//============================================================================

#include "Javelin/JavelinBase.h"

//============================================================================

#if defined(JPLATFORM_WIN32) || defined(JPLATFORM_WIN64)
	#include "Javelin/Thread/Windows/Semaphore.cpp"
#elif defined(JPLATFORM_APPLEOS)
	#include "Javelin/Thread/appleOS/Semaphore.cpp"
#else
	#include "Javelin/Thread/Unix/Semaphore.cpp"
#endif

//============================================================================
