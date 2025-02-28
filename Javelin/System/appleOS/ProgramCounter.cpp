//============================================================================

#include "ProgramCounter.h"

//============================================================================

using namespace Javelin;

//============================================================================

ProgramCounter::Data ProgramCounter::data;

//============================================================================

__attribute__((noinline)) void ProgramCounter::Begin()
{
	data.address = __builtin_return_address(0);
}

__attribute__((noinline)) void ProgramCounter::End()
{
	void* p = __builtin_return_address(0);
	data.offset = size_t(p) - size_t(data.address);
}

//============================================================================

size_t ProgramCounter::CalculateOffset()
{
	ProgramCounter a;
	a.Begin();
	a.End();
	return data.offset;
}

size_t ProgramCounter::GetOffset()
{
	size_t offset = data.offset;
	static size_t successiveCallOffset = CalculateOffset();
	return offset - successiveCallOffset;
}

//============================================================================
