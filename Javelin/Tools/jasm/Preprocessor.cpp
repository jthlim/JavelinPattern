//============================================================================

#include "Javelin/Tools/jasm/Preprocessor.h"

#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/Character.h"
#include "Javelin/Tools/jasm/CodeSegmentSource.h"
#include "Javelin/Tools/jasm/CommandLine.h"

//============================================================================

using namespace Javelin::Assembler;

//============================================================================

void Preprocessor::Preprocess(SourceFileSegments& result, const CodeSegment &codeSegment)
{
	ProcessCodeSegment(result, codeSegment);
}

//============================================================================

void Preprocessor::ProcessCodeSegment(SourceFileSegments& result,
									  const CodeSegment& input,
									  const MacroParameters *parameters)
{
	CodeSegmentSource source(input.contents, input.indent);
	while(source.HasData())
	{
		if(!ProcessLine(result, source, parameters)) return;
	}
}

// Returns true if the current scope should continue iterating.
bool Preprocessor::ProcessLine(SourceFileSegments& result,
							   CodeSegmentSource &source,
							   const MacroParameters *parameters)
{
	// process the first token on the line.
	// if it is a keyword (.define, .mecro), then handle those.
	// If it is a macro name, do the substitution.
	// Otherwise, scan the line's token for define substitutions.
	std::string token = TransferWhitespaceAndGetNextToken(nullptr, source);

	if(token.length() == 0)
	{
		return true;
	}
	else if(token == ".define")
	{
		ProcessDefine(source);
	}
	else if(token == ".macro")
	{
		ProcessMacroDefinition(source);
	}
	else if(token == ".endm")
	{
		return false;
	}
	else if (token == ".code")
	{
		segment = CodeSegment::Segment::Code;
	}
	else if (token == ".data")
	{
		segment = CodeSegment::Segment::Data;
	}
	
	else
	{
		if(parameters)
		{
			if(const auto it = parameters->find(token);
			   it != parameters->end())
			{
				token = it->second;
			}
		}
		for(;;)
		{
			if(const auto it = macros.find(token);
			   it != macros.end())
			{
				ProcessMacro(result, it->first, it->second, source);
				break;
			}
			if(const auto it = defines.find(token);
			   it == defines.end())
			{
				ProcessLineTokens(result, source, token, parameters);
				break;
			}
			else
			{
				// A define at the start of a line is processed as an input line.
				CodeSegment codeSegment(CodeSegment::ContentType::Assembler, segment);
				codeSegment.contents.push_back(it->second);
				ProcessCodeSegment(result, codeSegment);
				break;
			}
		}
	}
	return true;
}

void Preprocessor::ProcessLineTokens(SourceFileSegments& result,
									 CodeSegmentSource& source,
									 std::string line,
									 const MacroParameters *parameters)
{
	int lineNumber = source.GetCurrentLineNumber();
	bool isPreprocessor = source.GetCurrentLineIsPreprocessor();
	
	for(;;)
	{
		std::string token = TransferWhitespaceAndGetNextToken(&line, source);
		if(token.length() == 0) break;
		
		if(parameters)
		{
			if(const auto it = parameters->find(token);
			   it != parameters->end())
			{
				token = it->second;
			}
		}
		for(;;)
		{
			if(const auto it = defines.find(token);
			   it != defines.end())
			{
				token = it->second.line;
				continue;
			}
			break;
		}
		line.append(token);
	}
	
	AssemblerType type(line);
	if(type.IsValid())
	{
		result.AddSegment(CodeSegment::ContentType::AssemblerTypeChange,
						  segment,
						  lineNumber,
						  source.GetIndent(),
						  isPreprocessor,
						  line,
						  source.GetCurrentFileIndex());
	}
	else
	{
		result.AddSegment(isPreprocessor ? CodeSegment::ContentType::Literal : CodeSegment::ContentType::Assembler,
						  segment,
						  lineNumber,
						  source.GetIndent(),
						  isPreprocessor,
						  line,
						  source.GetCurrentFileIndex());
	}
}

void Preprocessor::ProcessDefine(CodeSegmentSource& source)
{
	int lineNumber = source.GetCurrentLineNumber();
	int fileIndex = source.GetCurrentFileIndex();
	std::string defineKey = ReadToken(source);
	SkipWhitespace(source);
	std::string defineValue = ReadToEndOfLine(source);
	defines.insert({defineKey, {false, lineNumber, defineValue, fileIndex}});
}

