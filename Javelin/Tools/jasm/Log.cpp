//============================================================================

#include "Log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

//============================================================================

using namespace Javelin::Assembler;

//============================================================================

#define ANSI_BLACK		"\e[30m"
#define ANSI_RED		"\e[31m"
#define ANSI_GREEN		"\e[32m"
#define ANSI_YELLOW		"\e[33m"
#define ANSI_BLUE		"\e[34m"
#define ANSI_MAGENTA	"\e[35m"
#define ANSI_CYAN		"\e[36m"
#define ANSI_GREY		"\e[37m"
#define ANSI_EOL		"\e[m\n"

static const char *const LEVEL_NAMES[] =
{
	"",
	"[VERBOSE] ",
	"[DEBUG]   ",
	"[INFO]    ",
	"[WARNING] ",
	""
};
static const char *const ANSI_COLORS_BY_LEVEL[] =
{
	"",
	ANSI_GREY,
	ANSI_GREEN,
	ANSI_BLUE,
	ANSI_YELLOW,
	ANSI_RED,
};

FILE* Log::fd = stderr;
LogLevel Log::level = LogLevel::Info;
const char *const *Log::PREFIX_LIST = Log::PrefixList();

size_t Log::numberOfWarnings = 0;
size_t Log::numberOfErrors = 0;

//============================================================================

bool Log::ShouldUseAnsi(FILE* fd)
{
	if(!isatty(fileno(fd))) return false;
	
	const char* term = getenv("TERM");
	if(!term) return false;
	
	return strcmp(term, "dumb") != 0;
}

const char* const *Log::PrefixList()
{
	return ShouldUseAnsi(fd) ? ANSI_COLORS_BY_LEVEL : LEVEL_NAMES;
}

//============================================================================

void Log::SetFile(FILE* f)
{
	setbuf(f, nullptr);
	fd = f;
	PREFIX_LIST = PrefixList();
}

//============================================================================

// This writes the text in one single write call to avoid multithreaded log interleaving
void Log::Write(LogLevel level, const char* s, va_list a)
{
	char buffer[10100];
	char* p = buffer;
	
	const char* prefix = PREFIX_LIST[(int) level];
	strcpy(buffer, prefix);
	p += strlen(prefix);
	
	size_t length = vsnprintf(p, 10000, s, a);
	if(length > 10000) length = 10000;
	p += length;
	
	if(PREFIX_LIST == ANSI_COLORS_BY_LEVEL)
	{
		strcpy(p, ANSI_EOL);
		p += sizeof(ANSI_EOL)-1;
	}
	else
	{
		*p++ = '\n';
	}
	
	fwrite(buffer, 1, p-buffer, fd);
	
	if(level == LogLevel::Error) ++numberOfErrors;
	if(level == LogLevel::Warning) ++numberOfWarnings;
}

void Log::VText(const char* s, va_list args)
{
	char buffer[10002];
	
	size_t length = vsnprintf(buffer, 10000, s, args);
	if(length > 10000) length = 10000;
	char* p = buffer+length;
	*p++ = '\n';
	
	fwrite(buffer, 1, p-buffer, fd);
}

void Log::Text(const char* s, ...)
{
	va_list args;
	va_start(args, s);
	VText(s, args);
	va_end(args);
}

void Log::LiteralText(const char* s)
{
	char buffer[10002];
	size_t length = strlen(s);
	if(length > 10000) length = 10000;
	memcpy(buffer, s, length);
	buffer[length] = '\n';
	
	fwrite(buffer, 1, length+1, fd);	
}

//============================================================================

void Log::Verbose(const char* s, ...)
{
	if(level > LogLevel::Verbose) return;
	va_list args;
	va_start(args, s);
	Write(LogLevel::Verbose, s, args);
	va_end(args);
}

void Log::VVerbose(const char* s, va_list a)
{
	if(level > LogLevel::Verbose) return;
	Write(LogLevel::Verbose, s, a);
}

