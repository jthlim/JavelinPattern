//============================================================================

#pragma once
#include "Javelin/Tools/jasm/CodeSegmentSource.h"
#include "Javelin/Tools/jasm/x64/Token.h"

//============================================================================

namespace Javelin::Assembler::x64
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
		int 			GetCurrentLineNumber() const { return currentLineNumber; }
		int				GetCurrentFileIndex() const { return currentFileIndex; }

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
		int					currentFileIndex = 0;
		std::vector<int>	labelScopeIds;
		CodeSegmentSource	source;
		Token 				token;
		
		
		Token InternalGetToken();
	};
	
//============================================================================
}
//============================================================================
