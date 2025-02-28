//============================================================================

#include "Javelin/System/Unix/StackWalk.h"
#include "Javelin/System/Assert.h"
#include "Javelin/Type/String.h"
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <stdio.h>

//============================================================================

using namespace Javelin;

//============================================================================

StackWalkInformation::StackWalkInformation(void* address)
{
	Dl_info info;
	if(dladdr(address, &info) != 0)
	{
		offset = (char*) address - (char*) info.dli_saddr;
		DemangleFunctionNameToBuffer(info.dli_sname);
	}
}

StackWalkInformation::~StackWalkInformation()
{
	free(functionName);
}

void StackWalkInformation::DemangleFunctionNameToBuffer(const char* name)
{
	int status;
	functionName = abi::__cxa_demangle(name, nullptr, nullptr, &status);
	if(!functionName)
	{
		functionName = strdup(name);
	}
}

String StackWalkInformation::GetDescription() const
{
	const size_t MAXIMUM_BUFFER_SIZE = 2048;
	
	char buffer[MAXIMUM_BUFFER_SIZE];
	int length = snprintf(buffer, MAXIMUM_BUFFER_SIZE, "%s +0x%zu", GetFunctionName(), GetOffset());
	return String{buffer, size_t(length)};
}

//============================================================================

void StackWalk::Capture(size_t skipDepth)
{
	const size_t MAXIMUM_SKIP_DEPTH = 4;
	JASSERT(skipDepth <= MAXIMUM_SKIP_DEPTH);

	void* data[MAXIMUM_STACK_DEPTH + MAXIMUM_SKIP_DEPTH];
	size_t count = backtrace(data, MAXIMUM_STACK_DEPTH + MAXIMUM_SKIP_DEPTH);
	if(count > skipDepth + MAXIMUM_STACK_DEPTH) count = skipDepth + MAXIMUM_STACK_DEPTH;

	for(size_t i = skipDepth; i < count; ++i)
	{
		Inherited::Append(data[i]);
	}
}

//============================================================================
