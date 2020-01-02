//============================================================================

#pragma once
#include "CodeSegment.h"
#include "SourceFileSegments.h"
#include <vector>
#include <unordered_map>

//============================================================================

namespace Javelin::Assembler
{
//============================================================================
	
	class CodeSegmentSource;
	
	/**
	 * This class is responsible for handling defines and macros.
	 * syntax:
	 *
	 * .define keyword [replacement string]
	 *
	 * .macro macroName param1, param2
	 *    {code}
	 * .endm
	 */
	class Preprocessor
	{
	public:
		void Preprocess(SourceFileSegments& result, const CodeSegment &codeSegment);

	private:
		typedef std::unordered_map<std::string, std::string> MacroParameters;
		typedef std::vector<std::string> MacroParameterNames;
		
		struct Macro
		{
			SourceFileSegments code;
			MacroParameterNames parameterNames;
		};
		
		CodeSegment::Segment segment = CodeSegment::Segment::Code;
		std::unordered_map<std::string, CodeLine> defines;
		std::unordered_map<std::string, Macro> macros;
		
		void ProcessCodeSegment(SourceFileSegments& result, const CodeSegment& input, const MacroParameters *parameters = nullptr);
		bool ProcessLine(SourceFileSegments& result, CodeSegmentSource& source, const MacroParameters *parameters);
		void ProcessDefine(CodeSegmentSource& source);
		void ProcessMacroDefinition(CodeSegmentSource& source);
		void ProcessMacro(SourceFileSegments& result, const std::string &macroName, const Macro& macro, CodeSegmentSource& source);
		void ProcessLineTokens(SourceFileSegments& result, CodeSegmentSource& source, std::string line, const MacroParameters *parameters);

		static void SkipWhitespace(CodeSegmentSource& source);
		static std::string ReadToken(CodeSegmentSource& source);
		static std::string ReadToEndOfLine(CodeSegmentSource& source);
		static std::string TransferWhitespaceAndGetNextToken(std::string *line, CodeSegmentSource& source);
	};

//============================================================================
}
//============================================================================
