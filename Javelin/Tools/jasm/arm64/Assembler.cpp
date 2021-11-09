//============================================================================

#include "Javelin/Tools/jasm/arm64/Assembler.h"

#include "Javelin/Assembler/arm64/ActionType.h"
#include "Javelin/Assembler/arm64/RelEncoding.h"
#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/CommandLine.h"
#include "Javelin/Tools/jasm/Log.h"
#include "Javelin/Tools/jasm/arm64/Instruction.h"
#include "Javelin/Tools/jasm/arm64/Register.h"
#include "Javelin/Tools/jasm/arm64/Tokenizer.h"

#include <assert.h>
#include <string.h>

//============================================================================

using namespace Javelin::arm64Assembler;
namespace Javelin::Assembler::arm64
{
//============================================================================

class OperandMap : public std::unordered_map<std::string, Operand*>
{
public:
	OperandMap()
	{
		(*this)["lsl"] = new Operand(Operand::Shift::LSL, MatchShift|MatchLSL);
		(*this)["lsr"] = new Operand(Operand::Shift::LSR, MatchShift);
		(*this)["asr"] = new Operand(Operand::Shift::ASR, MatchShift);
		(*this)["ror"] = new Operand(Operand::Shift::ROR, MatchROR);

		(*this)["uxtb"] = new Operand(Operand::Extend::UXTB, MatchExtendReg32);
		(*this)["uxth"] = new Operand(Operand::Extend::UXTH, MatchExtendReg32);
		(*this)["uxtw"] = new Operand(Operand::Extend::UXTW, MatchExtendReg32|MatchExtendLoadStoreReg32);
		(*this)["uxtx"] = new Operand(Operand::Extend::UXTX, MatchExtendReg64);
		(*this)["sxtb"] = new Operand(Operand::Extend::SXTB, MatchExtendReg32);
		(*this)["sxth"] = new Operand(Operand::Extend::SXTH, MatchExtendReg32);
		(*this)["sxtw"] = new Operand(Operand::Extend::SXTW, MatchExtendReg32|MatchExtendLoadStoreReg32);
		(*this)["sxtx"] = new Operand(Operand::Extend::SXTX, MatchExtendReg64);

		(*this)["eq"] = new Operand(Operand::Condition::EQ);
		(*this)["ne"] = new Operand(Operand::Condition::NE);
		(*this)["cs"] = new Operand(Operand::Condition::CS);
		(*this)["cc"] = new Operand(Operand::Condition::CC);
		(*this)["mi"] = new Operand(Operand::Condition::MI);
		(*this)["pl"] = new Operand(Operand::Condition::PL);
		(*this)["vs"] = new Operand(Operand::Condition::VS);
		(*this)["vc"] = new Operand(Operand::Condition::VC);
		(*this)["hi"] = new Operand(Operand::Condition::HI);
		(*this)["ls"] = new Operand(Operand::Condition::LS);
		(*this)["ge"] = new Operand(Operand::Condition::GE);
		(*this)["lt"] = new Operand(Operand::Condition::LT);
		(*this)["gt"] = new Operand(Operand::Condition::GT);
		(*this)["le"] = new Operand(Operand::Condition::LE);
		(*this)["al"] = new Operand(Operand::Condition::AL);

		(*this)["hs"] = new Operand(Operand::Condition::CS);
		(*this)["lo"] = new Operand(Operand::Condition::CC);
	}
	
