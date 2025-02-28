//============================================================================

#include "Javelin/JavelinBase.h"

//============================================================================

#if defined(JPLATFORM_WIN32) || defined(JPLATFORM_WIN64)
	#include "Javelin/System/Windows/StackWalk.cpp"
#elif defined(JPLATFORM_POSIX)
	#include "Javelin/System/Unix/StackWalk.cpp"
#else
	#warning "No StackWalk implemented"
#endif

//============================================================================

