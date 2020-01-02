//============================================================================

#pragma once
#include <string>
#include <vector>

//============================================================================

namespace Javelin::Assembler
{
//============================================================================
	
	struct CodeLine
	{
		bool isPreprocessor;
		int lineNumber;
		int fileIndex;
		std::string line;
		
		CodeLine() = default;
		CodeLine(bool aIsPreprocessor, int aLineNumber, const std::string &aLine, int aFileIndex)
		  : isPreprocessor(aIsPreprocessor), lineNumber(aLineNumber), line(aLine), fileIndex(aFileIndex) { }
	};
	
	struct CodeSegmentData : std::vector<CodeLine>
	{
	};
	
	struct CodeSegment
	{
		enum class ContentType : uint8_t
		{
			Literal,
			Assembler,
			AssemblerTypeChange,
		};
		
		enum class Segment : uint8_t
		{
			Unknown,
			Code,
			Data,
		};
		
		ContentType		contentType;
		Segment			segment;
		int				indent = 0;
		CodeSegmentData	contents;
		
		CodeSegment(ContentType aContentType, Segment aSegment) : contentType(aContentType), segment(aSegment) { }
		
		void Dump() const;
		int GetStartingLine() const;
		int GetStartingFileIndex() const;
		
		const char* GetContentType() const;
		const char* GetSegment() const;
	};

//============================================================================
}
//============================================================================
