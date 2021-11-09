//============================================================================

#pragma once
#include "Javelin/Assembler/arm64/RelEncoding.h"
#include "Javelin/Tools/jasm/arm64/Encoder.h"
#include "Javelin/Tools/jasm/Common/Action.h"
#include <string>
#include <unordered_map>
#include <vector>

//============================================================================

namespace Javelin::Assembler::arm64
{
//============================================================================

	using Common::Action;
	class Assembler;
	class ListAction;

//============================================================================

	enum class MatchBitIndex : uint8_t
	{
		#define TAG(x,y) x,
		#include "MatchTags.h"
		#undef TAG
	};

	enum
	{
		#define TAG(x,y) Match##x = 1ULL << (int) MatchBitIndex::x,
		#include "MatchTags.h"
		#undef TAG

		MatchOpAll = MatchOp0 | MatchOp1 | MatchOp2 | MatchOp3 | MatchOp4 | MatchOp5 | MatchOp6 | MatchOp7,
		MatchImmAll = MatchHwImm16 | MatchNot32HwImm16 | MatchNot64HwImm16 | MatchUImm12 | MatchUImm16 | MatchImm,
		MatchRegAll = MatchReg32 | MatchReg64,
		MatchFpAll = MatchH | MatchS | MatchD,
		MatchV8B16B = MatchV8B | MatchV16B,
		MatchV4H8H = MatchV4H | MatchV8H,
		MatchV2S4S = MatchV2S | MatchV4S,
		MatchV8B16B4H8H2S4S = MatchV8B16B | MatchV4H8H | MatchV2S4S,
		MatchV2S4S2D = MatchV2S4S | MatchV2D,
		MatchV8B16B4H8H2S4S2D = MatchV8B | MatchV4H | MatchV2S | MatchV16B | MatchV8H | MatchV4S | MatchV2D,
		MatchVAll = MatchV8B | MatchV4H | MatchV2S | MatchV1D | MatchV16B | MatchV8H | MatchV4S | MatchV2D,
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
			Immediate,
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
			LeftSquareBracket,
			RightSquareBracket,
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
		
		bool MayHaveMultipleRepresentations() const { return expressionIndex != 0; }

		bool IsExpression() const 	{ return expressionIndex != 0; }
		
	protected:
		constexpr Operand(Type aType, uint8_t aIndex, MatchBitfield aMatchBitfield)
		: type(aType), expressionIndex(0), index(aIndex), registerData(0), matchBitfield(aMatchBitfield) { }
		constexpr Operand(Type aType, uint8_t aIndex, uint8_t aRegisterData, MatchBitfield aMatchBitfield)
		: type(aType), expressionIndex(0), index(aIndex), registerData(aRegisterData), matchBitfield(aMatchBitfield) { }
		constexpr Operand(Syntax aSyntax, MatchBitfield aMatchBitfield)
		: type(Type::Syntax), syntax(aSyntax), matchBitfield(aMatchBitfield), registerData(0), expressionIndex(0) { }
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
		
		ImmediateOperand() : Operand(Type::Number) { immediateType = ImmediateType::Integer; matchBitfield = MatchNumber; }
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
		
		Action* CreatePatchAction(arm64Assembler::RelEncoding relEncoding, int offset) const;
	};

//============================================================================
	
	struct SyntaxOperand : Operand
	{
		static SyntaxOperand Colon;
		static SyntaxOperand Comma;
		static SyntaxOperand ExclamationMark;
		static SyntaxOperand LeftSquareBracket;
		static SyntaxOperand RightSquareBracket;
		static SyntaxOperand LeftBrace;
		static SyntaxOperand RightBrace;
		
		constexpr SyntaxOperand(Syntax syntax, MatchBitfield matchBitfield)	: Operand(syntax, matchBitfield) { }
	};

//============================================================================

	struct EncodingVariant
	{
		Encoder encoder;
		uint8_t operandMatchMasksLength : 5;
		uint32_t data : 19;
		uint32_t opcode;
		const MatchBitfield *operandMatchMasks;
		
		bool Match(int index, const Operand &operand) const { return (operandMatchMasks[index] & operand.matchBitfield) != 0; }
		bool Match(int operandLength, const Operand* *const operands) const;
		int GetNP1Count() const;
	};
	static_assert(sizeof(EncodingVariant) == 16, "EncodingVariant is larger than expected");
	
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
} // namespace Javelin::Assembler::arm64
//============================================================================
