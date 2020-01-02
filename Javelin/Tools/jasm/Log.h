//============================================================================

#pragma once
#include "File.h"
#include <stdio.h>
#include <stdarg.h>

//============================================================================

namespace Javelin::Assembler
{
//============================================================================
	
#if defined(__clang__)
	#define CHECK_STATIC_PRINTF_FORMAT	__attribute__((__format__ (__printf__, 1, 2)))
	#define CHECK_PRINTF_FORMAT			__attribute__((__format__ (__printf__, 2, 3)))
#else
	#define CHECK_STATIC_PRINTF_FORMAT
    #define CHECK_PRINTF_FORMAT
#endif

	/// Logging utilities
	
	/// Logging level can be changed via command line parameter "-l" or via
	/// configuration file.
	///
	/// Supported logging levels:
	/// - Verbose (all)
	/// - Debug
	/// - Info
	/// - Warning
	/// - Error
	/// - None
	///
	/// Log uses ANSI coloration if possible.
    
    enum class LogLevel
    {
        Unspecified,
        Verbose,
        Debug,
        Info,
        Warning,
        Error,
		None
    };

	/// Provides convenient static methods to log
	class Log
	{
	public:
		static LogLevel GetLogLevel() { return level; }
		
		static void SetLogLevel(LogLevel newLevel);
		static void SetLogLevelImmediate(LogLevel newLevel) { level = newLevel; }
		
		static void Write(LogLevel level, const char* s, va_list a);

		static bool IsVerboseEnabled() { return level <= LogLevel::Verbose; }
		static void VVerbose(const char* s, va_list a);
		CHECK_STATIC_PRINTF_FORMAT static void Verbose(const char* s, ...);

		static bool IsDebugEnabled() { return level <= LogLevel::Debug; }
		static void VDebug(const char* s, va_list a);
		CHECK_STATIC_PRINTF_FORMAT static void Debug(const char* s, ...);
		
		static bool IsInfoEnabled() { return level <= LogLevel::Info; }
		static void VInfo(const char* s, va_list a);
		CHECK_STATIC_PRINTF_FORMAT static void Info(const char* s, ...);

		static bool IsWarningEnabled() { return level <= LogLevel::Warning; }
		static void VWarning(const char* s, va_list a);
		CHECK_STATIC_PRINTF_FORMAT static void Warning(const char* s, ...);
		static size_t GetNumberOfWarnings() { return numberOfWarnings; }

		static bool IsErrorEnabled()  { return level <= LogLevel::Error; }
		static void VError(const char* s, va_list a);
		CHECK_STATIC_PRINTF_FORMAT static void Error(const char* s, ...);
		static void IncrementErrorCount() { ++numberOfErrors; }
		static size_t GetNumberOfErrors() { return numberOfErrors; }
		
		static void VText(const char* s, va_list a);
		CHECK_STATIC_PRINTF_FORMAT static void Text(const char* s, ...);
		
		static void LiteralText(const char* s);
		
		// Prints out a hex dump
		static void DebugData(const void* data, size_t length);

		static void SetLogFileImmediate(const char* logFile);
		static void SetLogFile(const char* logFile);

	protected:
		static FILE* fd;
		static LogLevel level;
		
		static const char *const *PREFIX_LIST;
		
	private:
		static bool ShouldUseAnsi(FILE* fd);
		
		static void SetFile(FILE* f);
		static const char* const *PrefixList();
		
		static size_t numberOfWarnings;
		static size_t numberOfErrors;
	};
	
#undef CHECK_STATIC_PRINTF_FORMAT
#undef CHECK_PRINTF_FORMAT

//============================================================================
}
//============================================================================
