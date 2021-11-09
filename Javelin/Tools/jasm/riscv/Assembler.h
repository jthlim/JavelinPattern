//============================================================================

#pragma once
#include "Javelin/Tools/jasm/AutoDeleteVector.h"
#include "Javelin/Tools/jasm/CodeSegment.h"
#include "Javelin/Tools/jasm/riscv/Action.h"
#include <stdio.h>

//============================================================================

namespace Javelin::Assembler::riscv
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
        Assembler(bool useCompressedInstructions);
		~Assembler();
		
		void AssembleSegment(const CodeSegment &segment, const std::vector<std::string> &filenameList);
		
		void Dump() const;
		
		void UpdateExpressionBitWidth(int index, int value);
		
		void Write(FILE *f, int startingLine, int *expectedFileIndex, const std::vector<std::string> &filenameList) const;
		
		int AddExpression(const std::string& expression, int maxBitWidth, int sourceLine, int fileIndex);
		const std::string &GetExpression(int expressionIndex) const { return expressionList[expressionIndex-1].expression; }
		int GetExpressionSourceLine(int expressionIndex) const { return expressionList[expressionIndex-1].sourceLine; }
		int GetExpressionFileIndex(int expressionIndex) const { return expressionList[expressionIndex-1].fileIndex; }
        
        bool IsEquivalentByteCode(const Assembler& other) const;

        bool UseCompressedInstructions() const { return useCompressedInstructions; }
        
	private:
		ListAction rootListAction;
		
		struct Expression
		{
			int 		maxBitWidth;
			int			index;
			int			offset;
			int 		sourceLine;
			int			fileIndex;
			std::string expression;
		};
		std::vector<Expression> expressionList;
		bool isDataSegment;
        bool useCompressedInstructions;
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
		
		void UpdateExpressionList();
		RegisterOperand *ParseRegisterExpression(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands, const Token &token, int registerData, MatchBitfield matchBitField);

		Operand *ParseOperandPrimary(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands);
		Operand *ParseOperandUnary(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands);
		Operand *ParseOperandProduct(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands);
		Operand *ParseOperandSum(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands);
		Operand *ParseOperandComparison(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands);
		Operand *ParseOperandLogical(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands);
		Operand *ParseOperand(Tokenizer &tokenizer, AutoDeleteVector &temporaryOperands) { return ParseOperandLogical(tokenizer, temporaryOperands); }
        
        // Returns expression offset.
        int AddExpressionInfoToContext(ActionWriteContext &context) const;
	};

//============================================================================
} // namespace Javelin::Assembler::riscv
//============================================================================
