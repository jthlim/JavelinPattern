//============================================================================

#include "Javelin/Pattern/Internal/PatternTokenizerGlob.h"
#include "Javelin/Pattern/Pattern.h"
#include "Javelin/Template/Utility.h"
#include "Javelin/Type/Exception.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

GlobTokenizer::GlobTokenizer(Utf8Pointer aP, Utf8Pointer aEnd, bool aUseUtf8)
{
	useUtf8 = aUseUtf8;
	p		= aP;
	end 	= aEnd;
	phase   = Phase::General;

	ProcessTokens();
}

//============================================================================

JINLINE char GlobTokenizer::PeekCharacter()
{
	// Since input strings are guaranteed to be null terminated, we can peek past the end! It'll just return '\0'
	return *reinterpret_cast<const char*>(p.GetCharPointer());
}

Character GlobTokenizer::GetCharacter()
{
	JPATTERN_VERIFY(p < end, UnexpectedEndOfPattern, nullptr);
	if(useUtf8) return *p++;
	else return *pUC++;
}

JINLINE void GlobTokenizer::ConsumeCharacter()
{
	if(useUtf8) ++p;
	else ++pUC;
}

//============================================================================

void GlobTokenizer::ProcessTokens()
{
	switch(phase)
	{
	case Phase::Wildcard:
		currentToken.type = TokenType::ZeroOrMoreMaximal;
		phase = Phase::General;
		return;
			
	case Phase::General:
		switch(PeekCharacter())
		{
		case '\0':
			currentToken.type = TokenType::End;
			return;

		case '*':
			phase = Phase::Wildcard;
			ConsumeCharacter();
			if(PeekCharacter() == '*')
			{
				ConsumeCharacter();
				currentToken.type = TokenType::Wildcard;
			}
			else
			{
				currentToken.type = TokenType::NotRange;
				currentToken.rangeList.SetCount(0);
				currentToken.rangeList.Append('/');
			}
			return;
				
		case '?':
			ConsumeCharacter();
			currentToken.type = TokenType::NotRange;
			currentToken.rangeList.SetCount(0);
			currentToken.rangeList.Append('/');
			return;

		case '[':
			ConsumeCharacter();
			if(PeekCharacter() == '!')
			{
				ConsumeCharacter();
				currentToken.type = TokenType::NotRange;
			}
			else
			{
				currentToken.type = TokenType::Range;
			}
			currentToken.rangeList.SetCount(0);
			if(PeekCharacter() == '-' || PeekCharacter() == ']')
			{
				currentToken.rangeList.Append(PeekCharacter());
				ConsumeCharacter();
			}
			while(PeekCharacter() != ']')
			{
				CharacterRange interval;
				
				JVERIFY(PeekCharacter() != '\0');
				interval.min = GetCharacter();
				if(PeekCharacter() == '-' && pUC+1 < pUCEnd && pUC[1] != ']')
				{
					ConsumeCharacter();
					interval.max = GetCharacter();
				}
				else
				{
					interval.max = interval.min;
				}
				currentToken.rangeList.Add(interval);
			}
				
			if(currentToken.rangeList.GetCount() == 1)
			{
				if(currentToken.type == TokenType::Range
				   && currentToken.rangeList[0].GetSize() == 0
				   && currentToken.rangeList[0].min < 128)
				{
					currentToken.type = TokenType::Character;
					currentToken.c = currentToken.rangeList[0].min;
				}
			}
			else
			{
				currentToken.rangeList.Sort();
			}
			ConsumeCharacter();
			return;

		default:
			currentToken.type = TokenType::Character;
			currentToken.c = *pUC++;
			return;
		}
		JPATTERN_ERROR(InternalError, pUC);
	}
}

//============================================================================

