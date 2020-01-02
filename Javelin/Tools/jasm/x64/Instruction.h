//============================================================================

#pragma once
#include "Javelin/Tools/jasm/x64/Encoder.h"
#include <string>
#include <unordered_map>
#include <vector>

//============================================================================

namespace Javelin::Assembler::x64
{
//============================================================================

	class Action;
	class Assembler;
	class ListAction;

//============================================================================

	enum class MatchBitIndex : uint8_t
	{
		#define TAG(x,y,z) x,
		#include "MatchTags.h"
		#undef TAG
	};

	enum MatchBitfield
	{
		#define TAG(x,y,z) Match##x = 1ULL << (int) MatchBitIndex::x,
		#include "MatchTags.h"
		#undef TAG
		
		MatchRM8 = MatchReg8 | MatchMem8,
		MatchRM16 = MatchReg16 | MatchMem16,
		MatchRM32 = MatchReg32 | MatchMem32,
		MatchRM64 = MatchReg64 | MatchMem64,
		MatchImmAll = MatchImm8 | MatchImm16 | MatchImm32 | MatchUImm32 | MatchImm64,
		MatchRegAll = MatchReg8 | MatchReg16 | MatchReg32 | MatchReg64,
		MatchMemAll = MatchMem8 | MatchMem16 | MatchMem32 | MatchMem64 | MatchMem128 | MatchMem256 | MatchMem512,
		MatchDirectMemAll = MatchDirectMem8 | MatchDirectMem16 | MatchDirectMem32 | MatchDirectMem64,
		MatchRelAll = MatchRel8 | MatchRel32,
		MatchRMAll = MatchRegAll | MatchMemAll,
		MatchMmMem64 = MatchMm | MatchMem64,
		MatchXmmMem128 = MatchXmm | MatchMem128,
		MatchYmmMem256 = MatchYmm | MatchMem256,
		MatchZmmMem512 = MatchZmm | MatchMem512,
	};

	std::string MatchBitfieldDescription(uint64_t matchBitfield);
	std::string MatchBitfieldsDescription(const uint64_t *matchBitfields, int numberOfBitfields);

	struct Operand
	{
		enum class Type : uint8_t
		{
			Unknown,
			Register,
			Memory,
			Immediate,
			Label,
		};
		
		enum class JumpType : uint8_t
		{
			Name,				// Use labelName
			Backward,			// Use labelValue
			Forward,			// Use labelValue
			BackwardOrForward,	// Use labelValue
		};

		enum class ImmediateType : uint8_t
		{
			Integer,
			Real,
		};

		Type type;
		uint8_t expressionIndex;
		union
		{
			uint8_t index;				 	// index for type == Register
			JumpType jumpType; 				// for type == Label
			ImmediateType immediateType; 	// for type == Immediate
		};
		uint64_t matchBitfield;

		Operand() { }
		Operand(Operand::Type aType) : type(aType) { }
		
		bool MayHaveMultipleRepresentations() const { return expressionIndex != 0 || type == Type::Label; }

		bool IsExpression() const 	{ return expressionIndex != 0; }
		
		bool HasMultipleMemoryWidths() const { return __builtin_popcountll(matchBitfield & MatchMemAll) > 1; }
		bool HasMultipleImmediateWidths() const { return __builtin_popcountll(matchBitfield & MatchImmAll) > 1; }

	protected:
		constexpr Operand(Type aType, uint8_t aIndex, uint64_t aMatchBitfield)
		: type(aType), expressionIndex(0), index(aIndex), matchBitfield(aMatchBitfield) { }
	};

//============================================================================

	struct RegisterOperand : public Operand
	{
		RegisterOperand() : Operand(Type::Register) { }
		constexpr RegisterOperand(uint8_t index, uint64_t matchBitfield) : Operand(Type::Register, index, matchBitfield) { }
		
		// Returns true for AH, BH, CH, DH
		bool IsHighByte() const		{ return (matchBitfield & MatchReg8Hi) != 0; }

		// Returns true for byte registers >= 4 (e.g. SPL, BPL, SIL, DIL)
		bool RequiresREX() const	{ return ((matchBitfield & (MatchReg8|MatchReg8Hi)) == MatchReg8 && index >= 4) || (index >= 8); }
	};
	
//============================================================================
	
	struct ImmediateOperand : public Operand
	{
		union
		{
			int64_t value;
			long double realValue;
		};
		
		ImmediateOperand() : Operand(Type::Immediate) { immediateType = ImmediateType::Integer; }
		ImmediateOperand(int64_t aValue) : Operand(Type::Immediate), value(aValue)
		{
			immediateType = ImmediateType::Integer;
			expressionIndex = 0;
			matchBitfield = 0;
		}
		explicit ImmediateOperand(long double aRealValue) : Operand(Type::Immediate), realValue(aRealValue)
		{
			immediateType = ImmediateType::Real;
			expressionIndex = 0;
			matchBitfield = 0;
		}
		
		bool operator==(const ImmediateOperand &a) const;
		bool operator!=(const ImmediateOperand &a) const;
		bool operator<(const ImmediateOperand &a) const;
		bool operator<=(const ImmediateOperand &a) const;
		bool operator>(const ImmediateOperand &a) const;
		bool operator>=(const ImmediateOperand &a) const;
		
		bool AsBool() const;
	};
	
//============================================================================

	struct MemoryOperand : public Operand
	{
		const ImmediateOperand *scale = nullptr;
		const RegisterOperand *index = nullptr;
		const RegisterOperand *base = nullptr;
		const ImmediateOperand *displacement = nullptr;
		
		MemoryOperand() : Operand(Type::Memory)
		{
			expressionIndex = 0;
		}
		
		bool IsDirectMemoryAdddress() const { return index == nullptr && base == nullptr; }
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
		
		Action* CreatePatchAction(int bytes, int offset) const;
	};
	
//============================================================================

	struct EncodingVariant
	{
		uint64_t operandMatchMasks[6];
		int16_t operationWidth;
		uint8_t operandMatchMasksLength;
		uint8_t opcodeLength;
		uint8_t opcodeData;
		Encoder encoder;
		uint8_t opcodePrefix;
		uint8_t opcodes[4];
		
		bool Match(int index, const Operand &operand) const { return (operandMatchMasks[index] & operand.matchBitfield) != 0; }
		bool Match(int operandLength, const Operand* *const operands) const;
		uint64_t GetLastOperandMatchMask() const { return operandMatchMasks[operandMatchMasksLength-1]; }
		
		std::vector<uint8_t> GetOpcodeVector() const { return {opcodes, opcodes+opcodeLength}; }
		
		int GetAvxPrefixValue() const;
		int GetAvxMmValue(const uint8_t* &outOpcodes, uint32_t &outOpcodeLength) const;
	};
	
	struct Instruction
	{
		// -1 means this instruction is a prefix
		int16_t defaultWidth;

		int8_t encodingVariantLength;
		
		const EncodingVariant *encodingVariants;
		
		bool IsPrefix() const { return defaultWidth == -1; }
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
} // namespace Javelin::Assembler::x64
//============================================================================
