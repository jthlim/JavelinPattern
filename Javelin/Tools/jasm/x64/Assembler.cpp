//============================================================================

#include "Javelin/Tools/jasm/x64/Assembler.h"

#include "Javelin/Assembler/x64/ActionType.h"
#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/CommandLine.h"
#include "Javelin/Tools/jasm/Log.h"
#include "Javelin/Tools/jasm/x64/Instruction.h"
#include "Javelin/Tools/jasm/x64/Register.h"
#include "Javelin/Tools/jasm/x64/Tokenizer.h"

#include <assert.h>
#include <inttypes.h>
#include <string.h>

//============================================================================

using namespace Javelin::Assembler;
using namespace Javelin::Assembler::x64;

//============================================================================

Assembler::Assembler()
{
}

Assembler::~Assembler()
{

}

void Assembler::UpdateExpressionBitWidth(int index, int value)
{
	assert(index >= 1);
	Expression &expression = expressionList[index-1];
	if(expression.maxBitWidth < value) expression.maxBitWidth = value;
}

void Assembler::Dump() const
{
	if(!expressionList.empty())
	{
		printf("Expressions\n");
		for(const Expression &expression : expressionList)
		{
			printf("%d: %d bits, {%s}\n", expression.index, expression.maxBitWidth, expression.expression.c_str());
		}
	}
	printf("Actions\n");
	rootListAction.Dump();
	printf("\n");
}

//============================================================================

void Assembler::AssembleSegment(const CodeSegment &segment, const std::vector<std::string> &filenameList)
{
	isDataSegment = (segment.segment == CodeSegment::Segment::Data);
	Tokenizer tokenizer(segment.contents);
	ParseSegment(tokenizer, filenameList);
	rootListAction.ResolveRelativeAddresses();
	segmentIndent = segment.indent;
	try
	{
		rootListAction.DelayAndConsolidate();
	}
	catch(AssemblerException &ex)
	{
		ex.PopulateLocationIfNecessary(tokenizer.GetCurrentLineNumber(), tokenizer.GetCurrentFileIndex());
		for(const auto &it : ex) Log::Error("%s", it.GetLogString(filenameList).c_str());
	}
}

void Assembler::ParseSegment(Tokenizer &tokenizer, const std::vector<std::string> &filenameList)
{
	for(;;)
	{
		int lineNumber = tokenizer.GetCurrentLineNumber();
		int fileIndex = tokenizer.GetCurrentFileIndex();
		try
		{
			Token token = tokenizer.GetToken();
			switch(token.type)
			{
			case Token::Type::Semicolon:
			case Token::Type::Newline:
				continue;
				
			case Token::Type::EndOfFile:
				return;

			case Token::Type::Preprocessor:
				ParsePreprocessor(rootListAction, tokenizer, token, filenameList);
				break;

			default:
				ParseLine(rootListAction, tokenizer, token);
				break;
			}
		}
		catch(AssemblerException &ex)
		{
			ex.PopulateLocationIfNecessary(lineNumber, fileIndex);
			tokenizer.SkipToEndOfLine();
			
			for(const auto &it : ex) Log::Error("%s", it.GetLogString(filenameList).c_str());
		}
	}
}

void Assembler::ParsePreprocessor(ListAction& listAction, Tokenizer &tokenizer, const Token &token, const std::vector<std::string> &filenameList)
{
	if(token.sValue == ".if")
	{
		ParseIf(listAction, tokenizer, filenameList);
	}
	else if(token.sValue == ".assembler_variable_name")
	{
		Token name = tokenizer.GetToken();
		if(name.type != Token::Type::Identifier)
		{
			throw AssemblerException("Missing name for .assembler_variable_name");
		}
		rootListAction.Append(new SetAssemblerVariableNameAction(name.sValue));
	}
	else if(token.sValue == ".align")
	{
		Token alignToken = tokenizer.GetToken();
		if(alignToken.type != Token::Type::IntegerValue)
		{
			throw AssemblerException("Missing alignment value");
		}
		// Check that alignToken.iValue is a power of two.
		if(alignToken.iValue && (alignToken.iValue & (alignToken.iValue-1)) != 0)
		{
			throw AssemblerException("Alignment must be a power of 2");
		}
		listAction.Append(new AlignAction((int) alignToken.iValue));
	}
	else if(token.sValue == ".unalign")
	{
		Token alignToken = tokenizer.GetToken();
		if(alignToken.type != Token::Type::IntegerValue)
		{
			throw AssemblerException("Missing alignment value");
		}
		// Check that alignToken.iValue is a power of two.
		if(alignToken.iValue && (alignToken.iValue & (alignToken.iValue-1)) != 0)
		{
			throw AssemblerException("Unalignment must be a power of 2");
		}
		if(alignToken.iValue <= 4)
		{
			throw AssemblerException("Unalignment must be 8 or greater");
		}
		listAction.Append(new UnalignAction((int) alignToken.iValue));
	}
	else if(token.sValue == ".aligned")
	{
		Token alignToken = tokenizer.GetToken();
		if(alignToken.type != Token::Type::IntegerValue)
		{
			throw AssemblerException("Missing alignment value");
		}
		// Check that alignToken.iValue is a power of two.
		if(alignToken.iValue && (alignToken.iValue & (alignToken.iValue-1)) != 0)
		{
			throw AssemblerException("Alignment must be a power of 2");
		}
		listAction.Append(new AlignedAction((int) alignToken.iValue));
	}
	else if(token.sValue == ".begin_local_label_scope")
	{
		tokenizer.BeginLocalLabelScope();
	}
	else if(token.sValue == ".end_local_label_scope")
	{
		tokenizer.EndLocalLabelScope();
	}
	else
	{
		throw AssemblerException("Unexpected token: %s", token.GetDescription().c_str());
	}
}

void Assembler::ParseIf(ListAction& listAction, Tokenizer &tokenizer, const std::vector<std::string> &filenameList)
{
	// Syntax is:
	// .if <cond>
	//   <inst>...
	// .elif <cond>
	//   <inst>...
	// .else
	//   <inst>...
	// .endif
	AlternateAction *alternateAction = new AlternateAction();
	listAction.Append(alternateAction);
	
	const AlternateActionCondition *condition = ParseCondition(tokenizer);
	if(!condition)
	{
		throw AssemblerException("Unable to parse .if condition");
	}
	
	ListAction *sublist = new ListAction;
	bool hasParsedElse = false;
	int lineNumber;
	int fileIndex;
	
	try
	{
		for(;;)
		{
			lineNumber = tokenizer.GetCurrentLineNumber();
			fileIndex = tokenizer.GetCurrentFileIndex();
			Token token = tokenizer.PeekToken();
			switch(token.type)
			{
			case Token::Type::Newline:
			case Token::Type::Semicolon:
				tokenizer.NextToken();
				continue;
					
			case Token::Type::EndOfFile:
				throw AssemblerException("Missing .endif directive");
					
			case Token::Type::Preprocessor:
				tokenizer.NextToken();
				if(token.sValue == ".endif")
				{
					alternateAction->Add(sublist, condition);
					return;
				}
				if(token.sValue == ".else")
				{
					if(hasParsedElse)
					{
						throw AssemblerException("Cannot have multiple .else");
					}
					hasParsedElse = true;
					alternateAction->Add(sublist, condition);
					sublist = new ListAction();
					condition = &AlwaysAlternateActionCondition::instance;
					continue;
				}
				if(token.sValue == ".elseif" || token.sValue == ".elif")
				{
					if(hasParsedElse)
					{
						throw AssemblerException("Cannot have .elseif after .else");
					}
					alternateAction->Add(sublist, condition);
					sublist = new ListAction();
					condition = ParseCondition(tokenizer);
					if(!condition)
					{
						throw AssemblerException("Unable to parse .elseif condition");
					}
					continue;
				}
				ParsePreprocessor(*sublist, tokenizer, token, filenameList);
				continue;
					
			default:
				tokenizer.NextToken();
				ParseLine(*sublist, tokenizer, token);
				break;
			}
		}
	}
	catch(AssemblerException &ex)
	{
		ex.PopulateLocationIfNecessary(lineNumber, fileIndex);
		throw;
	}
}