	static const OperandMap instance;
};

const OperandMap OperandMap::instance;

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
		rootListAction.Group();
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
				if(token.sValue == ".byte"
						|| token.sValue == ".hword"
						|| token.sValue == ".word"
						|| token.sValue == ".quad")
				{
					ParseLine(rootListAction, tokenizer, token);
					break;
				}
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
					if(!condition) throw AssemblerException("Unable to parse .elseif condition");
					continue;
				}
				ParsePreprocessor(*sublist, tokenizer, token, filenameList);
				continue;
					
			default:
				try
				{
					tokenizer.NextToken();
					ParseLine(*sublist, tokenizer, token);
				}
				catch(AssemblerException &ex)
				{
					ex.PopulateLocationIfNecessary(lineNumber, fileIndex);
					for(const auto &it : ex) Log::Error("%s", it.GetLogString(filenameList).c_str());
				}
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
	if(token.sValue == "delta21" || token.sValue == "adr")
	{
		tokenizer.NextToken();
		token = tokenizer.GetToken();
		if(token.type != Token::Type::Expression)
		{
			throw AssemblerException("Unexpected token: %s (Expected expression for delta21 condition)", token.GetDescription().c_str());
		}
		int expressionIndex = AddExpression(token.sValue, 64, tokenizer.GetCurrentLineNumber(), tokenizer.GetCurrentFileIndex());
		return new Delta21AlternateActionCondition(expressionIndex);
	}
	else if(token.sValue == "delta26x4")
	{
		tokenizer.NextToken();
		token = tokenizer.GetToken();
		if(token.type != Token::Type::Expression)
		{
			throw AssemblerException("Unexpected token: %s (Expected expression for delta26x4 condition)", token.GetDescription().c_str());
		}
		int expressionIndex = AddExpression(token.sValue, 64, tokenizer.GetCurrentLineNumber(), tokenizer.GetCurrentFileIndex());
		return new Delta26x4AlternateActionCondition(expressionIndex);
	}
	else if(token.sValue == "adrp")
	{
		tokenizer.NextToken();
		token = tokenizer.GetToken();
		if(token.type != Token::Type::Expression)
		{
			throw AssemblerException("Unexpected token: %s (Expected expression for adrp condition)", token.GetDescription().c_str());
		}
		int expressionIndex = AddExpression(token.sValue, 64, tokenizer.GetCurrentLineNumber(), tokenizer.GetCurrentFileIndex());
		return new AdrpAlternateActionCondition(expressionIndex);
	}
	else if(token.type == Token::Type::Expression)
	{
		tokenizer.NextToken();
		std::string expression = "!(" + token.sValue + ")";
		
		int expressionIndex = AddExpression(expression, 8, tokenizer.GetCurrentLineNumber(), tokenizer.GetCurrentFileIndex());
		return new ZeroAlternateActionCondition(expressionIndex);
	}
	else
	{
		AutoDeleteVector temporaryOperands;
		Operand *result = ParseOperand(tokenizer, temporaryOperands);
		if(result->type != Operand::Type::Number)
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
				throw AssemblerException("Unexpected token after star: %s", token.GetDescription().c_str());
			}
			break;
		case Token::Type::Preprocessor:
			{
				if(token.sValue == ".byte")
				{
					AutoDeleteVector temporaryOperands;
					for(;;)
					{
						Operand *operand = ParseOperand(tokenizer, temporaryOperands);
						if(!operand || operand->type != Operand::Type::Number) throw AssemblerException("Parse error");

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
						if(token.type != Token::Type::Comma) throw AssemblerException("Expected comma. Found %s", token.GetDescription().c_str());
					}
					break;
				}
				if(token.sValue == ".hword")
				{
					AutoDeleteVector temporaryOperands;
					for(;;)
					{
						Operand *operand = ParseOperand(tokenizer, temporaryOperands);
						if(!operand || operand->type != Operand::Type::Number) throw AssemblerException("Parse error");

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
						if(token.type != Token::Type::Comma) throw AssemblerException("Expected comma. Found %s", token.GetDescription().c_str());
					}
					break;
				}
				if(token.sValue == ".word")
				{
					AutoDeleteVector temporaryOperands;
					for(;;)
					{
						Operand *operand = ParseOperand(tokenizer, temporaryOperands);
						if(!operand || operand->type != Operand::Type::Number) throw AssemblerException("Parse error");

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
						else
						{
							throw AssemblerException("Parse error");
						}
						
						token = tokenizer.GetToken();
						if(token.type == Token::Type::EndOfFile
						   || token.type == Token::Type::Newline) break;
						if(token.type != Token::Type::Comma) throw AssemblerException("Expected comma. Found %s", token.GetDescription().c_str());
					}
					break;
				}
				if(token.sValue == ".quad")
				{
					AutoDeleteVector temporaryOperands;
					for(;;)
					{
						Operand *operand = ParseOperand(tokenizer, temporaryOperands);
						if(!operand) throw AssemblerException("Parse error");

						if(operand->type == Operand::Type::Label)
						{
							uint64_t offset = 0;
							listAction.Append(new LiteralAction({(uint8_t*) &offset, (uint8_t*) &offset + 8}));
							listAction.Append(((LabelOperand*) operand)->CreatePatchAction(RelEncoding::Rel64, 8));
							listAction.Append(new PatchAbsoluteAddressAction());
						}
						else if(operand->type == Operand::Type::Number)
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
						else
						{
							throw AssemblerException("Unexpected token: %s\n", token.GetDescription().c_str());
						}

						token = tokenizer.GetToken();
						if(token.type == Token::Type::EndOfFile
						   || token.type == Token::Type::Newline) break;
						if(token.type != Token::Type::Comma) throw AssemblerException("Expected comma. Found %s", token.GetDescription().c_str());
					}
					break;
				}
				throw AssemblerException("Unexpected preprocessor token: %s", token.sValue.c_str());
			}
		case Token::Type::Identifier:
			{
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
				if(nextToken.type == Token::Type::Preprocessor)
				{
					if(nextToken.sValue == ".byte")
					{
						listAction.Append(new NamedLabelAction(globalLabel, token.sValue, 1));
						break;
					}
					if(nextToken.sValue == ".hword")
					{
						listAction.Append(new NamedLabelAction(globalLabel, token.sValue, 2));
						break;
					}
					if(nextToken.sValue == ".word")
					{
						listAction.Append(new NamedLabelAction(globalLabel, token.sValue, 4));
						break;
					}
					if(nextToken.sValue == ".quad")
					{
						listAction.Append(new NamedLabelAction(globalLabel, token.sValue, 8));
						break;
					}
				}
				
				throw AssemblerException("Unknown opcode %s", token.sValue.c_str());
			}
			break;
			
		case Token::Type::IntegerValue:
		// The only case where an immediate is allowed at the start of a line
		// is for a label.
		ParseNumericLabel:
			if(tokenizer.PeekToken().type != Token::Type::Colon) throw AssemblerException("Expected colon to define label");
			tokenizer.NextToken();
			listAction.Append(new NumericLabelAction(globalLabel, token.GetLabelValue()));
			break;
			
		case Token::Type::Instruction:
			ParseInstruction(listAction, tokenizer, token);
			break;
			
		case Token::Type::Expression:
		// An expression can start the line if it's a label.
		// It is always considered global.
		ParseExpressionLabel:
			{
				if(tokenizer.PeekToken().type != Token::Type::Colon) throw AssemblerException("Expected colon to define label");
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
			throw AssemblerException("Unexpected token: %s\n", token.GetDescription().c_str());
		}
	}
	catch(AssemblerException &ex)
	{
		tokenizer.SkipToEndOfLine();
		throw;
	}
}

