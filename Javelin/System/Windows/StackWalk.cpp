//============================================================================

#include "Javelin/System/StackWalk.h"
#include "Javelin/System/Assert.h"
#include "Javelin/Template/Memory.h"
#include <dbghelp.h>
#include <stdio.h>
#include <Psapi.h>

//============================================================================

using namespace Javelin;

//============================================================================

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "DbgHelp.lib")

#undef StackWalk

//============================================================================

static const HANDLE hProcess = GetCurrentProcess();

bool StackWalk::needToInitializeSymbols = true;

//============================================================================

StackWalkInformation::StackWalkInformation(void* address)
{
	static_assert(sizeof(imageHlpLine64Buffer) >= sizeof(IMAGEHLP_LINE64), "imageHlpLine64Buffer is not big enough!");

	IMAGEHLP_LINE64* buffer = reinterpret_cast<IMAGEHLP_LINE64*>(imageHlpLine64Buffer);
	buffer->SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	isDataValid = (SymGetLineFromAddr64(hProcess, (DWORD64) address, (PDWORD) &displacement, buffer) != 0);
}

const char* StackWalkInformation::GetFileName() const
{
	return reinterpret_cast<const IMAGEHLP_LINE64*>(imageHlpLine64Buffer)->FileName;
}

int StackWalkInformation::GetLineNumber() const
{
	return reinterpret_cast<const IMAGEHLP_LINE64*>(imageHlpLine64Buffer)->LineNumber;
}

String StackWalkInformation::GetDescription() const
{
    char buffer[1024];
    sprintf(buffer, "%s (%d)", GetFileName(), GetLineNumber());
    return String{buffer};
}

//============================================================================

void StackWalk::Capture(size_t skipDepth)
{
	if(needToInitializeSymbols) InitializeSymbols();

	CONTEXT context;
	RtlCaptureContext(&context);

	STACKFRAME64 stackFrame64;
	ClearMemory(stackFrame64);

#if defined(JPLATFORM_WIN32)
	stackFrame64.AddrPC.Offset = context.Eip;
	stackFrame64.AddrPC.Mode = AddrModeFlat;
	stackFrame64.AddrFrame.Offset = context.Ebp;
	stackFrame64.AddrFrame.Mode = AddrModeFlat;
	stackFrame64.AddrStack.Offset = context.Esp;
	stackFrame64.AddrStack.Mode = AddrModeFlat;

	const DWORD MACHINE_TYPE = IMAGE_FILE_MACHINE_I386;

#elif defined(JPLATFORM_WIN64)
	stackFrame64.AddrPC.Offset = context.Rip;
	stackFrame64.AddrPC.Mode = AddrModeFlat;
	stackFrame64.AddrFrame.Offset = context.Rbp;
	stackFrame64.AddrFrame.Mode = AddrModeFlat;
	stackFrame64.AddrStack.Offset = context.Rsp;
	stackFrame64.AddrStack.Mode = AddrModeFlat;

	const DWORD MACHINE_TYPE = IMAGE_FILE_MACHINE_AMD64;
	++skipDepth;
#else
	#error "Unsupported platform - unable to setup stack frame"
#endif

	HANDLE hThread = GetCurrentThread();

	for(size_t i = 0; i < skipDepth; ++i)
	{
		if(!StackWalk64(MACHINE_TYPE, hProcess, hThread, &stackFrame64, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) return;
	}

	for(size_t i = 0; i < MAXIMUM_STACK_DEPTH; ++i)
	{
		if(!StackWalk64(MACHINE_TYPE, hProcess, hThread, &stackFrame64, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) return;

		addresses.Append((void*) stackFrame64.AddrPC.Offset);
	}
}

void StackWalk::InitializeSymbols()
{
	SymSetOptions(SYMOPT_DEBUG | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
	SymInitialize(hProcess, nullptr, TRUE);

	char baseName[MAX_PATH+1];
	GetModuleFileNameExA(hProcess, nullptr, baseName, MAX_PATH);
	SymLoadModule64(hProcess, nullptr, baseName, nullptr, 0, 0);

	needToInitializeSymbols = false;
}

//============================================================================
