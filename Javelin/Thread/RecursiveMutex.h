//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

#if defined(JPLATFORM_WIN32) || defined(JPLATFORM_WIN64)
	#include "Javelin/Thread/Windows/RecursiveMutex.h"
//#elif defined(JPLATFORM_APPLEOS)
//	#include "Javelin/Thread/appleOS/RecursiveMutex.h"
#else
	#include "Javelin/Thread/Unix/RecursiveMutex.h"
#endif

//============================================================================
