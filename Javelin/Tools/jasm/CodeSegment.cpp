//============================================================================

#include "Javelin/Tools/jasm/CodeSegment.h"
#include "Javelin/Tools/jasm/Log.h"

//============================================================================

using namespace Javelin::Assembler;

//============================================================================

const char* CodeSegment::GetContentType() const
{
	switch(contentType)
	{
	case ContentType::Literal: return "Literal";
	case ContentType::Assembler: return "Assembler";
	case ContentType::AssemblerTypeChange: return "AssemblerTypeChange";
	}
}

const char* CodeSegment::GetSegment() const
{
	switch(segment)
	{
	case Segment::Unknown: return "Unknown";
	case Segment::Code: return "Code";
	case Segment::Data: return "Data";
	}
}

void CodeSegment::Dump() const
{
	for(size_t j = 0; j < contents.size(); ++j)
	{
		Log::Verbose(" [%zu] %d: %s", j, contents[j].lineNumber, contents[j].line.c_str());
	}
}

int CodeSegment::GetStartingLine() const
{
	if(contents.size() == 0) return 0;
	for(const auto & a : contents)
	{
		if(a.lineNumber != -1) return a.lineNumber;
	}
	return 0;
}

int CodeSegment::GetStartingFileIndex() const
{
	if(contents.size() == 0) return 0;
	for(const auto & a : contents)
	{
		if(a.fileIndex != -1) return a.fileIndex;
	}
	return 0;
}

//============================================================================
