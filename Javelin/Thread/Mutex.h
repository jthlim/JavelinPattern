//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

#if defined(JPLATFORM_WIN32) || defined(JPLATFORM_WIN64)
	#include "Javelin/Thread/Windows/Mutex.h"
//#elif defined(JPLATFORM_APPLEOS)
//	#include "Javelin/Thread/appleOS/Mutex.h"
#else
	#include "Javelin/Thread/Unix/Mutex.h"
#endif

//============================================================================
