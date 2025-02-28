//============================================================================

#pragma once
#include "Javelin/Pattern/Internal/PatternTypes.h"

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

	class TokenizerBase
	{
	public:
		virtual ~TokenizerBase() { }
		
		virtual const Token&	PeekCurrentToken() const = 0;
		virtual TokenType		PeekCurrentTokenType() const = 0;

		virtual void			ProcessTokens() = 0;
		virtual const void* 	GetExceptionData() const = 0;
	};

//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