const AlternateActionCondition *Assembler::ParseCondition(Tokenizer &tokenizer)
{
	Token token = tokenizer.PeekToken();
	if(token.sValue == "delta32")
	{
		tokenizer.NextToken();
		Token token = tokenizer.GetToken();
		if(token.type != Token::Type::Expression)
		{
			throw AssemblerException("Unexpected token: %s (Expected expression for delta32 condition)", token.GetDescription().c_str());
		}
		int expressionIndex = AddExpression(token.sValue, 64, tokenizer.GetCurrentLineNumber(), tokenizer.GetCurrentFileIndex());
		return new Delta32AlternateActionCondition(expressionIndex);
	}
	else if(token.type == Token::Type::Expression)
	{
		Token token = tokenizer.GetToken();
		std::string expression = "!(" + token.sValue + ")";
		
		int expressionIndex = AddExpression(expression, 8, tokenizer.GetCurrentLineNumber(), tokenizer.GetCurrentFileIndex());
		return new ZeroAlternateActionCondition(expressionIndex);
	}
	else
	{
		AutoDeleteVector temporaryOperands;
		Operand *result = ParseOperand(tokenizer, temporaryOperands);
		if(result->type != Operand::Type::Immediate)
		{
			throw AssemblerException("Unable to parse if condition");
		}
		return ((ImmediateOperand *)result)->AsBool() ?
				(const AlternateActionCondition *) &AlwaysAlternateActionCondition::instance :
				(const AlternateActionCondition *) &NeverAlternateActionCondition::instance;
	}
}

