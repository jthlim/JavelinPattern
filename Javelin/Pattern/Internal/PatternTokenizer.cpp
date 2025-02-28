//============================================================================

#include "Javelin/Pattern/Internal/PatternTokenizer.h"
#include "Javelin/Pattern/Pattern.h"
#include "Javelin/Template/Utility.h"
#include "Javelin/Type/Exception.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

Tokenizer::Tokenizer(Utf8Pointer aP, Utf8Pointer aEnd, bool aUseUtf8)
{
	useUtf8 = aUseUtf8;
	phase	= Phase::General;
	p		= aP;
	end 	= aEnd;

	ProcessTokens();
}

//============================================================================

Character Tokenizer::GetEscapedCharacter()
{
	JPATTERN_VERIFY(p < end, UnexpectedEndOfPattern, nullptr);

	unsigned char c;
	c = *pUC++;
	switch(c)
	{
	case 'a':
		return Character('\a');
			
	case 'e':
		return Character('\e');
		
	case 'f':
		return Character('\f');
			
	case 'v':
		return Character('\v');
			
	case 'n':
		return Character('\n');
		
	case 'r':
		return Character('\r');
		
	case 't':
		return Character('\t');
		
	case 'c':
		JPATTERN_VERIFY(p < end, UnexpectedEndOfPattern, nullptr);
		c = *pUC;
		JPATTERN_VERIFY(32 <= c && c <= 126, UnexpectedControlCharacter, pUC);
		++pUC;
		return Character::ToUpper(c) ^ 0x40;
		
	case 'u':
		if(*pUC == '{') goto BraceHexCode;
		else
		{
			uint32_t v = 0;
			for(int i = 0; i < 4; ++i)
			{
				uint32_t x = *pUC;
				JPATTERN_VERIFY(Character::IsHexCharacter(x), UnexpectedHexCharacter, pUC);
				++pUC;
				v = v<<4 | Character::GetHexValue(x);
			}
			return Character(v);
		}
			
	case 'x':
		if(*pUC == '{') goto BraceHexCode;
		else
		{
			uint32_t v = 0;
			for(int i = 0; i < 2; ++i)
			{
				uint32_t x = *pUC;
				JPATTERN_VERIFY(Character::IsHexCharacter(x), UnexpectedHexCharacter, pUC);
				++pUC;
				v = v<<4 | Character::GetHexValue(x);
			}
			return Character(v);
		}
			
	BraceHexCode:
		{
			++pUC;
			uint32_t v = 0;
			while(*pUC != '}')
			{
				uint32_t x = *pUC;
				JPATTERN_VERIFY(Character::IsHexCharacter(x), UnexpectedHexCharacter, pUC);
				++pUC;
				v = v<<4 | Character::GetHexValue(x);
			}
			++pUC;
			return Character(v);
		}
			
	default:
		return Character(c);
	}
}

Character Tokenizer::GetCharacter()
{
	JPATTERN_VERIFY(p < end, UnexpectedEndOfPattern, nullptr);

	Character c;
	if(useUtf8)
	{
		c = *p++;
		if(c != '\\') return c;
	}
	else
	{
		c = *pUC++;
		if(c != '\\') return c;
	}
	
	return GetEscapedCharacter();
}

JINLINE char Tokenizer::PeekCharacter()
{
	// Since input strings are guaranteed to be null terminated, we can peek past the end! It'll just return '\0'
	return *reinterpret_cast<const char*>(p.GetCharPointer());
}

JINLINE void Tokenizer::ConsumeCharacter()
{
	if(useUtf8) ++p;
	else ++pUC;
}

//============================================================================

