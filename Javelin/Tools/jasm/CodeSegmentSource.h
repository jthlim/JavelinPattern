//============================================================================

#pragma once
#include "Javelin/Tools/jasm/CodeSegment.h"

//============================================================================

namespace Javelin::Assembler
{
//============================================================================
	
	class CodeSegmentSource
	{
	public:
		CodeSegmentSource(const CodeSegmentData &aData, int aIndent = 0);
		
		int ReadByte();
		int PeekByte();
		void SkipByte();
		
		bool HasData() const { return index < data.size(); }

		const std::string GetPreviousLine() const;
		const std::string GetCurrentLine() const;
		int GetCurrentFileIndex() const;
		
		int GetIndent() const { return indent; }
		int GetCurrentLineNumber() const;
		bool GetCurrentLineIsPreprocessor() const;
		
	private:
		const CodeSegmentData &data;
		size_t index  = 0;
		size_t offset = 0;
		int indent;
	};
	
//============================================================================
}
//============================================================================