void Log::Debug(const char* s, ...)
{
	if(level > LogLevel::Debug) return;
	va_list args;
	va_start(args, s);
	Write(LogLevel::Debug, s, args);
	va_end(args);
}

void Log::VDebug(const char* s, va_list a)
{
	if(level > LogLevel::Debug) return;
	Write(LogLevel::Debug, s, a);
}

void Log::Info(const char* s, ...)
{
	if(level > LogLevel::Info) return;
	va_list args;
	va_start(args, s);
	Write(LogLevel::Info, s, args);
	va_end(args);
}

void Log::VInfo(const char* s, va_list a)
{
	if(level > LogLevel::Info) return;
	Write(LogLevel::Info, s, a);
}

void Log::Warning(const char* s, ...)
{
	if(level > LogLevel::Warning) return;
	va_list args;
	va_start(args, s);
	Write(LogLevel::Warning, s, args);
	va_end(args);
}

void Log::VWarning(const char* s, va_list a)
{
	if(level > LogLevel::Warning) return;
	Write(LogLevel::Warning, s, a);
}

void Log::Error(const char* s, ...)
{
	if(level > LogLevel::Error) return;
	va_list args;
	va_start(args, s);
	Write(LogLevel::Error, s, args);
	va_end(args);
}

void Log::VError(const char* s, va_list a)
{
	if(level > LogLevel::Error) return;
	Write(LogLevel::Error, s, a);
}

//============================================================================

void Log::DebugData(const void* data, size_t length)
{
	const unsigned char* p = static_cast<const unsigned char*>(data);
	
	size_t i = 0;
	
	while(i < length)
	{
		if(i % 16 == 0)
		{
			fprintf(fd, "%08zx: ", i);
		}
		fprintf(fd, i % 8 == 0 ? "  %02x" : " %02x", p[i]);
		++i;
		
		if(i % 16 == 0)
		{
			fprintf(fd, "   |");
			const unsigned char* x = p + i - 16;
			for(int a = 0; a < 16; ++a)
			{
				putc(' ' <= x[a] && x[a] < 0x7f ? x[a] : '.', fd);
			}
			fprintf(fd, "|\n");
		}
	}
	
	size_t remainder = i % 16;
	if(remainder != 0)
	{
		static const char SPACES[] = "                                                 |\n";
		size_t filler = (16 - remainder) * 3 + 4;
		if(remainder <= 8) filler++;
		fwrite((SPACES+sizeof(SPACES)-2) - filler, 1, filler, fd);
		const unsigned char* x = p + i - remainder;
		for(size_t a = 0; a < remainder; ++a)
		{
			putc(' ' <= x[a] && x[a] < 0x7f ? x[a] : '.', fd);
		}
		size_t endCharacterCount = 18-remainder;
		fwrite((SPACES+sizeof(SPACES)-1) - endCharacterCount, 1, endCharacterCount, fd);
	}
}

//============================================================================

void Log::SetLogLevel(LogLevel newLevel)
{
	if(level == newLevel) return;
 	level = newLevel;
	Log::Info("Switching to log level %s", LEVEL_NAMES[(int) level]);
}

//============================================================================

void Log::SetLogFileImmediate(const char* logFile)
{
	FILE* oldFile = fd;

	if(strcmp(logFile, "stdout") == 0) SetFile(stdout);
	else if(strcmp(logFile, "stderr") == 0) SetFile(stderr);
	else
	{
		FILE* newFile = fopen(logFile, "a+t");
		if(!newFile)
		{
			Error("Unable to open log: %s", logFile);
			return;
		}

		SetFile(newFile);
	}
	if(oldFile != stdout
	   && oldFile != stderr) fclose(oldFile);
}

void Log::SetLogFile(const char* logFile)
{
	Info("Closing log file");
	SetLogFileImmediate(logFile);
}

//============================================================================
