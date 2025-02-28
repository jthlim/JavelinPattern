//============================================================================

#pragma once
#include "Javelin/Pattern/Internal/PatternTokenizerBase.h"
#include "Javelin/Type/Character.h"
#include "Javelin/Type/Utf8Pointer.h"

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

	class Tokenizer final : public TokenizerBase
	{
	public:
		Tokenizer(Utf8Pointer aP, Utf8Pointer aEnd, bool useUtf8);

		const Token&	PeekCurrentToken()	const		{ return currentToken;	}
		TokenType		PeekCurrentTokenType()	const	{ return currentToken.type; }

		void		ProcessTokens();
		const void* GetExceptionData() const { return pUC; }

	private:
		enum class Phase
		{
			General,
			Counter,
			Literal
		};
		
		bool		useUtf8;
		Phase 		phase;
		
		union
		{
			Utf8Pointer	p;
			const unsigned char*	pUC;
		};
		union
		{
			Utf8Pointer	end;
			const unsigned char*	pUCEnd;
		};

		Token		currentToken;

		// Used for decoding character classes, eg. [a-z\n\t]
		Character 	GetCharacter();
		Character	GetEscapedCharacter();
		
		char 		PeekCharacter();
		inline void ConsumeCharacter();
	};

//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