void Preprocessor::ProcessMacroDefinition(CodeSegmentSource& source)
{
	std::string macroName = ReadToken(source);
	
	Macro macro;
	bool first = true;
	for(;;)
	{
		std::string token = ReadToken(source);
		if(token.length() == 0) break;
		
		if(first) first = false;
		else
		{
			if(token != ",") throw AssemblerException("Expected comma");
			token = ReadToken(source);
			if(token.length() == 0) AssemblerException("Expected token");
		}
		macro.parameterNames.push_back(token);
	}

	bool finished = false;
	while(source.HasData())
	{
		if(!ProcessLine(macro.code, source, nullptr))
		{
			finished = true;
			break;
		}
	}
	macros[macroName] = std::move(macro);

	if(!finished) throw AssemblerException("Missing .endm for macro named: %s", macroName.c_str());
}

void Preprocessor::ProcessMacro(SourceFileSegments& result,
								const std::string &macroName,
								const Macro& macro,
								CodeSegmentSource& source)
{
	MacroParameters parameters;
	for(const auto &parameterName : macro.parameterNames)
	{
		SkipWhitespace(source);
		std::string parameterValue;
		for(;;)
		{
			std::string token = ReadToken(source);
			if(token.length() == 0 || token == ",") break;
			for(auto it = defines.find(token); it != defines.end(); it = defines.find(token))
			{
				token = it->second.line;
			}
			parameterValue.append(token);
		}
		if(parameterValue.size() == 0) throw AssemblerException("Expected macro parameterValue");
		parameters.insert({parameterName, parameterValue});
	}
	result.AddSegment(CodeSegment::ContentType::Assembler, segment, -1, source.GetIndent(), false, ".begin_local_label_scope", -1);
	for(const CodeSegment *segment : macro.code)
	{
		switch(segment->contentType)
		{
		case CodeSegment::ContentType::Assembler:
			ProcessCodeSegment(result, *segment, &parameters);
			break;
		case CodeSegment::ContentType::AssemblerTypeChange:
			result.push_back(new CodeSegment(*segment));
			break;
		case CodeSegment::ContentType::Literal:
			{
				// Literals still need macro name substitution. Process that here.
				CodeSegmentSource literalSource(segment->contents, source.GetIndent());
				while(literalSource.HasData())
				{
					ProcessLineTokens(result,
									  literalSource,
									  "",
									  &parameters);
				}
			}
			break;
		}
	}
	result.AddSegment(CodeSegment::ContentType::Assembler, segment, -1, source.GetIndent(), false, ".end_local_label_scope", -1);
}

void Preprocessor::SkipWhitespace(CodeSegmentSource& source)
{
	int c = source.PeekByte();
	while(Character::IsIgnorableWhitespace(c))
	{
		source.SkipByte();
		c = source.PeekByte();
	}
}

std::string Preprocessor::ReadToken(CodeSegmentSource& source)
{
	int c;
	do
	{
		c = source.ReadByte();
	} while(Character::IsIgnorableWhitespace(c));

	if(c == EOF || c == '\n') return "";

	std::string returnValue;
	returnValue.push_back(c);
	if(c != '.' && !Character::IsWordCharacter(c)) return returnValue;
	for(;;)
	{
		c = source.PeekByte();
		if(c == EOF || !Character::IsWordCharacter(c)) return returnValue;
		
		returnValue.push_back(c);
		source.SkipByte();
	}
}

std::string Preprocessor::ReadToEndOfLine(CodeSegmentSource& source)
{
	std::string returnValue;
	for(;;)
	{
		int c = source.ReadByte();
		if(c == EOF || c == '\n') return returnValue;
		if(c == '/')
		{
			c = source.ReadByte();
			if(c == '/')
			{
				// Comment until end of line.
				for(;;)
				{
					int c = source.ReadByte();
					if(c == EOF || c == '\n') return returnValue;
				}
			}
			returnValue.push_back('/');
		}
		returnValue.push_back(c);
	}
}

std::string Preprocessor::TransferWhitespaceAndGetNextToken(std::string *line,
															CodeSegmentSource& source)
{
	int c = source.ReadByte();
	if(c == '\n' || c == EOF) return "";
	while(Character::IsIgnorableWhitespace(c))
	{
		if(line) line->push_back(c);
		c = source.ReadByte();
	}

	std::string returnValue;
	if(c == '\n') return "";
	returnValue.push_back(c);
	if(c != '.' && !Character::IsWordCharacter(c)) return returnValue;
	for(;;)
	{
		c = source.PeekByte();
		if(c == EOF || !Character::IsWordCharacter(c)) return returnValue;
		
		returnValue.push_back(c);
		source.SkipByte();
	}
}

//============================================================================
