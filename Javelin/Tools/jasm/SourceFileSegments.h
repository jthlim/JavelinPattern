//============================================================================

#pragma once
#include "File.h"
#include "CodeSegment.h"
#include <vector>

//============================================================================

namespace Javelin::Assembler
{
//============================================================================
	
	class SourceFileSegments : public std::vector<CodeSegment*>
	{
	private:
		typedef std::vector<CodeSegment*> Inherited;
		
	public:
		~SourceFileSegments();
		SourceFileSegments() = default;
		SourceFileSegments(const SourceFileSegments&) = delete;
		SourceFileSegments(SourceFileSegments&&);
		void operator=(const SourceFileSegments&) = delete;
		void operator=(SourceFileSegments&&);
		void AdoptFileNameList(SourceFileSegments &other);

		void AddSegmentsFromFile(File &f);
		
		void DumpSegments() const;
		void DumpTokens() const;
		void AddSegment(CodeSegment::ContentType type, CodeSegment::Segment segment, int lineNumber, int indent, bool isPreprocessor, const std::string &line, int fileIndex);
		
		const std::vector<std::string> &GetFilenameList() const { return filenameList; }
		
	private:
		struct Line
		{
			enum class Type : uint8_t
			{
				Whitespace,
				Literal,
				Preprocessor,
				Assembler,
			};
			
			Type type;
			size_t offset;
			std::string line;
		};
		
		std::vector<std::string> filenameList;
		
		Line GetLine(const std::string& line);
		void AddLineToSegment(CodeSegment::Segment segment, const Line& line, int lineNumber, int fileIndex);
		
		static bool IsAllWhitespace(const std::string& line);
		static bool IsAllComment(const std::string& line);
		static bool GetIncludePath(const std::string &line, std::string *includePath);
		static bool IsAbsolutePath(const std::string& includePath);
		static std::string GetFullIncludePath(const std::string& filename, const std::string &includePath);
	};

//============================================================================
}
//============================================================================