void Assembler::ParseInstruction(ListAction& listAction, Tokenizer &tokenizer, const Token &instructionToken)
{
	const Operand *operands[20];
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
		
		if(currentOperandIndex >= sizeof(operands)/sizeof(operands[0]))
		{
			throw AssemblerException("Too many operands on line: %s", tokenizer.GetCurrentLine().c_str());
		}
		
		Operand *op = ParseOperand(tokenizer, temporaryOperands);
		if(op == nullptr)
		{
			throw AssemblerException("Error parsing operand near: %s", tokenizer.PeekToken().GetDescription().c_str());
		}
		operands[currentOperandIndex++] = op;
	}
	
	// Resolve Reg+1 cases
	for(int i = currentOperandIndex-1; i >= 2; --i)
	{
		if(operands[i-2]->type == Operand::Type::Register
		   && operands[i-1]->type == Operand::Type::Syntax
		   && operands[i-1]->syntax == Operand::Syntax::Comma
		   && operands[i]->type == Operand::Type::Register)
		{
			MatchBitfield extraMatch = 0;
			if(operands[i-2]->matchBitfield == operands[i]->matchBitfield)
			{
				extraMatch = MatchRep;
			}
			
			if((operands[i-2]->matchBitfield & operands[i]->matchBitfield) != 0
				&& operands[i-2]->index+1 == operands[i]->index)
			{
				extraMatch |= MatchRegNP1;
			}
			
			if(extraMatch != 0)
			{
				// Need to tag the register as MatchRegN+1, so take a copy first.
				DeleteWrapper<RegisterOperand> *reg = new DeleteWrapper<RegisterOperand>(*(RegisterOperand*) operands[i]);
				reg->matchBitfield |= extraMatch;
				temporaryOperands.push_back(reg);
				operands[i] = reg;
			}
		}
	}

	instructionToken.instruction->AddToAssembler(instructionToken.sValue, *this, listAction, currentOperandIndex, operands);
}