void Assembler::ParseLine(ListAction& listAction, Tokenizer &tokenizer, Token token)
{
	try
	{
		bool globalLabel = false;
		switch(token.type)
		{
		case Token::Type::Star:
			{
				// This marks a global variable
				globalLabel = true;
				token = tokenizer.GetToken();
				switch(token.type)
				{
				case Token::Type::Identifier:
					goto ParseNamedLabel;
				case Token::Type::IntegerValue:
					goto ParseNumericLabel;
				case Token::Type::Expression:
					goto ParseExpressionLabel;
				default:
					Log::Error("Unexpected token after star: %s", token.GetDescription().c_str());
					goto SkipToEndOfLine;
				}
			}
			break;
		case Token::Type::Identifier:
			{
				if(token.sValue == "db")
				{
					AutoDeleteVector temporaryOperands;
					for(;;)
					{
						Operand *operand = ParseOperand(tokenizer, temporaryOperands);
						if(!operand || operand->type != Operand::Type::Immediate) goto SkipToEndOfLine;
						ImmediateOperand *imm = (ImmediateOperand*) operand;

						if(imm->IsExpression())
						{
							UpdateExpressionBitWidth(imm->expressionIndex, 8);
							listAction.Append(new ExpressionAction(1, imm->expressionIndex));
						}
						else
						{
							uint8_t value = ((ImmediateOperand*) operand)->value;
							listAction.Append(new LiteralAction({(uint8_t*) &value, (uint8_t*) &value + 1}));
						}
						
						token = tokenizer.GetToken();
						if(token.type == Token::Type::EndOfFile
						   || token.type == Token::Type::Newline) break;
						if(token.type != Token::Type::Comma) goto SkipToEndOfLine;
					}
					break;
				}
				if(token.sValue == "dw")
				{
					AutoDeleteVector temporaryOperands;
					for(;;)
					{
						Operand *operand = ParseOperand(tokenizer, temporaryOperands);
						if(!operand || operand->type != Operand::Type::Immediate) goto SkipToEndOfLine;
						ImmediateOperand *imm = (ImmediateOperand*) operand;

						if(imm->IsExpression())
						{
							UpdateExpressionBitWidth(imm->expressionIndex, 16);
							listAction.Append(new ExpressionAction(2, imm->expressionIndex));
						}
						else
						{
							uint16_t value = ((ImmediateOperand*) operand)->value;
							listAction.Append(new LiteralAction({(uint8_t*) &value, (uint8_t*) &value + 2}));
						}
						
						token = tokenizer.GetToken();
						if(token.type == Token::Type::EndOfFile
						   || token.type == Token::Type::Newline) break;
						if(token.type != Token::Type::Comma) goto SkipToEndOfLine;
					}
					break;
				}
				if(token.sValue == "dd")
				{
					AutoDeleteVector temporaryOperands;
					for(;;)
					{
						Operand *operand = ParseOperand(tokenizer, temporaryOperands);
						if(!operand || operand->type != Operand::Type::Immediate) goto SkipToEndOfLine;
						ImmediateOperand *imm = (ImmediateOperand*) operand;

						if(imm->immediateType == Operand::ImmediateType::Integer)
						{
							if(imm->IsExpression())
							{
								UpdateExpressionBitWidth(imm->expressionIndex, 32);
								listAction.Append(new ExpressionAction(4, imm->expressionIndex));
							}
							else
							{
								int32_t value = (int32_t) imm->value;
								listAction.Append(new LiteralAction({(uint8_t*) &value, (uint8_t*) &value + 4}));
							}
						}
						else if(imm->immediateType == Operand::ImmediateType::Real)
						{
							if(imm->IsExpression())
							{
								UpdateExpressionBitWidth(imm->expressionIndex, 32);
								listAction.Append(new ExpressionAction(4, imm->expressionIndex));
							}
							else
							{
								float value = (float) imm->realValue;
								listAction.Append(new LiteralAction({(uint8_t*) &value, (uint8_t*) &value + 4}));
							}
						}
						else goto SkipToEndOfLine;
						
						token = tokenizer.GetToken();
						if(token.type == Token::Type::EndOfFile
						   || token.type == Token::Type::Newline) break;
						if(token.type != Token::Type::Comma) goto SkipToEndOfLine;
					}
					break;
				}
				if(token.sValue == "dq")
				{
					AutoDeleteVector temporaryOperands;
					for(;;)
					{
						Operand *operand = ParseOperand(tokenizer, temporaryOperands);
						if(!operand ) goto SkipToEndOfLine;
						
						if(operand->type == Operand::Type::Label)
						{
							uint64_t offset = 8;
							listAction.Append(new LiteralAction({(uint8_t*) &offset, (uint8_t*) &offset + 8}));
							listAction.Append(((LabelOperand*) operand)->CreatePatchAction(8, 8));
							listAction.Append(new PatchAbsoluteAddressAction());
						}
						else if(operand->type == Operand::Type::Immediate)
						{
							ImmediateOperand *imm = (ImmediateOperand*) operand;
							if(imm->immediateType == Operand::ImmediateType::Integer)
							{
								if(imm->IsExpression())
								{
									UpdateExpressionBitWidth(imm->expressionIndex, 64);
									listAction.Append(new ExpressionAction(8, imm->expressionIndex));
								}
								else
								{
									listAction.Append(new LiteralAction({(uint8_t*) &imm->value, (uint8_t*) &imm->value + 8}));
								}
							}
							else if(imm->immediateType == Operand::ImmediateType::Real)
							{
								if(imm->IsExpression())
								{
									UpdateExpressionBitWidth(imm->expressionIndex, 64);
									listAction.Append(new ExpressionAction(8, imm->expressionIndex));
								}
								else
								{
									double value = (double) imm->realValue;
									listAction.Append(new LiteralAction({(uint8_t*) &value, (uint8_t*) &value + 8}));
								}
							}
						}
						else goto SkipToEndOfLine;

						token = tokenizer.GetToken();
						if(token.type == Token::Type::EndOfFile
						   || token.type == Token::Type::Newline) break;
						if(token.type != Token::Type::Comma) goto SkipToEndOfLine;
					}
					break;
				}
				if(token.sValue == "dt")
				{
					AutoDeleteVector temporaryOperands;
					for(;;)
					{
						Operand *operand = ParseOperand(tokenizer, temporaryOperands);
						if(!operand || operand->type != Operand::Type::Immediate) goto SkipToEndOfLine;
						ImmediateOperand *imm = (ImmediateOperand*) operand;
						
						if(imm->immediateType == Operand::ImmediateType::Real)
						{
							if(imm->IsExpression())
							{
								UpdateExpressionBitWidth(imm->expressionIndex, 80);
								listAction.Append(new ExpressionAction(10, imm->expressionIndex));
							}
							else
							{
								double value = (double) imm->realValue;
								listAction.Append(new LiteralAction({(uint8_t*) &value, (uint8_t*) &value + 10}));
							}
						}
						else goto SkipToEndOfLine;

						token = tokenizer.GetToken();
						if(token.type == Token::Type::EndOfFile
						   || token.type == Token::Type::Newline) break;
						if(token.type != Token::Type::Comma) goto SkipToEndOfLine;
					}
					break;
				}
				
				// An identifier can start a line if it's:
				//  a) A label,            e.g. namedLabel:
				//  b) A data declaration, e.g. value DB xx, xx, xx
			ParseNamedLabel:
				Token nextToken = tokenizer.PeekToken();
				if(nextToken.type == Token::Type::Colon)
				{
					tokenizer.NextToken();
					listAction.Append(new NamedLabelAction(globalLabel, token.sValue));
					break;
				}
				if(nextToken.type == Token::Type::Identifier)
				{
					if(nextToken.sValue == "db")
					{
						listAction.Append(new NamedLabelAction(globalLabel, token.sValue, 1));
						break;
					}
					if(nextToken.sValue == "dw")
					{
						listAction.Append(new NamedLabelAction(globalLabel, token.sValue, 2));
						break;
					}
					if(nextToken.sValue == "dd")
					{
						listAction.Append(new NamedLabelAction(globalLabel, token.sValue, 4));
						break;
					}
					if(nextToken.sValue == "dq")
					{
						listAction.Append(new NamedLabelAction(globalLabel, token.sValue, 8));
						break;
					}
				}
				
				goto SkipToEndOfLine;
			}
			break;
			
		case Token::Type::IntegerValue:
			{
				// The only case where an immediate is allowed at the start of a line
				// is for a label.
			ParseNumericLabel:
				if(tokenizer.PeekToken().type != Token::Type::Colon) goto SkipToEndOfLine;
				tokenizer.NextToken();
				listAction.Append(new NumericLabelAction(globalLabel, token.GetLabelValue()));
			}
			break;
			
		case Token::Type::Instruction:
			// Shortcut processing of prefix instructions.
			if(token.instruction->IsPrefix())
			{
				token.instruction->AddToAssembler(token.sValue, *this, listAction, 0, nullptr);
				return;
			}
			ParseInstruction(listAction, tokenizer, token);
			break;
			
		case Token::Type::Expression:
			{
				// An expression can start the line if it's a label.
				// It is always considered global.
			ParseExpressionLabel:
				if(tokenizer.PeekToken().type != Token::Type::Colon) goto SkipToEndOfLine;
				int expressionIndex = AddExpression(token.sValue, 32, tokenizer.GetCurrentLineNumber(), tokenizer.GetCurrentFileIndex());
				tokenizer.NextToken();
				listAction.Append(new ExpressionLabelAction(expressionIndex));
			}
			break;
				
		case Token::Type::Semicolon:
		case Token::Type::Newline:
			return;
			
		case Token::Type::EndOfFile:
			return;
			
		default:
		SkipToEndOfLine:
			throw AssemblerException("Unexpected token: %s\n", token.GetDescription().c_str());
		}
	}
	catch(AssemblerException &ex)
	{
		// Skip until end of line.
		tokenizer.SkipToEndOfLine();
		throw;
	}
}

void Assembler::ParseInstruction(ListAction& listAction, Tokenizer &tokenizer, const Token &instructionToken)
{
	const Operand *operands[4];
	int currentOperandIndex = 0;
	
	AutoDeleteVector temporaryOperands;
	
	for(;;)
	{
		Token::Type tokenType = tokenizer.PeekTokenType();
		if(tokenType == Token::Type::Semicolon
		   || tokenType == Token::Type::Newline
		   || tokenType == Token::Type::EndOfFile)
		{
			tokenizer.NextToken();
			break;
		}
		
		if(currentOperandIndex != 0)
		{			
			if(tokenType != Token::Type::Comma)
			{
				Token token(tokenType);
				throw AssemblerException("Expected comma. Found %s", token.GetDescription().c_str());
			}
			tokenizer.NextToken();
		}

		Operand *op = ParseOperand(tokenizer, temporaryOperands);
		if(op == nullptr)
		{
			throw AssemblerException("Error parsing operand near %s", tokenizer.PeekToken().GetDescription().c_str());
		}
		operands[currentOperandIndex++] = op;
	}
	
	instructionToken.instruction->AddToAssembler(instructionToken.sValue, *this, listAction, currentOperandIndex, operands);
}

