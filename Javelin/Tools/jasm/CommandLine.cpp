//============================================================================

#include "CommandLine.h"
#include "Character.h"
#include "Log.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

//============================================================================

using namespace Javelin::Assembler;

//============================================================================

CommandLine* CommandLine::instance = NULL;

//============================================================================

static const char VERSION_TEXT[] = "Javelin Assembler Copyright (C) Jeffrey Lim 2020. Built " __DATE__ " " __TIME__;

//============================================================================

AssemblerType::AssemblerType(const std::string& typeString)
{
	type = Unknown;

	const char *data = typeString.data();
	const char *end = data + typeString.size();
	
	while(data < end && Character::IsWhitespace(*data)) ++data;

	if(data >= end || *data != '.') return;
	++data;
	
	if(HasPrefix(data, end, "x64", 3))
	{
		type = X64;
		return;
	}
	if(HasPrefix(data, end, "arm64", 5))
	{
		type = Arm64;
		return;
	}
}

bool AssemblerType::HasPrefix(const char *data, const char *end, const char *prefix, int length)
{
	if(data + length > end) return false;
	if(memcmp(data, prefix, length) != 0) return false;
	if(data + length == end) return true;
	return Character::IsWhitespace(data[length]);
}

//============================================================================

CommandLine::CommandLine(int argc, const char** argv)
{
	assert(instance == NULL);
	instance = this;
	
	if(argc <= 1) ShowHelpTextAndExit();
	
	for(int i = 1; i < argc;)
	{
		if(strcmp(argv[i], "-h") == 0)
		{
			ShowHelpTextAndExit();
		}
		else if(strcmp(argv[i], "-f") == 0)
		{
			if(++i >= argc)
			{
				Log::Error("Missing command line parameter for input source filename");
				exit(1);
			}
			inputFilename = argv[i++];
		}
		else if(strcmp(argv[i], "-l") == 0)
		{
			if(++i >= argc)
			{
				Log::Error("Missing command line parameter for log level");
				exit(1);
			}
			ProcessLogLevel(argv[i++]);
		}
		else if(strcmp(argv[i], "-o") == 0)
		{
			if(++i >= argc)
			{
				Log::Error("Missing command line parameter for output source filename");
				exit(1);
			}
			outputFilename = argv[i++];
		}
		else if(strcmp(argv[i], "-O") == 0)
		{
			useUnsafeOptimizations = true;
			++i;
		}
		else if(strcmp(argv[i], "-Ofast") == 0)
		{
			useSpeedOptimizations = true;
			++i;
		}
		else if(strcmp(argv[i], "-v") == 0)
		{
			ShowVersionTextAndExit();
		}
		else if(strcmp(argv[i], "-prefix") == 0)
		{
			if(++i >= argc)
			{
				Log::Error("Missing command line parameter for assembler prefix");
				exit(1);
			}
			assemblerPrefix = argv[i++];
		}
		else if(strcmp(argv[i], "-pprefix") == 0)
		{
			if(++i >= argc)
			{
				Log::Error("Missing command line parameter for preprocessor prefix");
				exit(1);
			}
			preprocessorPrefix = argv[i++];
		}
		else if(strcmp(argv[i], "-varname") == 0)
		{
			if(++i >= argc)
			{
				Log::Error("Missing command line parameter for assembler variable name");
				exit(1);
			}
			assemblerVariableName = argv[i++];
		}
		else if(strcmp(argv[i], "-arm64") == 0)
		{
			assemblerType = AssemblerType::Arm64;
			++i;
		}
		else if(strcmp(argv[i], "-x64") == 0
				|| strcmp(argv[i], "-x86_64") == 0)
		{
			assemblerType = AssemblerType::X64;
			++i;
		}
		else
		{
			Log::Error("Unhandled command line parameter \"%s\" (argv[%d])", argv[i], i);
			ShowHelpTextAndExit(1);
		}
	}
	
	if(!inputFilename)
	{
		Log::Error("No input file specified");
		exit(1);
	}
}

//============================================================================

void CommandLine::ShowHelpTextAndExit(int errorLevel)
{
	static const char HELP_TEXT[] =
    "\n"
    " -h                 This help text.\n"
    " -f <input-file>    Specify the input source file.\n"
	" -o <output-file>   Specify output source file.\n"
	" -l <log-level>     Specify log level [verbose,debug,info,warning,error,none].\n"
	" -O                 Optimize build. Some run time errors may not assert.\n"
	" -Ofast             Optimize build for speed. Size increase tradeoff.\n"
	" -prefix <str>      Set assembler prefix. Defaults to \"»\".\n"
	" -pprefix <str>     Set preprocessor prefix. Defaults to \"«\".\n"
    " -v                 Show version.\n"
	" -varname <name>    Set default assembler variable name. Defaults to \"assembler\".\n"
    "\n"
    ;
	
    Log::Text(VERSION_TEXT);
    Log::Text(HELP_TEXT);
    exit(errorLevel);
}

void CommandLine::ShowVersionTextAndExit()
{
	Log::Text(VERSION_TEXT);
	exit(0);
}

void CommandLine::ProcessLogLevel(const char* levelName)
{
	if(strcmp(levelName, "verbose") == 0) Log::SetLogLevelImmediate(LogLevel::Verbose);
	else if(strcmp(levelName, "debug") == 0) Log::SetLogLevelImmediate(LogLevel::Debug);
	else if(strcmp(levelName, "info") == 0) Log::SetLogLevelImmediate(LogLevel::Info);
	else if(strcmp(levelName, "warning") == 0) Log::SetLogLevelImmediate(LogLevel::Warning);
	else if(strcmp(levelName, "error") == 0) Log::SetLogLevelImmediate(LogLevel::Error);
	else if(strcmp(levelName, "none") == 0) Log::SetLogLevelImmediate(LogLevel::None);
	else
	{
		Log::Error("Invalid log level specified on command line: %s", levelName);
		exit(1);
	}
}

//============================================================================
