//============================================================================

#include "Javelin/Tools/jasm/AssemblerException.h"
#include <stdarg.h>
#include <string.h>

//============================================================================

using namespace Javelin::Assembler;

//============================================================================

const std::string &AssemblerExceptionLine::GetTypeString() const
{
	static const std::string LUT[] = { "note: ", "error: " };
	return LUT[type];
}

std::string AssemblerExceptionLine::GetLogString(const std::vector<std::string> &filenameList) const
{
	if(lineNumber == -1)
	{
		return GetTypeString() + message;
	}
	else
	{
		return filenameList[fileIndex]
			+ ":"
			+ std::to_string(lineNumber)
			+ ": "
			+ GetTypeString()
			+ message;
	}
}

//============================================================================

AssemblerException::AssemblerException(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char buffer[10000];
	vsnprintf(buffer, 10000, fmt, args);

	va_end(args);
	
	push_back({ .message = buffer });
}

AssemblerException::AssemblerException(const std::string &aMessage)
{
	push_back({ .message = aMessage });
}

bool AssemblerException::HasLocation() const
{
	return front().lineNumber != -1;
}

void AssemblerException::PopulateLocationIfNecessary(int lineNumber, int fileIndex)
{
//	auto &entry = front();
//	if (entry.lineNumber == -1)
//	{
//		entry.lineNumber = lineNumber;
//		entry.fileIndex = fileIndex;
//	}

	for(auto &entry : *this)
	{
		if (entry.lineNumber == -1)
		{
			entry.lineNumber = lineNumber;
			entry.fileIndex = fileIndex;
		}
	}
}

void AssemblerException::AddContext(const std::string &str)
{
	push_back({
		.type = AssemblerExceptionLine::Note,
		.message = str
	});
}

void AssemblerException::AddContext(int lineNumber, int fileIndex, const std::string &str)
{
	push_back({
		.type = AssemblerExceptionLine::Note,
		.lineNumber = lineNumber,
		.fileIndex = fileIndex,
		.message = str
	});
}

//============================================================================
