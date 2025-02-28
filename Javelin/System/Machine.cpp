//============================================================================

#include "Javelin/JavelinBase.h"

//============================================================================

#if defined(JPLATFORM_WIN32) || defined(JPLATFORM_WIN64)
#include "Javelin/System/Windows/Machine.cpp"
#else
#include "Javelin/System/Unix/Machine.cpp"
#endif

//============================================================================
