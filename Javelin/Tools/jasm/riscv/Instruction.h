//============================================================================

#pragma once
#include "Javelin/Assembler/riscv/RelEncoding.h"
#include "Javelin/Tools/jasm/riscv/Encoder.h"
#include "Javelin/Tools/jasm/Common/Action.h"
#include <string>
#include <unordered_map>
#include <vector>

//============================================================================

namespace Javelin::Assembler::riscv
{
//============================================================================

	using Common::Action;
	class Assembler;
    class ListAction;

//============================================================================

	enum class MatchBitIndex : uint8_t
	{
		#define TAG(x,y,z) x,
		#include "MatchTags.h"
		#undef TAG
	};

	enum
	{
		#define TAG(x,y,z) Match##x = 1ULL << (int) MatchBitIndex::x,
		#include "MatchTags.h"
		#undef TAG

		MatchOpAll = MatchOp0 | MatchOp1 | MatchOp2 | MatchOp3 | MatchOp4 | MatchOp5 | MatchOp6 | MatchOp7,
        MatchImmAll = MatchImm | MatchImm6 | MatchImm12 | MatchImm20 | MatchUimm20 | MatchShiftImm | MatchCShiftImm,
        MatchRelAll = MatchBRel | MatchJRel | MatchCBRel | MatchCJRel,
	};
	
	typedef uint64_t MatchBitfield;

	std::string MatchBitfieldDescription(MatchBitfield matchBitfield);
	std::string MatchBitfieldsDescription(const MatchBitfield *matchBitfields, int numberOfBitfields);

	struct Operand
	{
		enum class Type : uint8_t
		{
			Unknown,
			Register,
			Number,
			Label,
			Syntax,
			Condition,
			Shift,
			Extend,
		};
		
		enum class ImmediateType : uint8_t
		{
			Integer,
			Real,
		};
		
		enum class Syntax : uint8_t
		{
			Colon,
			Comma,
			ExclamationMark,
			LeftParenthesis,
			RightParenthesis,
			LeftBrace,
			RightBrace,
		};

		enum class Condition : uint8_t
		{
			// Order matches encoding order.
			EQ,
			NE,
			CS,
			CC,
			MI,
			PL,
			VS,
			VC,
			HI,
			LS,
			GE,
			LT,
			GT,
			LE,
			AL,
		};
		
		enum class Shift : uint8_t
		{
			// Order matches encoding order.
			LSL,
			LSR,
			ASR,
			ROR,
		};
		
		enum class Extend : uint8_t
		{
			// Order matches encoding order.
			UXTB,
			UXTH,
			UXTW,
			UXTX,
			SXTB,
			SXTH,
			SXTW,
			SXTX,
		};
		
		Type type;
		uint8_t expressionIndex;
		union
		{
			uint8_t index;				 // index for type == Register
			JumpType jumpType; 			 // for type == Label
			ImmediateType immediateType; // for type == Immediate
			Syntax syntax;		 		 // for type == Syntax
			Condition condition;		 // for type == Condition
			Shift shift;
			Extend extend;
		};
		union
		{
			// For (V)BHSDQ registers:
			//  Bit 1:0 = size: 0 = 8 bit, 1 = 16 bit, 2 = 32 bit, 3 = 64 bit
			//  Bit 2 = Q
			//  Bit 5:4 = FP Size: 0 = 32 bit, 1 = 64 bit, 3 = 16 bit
			uint8_t registerData;
		};
		MatchBitfield matchBitfield;

		Operand() { }
		Operand(Type aType) : type(aType) { }
		constexpr Operand(Condition aCondition)
			: type(Type::Condition),
			  condition(aCondition),
			  matchBitfield(MatchCondition),
			  registerData(0),
			  expressionIndex(0) { }
		
		constexpr Operand(Shift aShift, MatchBitfield aMatchBitfield)
			: type(Type::Shift),
		 	  shift(aShift),
		  	  matchBitfield(aMatchBitfield),
			  registerData(0),
			  expressionIndex(0) { }
		
		constexpr Operand(Extend aExtend, MatchBitfield aMatchBitfield)
		: type(Type::Extend),
		  extend(aExtend),
		  matchBitfield(aMatchBitfield),
		  registerData(0),
		  expressionIndex(0) { }
		
        bool MayHaveMultipleRepresentations() const { return expressionIndex != 0 || type == Type::Label; }

		bool IsExpression() const 	{ return expressionIndex != 0; }
		
	protected:
		constexpr Operand(Type aType, uint8_t aIndex, MatchBitfield aMatchBitfield)
		: type(aType), expressionIndex(0), index(aIndex), registerData(0), matchBitfield(aMatchBitfield) { }
		
        constexpr Operand(Type aType, uint8_t aIndex, uint8_t aRegisterData, MatchBitfield aMatchBitfield)
		: type(aType), expressionIndex(0), index(aIndex), registerData(aRegisterData), matchBitfield(aMatchBitfield) { }
		
