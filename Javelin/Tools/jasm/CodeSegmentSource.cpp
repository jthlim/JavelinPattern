//============================================================================

#include "Javelin/Tools/jasm/CodeSegmentSource.h"

//============================================================================

using namespace Javelin::Assembler;

//============================================================================

CodeSegmentSource::CodeSegmentSource(const CodeSegmentData &aData, int aIndent)
: data(aData), indent(aIndent)
{
}

int CodeSegmentSource::ReadByte()
{
	if(index >= data.size()) return EOF;
	
	const std::string& s = data[index].line;
	if(offset >= s.size())
	{
		offset = 0;
		++index;
		return '\n';
	}
	
	return (uint8_t) s[offset++];
}

int CodeSegmentSource::PeekByte()
{
	if(index >= data.size()) return EOF;
	
	const std::string& s = data[index].line;
	if(offset >= s.size()) return '\n';
	
	return (uint8_t) s[offset];
}

void CodeSegmentSource::SkipByte()
{
	if(index >= data.size()) return;
	
	const std::string& s = data[index].line;
	if(offset >= s.size())
	{
		offset = 0;
		++index;
	}
	else
	{
		++offset;
	}
}

//============================================================================

int CodeSegmentSource::GetCurrentLineNumber() const
{
	if(data.size() == 0) return 0;
	if(index >= data.size()) return data[index-1].lineNumber;
	return data[index].lineNumber;
}

const std::string CodeSegmentSource::GetPreviousLine() const
{
	if(data.size() == 0) return "";
	if(index == 0) return data[0].line;
	return data[index-1].line;
}

const std::string CodeSegmentSource::GetCurrentLine() const
{
	if(data.size() == 0) return "";
	if(index >= data.size()) return data[index-1].line;
	return data[index].line;
}

int CodeSegmentSource::GetCurrentFileIndex() const
{
	if(data.size() == 0) return 0;
	if(index >= data.size()) return data[index-1].fileIndex;
	return data[index].fileIndex;
}

bool CodeSegmentSource::GetCurrentLineIsPreprocessor() const
{
	if(data.size() == 0) return false;
	if(index >= data.size()) return data[index-1].isPreprocessor;
	return data[index].isPreprocessor;
}

//============================================================================