RegisterOperand *Assembler::ParseRegisterExpression(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands, const Token &token, int registerData, MatchBitfield matchBitField)
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
	registerOperand->registerData = registerData;
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
        Expression &exp = expressionList[index-1];
        if(exp.maxBitWidth < maxBitWidth) exp.maxBitWidth = maxBitWidth;
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
	for(Expression &e : expressionList)
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
    static const int appendAssemblerReferenceSize = 12;
    
	if(!rootListAction.HasData()) return;

	char *prefix = (char*) alloca(segmentIndent+1);
	memset(prefix, ' ', segmentIndent);
	prefix[segmentIndent] = '\0';
	
	fprintf(f, "%s{  // begin jasm block\n", prefix);

	ActionWriteContext context(isDataSegment, appendAssemblerReferenceSize, CommandLine::GetInstance().GetAssemblerVariableName());
	
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
		case 0:
			// expression not used.
			break;
		case 8:
			expressionOffset++;
			break;
		case 16:
			expressionOffset += 2;
			break;
		case 32:
			expressionOffset += 4;
			break;
		case 64:
			expressionOffset += 8;
			break;
		default:
			throw AssemblerException("Unexpected bit width");
		}
		context.expressionInfo.push_back(exp);
	}
	
	std::vector<uint8_t> bytes;
	rootListAction.WriteByteCode(bytes, context);
	bytes.push_back((int) arm64Assembler::ActionType::Return);

	// Padding for some assembler-time tricks.
	for(int i = 0; i < 3; ++i) bytes.push_back(0);
	if(expressionOffset != 0)
	{
		fprintf(f, "%s#pragma pack(push, 1)\n", prefix);
		fprintf(f, "%s  struct JasmExpressionData {\n", prefix);
		
		// These fields are from arm64 Assembler::AppendAssemblyReference.
		fprintf(f, "%s    const void* _assemblerData;\n", prefix);
		fprintf(f, "%s    uint32_t _referenceSize;\n", prefix);
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
		
		fprintf(f, "%s  static_assert(sizeof(JasmExpressionData) == %d, \"jasm internal error: Unexpected size for data struct\");\n", prefix, expressionOffset+appendAssemblerReferenceSize);
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
	
	uint32_t length = (uint32_t) rootListAction.GetMaximumLength();
	
	if(startingLine != -1) fprintf(f, "#line %d\n", startingLine);
	if(expressionOffset > 0)
	{
		const int appendAssemblyReferenceSize = 12;
		uint32_t dataSize = ((expressionOffset + appendAssemblyReferenceSize) + 7) & -8;
		
		if(labelData)
		{
			fprintf(f,
					"%s  JasmExpressionData *jasmExpressionData = (JasmExpressionData*) %sAppendInstructionData(%u, jasmData, %u, 0x%x);\n",
					prefix,
					context.GetAssemblerCallPrefix(),
					length,
					dataSize,
					labelData);
		}
		else
		{
			fprintf(f,
					"%s  JasmExpressionData *jasmExpressionData = (JasmExpressionData*) %sAppendInstructionData(%u, jasmData, %u);\n",
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
			const int appendAssemblyReferenceSize = 12;
			uint32_t dataSize = (appendAssemblyReferenceSize + 7) & -8;
			fprintf(f,
					"%s  %sAppendInstructionData(%u, jasmData, %u, %d);\n",
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
	uint64_t labelMatch = MatchRel;
Next:
	Token token = tokenizer.PeekToken();
	int lineNumber = tokenizer.GetCurrentLineNumber();
	int fileIndex = tokenizer.GetCurrentFileIndex();
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
		   	if(nextToken.type == Token::Type::Instruction && nextToken.sValue == "b")
			{
				tokenizer.NextToken();
				DeleteWrapper<LabelOperand> *labelOperand = new DeleteWrapper<LabelOperand>();
				temporaryOperands.push_back(labelOperand);
				labelOperand->global = globalLabel;
				labelOperand->jumpType = JumpType::Backward;
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
				labelOperand->jumpType = JumpType::Forward;
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
				labelOperand->jumpType = JumpType::BackwardOrForward;
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
				immediateOperand->matchBitfield = MatchNumber;
				return immediateOperand;
			}
		}
	case Token::Type::RealValue:
		{
			tokenizer.NextToken();
			DeleteWrapper<ImmediateOperand> *immediateOperand = new DeleteWrapper<ImmediateOperand>(token.rValue);
			temporaryOperands.push_back(immediateOperand);
			immediateOperand->matchBitfield = MatchNumber;
			return immediateOperand;
		}
	case Token::Type::Expression:
		{
			tokenizer.NextToken();
			// Potentially relative label.
		ParseExpressionLabel:
			int expressionIndex = AddExpression(token.sValue, widthSpecified, lineNumber, fileIndex);
			Token nextToken = tokenizer.PeekToken();
			if(nextToken.type == Token::Type::Instruction && nextToken.sValue == "b")
			{
				tokenizer.NextToken();
				UpdateExpressionBitWidth(expressionIndex, 32);
				// Backwards reference to expression label.
				DeleteWrapper<LabelOperand> *labelOperand = new DeleteWrapper<LabelOperand>();
				temporaryOperands.push_back(labelOperand);
				labelOperand->global = globalLabel;
				labelOperand->jumpType = JumpType::Backward;
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
				labelOperand->jumpType = JumpType::Forward;
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
				labelOperand->jumpType = JumpType::BackwardOrForward;
				labelOperand->matchBitfield = labelMatch;
				labelOperand->expressionIndex = expressionIndex;
				labelOperand->reference = labelReference++;
				return labelOperand;
			}

			DeleteWrapper<ImmediateOperand> *immediateOperand = new DeleteWrapper<ImmediateOperand>(int64_t(0));
			temporaryOperands.push_back(immediateOperand);
			immediateOperand->type = Operand::Type::Number;
			immediateOperand->expressionIndex = AddExpression(token.sValue, 0, lineNumber, fileIndex);
			immediateOperand->matchBitfield = MatchNumber | MatchRel;
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
	case Token::Type::Instruction:
		if(const auto it = OperandMap::instance.find(token.sValue);
		   it != OperandMap::instance.end())
		{
			tokenizer.NextToken();
			return it->second;
		}
		throw AssemblerException("Unexpected token near %s", token.sValue.c_str());

	case Token::Type::Identifier:
		tokenizer.NextToken();
			
		// Parse reg32, reg64, etc.
		{
			auto it = ParameterizedRegisterMap::instance.find(token.sValue);
			if(it != ParameterizedRegisterMap::instance.end())
			{
				return ParseRegisterExpression(tokenizer,
											   temporaryOperands,
											   token,
											   it->second.registerData,
											   it->second.matchBitField);
			}
		}

		if(const auto it = OperandMap::instance.find(token.sValue);
		   it != OperandMap::instance.end())
		{
			return it->second;
		}

		// Block to scope labelOperand.
		{
		ParseNamedLabel:
			DeleteWrapper<LabelOperand> *labelOperand = new DeleteWrapper<LabelOperand>();
			temporaryOperands.push_back(labelOperand);
			labelOperand->global = globalLabel;
			labelOperand->jumpType = JumpType::Name;
			labelOperand->matchBitfield = labelMatch;
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
	case Token::Type::Colon:
		tokenizer.NextToken();
		return &SyntaxOperand::Colon;
	case Token::Type::Comma:
		tokenizer.NextToken();
		return &SyntaxOperand::Comma;
	case Token::Type::ExclamationMark:
		tokenizer.NextToken();
		return &SyntaxOperand::ExclamationMark;
	case Token::Type::LeftSquareBracket:
		tokenizer.NextToken();
		return &SyntaxOperand::LeftSquareBracket;
	case Token::Type::RightSquareBracket:
		tokenizer.NextToken();
		return &SyntaxOperand::RightSquareBracket;
	case Token::Type::LeftBrace:
		tokenizer.NextToken();
		return &SyntaxOperand::LeftBrace;
	case Token::Type::RightBrace:
		tokenizer.NextToken();
		return &SyntaxOperand::RightBrace;
	case Token::Type::Hash:
		{
			tokenizer.NextToken();
			Operand *operand = ParseOperandUnary(tokenizer, temporaryOperands);
			if(operand->type != Operand::Type::Number)
			{
				throw AssemblerException("Expected number after #");
			}
			operand->type = Operand::Type::Immediate;
			((ImmediateOperand *)operand)->UpdateMatchBitfield();
			return operand;
		}
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
		if(result->type != Operand::Type::Number)
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
	if(!result || result->type != Operand::Type::Number) return result;
	ImmediateOperand *immResult = (ImmediateOperand *)result;
	
	for(;;)
	{
		Token op = tokenizer.PeekToken();
		if(op.type != Token::Type::Star && op.type != Token::Type::Divide) break;
		
		tokenizer.NextToken();
		
		Operand *temp = ParseOperandUnary(tokenizer, temporaryOperands);
		if(!temp || temp->type != Operand::Type::Number) return nullptr;
		
		if(!result || result->type != Operand::Type::Number)
		{
			throw AssemblerException("Invalid operands for multiply/divide");
		}
		int64_t a = immResult->value;
		int64_t b = ((const ImmediateOperand*)temp)->value;
		int64_t value = (op.type == Token::Type::Star) ? a * b : a / b;
		immResult->value = value;
	}
	return immResult;
}

Operand* Assembler::ParseOperandSum(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands)
{
	Operand *result = ParseOperandProduct(tokenizer, temporaryOperands);
	if(!result) return nullptr;
	if(result->type != Operand::Type::Number
	   && result->type != Operand::Type::Label) return result;
	
	for(;;)
	{
		Token op = tokenizer.PeekToken();
		if(op.type != Token::Type::Add && op.type != Token::Type::Subtract) break;
		
		tokenizer.NextToken();
		
		Operand *temp = ParseOperandProduct(tokenizer, temporaryOperands);
		if(!temp) return nullptr;
		if(result->type == Operand::Type::Label
		   && temp->type == Operand::Type::Number)
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
		else if(result->type == Operand::Type::Number
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
		else if(result->type == Operand::Type::Number
			  	&& temp->type == Operand::Type::Number)
		{
			ImmediateOperand *immResult = (ImmediateOperand *)result;
			int64_t a = immResult->value;
			int64_t b = ((const ImmediateOperand*)temp)->value;
			int64_t value = (op.type == Token::Type::Add) ? a + b : a - b;
			immResult->value = value;
		}
		else
		{
			throw AssemblerException("Invalid operands for addition/subtraction");
		}
	}
	return result;
}

Operand* Assembler::ParseOperandComparison(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands)
{
	Operand *result = ParseOperandSum(tokenizer, temporaryOperands);
	if(!result) return nullptr;
	if(result->type != Operand::Type::Number) return result;
	
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
		if(rhs->type != Operand::Type::Number) throw AssemblerException("Unable to parse comparison");

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
	if(result->type != Operand::Type::Number) return result;
	
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
		if(rhs->type != Operand::Type::Number) throw AssemblerException("Unable to parse logical statement");

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

//============================================================================

} // namespace Javelin::Assembler::arm64

//============================================================================
