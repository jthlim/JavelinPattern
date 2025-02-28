//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

#if defined(JPLATFORM_WIN32) || defined(JPLATFORM_WIN64)
	#include "Javelin/Thread/Windows/CriticalSection.h"
#else
	#include "Javelin/Thread/Unix/CriticalSection.h"
#endif

//============================================================================