Operand* Assembler::ParseAddress(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands, int widthSpecified)
{
	DeleteWrapper<MemoryOperand> *memoryOperand = new DeleteWrapper<MemoryOperand>();
	temporaryOperands.push_back(memoryOperand);
	Operand* result = ParseAddressSum(tokenizer, *memoryOperand, temporaryOperands);

	switch(widthSpecified)
	{
	case 0:
		memoryOperand->matchBitfield = memoryOperand->IsDirectMemoryAdddress() ?
		MatchDirectMemAll : MatchMemAll;
		break;
	case 8:
		memoryOperand->matchBitfield = memoryOperand->IsDirectMemoryAdddress() ?
		MatchDirectMem8 : MatchMem8;
		break;
	case 16:
		memoryOperand->matchBitfield = memoryOperand->IsDirectMemoryAdddress() ?
		MatchDirectMem16 : MatchMem16;
		break;
	case 32:
		memoryOperand->matchBitfield = memoryOperand->IsDirectMemoryAdddress() ?
		MatchDirectMem32 : MatchMem32;
		break;
	case 64:
		memoryOperand->matchBitfield = memoryOperand->IsDirectMemoryAdddress() ?
		MatchDirectMem64 : MatchMem64;
		break;
	case 80:
		memoryOperand->matchBitfield = MatchMem80;
		break;
	}
	
	Token t = tokenizer.GetToken();
	if(t.type != Token::Type::CloseSquareBracket)
	{
		throw AssemblerException("Expected ']', found %s instead", t.GetDescription().c_str());
	}
	
	if (result->type == Operand::Type::Label) {
		result->matchBitfield &= ~MatchRelAll;
		return result;
	}

	if(memoryOperand->scale
	   && !memoryOperand->scale->IsExpression()
	   && memoryOperand->scale->value == 1)
	{
		if(memoryOperand->base == nullptr)
		{
			std::swap(memoryOperand->index, memoryOperand->base);
		}
		if(memoryOperand->index == &Register::RSP)
		{
			std::swap(memoryOperand->index, memoryOperand->base);
		}

	}

	if(memoryOperand->base
	   && (memoryOperand->base->matchBitfield & (MatchReg64 | MatchRIP)) == 0)
	{
		throw AssemblerException("jasm only supports 64 bit base registers");
	}
	
	if(memoryOperand->index
	   && (memoryOperand->index->matchBitfield & MatchReg64) == 0)
	{
		throw AssemblerException("jasm only supports 64 bit index registers");
	}
	
	if(memoryOperand->scale &&
	   !memoryOperand->scale->IsExpression()
	   && memoryOperand->scale->value != 1
	   && memoryOperand->scale->value != 2
	   && memoryOperand->scale->value != 4
	   && memoryOperand->scale->value != 8)
	{
		throw AssemblerException("Invalid scale value %" PRId64, memoryOperand->scale->value);
	}
	if(memoryOperand->index == &Register::RSP)
	{
		throw AssemblerException("RSP cannot be used as an index");
	}
	return memoryOperand;
}

RegisterOperand *Assembler::ParseRegisterExpression(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands, const Token &token, MatchBitfield matchBitField)
{
	Token t = tokenizer.GetToken();
	if(t.type != Token::Type::Expression)
	{
		throw AssemblerException("Expected expression after \"%s\" keyword, found %s instead",
				   token.GetDescription().c_str(),
				   t.GetDescription().c_str());
	}
	DeleteWrapper<RegisterOperand> *registerOperand = new DeleteWrapper<RegisterOperand>();
	temporaryOperands.push_back(registerOperand);
	registerOperand->expressionIndex = AddExpression(t.sValue, 8, tokenizer.GetCurrentLineNumber(), tokenizer.GetCurrentFileIndex());
	registerOperand->index = 0;
	registerOperand->matchBitfield = matchBitField;
	return registerOperand;
}

//============================================================================

int Assembler::AddExpression(const std::string& expression, int maxBitWidth, int sourceLine, int fileIndex)
{
	ExpressionToIndexMap::const_iterator it = expressionToIndexMap.find(expression);
	if(it != expressionToIndexMap.end())
	{
		int index = it->second;
		if(expressionList[index-1].maxBitWidth < maxBitWidth)
		{
			expressionList[index-1].maxBitWidth = maxBitWidth;
		}
		return index;
	}
	
	int index = (int) expressionList.size() + 1;

	Expression exp;
	exp.maxBitWidth = maxBitWidth;
	exp.index = index;
	exp.expression = expression;
	exp.sourceLine = sourceLine;
	exp.fileIndex = fileIndex;
	expressionList.push_back(exp);
	expressionToIndexMap.insert({expression,index});
	return index;
}

void Assembler::UpdateExpressionList()
{
	int expressionOffset = 0;
	for(Expression & e : expressionList)
	{
		if(e.maxBitWidth == 0) e.maxBitWidth = 32;
		e.offset = expressionOffset;
		switch(e.maxBitWidth)
		{
			case 8:	 expressionOffset += 1; break;
			case 16: expressionOffset += 2; break;
			case 32: expressionOffset += 4; break;
			case 64: expressionOffset += 8; break;
		}
	}
}

//============================================================================

