//============================================================================

#include "Javelin/System/Windows/Machine.h"

//============================================================================

using namespace Javelin;

//============================================================================

size_t Machine::GetPageSize()
{
	SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
	
    return SystemInfo.dwPageSize;
}

size_t Machine::GetNumberOfProcessors()
{
	SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
	
    return SystemInfo.dwNumberOfProcessors;
}

//============================================================================
