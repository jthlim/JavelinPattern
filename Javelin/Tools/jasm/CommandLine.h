//============================================================================

#pragma once
#include <string>

//============================================================================

namespace Javelin::Assembler
{
//============================================================================

	class AssemblerType
	{
	public:
		enum Type : uint8_t
		{
			Unknown,
			Arm64,
			X64,
		};

		AssemblerType(Type aType) : type(aType) { }
		AssemblerType(const std::string& type);

		operator Type() const { return type; }
		explicit operator bool() const = delete;
		
		constexpr bool IsValid() const { return type != Unknown; }
		
	private:
		Type type;

		// Checks that there is sufficient data for the prefix, and that following the word it is
		// either the end of line, or a whitespace.
		static bool HasPrefix(const char *data, const char *end, const char *prefix, int length);
	};
	
	
	/// Configuration that needs to be specified on the command line
	class CommandLine
	{
	public:
		CommandLine(int argc, const char** argv);

		static const CommandLine& GetInstance() 					{ return *instance; }

		bool				UseSpeedOverSizeOptimizations() const	{ return useSpeedOptimizations; }
		bool            	UseUnsafeOptimizations() const			{ return useUnsafeOptimizations; }
		const std::string&	GetAssemblerPrefix() const				{ return assemblerPrefix; 	}
		const std::string&	GetPreprocessorPrefix() const			{ return preprocessorPrefix; 	}
		const char*			GetOutputFilename() const				{ return outputFilename; }
		const char*			GetInputFilename() const				{ return inputFilename;		}
		const char* 		GetAssemblerVariableName() const 		{ return assemblerVariableName; }
		AssemblerType		GetAssemblerType() const				{ return assemblerType; }

		void SetAssemblerType(AssemblerType aAssemblerType)			{ assemblerType = aAssemblerType; }
		
	private:
		static CommandLine* instance;
		
		void ShowHelpTextAndExit(int errorLevel=0);
		void ShowVersionTextAndExit();
		void ProcessLogLevel(const char* levelName);

		bool            useUnsafeOptimizations  	= false;
		bool			useSpeedOptimizations		= false;
		AssemblerType	assemblerType				= AssemblerType::X64;
		std::string		assemblerPrefix				= "»";
		std::string		preprocessorPrefix			= "«";
		const char*		outputFilename				= nullptr;
		const char*		inputFilename 				= nullptr;
		const char*		assemblerVariableName		= "assembler";
	};
	
//============================================================================
}  // Javelin::Assembler
//============================================================================