void Assembler::Write(FILE *f, int startingLine, int *expectedFileIndex, const std::vector<std::string> &filenameList) const
{
	if(!rootListAction.HasData()) return;
	
	char *prefix = (char*) alloca(segmentIndent+1);
	memset(prefix, ' ', segmentIndent);
	prefix[segmentIndent] = '\0';
	
	fprintf(f, "%s{  // begin jasm block\n", prefix);

	ActionWriteContext context(isDataSegment, CommandLine::GetInstance().GetAssemblerVariableName());
	
	int expressionOffset = 0;
	for(const Expression &e : expressionList)
	{
		ActionWriteContext::ExpressionInfo exp;
		exp.bitWidth = e.maxBitWidth;
		exp.offset = expressionOffset;
		exp.expression = e.expression;
		exp.sourceLine = e.sourceLine;
		exp.fileIndex = e.fileIndex;
		switch(e.maxBitWidth)
		{
		case 8:
			expressionOffset++;
			break;
		case 16:
			expressionOffset += 2;
			break;
		case 0:
			exp.bitWidth = 32;		// Promote it to 32 bits now.
			[[fallthrough]];
		case 32:
			expressionOffset += 4;
			break;
		case 64:
			expressionOffset += 8;
			break;
		default:
			assert(!"Unexpected bit width");
			break;
		}
		context.expressionInfo.push_back(exp);
	}
	
	std::vector<uint8_t> bytes;
	rootListAction.WriteByteCode(bytes, context);
	bytes.push_back((int) x64Assembler::ActionType::Return);

	// Padding for some assembler-time tricks.
	for(int i = 0; i < 3; ++i) bytes.push_back(0);
	if(expressionOffset != 0)
	{
		fprintf(f, "%s#pragma pack(push, 1)\n", prefix);
		fprintf(f, "%s  struct JasmExpressionData {\n", prefix);

		// These fields are from x64 Assembler::AppendAssemblyReference.
		fprintf(f, "%s    uint16_t _referenceSize;\n", prefix);
		fprintf(f, "%s    uint16_t _blockByteCodeSize;\n", prefix);
		fprintf(f, "%s    uint32_t _forwardLabelReferenceOffset;\n", prefix);
		fprintf(f, "%s    const void* _assemblerData;\n", prefix);
		for(const auto &e : context.expressionInfo)
		{
			switch(e.bitWidth) {
			case 8:
				fprintf(f, "%s    int8_t data%d;\n", prefix, e.offset);
				break;
			case 16:
				fprintf(f, "%s    int16_t data%d;\n", prefix, e.offset);
				break;
			case 32:
				fprintf(f, "%s    int32_t data%d;\n", prefix, e.offset);
				break;
			case 64:
				fprintf(f, "%s    int64_t data%d;\n", prefix, e.offset);
				break;
			}
		}
		fprintf(f, "%s  };\n", prefix);
		fprintf(f, "%s#pragma pack(pop)\n", prefix);
		
		fprintf(f, "%s  static_assert(sizeof(JasmExpressionData) == %d, \"jasm internal error: Unexpected size for data struct\");\n", prefix, expressionOffset+16);
	}
	if(startingLine != -1) fprintf(f, "#line %d\n", startingLine);
	fprintf(f, "%s  static const uint8_t *jasmData = (const uint8_t*)", prefix);
	for(size_t i = 0; i < bytes.size(); ++i)
	{
		if(i % 16)
		{
			fprintf(f, "\\x%02x", bytes[i]);
		}
		else
		{
			fprintf(f, "%s\n%s    \"\\x%02x", i == 0 ? "" : "\"", prefix, bytes[i]);
		}
	}
	fprintf(f, "\";\n");
	
	assert(context.numberOfLabels <= 255);
	assert(context.GetNumberOfForwardLabelReferences() <= 255);

	int32_t labelData = context.numberOfLabels +
						0x100 * context.GetNumberOfForwardLabelReferences();
	
	uint32_t length;
	if(labelData != 0)
	{
		length = 0;
	}
	else
	{
		length = (uint32_t) rootListAction.GetMaximumLength();
		if(!CommandLine::GetInstance().UseSpeedOverSizeOptimizations())
		{
			if(length != rootListAction.GetMinimumLength()) length = 0;
		}
	}
	
	if(startingLine != -1) fprintf(f, "#line %d\n", startingLine);

	const int appendAssemblyReferenceSize = 16;
	if(expressionOffset > 0)
	{
		uint32_t dataSize = ((expressionOffset + appendAssemblyReferenceSize) + 7) & -8;

		if(labelData)
		{
			fprintf(f,
					"%s  JasmExpressionData *jasmExpressionData = (JasmExpressionData*) %sAppendInstructionData(%u, jasmData, %d, 0x%x);\n",
					prefix,
					context.GetAssemblerCallPrefix(),
					length,
					dataSize,
					labelData);
		}
		else
		{
			fprintf(f,
					"%s  JasmExpressionData *jasmExpressionData = (JasmExpressionData*) %sAppendInstructionData(%u, jasmData, %d);\n",
					prefix,
					context.GetAssemblerCallPrefix(),
					length,
					dataSize);
		}
		
		for(const auto &e : context.expressionInfo)
		{
			if(e.bitWidth == 0) continue;
			if (e.fileIndex != *expectedFileIndex)
			{
				fprintf(f, "#line %d \"%s\"\n", e.sourceLine, filenameList[e.fileIndex].c_str());
				*expectedFileIndex = e.fileIndex;
			}
			else
			{
				fprintf(f, "#line %d\n", e.sourceLine);
			}
			switch(e.bitWidth) {
			case 8:
				fprintf(f, "%s  jasmExpressionData->data%d = (int8_t) (%s);\n", prefix, e.offset, e.expression.c_str());
				break;
			case 16:
				fprintf(f, "%s  jasmExpressionData->data%d = (int16_t) (%s);\n", prefix, e.offset, e.expression.c_str());
				break;
			case 32:
				fprintf(f, "%s  jasmExpressionData->data%d = (int32_t) (%s);\n", prefix, e.offset, e.expression.c_str());
				break;
			case 64:
				fprintf(f, "%s  jasmExpressionData->data%d = (int64_t) (%s);\n", prefix, e.offset, e.expression.c_str());
				break;
			}
		}
	}
	else
	{
		if(labelData)
		{
			uint32_t dataSize = (appendAssemblyReferenceSize + 7) & -8;
			fprintf(f,
					"%s  %sAppendInstructionData(%u, jasmData, %d, 0x%x);\n",
					prefix,
					context.GetAssemblerCallPrefix(),
					length,
					dataSize,
					labelData);
		}
		else
		{
			fprintf(f,
					"%s  %sAppendInstructionData(%u, jasmData);\n",
					prefix,
					context.GetAssemblerCallPrefix(),
					length);
		}
	}
	fprintf(f, "%s}  // end jasm block\n", prefix);
}

//============================================================================

