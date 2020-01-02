//============================================================================

#pragma once
#include <string>
#include <vector>

//============================================================================

namespace Javelin::Assembler
{
//============================================================================

#if defined(__clang__)
	#define JASM_CHECK_PRINTF_FORMAT __attribute__((__format__(__printf__, 2, 3)))
#else
	#define JASM_CHECK_PRINTF_FORMAT
#endif

	struct AssemblerExceptionLine
	{
		enum Type
		{
			Note,
			Error,
		};
		
		Type type = Error;
		int lineNumber = -1;
		int fileIndex = 0;
		std::string message;
		
		// Formats the error as "<filename>:<lineNumber>: error: message" if lineNumber != -1.
		// Else returns "error: message".
		std::string GetLogString(const std::vector<std::string> &filenameList) const;
		const std::string &GetTypeString() const;
	};
	
	class AssemblerException : public std::vector<AssemblerExceptionLine>
	{
	public:
		JASM_CHECK_PRINTF_FORMAT AssemblerException(const char* fmt, ...);
		AssemblerException(const std::string &aMessage);

		bool HasLocation() const;
		void PopulateLocationIfNecessary(int lineNumber, int fileIndex);
		void AddContext(const std::string &str);
		void AddContext(int lineNumber, int fileIndex, const std::string &str);
	};
	
#undef JASM_CHECK_PRINTF_FORMAT

//============================================================================
}
//============================================================================
