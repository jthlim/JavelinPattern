//============================================================================

#include "Javelin/Tools/jasm/SourceFileSegments.h"
#include "Javelin/Tools/jasm/Character.h"
#include "Javelin/Tools/jasm/CommandLine.h"
#include "Javelin/Tools/jasm/Log.h"

#include "Javelin/Tools/jasm/x64/Tokenizer.h"

#include <memory.h>

//============================================================================

using namespace Javelin::Assembler;

//============================================================================

SourceFileSegments::SourceFileSegments(SourceFileSegments&& other)
: Inherited(std::move(other))
{
}

SourceFileSegments::~SourceFileSegments()
{
	for(CodeSegment *segment : *this) delete segment;
}

void SourceFileSegments::operator=(SourceFileSegments&& other)
{
	for(CodeSegment *segment : *this) delete segment;
	Inherited::operator=(std::move(other));
	filenameList = std::move(other.filenameList);
}

void SourceFileSegments::AdoptFileNameList(SourceFileSegments &other)
{
	filenameList = std::move(other.filenameList);
}


void SourceFileSegments::AddSegmentsFromFile(File &f)
{
	int lineNumber = 1;
	int fileIndex = (int) filenameList.size();
	filenameList.push_back(f.GetFilename());

	std::string includePath;

	while(!f.IsEof())
	{
		Line line = GetLine(f.ReadLine());
		
		if(line.type == Line::Type::Assembler
		   && GetIncludePath(line.line, &includePath))
		{
			Log::Verbose("Including \"%s\"", includePath.c_str());
			std::string fullIncludePath;
			if(IsAbsolutePath(includePath))
			{
				fullIncludePath = includePath;
			}
			else
			{
				fullIncludePath = GetFullIncludePath(f.GetFilename(), includePath);
				Log::Verbose("FullIncludePath: \"%s\"", fullIncludePath.c_str());
			}
			File includeFile(fullIncludePath);
			if(includeFile.IsValid())
			{
				AddSegmentsFromFile(includeFile);
			}
			else
			{
				Log::Error("%s:%d: Unable to open file \"%s\"", f.GetFilename().c_str(), lineNumber, includePath.c_str());
			}
		}
		else
		{
			AddLineToSegment(CodeSegment::Segment::Unknown, line, lineNumber, fileIndex);
		}
		++lineNumber;
	}
}

void SourceFileSegments::DumpSegments() const
{
	for(size_t i = 0; i < size(); ++i)
	{
		CodeSegment *segment = (*this)[i];
		Log::Verbose("Segment %zu: %s (%s)",
					 i,
					 segment->GetContentType(),
					 segment->GetSegment());
		segment->Dump();
	}
}

void SourceFileSegments::DumpTokens() const
{
	for(CodeSegment *segment : *this)
	{
		if(segment->contentType != CodeSegment::ContentType::Assembler) continue;

		x64::Tokenizer tokenizer(segment->contents);
		for(;;)
		{
			x64::Token token = tokenizer.GetToken();
			if(token.type == x64::Token::Type::EndOfFile
			   || token.type == x64::Token::Type::Newline)
			{
				printf("%s\n", token.GetDescription().c_str());
			}
			else
			{
				printf("%s, ", token.GetDescription().c_str());
			}
			if(token.type == x64::Token::Type::EndOfFile) break;
		}
	}
}

void SourceFileSegments::AddSegment(CodeSegment::ContentType type, CodeSegment::Segment segment, int lineNumber, int indent, bool isPreprocessor, const std::string &line, int fileIndex)
{
	if(size() > 0 && IsAllWhitespace(line))
	{
		back()->contents.push_back({false, lineNumber, line, fileIndex});
		return;
	}
	
	// If the whole line is a comment, then add it to the code segment.
	if(size() > 0 && IsAllComment(line))
	{
		back()->contents.push_back({false, lineNumber, line, fileIndex});
		return;
	}
	
	CodeSegment *codeSegment;
	if (size() > 0 && back()->contentType == type && back()->segment == segment)
	{
		codeSegment = back();
	}
	else
	{
		codeSegment = new CodeSegment(type, segment);
		codeSegment->indent = indent;
		push_back(codeSegment);
	}
	codeSegment->contents.push_back({isPreprocessor, lineNumber, line, fileIndex});
}