Operand* Assembler::ParseOperandPrimary(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands)
{
	bool globalLabel = false;
	int widthSpecified = 0;
	uint64_t labelMatch = MatchRelAll | MatchMemAll;
Next:
	int lineNumber = tokenizer.GetCurrentLineNumber();
	int fileIndex = tokenizer.GetCurrentFileIndex();
	Token token = tokenizer.PeekToken();
	switch(token.type)
	{
	case Token::Type::Register:
		tokenizer.NextToken();
		return (Operand *)token.reg;
	case Token::Type::IntegerValue:
		{
			tokenizer.NextToken();
		ParseNumericLabel:
			Token nextToken = tokenizer.PeekToken();
		   	if(nextToken.type == Token::Type::Identifier && nextToken.sValue == "b")
			{
				tokenizer.NextToken();
				DeleteWrapper<LabelOperand> *labelOperand = new DeleteWrapper<LabelOperand>();
				temporaryOperands.push_back(labelOperand);
				labelOperand->global = globalLabel;
				labelOperand->jumpType = LabelOperand::JumpType::Backward;
				labelOperand->matchBitfield = labelMatch;
				labelOperand->labelValue = token.GetLabelValue();
				labelOperand->reference = labelReference++;
				return labelOperand;
			}
			else if(nextToken.type == Token::Type::Identifier && nextToken.sValue == "f")
			{
				tokenizer.NextToken();
				DeleteWrapper<LabelOperand> *labelOperand = new DeleteWrapper<LabelOperand>();
				temporaryOperands.push_back(labelOperand);
				labelOperand->global = globalLabel;
				labelOperand->jumpType = LabelOperand::JumpType::Forward;
				labelOperand->matchBitfield = labelMatch;
				labelOperand->labelValue = token.GetLabelValue();
				labelOperand->reference = labelReference++;
				return labelOperand;
			}
			else if(nextToken.type == Token::Type::Identifier && nextToken.sValue == "bf")
			{
				tokenizer.NextToken();
				DeleteWrapper<LabelOperand> *labelOperand = new DeleteWrapper<LabelOperand>();
				temporaryOperands.push_back(labelOperand);
				labelOperand->global = globalLabel;
				labelOperand->jumpType = LabelOperand::JumpType::BackwardOrForward;
				labelOperand->matchBitfield = labelMatch;
				labelOperand->labelValue = token.GetLabelValue();
				labelOperand->reference = labelReference++;
				return labelOperand;
			}
			else
			{
			ProcessIntegerValue:
				DeleteWrapper<ImmediateOperand> *immediateOperand = new DeleteWrapper<ImmediateOperand>(token.iValue);
				temporaryOperands.push_back(immediateOperand);
				
				int64_t value = token.iValue;
				if(value == (int8_t) value)
				{
					immediateOperand->matchBitfield = MatchImmAll;
					switch(value)
					{
					case 0: immediateOperand->matchBitfield |= MatchConstant0; break;
					case 1: immediateOperand->matchBitfield |= MatchConstant1; break;
					case 3: immediateOperand->matchBitfield |= MatchConstant3; break;
					}
				}
				else if(value == (uint8_t) value)
				{
					immediateOperand->matchBitfield = MatchUImm32 | MatchImm16 | MatchImm32 | MatchImm64;
				}
				else if(value == (int16_t) value)
				{
					immediateOperand->matchBitfield = MatchImm16 | MatchImm32 | MatchImm64;
				}
				else if(value == (int32_t) value)
				{
					immediateOperand->matchBitfield = MatchImm32 | MatchImm64;
				}
				else
				{
					immediateOperand->matchBitfield = MatchImm64;
				}
				if((value >> 32) == 0) immediateOperand->matchBitfield |= MatchUImm32;
				return immediateOperand;
			}
		}
	case Token::Type::RealValue:
		{
			tokenizer.NextToken();
			DeleteWrapper<ImmediateOperand> *immediateOperand = new DeleteWrapper<ImmediateOperand>(token.rValue);
			temporaryOperands.push_back(immediateOperand);
			immediateOperand->matchBitfield = MatchImm32 | MatchImm64;
			return immediateOperand;
		}
	case Token::Type::Expression:
		{
			tokenizer.NextToken();
			// Potentially relative label.
		ParseExpressionLabel:
			int expressionIndex = AddExpression(token.sValue, widthSpecified, lineNumber, fileIndex);
			Token nextToken = tokenizer.PeekToken();
			if(nextToken.type == Token::Type::Identifier && nextToken.sValue == "b")
			{
				tokenizer.NextToken();
				UpdateExpressionBitWidth(expressionIndex, 32);
				// Backwards reference to expression label.
				DeleteWrapper<LabelOperand> *labelOperand = new DeleteWrapper<LabelOperand>();
				temporaryOperands.push_back(labelOperand);
				labelOperand->global = globalLabel;
				labelOperand->jumpType = LabelOperand::JumpType::Backward;
				labelOperand->matchBitfield = labelMatch;
				labelOperand->expressionIndex = expressionIndex;
				labelOperand->reference = labelReference++;
				return labelOperand;
			}
			else if(nextToken.type == Token::Type::Identifier && nextToken.sValue == "f")
			{
				tokenizer.NextToken();
				UpdateExpressionBitWidth(expressionIndex, 32);
				// Forward reference to expression label.
				DeleteWrapper<LabelOperand> *labelOperand = new DeleteWrapper<LabelOperand>();
				temporaryOperands.push_back(labelOperand);
				labelOperand->global = globalLabel;
				labelOperand->jumpType = LabelOperand::JumpType::Forward;
				labelOperand->matchBitfield = labelMatch;
				labelOperand->expressionIndex = expressionIndex;
				labelOperand->reference = labelReference++;
				return labelOperand;
			}
			else if(nextToken.type == Token::Type::Identifier && nextToken.sValue == "bf")
			{
				tokenizer.NextToken();
				UpdateExpressionBitWidth(expressionIndex, 32);
				// Forward reference to expression label.
				DeleteWrapper<LabelOperand> *labelOperand = new DeleteWrapper<LabelOperand>();
				temporaryOperands.push_back(labelOperand);
				labelOperand->global = globalLabel;
				labelOperand->jumpType = LabelOperand::JumpType::BackwardOrForward;
				labelOperand->matchBitfield = labelMatch;
				labelOperand->expressionIndex = expressionIndex;
				labelOperand->reference = labelReference++;
				return labelOperand;
			}

			DeleteWrapper<ImmediateOperand> *immediateOperand = new DeleteWrapper<ImmediateOperand>(int64_t(0));
			temporaryOperands.push_back(immediateOperand);
			immediateOperand->expressionIndex = AddExpression(token.sValue, 0, lineNumber, fileIndex);
			switch(widthSpecified)
			{
			case 0: immediateOperand->matchBitfield = MatchImmAll|MatchRel32; break;
			case 8: immediateOperand->matchBitfield = MatchImm8; break;
			case 16: immediateOperand->matchBitfield = MatchImm16; break;
			case 32: immediateOperand->matchBitfield = MatchImm32|MatchUImm32; break;
			case 64: immediateOperand->matchBitfield = MatchImm64|MatchRel32; break;
			default:
				throw AssemblerException("Unexpected width %d specified", widthSpecified);
			}
			return immediateOperand;
		}
	case Token::Type::Star:
		{
			tokenizer.NextToken();
			globalLabel = true;
			token = tokenizer.GetToken();
			switch(token.type)
			{
			case Token::Type::Identifier:
				goto ParseNamedLabel;
			case Token::Type::IntegerValue:
				goto ParseNumericLabel;
			case Token::Type::Expression:
				goto ParseExpressionLabel;
			default:
				throw AssemblerException("Unexpected token after star: %s", token.GetDescription().c_str());
			}
		}
	case Token::Type::Identifier:
		tokenizer.NextToken();
		if(token.sValue == "byte" || token.sValue == "imm8")
		{
			widthSpecified = 8;
			labelMatch = MatchMem8;
			goto Next;
		}
		if(token.sValue == "word" || token.sValue == "imm16")
		{
			widthSpecified = 16;
			labelMatch = MatchMem16;
			goto Next;
		}
		if(token.sValue == "dword" || token.sValue == "imm32")
		{
			widthSpecified = 32;
			labelMatch = MatchMem32;
			goto Next;
		}
		if(token.sValue == "qword" || token.sValue == "imm64")
		{
			widthSpecified = 64;
			labelMatch = MatchMem64;
			goto Next;
		}
		if(token.sValue == "tbyte" || token.sValue == "imm80")
		{
			widthSpecified = 80;
			labelMatch = MatchMem80;
			goto Next;
		}
		if(token.sValue == "ptr")
		{
			if(widthSpecified == 0)
			{
				throw AssemblerException("Unexpected \"ptr\"");
			}
			Token t = tokenizer.GetToken();
			if(t.type != Token::Type::OpenSquareBracket)
			{
				throw AssemblerException("Expected '[', found %s instead", t.GetDescription().c_str());
			}
			
			return ParseAddress(tokenizer, temporaryOperands, widthSpecified);
		}
			
		// Parse reg32, reg64, etc.
		{
			auto it = ParameterizedRegisterMap::instance.find(token.sValue);
			if(it != ParameterizedRegisterMap::instance.end())
			{
				return ParseRegisterExpression(tokenizer,
											   temporaryOperands,
											   token,
											   it->second);
			}
		}


		if(token.sValue == "short")
		{
			labelMatch = MatchRel8;
			goto Next;
		}
		else if(token.sValue == "near")
		{
			labelMatch = MatchRel32;
			goto Next;
		}
		
		// Block to scope labelOperand.
		{
		ParseNamedLabel:
			DeleteWrapper<LabelOperand> *labelOperand = new DeleteWrapper<LabelOperand>();
			temporaryOperands.push_back(labelOperand);
			labelOperand->global = globalLabel;
			labelOperand->jumpType = LabelOperand::JumpType::Name;
			labelOperand->matchBitfield = labelMatch ? labelMatch : (MatchRelAll | MatchMemAll);
			labelOperand->labelName = token.sValue;
			labelOperand->reference = labelReference++;
			return labelOperand;
		}
	case Token::Type::OpenParenthesis:
		{
			tokenizer.NextToken();
			Operand *result = ParseOperandSum(tokenizer, temporaryOperands);
			if(tokenizer.GetTokenType() != Token::Type::CloseParenthesis)
			{
				throw AssemblerException("Expected CloseParenthesis");
			}
			return result;
		}
	case Token::Type::OpenSquareBracket:
		tokenizer.NextToken();
		return ParseAddress(tokenizer, temporaryOperands, widthSpecified);

	default:
		return nullptr;
	}
}

