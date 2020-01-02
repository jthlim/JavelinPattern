//============================================================================

#include "Javelin/Tools/jasm/arm64/Tokenizer.h"

#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/Character.h"
#include "Javelin/Tools/jasm/Log.h"
#include "Javelin/Tools/jasm/arm64/Instruction.h"
#include "Javelin/Tools/jasm/arm64/Register.h"

#include <math.h>

//============================================================================

using namespace Javelin::Assembler::arm64;

//============================================================================

Tokenizer::Tokenizer(const CodeSegmentData &aSource)
: source(aSource),
  token(InternalGetToken())
{
}

Tokenizer::~Tokenizer()
{
}

//============================================================================

Token Tokenizer::GetToken()
{
	Token result = token;
	NextToken();
	return result;
}

void Tokenizer::NextToken()
{
	token = InternalGetToken();
}

Token::Type Tokenizer::GetTokenType()
{
	Token::Type result = token.type;
	token = InternalGetToken();
	return result;
}

void Tokenizer::SkipToEndOfLine()
{
	for(;;)
	{
		Token::Type type = GetTokenType();
		if(type == Token::Type::EndOfFile || type == Token::Type::Newline) return;
	}
}

Token Tokenizer::InternalGetToken()
{
NextToken:
	currentLineNumber = source.GetCurrentLineNumber();
	currentFileindex = source.GetCurrentFileIndex();
	while(Character::IsIgnorableWhitespace(c)) c = source.ReadByte();
	
	switch(c)
	{
	case '(':
		c = ' ';
		return Token(Token::Type::OpenParenthesis);
		
	case ')':
		c = ' ';
		return Token(Token::Type::CloseParenthesis);
		
	case '[':
		c = ' ';
		return Token(Token::Type::LeftSquareBracket);
		
	case ']':
		c = ' ';
		return Token(Token::Type::RightSquareBracket);
			
	case '#':
		c = ' ';
		return Token(Token::Type::Hash);
		
	case '.':
		{
			Token result(Token::Type::Preprocessor);
			do
			{
				result.sValue.push_back(c);
				c = source.ReadByte();
			} while(Character::IsWordCharacter(c));

			if(result.sValue.size() == 1) return Token(Token::Type::Unknown);
			return result;
		}

	case ',':
		c = ' ';
		return Token(Token::Type::Comma);
		
	case ':':
		c = ' ';
		return Token(Token::Type::Colon);
		
	case ';':
		c = ' ';
		return Token(Token::Type::Semicolon);
		
	case '+':
		c = ' ';
		return Token(Token::Type::Add);
		
	case '-':
		c = ' ';
		return Token(Token::Type::Subtract);
		
	case '*':
		c = ' ';
		return Token(Token::Type::Star);
		
	case '!':
		c = source.ReadByte();
		if(c == '=')
		{
			c = ' ';
			return Token(Token::Type::NotEquals);
		}
		else return Token(Token::Type::ExclamationMark);

	case '=':
		c = source.ReadByte();
		if(c == '=')
		{
			c = ' ';
			return Token(Token::Type::Equals);
		}
		else return Token(Token::Type::Unknown);
			
	case '<':
		c = source.ReadByte();
		if(c == '=')
		{
			c = ' ';
			return Token(Token::Type::LessThanOrEquals);
		}
		else return Token(Token::Type::LessThan);

	case '>':
		c = source.ReadByte();
		if(c == '=')
		{
			c = ' ';
			return Token(Token::Type::GreaterThanOrEquals);
		}
		else return Token(Token::Type::GreaterThan);

	case '&':
		c = source.ReadByte();
		if(c == '&')
		{
			c = ' ';
			return Token(Token::Type::LogicalAnd);
		}
		else return Token(Token::Type::Unknown);

	case '|':
		c = source.ReadByte();
		if(c == '|')
		{
			c = ' ';
			return Token(Token::Type::LogicalOr);
		}
		else return Token(Token::Type::Unknown);

	case '/':
		c = source.ReadByte();
		if(c == '/')
		{
			// Comment until end of line.
			// Since newlines are important, read until EOF or newline.
			for(;;)
			{
				c = source.ReadByte();
				if(c == '\n')
				{
					c = ' ';
					return Token(Token::Type::Newline);
				}
				if(c == EOF)
				{
					return Token(Token::Type::EndOfFile);
				}
			}
		}
		return Token(Token::Type::Divide);
			
	case EOF:
		return Token(Token::Type::EndOfFile);
		
	case '\n':
		c = ' ';
		return Token(Token::Type::Newline);
			
	case '\'':
		{
			int64_t value = 0;
			for(;;)
			{
				c = source.ReadByte();
				if(c == '\'')
				{
					c = ' ';
					break;
				}
				
				if(c == '\n' || c == EOF)
				{
					Log::Error("Unexpected character inside character literal.");
					break;
				}
				if(c == '\\')
				{
					c = source.ReadByte();
					if(c == '\n') break;
					if(c == EOF) break;
					value *= 256;
					switch(c)
					{
					case 'a':   value += '\a'; break;
					case 'e':   value += '\e'; break;
					case 'f':   value += '\f'; break;
					case 'n':   value += '\n'; break;
					case 'r':   value += '\r'; break;
					case 't':   value += '\t'; break;
					case 'v':   value += '\v'; break;
					default:    value += c;    break;
					}
				}
				else
				{
					if((value >> 56) != 0)
					{
						Log::Error("Overflow in character literal");
					}
					value = 256*value + c;
				}
			}
			return Token(value, activeLabelScopeId);
		}
			
	case '{':
		{
			c = source.ReadByte();
			if(c == '{')
			{
				c = ' ';
				return Token(Token::Type::LeftBrace);
			}

			Token expression(Token::Type::Expression);
			int depth = 1;
			for(;;)
			{
				if(c == EOF) return Token(Token::Type::Unknown);
				else if(c == '{')
				{
					++depth;
				}
				else if(c == '}')
				{
					--depth;
					if(depth == 0)
					{
						c = ' ';
						break;
					}
				}
				expression.sValue.push_back(c);
				c = source.ReadByte();
			}
			return expression;
		}

	case '}':
		{
			c = source.ReadByte();
			if(c != '}') throw AssemblerException("Found single unmatched '}'");
			c = ' ';
			
			return Token(Token::Type::RightBrace);
		}
			
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
		{
			int64_t value = c - '0';
			c = source.ReadByte();
			if(value == 0 && c == 'b')
			{
				// Binary mode.
				int peek = source.PeekByte();
				if (peek != '0' && peek != '1') return Token(value, activeLabelScopeId);

				for(;;)
				{
					c = source.ReadByte();
					if(c < '0' || c > '1') break;
					value = 2*value + c-'0';
				}
			}
			else if(value == 0 && c == 'x')
			{
				// Hex mode.
				if(Character::HexValueForCharacter(source.PeekByte()) == -1)
				{
					return Token(value, activeLabelScopeId);
				}
				
				for(;;)
				{
					c = source.ReadByte();
					int nibbleValue = Character::HexValueForCharacter(c);
					if(nibbleValue == -1) break;
					value = 16*uint64_t(value) + nibbleValue;
				}
			}
			else
			{
				// Decimal mode.
				while('0' <= c && c <= '9')
				{
					value = 10*value + c-'0';
					c = source.ReadByte();
				}
				if(c == '.' || c == 'e' || c == 'E')
				{
					double dValue = value;
					int divisorExponent = 0;
					
					if(c == '.')
					{
						for(;;)
						{
							c = source.ReadByte();
							if(c < '0' || c > '9') break;
							dValue = 10*dValue + c-'0';
							--divisorExponent;
						}
					}
					if(c == 'e' || c == 'E')
					{
						int sign = 1;
						int exponent = 0;
						c = source.ReadByte();
						if(c == '-')
						{
							c = source.ReadByte();
							sign = -1;
						}
						while('0' <= c && c <= '9')
						{
							exponent = 10*exponent + c-'0';
							c = source.ReadByte();
						}
						divisorExponent += sign*exponent;
					}
					dValue = dValue * pow(10, divisorExponent);
					return Token(dValue);
				}
			}
			return Token(value, activeLabelScopeId);
		}
			
	default:
		{
			if(!Character::IsWordCharacter(c))
			{
				c = ' ';
				return Token(Token::Type::Unknown);
			}
			
			Token result(Token::Type::Identifier);
			do
			{
				result.sValue.push_back(c);
				c = source.ReadByte();
			} while(Character::IsWordCharacter(c) || c == '.');
			
			InstructionMap::const_iterator inst_it = InstructionMap::GetInstance().find(result.sValue);
			if(inst_it != InstructionMap::GetInstance().end())
			{
				result.type = Token::Type::Instruction;
				result.instruction = inst_it->second;
			}
			else
			{
				RegisterMap::const_iterator reg_it = RegisterMap::GetInstance().find(result.sValue);
				if(reg_it != RegisterMap::GetInstance().end())
				{
					result.type = Token::Type::Register;
					result.reg = reg_it->second;
				}
			}
			
			return result;
		}
	}
}

//============================================================================