void SourceFileSegments::AddLineToSegment(CodeSegment::Segment segment, const Line& line, int lineNumber, int fileIndex)
{
	if (size() > 0 && line.type == Line::Type::Whitespace)
	{
		// Lines are still pushed so that source line numbers match.
		back()->contents.push_back({false, lineNumber, line.line, fileIndex});
		return;
	}

	// If the whole line is a comment, then add it to the code segment
	// only if it's a literal.
	if(size() > 0 && IsAllComment(line.line))
	{
		back()->contents.push_back({false, lineNumber, line.line, fileIndex});
		return;
	}

	CodeSegment::ContentType segmentType = (line.type == Line::Type::Literal || line.type == Line::Type::Whitespace) ?
		CodeSegment::ContentType::Literal : CodeSegment::ContentType::Assembler;
	
	CodeSegment *codeSegment;
	if (size() > 0 && back()->contentType == segmentType && back()->segment == segment)
	{
		codeSegment = back();
	}
	else
	{
		codeSegment = new CodeSegment(segmentType, segment);
		codeSegment->indent = (int32_t) line.offset;
		push_back(codeSegment);
	}
	
	codeSegment->contents.push_back({line.type == Line::Type::Preprocessor, lineNumber, line.line, fileIndex});
}

bool SourceFileSegments::IsAllWhitespace(const std::string& line)
{
	for(uint32_t c : line)
	{
		if(!Character::IsWhitespace(c)) return false;
	}
	return true;
}

bool SourceFileSegments::IsAllComment(const std::string& line)
{
	int slashCount = 0;
	for(uint32_t c : line)
	{
		if(c == '/')
		{
			if(++slashCount >= 2) return true;
		}
		else
		{
			if(slashCount) return false;
			if(!Character::IsWhitespace(c)) return false;
		}
	}
	return true;
}

SourceFileSegments::Line SourceFileSegments::GetLine(const std::string& line)
{
	Line result;
	
	size_t length = line.size();
	for(size_t i = 0; i < length; ++i)
	{
		int c = line[i];
		if(Character::IsWhitespace(c)) continue;

		const std::string& preprocessorPrefix = CommandLine::GetInstance().GetPreprocessorPrefix();
		if(i + preprocessorPrefix.size() <= length)
		{
			if (memcmp(&line[i], &preprocessorPrefix[0], preprocessorPrefix.size()) == 0)
			{
				std::string source = line.substr(i + preprocessorPrefix.size());
				return Line { .type = Line::Type::Preprocessor, .offset = i, .line = source };
			}
		}

		const std::string& assemblerPrefix = CommandLine::GetInstance().GetAssemblerPrefix();
		if(i + assemblerPrefix.size() <= length)
		{
			if (memcmp(&line[i], &assemblerPrefix[0], assemblerPrefix.size()) == 0)
			{
				std::string source = line.substr(i + assemblerPrefix.size());
				return Line { .type = Line::Type::Assembler, .offset = i, .line = source };
			}
		}
		return Line { .type = Line::Type::Literal, .offset = 0, .line = line };
		break;
	}
	return Line { .type = Line::Type::Whitespace, .offset = 0 };
}

//============================================================================

bool SourceFileSegments::GetIncludePath(const std::string& line, std::string *includePath)
{
	size_t offset = 0;
	while(offset < line.size())
	{
		if(Character::IsWhitespace(line[offset])) ++offset;
		else break;
	}
	size_t includeStringLength = sizeof(".include") - 1;
	
	if(offset + includeStringLength > line.size()) return false;
	if(memcmp(line.data()+offset, ".include", includeStringLength) != 0) return false;
	
	offset += includeStringLength;
	
	while(offset < line.size() && Character::IsWhitespace(line[offset])) ++offset;
	if(offset == line.size()) return false;
	
	size_t end = line.size();
	while(end > offset && Character::IsWhitespace(line[end-1])) --end;
	*includePath = line.substr(offset, end-offset);
	return true;
}

bool SourceFileSegments::IsAbsolutePath(const std::string& includePath)
{
	if(includePath.size() > 0)
	{
		if(includePath[0] == '/' || includePath[0] == '/') return true;
	}
	if(includePath.size() > 1)
	{
		// DOS drive path
		if(includePath[1] == ':') return true;
	}
	return false;
}

std::string SourceFileSegments::GetFullIncludePath(const std::string& filename, const std::string &includePath)
{
	auto pos = filename.find_last_of("/\\");
	if(pos == std::string::npos) return includePath;
	return filename.substr(0, pos+1) + includePath;
}

//============================================================================
