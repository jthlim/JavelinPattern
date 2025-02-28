//============================================================================

#include "Javelin/System/Assert.h"
#include "Javelin/System/StackWalk.h"
#include "Javelin/Type/String.h"
#include <stdio.h>

//============================================================================

void Javelin::Assert(const char* message, const char* fileName, unsigned lineNumber, const char* function)
{
#if defined(JPLATFORM_WIN32) || defined(JPLATFORM_WIN64)
	char buffer[1024];
	sprintf_s(buffer, 1024, "%s(%d) : Assert : %s : %s\n", fileName, lineNumber, function, message);
	OutputDebugStringA(buffer);
#else
	printf("%s(%d) : Assert : %s : %s\n", fileName, lineNumber, function, message);
#endif
    
    StackWalk stackWalk;
    stackWalk.Capture();
    
    for(void* p : stackWalk)
    {
        StackWalkInformation info(p);
        if(info.IsValid())
        {
#if defined(JPLATFORM_WIN32) || defined(JPLATFORM_WIN64)
            char buffer[1024];
            sprintf_s(buffer, 1024, " %s\n", info.GetDescription().AsUtf8String());
            OutputDebugStringA(buffer);
#else
            printf(" %s\n", info.GetDescription().AsUtf8String());
#endif
        }
    }

	fflush(NULL);
	
	BreakPoint();
}

//============================================================================

