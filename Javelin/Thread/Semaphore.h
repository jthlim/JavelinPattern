//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

#if defined(JPLATFORM_WIN32) || defined(JPLATFORM_WIN64)
	#include "Javelin/Thread/Windows/Semaphore.h"
#elif defined(JPLATFORM_APPLEOS)
	#include "Javelin/Thread/appleOS/Semaphore.h"
#else
	#include "Javelin/Thread/Unix/Semaphore.h"
#endif

//============================================================================