void Tokenizer::ProcessTokens()
{
	while(1)
	{
	Loop:
		switch(phase)
		{
		case Phase::General:
			switch(PeekCharacter())
			{
			case '\0':
				currentToken.type = TokenType::End;
				return;

			case '+':
				ConsumeCharacter();
				if(PeekCharacter() == '?')
				{
					ConsumeCharacter();
					currentToken.type = TokenType::OneOrMoreMinimal;
				}
				else if(PeekCharacter() == '+')
				{
					ConsumeCharacter();
					currentToken.type = TokenType::OneOrMorePossessive;
				}
				else currentToken.type = TokenType::OneOrMoreMaximal;
				return;

			case '*':
				ConsumeCharacter();
				if(PeekCharacter() == '?')
				{
					ConsumeCharacter();
					currentToken.type = TokenType::ZeroOrMoreMinimal;
				}
				else if(PeekCharacter() == '+')
				{
					ConsumeCharacter();
					currentToken.type = TokenType::ZeroOrMorePossessive;
				}
				else currentToken.type = TokenType::ZeroOrMoreMaximal;
				return;

			case '?':
				ConsumeCharacter();
				if(PeekCharacter() == '?')
				{
					ConsumeCharacter();
					currentToken.type = TokenType::ZeroOrOneMinimal;
				}
				else if(PeekCharacter() == '+')
				{
					ConsumeCharacter();
					currentToken.type = TokenType::ZeroOrOnePossessive;
				}
				else currentToken.type = TokenType::ZeroOrOneMaximal;
				return;

			case '.':
				ConsumeCharacter();
				currentToken.type = TokenType::Wildcard;
				return;

			case '^':
				ConsumeCharacter();
				currentToken.type = TokenType::StartOfLine;
				return;

			case '$':
				ConsumeCharacter();
				currentToken.type = TokenType::EndOfLine;
				return;

			case '|':
				ConsumeCharacter();
				currentToken.type = TokenType::Alternate;
				return;

			case '(':
				ConsumeCharacter();
				switch(PeekCharacter())
				{
				case '*':
					ConsumeCharacter();
					if(memcmp(pUC, "ACCEPT)", 7) == 0)
					{
						pUC += 7;
						currentToken.type = TokenType::Accept;
						return;
					}
					if(memcmp(pUC, "FAIL)", 5) == 0)
					{
						pUC += 5;
						currentToken.type = TokenType::Fail;
						return;
					}
					if(memcmp(pUC, "F)", 2) == 0)
					{
						pUC += 2;
						currentToken.type = TokenType::Fail;
						return;
					}
					JPATTERN_ERROR(UnableToParseGroupType, pUC);
					
				case '?':
					ConsumeCharacter();
						
					if(PeekCharacter() == '#')
					{
						ConsumeCharacter();
						while(GetCharacter() != ')') { }
						continue;
					}
					else
					{
						bool turnOn = true;
						bool signSpecified = false;
						currentToken.optionSet  = 0;
						currentToken.optionMask = 0;

						for(;;)
						{
							switch(PeekCharacter())
							{
							case ':':
								ConsumeCharacter();
								currentToken.type = TokenType::ClusterStart;
								return;
									
							case '=':
								ConsumeCharacter();
								currentToken.type = TokenType::PositiveLookAhead;
								return;

							case '!':
								ConsumeCharacter();
								currentToken.type = TokenType::NegativeLookAhead;
								return;
									
							case '<':
								ConsumeCharacter();
								switch(PeekCharacter())
								{
								case '=':
									ConsumeCharacter();
									currentToken.type = TokenType::PositiveLookBehind;
									return;
									
								case '!':
									ConsumeCharacter();
									currentToken.type = TokenType::NegativeLookBehind;
									return;
									
								default:
									JPATTERN_ERROR(UnexpectedLookBehindType, pUC);
								}
								break;
								
							case 'i':
								if(turnOn) currentToken.optionSet |= Pattern::IGNORE_CASE;
								currentToken.optionMask |= Pattern::IGNORE_CASE;
								ConsumeCharacter();
								break;
									
							case 'm':
								if(turnOn) currentToken.optionSet |= Pattern::MULTILINE;
								currentToken.optionMask |= Pattern::MULTILINE;
								ConsumeCharacter();
								break;
								
							case 's':
								if(turnOn) currentToken.optionSet |= Pattern::DOTALL;
								currentToken.optionMask |= Pattern::DOTALL;
								ConsumeCharacter();
								break;
									
							case 'u':
								if(turnOn) currentToken.optionSet |= Pattern::UNICODE_CASE;
								currentToken.optionMask |= Pattern::UNICODE_CASE;
								ConsumeCharacter();
								break;
								
							case 'U':
								if(turnOn) currentToken.optionSet |= Pattern::UNGREEDY;
								currentToken.optionMask |= Pattern::UNGREEDY;
								ConsumeCharacter();
								break;
								
							case '-':
								turnOn = false;
								signSpecified = true;
								ConsumeCharacter();
								break;
								
							case '+':
								turnOn = true;
								signSpecified = true;
								ConsumeCharacter();
								break;
									
							case '(':
								JPATTERN_VERIFY(currentToken.optionSet == 0 && currentToken.optionMask == 0 && signSpecified == false, UnexpectedGroupOptions, pUC);
								currentToken.type = TokenType::Conditional;
								return;
									
							case '>':
								ConsumeCharacter();
								currentToken.type = TokenType::AtomicGroup;
								return;
									
							case ')':
								ConsumeCharacter();
								currentToken.type = TokenType::OptionChange;
								return;
							
							case 'R':
								JPATTERN_VERIFY(currentToken.optionSet == 0 && currentToken.optionMask == 0 && signSpecified == false, UnexpectedGroupOptions, pUC);
								ConsumeCharacter();
								currentToken.type = TokenType::Recurse;
								currentToken.i = 0;
								JPATTERN_VERIFY(GetCharacter() == ')', ExpectedCloseGroup, pUC);
								return;
									
							case '0':
							case '1':
							case '2':
							case '3':
							case '4':
							case '5':
							case '6':
							case '7':
							case '8':
							case '9':
								// Recurse reference
								JPATTERN_VERIFY(currentToken.optionSet == 0 && currentToken.optionMask == 0, UnexpectedGroupOptions, pUC);
								currentToken.type = TokenType::Recurse;
								currentToken.i = PeekCharacter() - '0';
								ConsumeCharacter();
								
								while(1)
								{
									char c = PeekCharacter();
									if(c < '0' || c > '9') break;
									
									currentToken.i = currentToken.i * 10 + (c - '0');
									ConsumeCharacter();
								}
								if(signSpecified)
								{
									currentToken.type = TokenType::RecurseRelative;
									if(!turnOn) currentToken.i = -currentToken.i;
								}
								JPATTERN_VERIFY(GetCharacter() == ')', ExpectedCloseGroup, pUC);
								return;

							default:
								JPATTERN_ERROR(UnableToParseGroupType, pUC);
							}
						}
					}
					break;

				default:
					currentToken.type = TokenType::CaptureStart;
					break;
				}
				return;

			case '[':
				ConsumeCharacter();
				if(PeekCharacter() == '^')
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
					if(pUC[0] == '[' && pUC[1] == ':')
					{
						if(memcmp(pUC, "[:alnum:]", 9) == 0)
						{
							pUC += 9;
							currentToken.rangeList.Add('0', '9');
							currentToken.rangeList.Add('A', 'Z');
							currentToken.rangeList.Add('a', 'z');
						}
						else if (memcmp(pUC, "[:alpha:]", 9) == 0)
						{
							pUC += 9;
							currentToken.rangeList.Add('A', 'Z');
							currentToken.rangeList.Add('a', 'z');
						}
						else if (memcmp(pUC, "[:blank:]", 9) == 0)
						{
							pUC += 9;
							currentToken.rangeList.Add('\t');
							currentToken.rangeList.Add(' ');
						}
						else if (memcmp(pUC, "[:cntrl:]", 9) == 0)
						{
							pUC += 9;
							currentToken.rangeList.Add('\x00', '\x1f');
							currentToken.rangeList.Add('\x7f');
						}
						else if (memcmp(pUC, "[:digit:]", 9) == 0)
						{
							pUC += 9;
							currentToken.rangeList.Add('0', '9');
						}
						else if (memcmp(pUC, "[:graph:]", 9) == 0)
						{
							pUC += 9;
							currentToken.rangeList.Add('!', '~');
						}
						else if (memcmp(pUC, "[:lower:]", 9) == 0)
						{
							pUC += 9;
							currentToken.rangeList.Add('a', 'z');
						}
						else if (memcmp(pUC, "[:print:]", 9) == 0)
						{
							pUC += 9;
							currentToken.rangeList.Add(' ', '~');
						}
						else if (memcmp(pUC, "[:punct:]", 9) == 0)
						{
							pUC += 9;
							currentToken.rangeList.Add('!', '/');
							currentToken.rangeList.Add(':', '@');
							currentToken.rangeList.Add('\\');
							currentToken.rangeList.Add('[', '`');
							currentToken.rangeList.Add('{', '~');
						}
						else if (memcmp(pUC, "[:space:]", 9) == 0)
						{
							pUC += 9;
							currentToken.rangeList.Add('\t', '\r');
							currentToken.rangeList.Add(' ');
						}
						else if (memcmp(pUC, "[:upper:]", 9) == 0)
						{
							pUC += 9;
							currentToken.rangeList.Add('A', 'Z');
						}
						else if (memcmp(pUC, "[:xdigit:]", 10) == 0)
						{
							pUC += 10;
							currentToken.rangeList.Add('0', '9');
							currentToken.rangeList.Add('A', 'F');
							currentToken.rangeList.Add('a', 'f');
						}
						else
						{
							JPATTERN_ERROR(UnknownPosixCharacterClass, pUC);
						}
					}
					else
					{
						CharacterRange interval;
						
						JVERIFY(PeekCharacter() != '\0');
						
						if(pUC[0] == '\\')
						{
							++pUC;
							switch(*pUC)
							{
							case 'd':
								++pUC;
								currentToken.rangeList.Add('0', '9');
								continue;
									
							case 's':
								++pUC;
								for(const CharacterRange& range : CharacterRangeList::WHITESPACE_CHARACTERS)
								{
									currentToken.rangeList.Add(range);
								}
								continue;
									
							case 'w':
								++pUC;
								for(const CharacterRange& range : CharacterRangeList::WORD_CHARACTERS)
								{
									currentToken.rangeList.Add(range);
								}
								continue;
									
							case 'D':
								++pUC;
								currentToken.rangeList.Add(0, '0'-1);
								currentToken.rangeList.Add('9'+1, Character::Maximum());
								continue;
								
							case 'S':
								++pUC;
								for(const CharacterRange& range : CharacterRangeList::WHITESPACE_CHARACTERS.CreateComplement())
								{
									currentToken.rangeList.Add(range);
								}
								continue;
								
							case 'W':
								++pUC;
								for(const CharacterRange& range : CharacterRangeList::WORD_CHARACTERS.CreateComplement())
								{
									currentToken.rangeList.Add(range);
								}
								continue;
								
							default:
								interval.min = GetEscapedCharacter();
								break;
							}
						}
						else interval.min = GetCharacter();
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

			case ')':
				ConsumeCharacter();
				currentToken.type = TokenType::CloseBracket;
				return;

			case '{':
				ConsumeCharacter();
				currentToken.type = TokenType::CounterStart;
				phase = Phase::Counter;
				return;

			case '}':
				ConsumeCharacter();
				currentToken.type = TokenType::CounterEnd;
				return;

			case '\\':
				ConsumeCharacter();

				switch(PeekCharacter())
				{
				case '.':
				case '^':
				case '$':
				case '\\':
				case '|':
				case '+':
				case '?':
				case '(':
				case ')':
				case '<':
				case '>':
				case '[':
				case ']':
				case '{':
				case '*':
				case '}':
					currentToken.type = TokenType::Character;
					currentToken.c = PeekCharacter();
					ConsumeCharacter();
					return;

				case 'A':
					ConsumeCharacter();
					currentToken.type = TokenType::StartOfInput;
					return;
						
				case 'a':
					ConsumeCharacter();
					currentToken.type = TokenType::Character;
					currentToken.c = '\a';
					return;
					
				case 'b':
					ConsumeCharacter();
					currentToken.type = TokenType::WordBoundary;
					return;

				case 'B':
					ConsumeCharacter();
					currentToken.type = TokenType::NotWordBoundary;
					return;
						
				case 'C':
					ConsumeCharacter();
					currentToken.type = TokenType::AnyByte;
					return;

				case 'd':
					ConsumeCharacter();
					currentToken.type = TokenType::Range;
					currentToken.rangeList.SetCount(0);
					currentToken.rangeList.Append('0', '9');
					return;

				case 'c':
				case 'x':
				case 'u':
					{
						currentToken.c = GetEscapedCharacter();
						if(useUtf8 && currentToken.c >= 128)
						{
							currentToken.type = TokenType::Range;
							currentToken.rangeList.SetCount(0);
							currentToken.rangeList.Append(currentToken.c);
						}
						else
						{
							currentToken.type = TokenType::Character;
						}
					}
					return;
						
				case 'D':
					ConsumeCharacter();
					currentToken.type = TokenType::NotRange;
					currentToken.rangeList.SetCount(0);
					currentToken.rangeList.Append('0', '9');
					return;

				case 'e':
					ConsumeCharacter();
					currentToken.type = TokenType::Character;
					currentToken.c = '\e';
					return;
					
				case 'f':
					ConsumeCharacter();
					currentToken.type = TokenType::Character;
					currentToken.c = '\f';
					return;
					
				case 'G':
					ConsumeCharacter();
					currentToken.type = TokenType::StartOfSearch;
					return;
					
				case 'h':
					ConsumeCharacter();
					currentToken.type = TokenType::Range;
					currentToken.rangeList.SetCount(0);
					currentToken.rangeList.Append('\t');
					currentToken.rangeList.Append(' ');
					return;
						
				case 'K':
					ConsumeCharacter();
					currentToken.type = TokenType::ResetCapture;
					return;
					
				case 'n':
					ConsumeCharacter();
					currentToken.type = TokenType::Character;
					currentToken.c = '\n';
					return;

				case 'Q':
					ConsumeCharacter();
					phase = Phase::Literal;
					goto Loop;
						
				case 'r':
					ConsumeCharacter();
					currentToken.type = TokenType::Character;
					currentToken.c = '\r';
					return;

				case 't':
					ConsumeCharacter();
					currentToken.type = TokenType::Character;
					currentToken.c = '\t';
					return;

				case 'v':
					ConsumeCharacter();
					currentToken.type = TokenType::Character;
					currentToken.c = '\v';
					return;
					
				case 'w':
					ConsumeCharacter();
					currentToken.type = TokenType::WordCharacter;
					return;

				case 'W':
					ConsumeCharacter();
					currentToken.type = TokenType::NotWordCharacter;
					return;

				case 's':
					ConsumeCharacter();
					currentToken.type = TokenType::WhitespaceCharacter;
					return;

				case 'S':
					ConsumeCharacter();
					currentToken.type = TokenType::NotWhitespaceCharacter;
					return;

				case 'z':
					ConsumeCharacter();
					currentToken.type = TokenType::EndOfInput;
					return;
					
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					// Backreference
					currentToken.type = TokenType::BackReference;
					currentToken.i = PeekCharacter() - '0';
					ConsumeCharacter();
					
					while(1)
					{
						char c = PeekCharacter();
						if(c < '0' || c > '9') break;
						
						currentToken.i = currentToken.i * 10 + (c - '0');
						ConsumeCharacter();
					}
						
					if(currentToken.i == 0)
					{
						currentToken.type = TokenType::Character;
						currentToken.c = '\0';
					}
					return;
				}
				JPATTERN_ERROR(UnknownEscape, pUC);

			default:
				if(useUtf8)
				{
					// This could be a single character, or a utf8 sequence!
					if(p->GetNumberOfBytes() == 1)
					{
						currentToken.type = TokenType::Character;
						currentToken.c = *p;
					}
					else
					{
						currentToken.type = TokenType::Range;
						currentToken.rangeList.SetCount(0);
						currentToken.rangeList.Append(*p);
					}
					++p;
				}
				else
				{
					currentToken.type = TokenType::Character;
					currentToken.c = *pUC++;
				}
				return;
			}
			JPATTERN_ERROR(InternalError, pUC);

		case Phase::Counter:
			switch(PeekCharacter())
			{
			case '\0':
				currentToken.type = TokenType::End;
				return;

			case ',':
				ConsumeCharacter();
				currentToken.type = TokenType::CounterSeparator;
				return;

			case '}':
				ConsumeCharacter();
				if(PeekCharacter() == '?')
				{
					ConsumeCharacter();
					currentToken.type = TokenType::CounterEndMinimal;
				}
				else if(PeekCharacter() == '+')
				{
					ConsumeCharacter();
					currentToken.type = TokenType::CounterEndPossessive;
				}
				else currentToken.type = TokenType::CounterEnd;
				phase = Phase::General;
				return;
					
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				currentToken.type = TokenType::CounterValue;
				currentToken.i = PeekCharacter() - '0';
				ConsumeCharacter();
				
				while(1)
				{
					char c = PeekCharacter();
					if(c < '0' || c > '9') break;
					
					currentToken.i = currentToken.i * 10 + (c - '0');
					ConsumeCharacter();
				}
				return;

			default:
				JPATTERN_ERROR(UnableToParseRepetition, pUC);
			}
			break;
				
		case Phase::Literal:
			switch(PeekCharacter())
			{
			case '\0':
				currentToken.type = TokenType::End;
				return;
					
			case '\\':
				ConsumeCharacter();
				if(PeekCharacter() == 'E')
				{
					ConsumeCharacter();
					phase = Phase::General;
					goto Loop;
				}
				currentToken.type = TokenType::Character;
				currentToken.c = '\\';
				return;
					
			default:
				currentToken.type = TokenType::Character;
				currentToken.c = *pUC++;
				return;
			}
		}
	}
}

//============================================================================

