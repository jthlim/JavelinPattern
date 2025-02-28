//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

#if defined(JPLATFORM_WIN32) || defined(JPLATFORM_WIN64)
	#include "Javelin/System/Windows/StackWalk.h"
#elif defined(JPLATFORM_POSIX)
	#include "Javelin/System/Unix/StackWalk.h"
#else
	#warning "No StackWalk implemented"
#endif

//============================================================================