Operand* Assembler::ParseOperandUnary(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands)
{
	bool hasSign = false;
	int sign = 1;
	switch(tokenizer.PeekToken().type)
	{
	case Token::Type::Add:
		hasSign = true;
		tokenizer.NextToken();
		break;
	case Token::Type::Subtract:
		sign = -1;
		hasSign = true;
		tokenizer.NextToken();
		break;
	default:
		break;
	}
	Operand* result = ParseOperandPrimary(tokenizer, temporaryOperands);
	if(hasSign)
	{
		if(result->type != Operand::Type::Immediate)
		{
			throw AssemblerException("Unable to process unary on non-immediate operand");
		}
		if(result->immediateType == Operand::ImmediateType::Integer)
		{
			((ImmediateOperand *)result)->value *= sign;
		}
		else
		{
			((ImmediateOperand *)result)->realValue *= sign;
		}
	}
	return result;
}

Operand* Assembler::ParseOperandProduct(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands)
{
	Operand *result = ParseOperandUnary(tokenizer, temporaryOperands);
	if(!result || result->type != Operand::Type::Immediate) return result;
	ImmediateOperand *immResult = (ImmediateOperand *)result;
	
	for(;;)
	{
		Token op = tokenizer.PeekToken();
		if(op.type != Token::Type::Star && op.type != Token::Type::Divide) break;
		
		tokenizer.NextToken();
		
		Operand *temp = ParseOperandUnary(tokenizer, temporaryOperands);
		if(!temp || temp->type != Operand::Type::Immediate) return nullptr;
		
		assert(result && result->type == Operand::Type::Immediate);
		int64_t a = immResult->value;
		int64_t b = ((const ImmediateOperand*)temp)->value;
		int64_t value = (op.type == Token::Type::Star) ? a * b : a / b;
		immResult->value = value;
		immResult->matchBitfield |= temp->matchBitfield;
	}
	return immResult;
}

Operand* Assembler::ParseOperandSum(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands)
{
	Operand *result = ParseOperandProduct(tokenizer, temporaryOperands);
	if(!result) return nullptr;
	if(result->type != Operand::Type::Immediate
	   && result->type != Operand::Type::Label) return result;
	
	for(;;)
	{
		Token op = tokenizer.PeekToken();
		if(op.type != Token::Type::Add && op.type != Token::Type::Subtract) break;
		
		tokenizer.NextToken();
		
		Operand *temp = ParseOperandProduct(tokenizer, temporaryOperands);
		if(!temp) return nullptr;
		if(result->type == Operand::Type::Label
		   && temp->type == Operand::Type::Immediate)
		{
			LabelOperand *label = (LabelOperand*) result;
			ImmediateOperand *imm0 = label->displacement;
			ImmediateOperand *imm1 = (ImmediateOperand*) temp;

			if(!imm0)
			{
				label->displacement = imm1;
				if(op.type == Token::Type::Subtract) label->displacement->value = -label->displacement->value;
			}
			else
			{
				imm0->value = (op.type == Token::Type::Add) ?
								(int32_t) (imm0->value + imm1->value) :
								(int32_t) (imm0->value - imm1->value);
			}
		}
		else if(result->type == Operand::Type::Immediate
				&& temp->type == Operand::Type::Label)
		{
			assert(op.type == Token::Type::Add);
			ImmediateOperand *imm0 = (ImmediateOperand*) result;
			LabelOperand *label = (LabelOperand*) temp;
			ImmediateOperand *imm1 = label->displacement;

			if(!imm1)
			{
				label->displacement = imm0;
			}
			else
			{
				imm1->value += imm0->value;
			}
			result = label;
		}
		else if(result->type == Operand::Type::Immediate
			  	&& temp->type == Operand::Type::Immediate)
		{
			ImmediateOperand *immResult = (ImmediateOperand *)result;
			int64_t a = immResult->value;
			int64_t b = ((const ImmediateOperand*)temp)->value;
			int64_t value = (op.type == Token::Type::Add) ? a + b : a - b;
			immResult->value = value;
			immResult->matchBitfield |= temp->matchBitfield;
		}
		else
		{
			return nullptr;
		}
	}
	return result;
}

Operand* Assembler::ParseOperandComparison(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands)
{
	Operand *result = ParseOperandSum(tokenizer, temporaryOperands);
	if(!result) return nullptr;
	if(result->type != Operand::Type::Immediate) return result;
	
	for(;;)
	{
		Token op = tokenizer.PeekToken();
		switch(op.type)
		{
		case Token::Type::Equals:
		case Token::Type::NotEquals:
		case Token::Type::LessThan:
		case Token::Type::LessThanOrEquals:
		case Token::Type::GreaterThan:
		case Token::Type::GreaterThanOrEquals:
			tokenizer.NextToken();
			break;

		default:
			return result;
		}
		
		Operand *rhs = ParseOperandSum(tokenizer, temporaryOperands);
		if(!rhs) return nullptr;
		if(rhs->type != Operand::Type::Immediate) throw AssemblerException("Unable to parse comparison");

		switch(op.type)
		{
		case Token::Type::Equals:
			((ImmediateOperand*) result)->value = (*(ImmediateOperand *) result == *(ImmediateOperand *) rhs);
			break;
			
		case Token::Type::NotEquals:
			((ImmediateOperand*) result)->value = (*(ImmediateOperand *) result != *(ImmediateOperand *) rhs);
			break;
				
		case Token::Type::LessThan:
			((ImmediateOperand*) result)->value = (*(ImmediateOperand *) result < *(ImmediateOperand *) rhs);
			break;

		case Token::Type::LessThanOrEquals:
			((ImmediateOperand*) result)->value = (*(ImmediateOperand *) result <= *(ImmediateOperand *) rhs);
			break;

		case Token::Type::GreaterThan:
			((ImmediateOperand*) result)->value = (*(ImmediateOperand *) result > *(ImmediateOperand *) rhs);
			break;

		case Token::Type::GreaterThanOrEquals:
			((ImmediateOperand*) result)->value = (*(ImmediateOperand *) result >= *(ImmediateOperand *) rhs);
			break;
			
		default:
			break;
		}
		result->immediateType = Operand::ImmediateType::Integer;
		result->matchBitfield = MatchImmAll;
	}
}

