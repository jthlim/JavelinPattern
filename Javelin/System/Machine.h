//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

#if defined(JPLATFORM_WIN32) || defined(JPLATFORM_WIN64)
	#include "Javelin/System/Windows/Machine.h"
#else
	#include "Javelin/System/Unix/Machine.h"
#endif

//============================================================================
