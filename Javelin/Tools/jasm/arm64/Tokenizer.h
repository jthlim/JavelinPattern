//============================================================================

#pragma once
#include "Javelin/Tools/jasm/CodeSegmentSource.h"
#include "Javelin/Tools/jasm/arm64/Token.h"

//============================================================================

namespace Javelin::Assembler::arm64
{
//============================================================================
	
	class Tokenizer
	{
	public:
		Tokenizer(const CodeSegmentData &aSource);
		~Tokenizer();
		
		void			NextToken();
		Token			GetToken();
		Token::Type		GetTokenType();
		const Token& 	PeekToken() const { return token; }
		Token::Type		PeekTokenType() const { return token.type; }
		const std::string GetCurrentLine() const { return source.GetCurrentLine(); }
		int				GetCurrentLineNumber() const { return currentLineNumber; }
		int				GetCurrentFileIndex() const { return currentFileindex; }

		void SkipToEndOfLine();
		void BeginLocalLabelScope()		{
											labelScopeIds.push_back(activeLabelScopeId);
											activeLabelScopeId = ++totalLabelScopeId;
										}
		void EndLocalLabelScope()		{
											if(labelScopeIds.size())
											{
												activeLabelScopeId = labelScopeIds.back();
												labelScopeIds.pop_back();
											}
											else
											{
												activeLabelScopeId = ++totalLabelScopeId;
											}
										}
		
	private:
		int					c = ' ';
		int					activeLabelScopeId = 0;
		int					totalLabelScopeId = 0;
		int					currentLineNumber = 1;
		int					currentFileindex = 0;
		std::vector<int>	labelScopeIds;
		CodeSegmentSource	source;
		Token 				token;
		
		Token InternalGetToken();
	};
	
//============================================================================
}
//============================================================================