        constexpr Operand(Syntax aSyntax, MatchBitfield aMatchBitfield)
		: type(Type::Syntax), syntax(aSyntax), matchBitfield(aMatchBitfield), registerData(0), expressionIndex(0) { }

        constexpr Operand(Type aType, ImmediateType aImmediateType, MatchBitfield aMatchBitfield)
        : type(aType), expressionIndex(0), immediateType(aImmediateType), registerData(0), matchBitfield(aMatchBitfield) { }
        
};

//============================================================================

	struct RegisterOperand : public Operand
	{
		RegisterOperand() : Operand(Type::Register) { }
		constexpr RegisterOperand(uint8_t index, MatchBitfield matchBitfield) : Operand(Type::Register, index, matchBitfield) { }
		constexpr RegisterOperand(uint8_t index, uint8_t registerData, MatchBitfield matchBitfield) : Operand(Type::Register, index, registerData, matchBitfield) { }
	};
	
//============================================================================
	
	struct ImmediateOperand : public Operand
	{
		union
		{
			int64_t value;
			long double realValue;
		};
		
        ImmediateOperand() : Operand(Type::Number, ImmediateType::Integer, MatchImm) { }
		ImmediateOperand(int64_t aValue) : Operand(Type::Number), value(aValue)
		{
			immediateType = ImmediateType::Integer;
			expressionIndex = 0;
			matchBitfield = 0;
		}
		explicit ImmediateOperand(long double aRealValue) : Operand(Type::Number), realValue(aRealValue)
		{
			immediateType = ImmediateType::Real;
			expressionIndex = 0;
			matchBitfield = 0;
		}
        constexpr ImmediateOperand(int aValue, MatchBitfield aMatchBitfield)
        : Operand(Type::Number, ImmediateType::Integer, aMatchBitfield),
          value(aValue) { }
		
		void UpdateMatchBitfield();

		bool operator==(const ImmediateOperand &a) const;
		bool operator!=(const ImmediateOperand &a) const;
		bool operator<(const ImmediateOperand &a) const;
		bool operator<=(const ImmediateOperand &a) const;
		bool operator>(const ImmediateOperand &a) const;
		bool operator>=(const ImmediateOperand &a) const;
		
		bool AsBool() const;
	};
	
//============================================================================
	
	// Note that expression labels are ImmediateOperands with IsExpression() == true, not LabelOperands
	struct LabelOperand : Operand
	{
		bool			global = false;
		int32_t 		reference = 0;		// A unique identifier for relative jumps of the same value.
		int64_t			labelValue;
		std::string 	labelName;
		ImmediateOperand* displacement = nullptr;
		
		LabelOperand() : Operand(Type::Label)
		{
			expressionIndex = 0;
			reference = 0;
		}
		
		Action* CreatePatchAction(riscvAssembler::RelEncoding relEncoding, int offset) const;
	};

//============================================================================
	
	struct SyntaxOperand : Operand
	{
		static SyntaxOperand Comma;
		static SyntaxOperand LeftParenthesis;
		static SyntaxOperand RightParenthesis;
		
		constexpr SyntaxOperand(Syntax syntax, MatchBitfield matchBitfield)	: Operand(syntax, matchBitfield) { }
	};

//============================================================================

    enum ExtensionBitmask : uint32_t
    {
        RV32I   = 1,
        RV64I   = 2,
        RV128I  = 4,
        Zifenci = 8,
        Zicsr   = 0x10,
        C       = 0x20,
        M       = 0x40,
        F       = 0x80,
        D       = 0x100,
        Q       = 0x200,
    };

	struct EncodingVariant
	{
		Encoder encoder;
		uint8_t operandMatchMasksLength : 5;
        uint32_t extensionBitmask : 19;
		uint32_t opcode;
		const MatchBitfield *operandMatchMasks;
		
		bool Match(int index, const Operand &operand) const { return (operandMatchMasks[index] & operand.matchBitfield) != 0; }
		bool Match(int operandLength, const Operand* *const operands) const;
		int GetNP1Count() const;
	};
	static_assert(sizeof(EncodingVariant) == 8 + sizeof(void*), "EncodingVariant is larger than expected");
	
	struct Instruction
	{
		int8_t encodingVariantLength;
		const EncodingVariant *encodingVariants;
		
		const EncodingVariant *FindFirstMatch(int operandLength, const Operand* *const operands) const;
		
		void AddToAssembler(const std::string &opcodeName, Assembler &assembler, ListAction &listAction, int operandLength, const Operand* *const operands) const;
	};

	class InstructionMap : public std::unordered_map<std::string, const Instruction*>
	{
	public:
		static const InstructionMap& GetInstance() { return instance; }

	private:
		InstructionMap();
		
		static const InstructionMap instance;
	};

//============================================================================
} // namespace Javelin::Assembler::riscv
//============================================================================
