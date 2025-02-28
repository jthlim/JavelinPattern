//============================================================================

#include "Javelin/JavelinBase.h"

//============================================================================

#if defined(JPLATFORM_WIN32) || defined(JPLATFORM_WIN64)
  #include "Javelin/Thread/Windows/RecursiveMutex.cpp"
// #elif defined(JPLATFORM_APPLEOS)
//	#include "Javelin/Thread/appleOS/RecursiveMutex.cpp"
#else
  #include "Javelin/Thread/Unix/RecursiveMutex.cpp"
#endif

//============================================================================