Operand* Assembler::ParseOperandLogical(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands)
{
	Operand *result = ParseOperandComparison(tokenizer, temporaryOperands);
	if(!result) return nullptr;
	if(result->type != Operand::Type::Immediate) return result;
	
	for(;;)
	{
		Token op = tokenizer.PeekToken();
		
		switch(op.type)
		{
		case Token::Type::LogicalOr:
		case Token::Type::LogicalAnd:
			tokenizer.NextToken();
			break;

		default:
			return result;
		}
		
		Operand *rhs = ParseOperandComparison(tokenizer, temporaryOperands);
		if(!rhs) return nullptr;
		if(rhs->type != Operand::Type::Immediate) throw AssemblerException("Unable to parse logical statement");

		switch(op.type)
		{
		case Token::Type::LogicalOr:
			((ImmediateOperand*) result)->value = (((ImmediateOperand *) result)->AsBool() || ((ImmediateOperand *) rhs)->AsBool());
			break;
			
		case Token::Type::LogicalAnd:
			((ImmediateOperand*) result)->value = (((ImmediateOperand *) result)->AsBool() && ((ImmediateOperand *) rhs)->AsBool());
			break;
			
		default:
			break;
		}
		result->immediateType = Operand::ImmediateType::Integer;
		result->matchBitfield = MatchImmAll;
	}
}

Operand* Assembler::ParseAddressProduct(Tokenizer &tokenizer, MemoryOperand &memoryOperand, AutoDeleteVector &temporaryOperands)
{
	bool assignedScale = false;
	
	Operand *result = ParseOperandPrimary(tokenizer, temporaryOperands);
	for(;;)
	{
		Token op = tokenizer.PeekToken();
		if(op.type != Token::Type::Star
		   && op.type != Token::Type::Divide) break;
		
		tokenizer.NextToken();
		
		Operand *temp = ParseOperandPrimary(tokenizer, temporaryOperands);
		if(op.type == Token::Type::Star)
		{
			if(result->type == Operand::Type::Register
			   && temp->type == Operand::Type::Immediate)
			{
				if(assignedScale || memoryOperand.index != nullptr)
				{
					throw AssemblerException("Already have an index for this address");
				}
				memoryOperand.index = (const RegisterOperand*)result;
				memoryOperand.scale = (const ImmediateOperand*)temp;
				if(memoryOperand.scale->IsExpression())
				{
					UpdateExpressionBitWidth(memoryOperand.scale->expressionIndex, 8);
				}
				result = temp;
				temp = nullptr;
				assignedScale = true;
			}
			else if(result->type == Operand::Type::Immediate
					&& temp->type == Operand::Type::Register)
			{
				if(assignedScale || memoryOperand.index != nullptr)
				{
					throw AssemblerException("Already have an index for this address");
				}
				memoryOperand.index = (const RegisterOperand *)temp;
				memoryOperand.scale = (const ImmediateOperand *)result;
				if(memoryOperand.scale->IsExpression())
				{
					UpdateExpressionBitWidth(memoryOperand.scale->expressionIndex, 8);
				}
				temp = nullptr;
				assignedScale = true;
			}
		}
		assert(result && result->type == Operand::Type::Immediate);
		int64_t a = (result->type == Operand::Type::Immediate) ? ((const ImmediateOperand*)result)->value : 1;
		int64_t b = (temp && temp->type == Operand::Type::Immediate) ? ((const ImmediateOperand*)temp)->value : 1;
		int64_t value = (op.type == Token::Type::Star) ? a * b : a / b;
		((ImmediateOperand*)result)->value = value;
	}
	if(assignedScale)
	{
		DeleteWrapper<ImmediateOperand> *immediateOperand = new DeleteWrapper<ImmediateOperand>(int64_t(0));
		temporaryOperands.push_back(immediateOperand);
		return immediateOperand;
	}
	return result;
}

Operand* Assembler::ParseAddressSum(Tokenizer &tokenizer, MemoryOperand &memoryOperand, AutoDeleteVector &temporaryOperands)
{
	Operand *result = ParseAddressProduct(tokenizer, memoryOperand, temporaryOperands);
	if(result->type == Operand::Type::Register)
	{
		memoryOperand.base = (RegisterOperand*)result;
	}
	else if(result->type == Operand::Type::Immediate && result->IsExpression())
	{
		memoryOperand.displacement = (ImmediateOperand *) result;
		UpdateExpressionBitWidth(result->expressionIndex, 32);
	}
	for(;;)
	{
		Token op = tokenizer.PeekToken();
		if(op.type != Token::Type::Add
		   && op.type != Token::Type::Subtract) break;
		
		tokenizer.NextToken();
		
		Operand *temp = ParseAddressProduct(tokenizer, memoryOperand, temporaryOperands);
		if(temp->type == Operand::Type::Register && op.type == Token::Type::Add)
		{
			if(memoryOperand.base == nullptr)
			{
				memoryOperand.base = (RegisterOperand*)temp;
			}
			else if(memoryOperand.index == nullptr)
			{
				memoryOperand.index = (RegisterOperand*)temp;
			}
			else
			{
				throw AssemblerException("Unable to parse address");
			}
		}
		else if(temp->type == Operand::Type::Immediate && temp->IsExpression())
		{
			if(memoryOperand.displacement)
			{
				throw AssemblerException("Displacement already set");
			}
			memoryOperand.displacement = (ImmediateOperand*)temp;
			UpdateExpressionBitWidth(temp->expressionIndex, 32);
		}
		
		int64_t a = (result->type == Operand::Type::Immediate) ? ((ImmediateOperand*)result)->value : 0;
		int64_t b = (temp->type == Operand::Type::Immediate) ? ((ImmediateOperand*)temp)->value : 0;
		
		result = result->type == Operand::Type::Immediate ? result :
					temp->type == Operand::Type::Immediate ? temp : nullptr;
		if(!result)
		{
			DeleteWrapper<ImmediateOperand> *immediateOperand = new DeleteWrapper<ImmediateOperand>(int64_t(0));
			temporaryOperands.push_back(immediateOperand);
			result = immediateOperand;
		}
		int64_t value = (op.type == Token::Type::Add) ? a + b : a - b;
		((ImmediateOperand *)result)->value = value;
	}
	
	if(result->type == Operand::Type::Immediate)
	{
		if(!result->IsExpression() && ((ImmediateOperand*) result)->value != 0)
		{
			if(memoryOperand.displacement)
			{
				throw AssemblerException("Displacement already set");
			}
			memoryOperand.displacement = (ImmediateOperand*)result;
		}
	}
	
	return result;
}

//============================================================================
