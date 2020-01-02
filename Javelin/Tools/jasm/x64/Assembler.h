//============================================================================

#pragma once
#include "Javelin/Tools/jasm/AutoDeleteVector.h"
#include "Javelin/Tools/jasm/CodeSegment.h"
#include "Javelin/Tools/jasm/x64/Action.h"
#include <stdio.h>

//============================================================================

namespace Javelin::Assembler::x64
{
//============================================================================

	class Operand;
	class MemoryOperand;
	class Token;
	class Tokenizer;

//============================================================================

	class Assembler
	{
	public:
		Assembler();
		~Assembler();
		
		void AssembleSegment(const CodeSegment &segment, const std::vector<std::string> &filenameList);
		
		void Dump() const;
		
		void UpdateExpressionBitWidth(int index, int value);
		
		void Write(FILE *f, int startingLine, int *expectedFileIndex, const std::vector<std::string> &filenameList) const;
		
	private:
		ListAction rootListAction;
		
		struct Expression
		{
			int 		maxBitWidth;
			int			index;
			int			offset;
			int			sourceLine;
			int			fileIndex;
			std::string expression;
		};
		std::vector<Expression> expressionList;
		bool isDataSegment;
		int segmentIndent = 0;
		int labelReference = 0;
		
		typedef std::unordered_map<std::string, int> ExpressionToIndexMap;
		ExpressionToIndexMap expressionToIndexMap;
		
		void ParseSegment(Tokenizer &tokenizer, const std::vector<std::string> &filenameList);
		void ParseLine(ListAction& listAction, Tokenizer &tokenizer, Token token);
		void ParseIf(ListAction& listAction, Tokenizer &tokenizer, const std::vector<std::string> &filenameList);
		const AlternateActionCondition *ParseCondition(Tokenizer &tokenizer);
		void ParsePreprocessor(ListAction& listAction, Tokenizer &tokenizer, const Token &token, const std::vector<std::string> &filenameList);
		void ParseInstruction(ListAction& listAction, Tokenizer &tokenizer, const Token &token);
		Operand* ParseAddress(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands, int widthSpecified);
		
		int AddExpression(const std::string& expression, int maxBitWidth, int sourceLine, int fileIndex);
		void UpdateExpressionList();
		RegisterOperand *ParseRegisterExpression(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands, const Token &token, MatchBitfield matchBitField);

		Operand *ParseOperandPrimary(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands);
		Operand *ParseOperandUnary(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands);
		Operand *ParseOperandProduct(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands);
		Operand *ParseOperandSum(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands);
		Operand *ParseOperandComparison(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands);
		Operand *ParseOperandLogical(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands);
		Operand *ParseOperand(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands) { return ParseOperandLogical(tokenizer, temporaryOperands); }

		Operand *ParseAddressProduct(Tokenizer &tokenizer, MemoryOperand &memoryOperand, AutoDeleteVector &temporaryOperands);
		Operand *ParseAddressSum(Tokenizer &tokenizer, MemoryOperand &memoryOperand, AutoDeleteVector &temporaryOperands);
	};

//============================================================================
} // namespace Javelin::Assembler::x64
//============================================================================
