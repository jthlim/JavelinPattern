//============================================================================

#pragma once
#include "Javelin/Tools/jasm/x64/Instruction.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

//============================================================================

namespace Javelin::Assembler::x64
{
//============================================================================

	class Action;
	class ListAction;
	class LiteralAction;
	class PatchLabelAction;

//============================================================================
	
	struct ActionOffset
	{
		// This is incremented every time a variable length instruction is encountered.
		int blockIndex;
		
		// Alignment, if any. A variable length instruction will reset this to 0.
		int alignment;
		
		//
		size_t offsetIntoBlock;
		
		size_t totalMinimumOffset;
		size_t totalMaximumOffset;
	};
	
	struct ActionContext
	{
		bool forwards;
		ActionOffset offset;

		typedef std::unordered_map<std::string, ActionOffset> NamedLabelMap;
		NamedLabelMap namedLabels;

		typedef std::unordered_map<int64_t, ActionOffset> NumericLabelMap;
		NumericLabelMap numericLabels;

		typedef std::unordered_map<int64_t, ActionOffset> ExpressionLabelMap;
		ExpressionLabelMap expressionLabels;

		typedef std::unordered_set<std::string> NamedReferenceSet;
		NamedReferenceSet namedReferenceSet;

		typedef std::unordered_set<int64_t> NumericReferenceSet;
		NumericReferenceSet numericReferenceSet;

		std::string assemblerVariableName;
	};
	
	struct ActionWriteContext
	{
		ActionWriteContext(bool aIsDataSegment, const std::string& variableName) : isDataSegment(aIsDataSegment) { SetVariableName(variableName); }

		bool isDataSegment;
		std::string variableName;
		std::string assemblerCallPrefix;

		void SetVariableName(const std::string &aVariableName) {
			variableName = aVariableName;
			if (isDataSegment)
			{
				assemblerCallPrefix = variableName + ".GetDataSegmentAssembler().";
			}
			else
			{
				assemblerCallPrefix = variableName + ".";
			}
		}
		const char *GetAssemblerCallPrefix() const {
			return assemblerCallPrefix.c_str();
		}		
		
		int numberOfLabels = 0;

		struct ExpressionInfo
		{
			int bitWidth;
			int index;
			int offset;
			int sourceLine;
			int fileIndex;
			std::string expression;
		};
		std::vector<ExpressionInfo> expressionInfo;
		
		// Maps label reference -> expressionLabelIndex.
		std::unordered_map<size_t, int> forwardLabelReferences;

		std::unordered_set<uint32_t> forwardLabelReferenceSet;

		// Maps labelId -> index
		std::unordered_map<int64_t, int> labelIdToIndexMap;
		
		int GetNumberOfForwardLabelReferences() const { return (int) forwardLabelReferenceSet.size(); }
		
		void AddForwardLabelReference(uint32_t reference)
		{
			forwardLabelReferenceSet.insert(reference);
		}
		
		int GetForwardLabelReferenceIndex(int32_t reference, int numBytes)
		{
			size_t key = size_t(reference) | (size_t(numBytes) << 56);
			
			const auto it = forwardLabelReferences.find(key);
			if(it != forwardLabelReferences.end()) return it->second;
			int index = (int) forwardLabelReferences.size();
			forwardLabelReferences[key] = index;
			forwardLabelReferenceSet.insert(reference);
			return index;
		}
		
		int GetIndexForLabelId(int64_t labelId)
		{
			const auto it = labelIdToIndexMap.find(labelId);
			if(it != labelIdToIndexMap.end()) return it->second;
			int index = int(labelIdToIndexMap.size());
			labelIdToIndexMap[labelId] = index;
			return index;
		}
	};
	
//============================================================================

	class AlternateActionCondition
	{
	public:
		enum class Result
		{
			Never,
			Maybe,
			Always,
		};
		
		virtual ~AlternateActionCondition() { }
		
		virtual Result IsValid(ActionContext &context, const Action *action) const = 0;
		virtual std::string GetDescription() const = 0;
		
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const = 0;
		virtual void Release() const { delete this; }
		virtual bool Equals(const AlternateActionCondition *other) const;
		
	protected:
		constexpr AlternateActionCondition() { }
		
	private:
		AlternateActionCondition(const AlternateActionCondition&) = delete;
		void operator=(const AlternateActionCondition&) = delete;
	};
	
	class AndAlternateActionCondition : public AlternateActionCondition
	{
	public:
		~AndAlternateActionCondition();
		
		virtual Result IsValid(ActionContext &context, const Action *action) const final;
		virtual std::string GetDescription() const final;
		virtual bool Equals(const AlternateActionCondition *other) const;

		size_t GetNumberOfConditions() const							{ return conditionList.size(); }
		void ClearConditions()											{ conditionList.clear(); }
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;

		const AlternateActionCondition *GetCondition(size_t i) const 	{ return conditionList[i]; }
		void AddCondition(const AlternateActionCondition *condition)	{ conditionList.push_back(condition); }
		
	private:
		std::vector<const AlternateActionCondition*> conditionList;
	};
	
	class AlwaysAlternateActionCondition : public AlternateActionCondition
	{
	public:
		virtual Result IsValid(ActionContext &context, const Action *action) const final	{ return Result::Always; 		}
		virtual std::string GetDescription() const final						 			{ return "Always";	}
		virtual void Release() const 														{  }
		
		// Do nothing. "Always" needs no extra byte code.
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;

		static const AlternateActionCondition* Create(const Operand *operand)				{ return &instance; }
		static const AlwaysAlternateActionCondition instance;

	private:
		constexpr AlwaysAlternateActionCondition() { }
	};

	class NeverAlternateActionCondition : public AlternateActionCondition
	{
	public:
		virtual Result IsValid(ActionContext &context, const Action *action) const final	{ return Result::Never; 	}
		virtual std::string GetDescription() const final						 			{ return "Never";	}
		virtual void Release() const 														{  }
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;

		static const AlternateActionCondition* Create(const Operand *operand)				{ return &instance; }
		static const NeverAlternateActionCondition instance;

	private:
		constexpr NeverAlternateActionCondition() { }
	};
	
	class ImmediateAlternateActionCondition : public AlternateActionCondition
	{
	public:
		virtual bool Equals(const AlternateActionCondition *other) const;
		virtual Result IsValid(ActionContext &context, const Action *action) const final	{ return Result::Maybe;		}

	protected:
		constexpr ImmediateAlternateActionCondition(int aExpressionIndex) : expressionIndex(aExpressionIndex) { }
		int expressionIndex;

		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action, int opcode) const;
	};

	class ZeroAlternateActionCondition : public ImmediateAlternateActionCondition
	{
	public:
		constexpr ZeroAlternateActionCondition(int aExpressionIndex) : ImmediateAlternateActionCondition(aExpressionIndex) { }
		static const AlternateActionCondition* Create(const Operand *operand);

		virtual std::string GetDescription() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};

	class Imm8AlternateActionCondition : public ImmediateAlternateActionCondition
	{
	public:
		constexpr Imm8AlternateActionCondition(int expressionIndex) : ImmediateAlternateActionCondition(expressionIndex) { }
		static const AlternateActionCondition* Create(const Operand *operand);

		virtual std::string GetDescription() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};

	class Imm16AlternateActionCondition : public ImmediateAlternateActionCondition
	{
	public:
		constexpr Imm16AlternateActionCondition(int expressionIndex) : ImmediateAlternateActionCondition(expressionIndex) { }
		static const AlternateActionCondition* Create(const Operand *operand);

		virtual std::string GetDescription() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};

	class Imm32AlternateActionCondition : public ImmediateAlternateActionCondition
	{
	public:
		constexpr Imm32AlternateActionCondition(int expressionIndex) : ImmediateAlternateActionCondition(expressionIndex) { }
		static const AlternateActionCondition* Create(const Operand *operand);

		virtual std::string GetDescription() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};

	class UImm32AlternateActionCondition : public ImmediateAlternateActionCondition
	{
	public:
		constexpr UImm32AlternateActionCondition(int expressionIndex) : ImmediateAlternateActionCondition(expressionIndex) { }
		static const AlternateActionCondition* Create(const Operand *operand);

		virtual std::string GetDescription() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};
	
	// Calculates a relative address from the end of action, and returns true if it fits into a SImm8 value.
	class Rel8AlternateActionCondition : public AlternateActionCondition
	{
	public:
		virtual Result IsValid(ActionContext &context, const Action *action) const final;
		virtual std::string GetDescription() const final;
		virtual bool Equals(const AlternateActionCondition *other) const;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;

		static const AlternateActionCondition* Create(const Operand *operand);
		
	private:
		Rel8AlternateActionCondition(const LabelOperand &aLabelOperand);
		
		LabelOperand labelOperand;
		Result IsValidForJumpType(ActionContext &context, Operand::JumpType jumpType) const;
		
		static Result IsValid(const ActionOffset &anchorOffset, const ActionOffset &destinationOffset);
	};

	class Rel32AlternateActionCondition : public AlternateActionCondition
	{
	public:
		virtual Result IsValid(ActionContext &context, const Action *action) const final;
		virtual std::string GetDescription() const final;
		virtual bool Equals(const AlternateActionCondition *other) const;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;

		static const AlternateActionCondition* Create(const Operand *operand);
		
	private:
		Rel32AlternateActionCondition(const LabelOperand &aLabelOperand);
		
		LabelOperand labelOperand;
		Result IsValidForJumpType(ActionContext &context, Operand::JumpType jumpType) const;
		
		static Result IsValid(const ActionOffset &anchorOffset, const ActionOffset &destinationOffset);
	};
	
	class Delta32AlternateActionCondition : public ImmediateAlternateActionCondition
	{
	public:
		constexpr Delta32AlternateActionCondition(int expressionIndex) : ImmediateAlternateActionCondition(expressionIndex) { }
		
		virtual std::string GetDescription() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};
	
//============================================================================

	class Action
	{
	public:
		virtual ~Action() { }
		
		virtual bool IsLiteral() const { return false; }
		virtual bool IsLiteralAction() const { return false; }
		virtual void Dump() const = 0;
		virtual void AppendToLiteral(LiteralAction *literal) const { }
		virtual const Action *GetLastAction() const { return this; }
		virtual size_t GetMinimumLength() const = 0;
		virtual size_t GetMaximumLength() const = 0;
		virtual bool CanDelay(int amount) const { return false; }
		virtual void Delay(int amount) { }
		virtual void DelayAndConsolidate() { }

		virtual bool ResolveRelativeAddresses(ActionContext &context);
		virtual bool Simplify(ListAction *parent, size_t index) { return false; }

		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const = 0;

		bool CanDelay() const { return CanDelay(0); }

		static void WriteSigned32(std::vector<uint8_t> &result, int32_t value) { WriteUnsigned32(result, value); }
		static void WriteUnsigned32(std::vector<uint8_t> &result, uint32_t value);
		static void WriteUnsignedVLE(std::vector<uint8_t> &result, uint32_t value);
		static void WriteSignedVLE(std::vector<uint8_t> &result, int32_t value);
		static void WriteExpressionOffset(std::vector<uint8_t> &result, const ActionWriteContext &context, int expressionIndex);
		
	protected:
		Action() = default;
		ActionOffset actionOffset;

	private:
		Action(const Action&) = delete;
		void operator=(const Action&) = delete;
	};

	class EmptyAction : public Action
	{
	public:
		virtual size_t GetMinimumLength() const final { return 0; }
		virtual size_t GetMaximumLength() const final { return 0; }
	};
	
	class SetAssemblerVariableNameAction : public EmptyAction
	{
	public:
		SetAssemblerVariableNameAction(const std::string &aVariableName) : variableName(aVariableName) { }

		void Dump() const;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final { }

	private:
		std::string variableName;
	};
	
	class LiteralAction : public Action
	{
	public:
		LiteralAction(const std::vector<uint8_t> &aBytes);
		~LiteralAction();
		
		virtual bool IsLiteral() const final { return true; }
		virtual bool IsLiteralAction() const { return true; }
		virtual void Dump() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
		bool Simplify(ListAction *parent, size_t index) final;

		virtual void AppendToLiteral(LiteralAction *literal) const final;
		
		virtual size_t GetMinimumLength() const final { return bytes.size(); }
		virtual size_t GetMaximumLength() const final { return bytes.size(); }

		int GetNumberOfBytes() const { return int(bytes.size()); }
		void AppendBytes(const std::vector<uint8_t>& literalBytes);
		void AppendBytes(const uint8_t *bytes, size_t length);

	private:
		std::vector<uint8_t> bytes;
		
		friend class PatchLabelAction;
	};

	class DelayableAction : public EmptyAction
	{
	public:
		DelayableAction(int initialDelay) : delay(initialDelay) { }
		virtual bool CanDelay(int amount) const final { return amount + delay < 256; }
		virtual void Delay(int amount) final { delay += amount; }
			
	protected:
		int delay;
	};
		
	class PatchAbsoluteAddressAction : public DelayableAction
	{
	public:
		PatchAbsoluteAddressAction() : DelayableAction(8) { }
		
		virtual void Dump() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};
			
	class ExpressionAction : public Action
	{
	public:
		ExpressionAction(uint8_t aNumberOfBytes, int aExpressionIndex)
		  : numberOfBytes(aNumberOfBytes), expressionIndex(aExpressionIndex) { }

		virtual void Dump() const override;
		virtual size_t GetMinimumLength() const override { return numberOfBytes; }
		virtual size_t GetMaximumLength() const override { return numberOfBytes; }
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const override;

	protected:
		uint8_t numberOfBytes;
		int expressionIndex;
	};
	
	class DeltaExpressionAction : public ExpressionAction
	{
	public:
		DeltaExpressionAction(uint8_t aNumberOfBytes, int aExpressionIndex)
		: ExpressionAction(aNumberOfBytes, aExpressionIndex) { }
		
		virtual void Dump() const final;
		virtual size_t GetMinimumLength() const override { return 0; }
		virtual size_t GetMaximumLength() const override { return 0; }
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};
	
	class AddExpressionAction : public ExpressionAction
	{
	public:
		AddExpressionAction(uint8_t aNumberOfBytes, int aExpressionIndex)
		: ExpressionAction(aNumberOfBytes, aExpressionIndex) { }
		
		virtual void Dump() const final;
		virtual size_t GetMinimumLength() const override { return 0; }
		virtual size_t GetMaximumLength() const override { return 0; }
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};
	
	class LabelAction : public EmptyAction
	{
	public:
		LabelAction(bool aGlobal) : global(aGlobal) { }

	protected:
		bool global;
		bool hasReference = false;
		
	private:
		bool Simplify(ListAction *parent, size_t index) final;
	};
	
	class NamedLabelAction : public LabelAction
	{
	private:
		typedef LabelAction Inherited;
		
	public:
		NamedLabelAction(bool global, std::string aValue, int aAccessWidth=0)
		  : Inherited(global), value(aValue), accessWidth(aAccessWidth) { }
		
		virtual void Dump() const final;
		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
		int accessWidth;
		std::string value;
	};
	
	class NumericLabelAction : public LabelAction
	{
	private:
		typedef LabelAction Inherited;
		
	public:
		NumericLabelAction(bool global, int64_t aValue)
		  : Inherited(global), value(aValue) { }

		virtual void Dump() const final;

		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
		int64_t value;
	};
	
	class ExpressionLabelAction : public LabelAction
	{
	private:
		typedef LabelAction Inherited;
		
	public:
		ExpressionLabelAction(int aExpressionIndex)
		  : Inherited(true), expressionIndex(aExpressionIndex) { }

		virtual void Dump() const final;
		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
		int expressionIndex;
	};
	
	class PatchLabelAction : public EmptyAction
	{
	public:
		PatchLabelAction(bool aGlobal, uint8_t aBytes, uint8_t aOffset, const LabelOperand &aLabelOperand)
		  : global(aGlobal), bytes(aBytes), offset(aOffset), labelOperand(aLabelOperand) { }
		
	protected:
		bool    global;
		bool    blockLocal;		// True if this can be resolved within the block.
		bool	hasFoundTarget = false;
		bool 	hasFixedDelta = false;
		uint8_t bytes;
		uint8_t offset;
		int64_t delta;
		LabelOperand labelOperand;

	private:
		bool Simplify(ListAction *parent, size_t index) final;
		
		friend class Rel8AlternateActionCondition;
	};
	
	class PatchNameLabelAction : public PatchLabelAction
	{
	private:
		typedef PatchLabelAction Inherited;

	public:
		PatchNameLabelAction(bool global, uint8_t bytes, uint8_t offset, std::string aValue, const LabelOperand &labelOperand)
		: PatchLabelAction(global, bytes, offset, labelOperand), value(aValue) { }
		
		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
		virtual void Dump() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
		std::string value;
	};
	
	class PatchNumericLabelAction : public PatchLabelAction
	{
	private:
		typedef PatchLabelAction Inherited;

	public:
		PatchNumericLabelAction(bool global, uint8_t bytes, uint8_t offset, const LabelOperand &labelOperand)
		  : PatchLabelAction(global, bytes, offset, labelOperand), value(labelOperand.labelValue) { }
		
		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
		virtual void Dump() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
		int64_t value;		// Numeric label value.
	};

	class PatchExpressionLabelAction : public PatchLabelAction
	{
	private:
		typedef PatchLabelAction Inherited;

	public:
		PatchExpressionLabelAction(uint8_t bytes, uint8_t offset, const LabelOperand &aLabelOperand);
		
		virtual void Dump() const final;
		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};

	class AlignedAction : public EmptyAction
	{
	public:
		AlignedAction(int aAlignment) : alignment(aAlignment) { }
		
		virtual void Dump() const final;
		
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
		
	private:
		int alignment;
		virtual bool ResolveRelativeAddresses(ActionContext &context) override;
	};

	class AlignAction : public Action
	{
	public:
		AlignAction(int aAlignment) : alignment(aAlignment) { }

		virtual void Dump() const final;

		virtual size_t GetMinimumLength() const final;
		virtual size_t GetMaximumLength() const final;
		virtual bool Simplify(ListAction *parent, size_t index) final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
		int alignment;
		bool isFixed = false;
		int fixedLength = 0;

		virtual bool ResolveRelativeAddresses(ActionContext &context) override;
	};

	class UnalignAction : public Action
	{
	public:
		UnalignAction(int aAlignment) : alignment(aAlignment) { }
		
		virtual void Dump() const final;
		
		virtual size_t GetMinimumLength() const final;
		virtual size_t GetMaximumLength() const final;
		virtual bool Simplify(ListAction *parent, size_t index) final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
		
	private:
		int alignment;
		bool isFixed = false;
		int fixedLength = 0;
		
		virtual bool ResolveRelativeAddresses(ActionContext &context) override;
	};
	
	class DynamicOpcodeR : public Action
	{
	public:
		DynamicOpcodeR(uint8_t aRex,
					   uint8_t aModRM,
					   int aRegExpressionIndex,
					   const std::vector<uint8_t> &aOpcodes)
		: rex(aRex),
		  modRM(aModRM),
		  regExpressionIndex(aRegExpressionIndex),
		  opcodes(aOpcodes) { }
		
		virtual void Dump() const final;
		virtual size_t GetMinimumLength() const final;
		virtual size_t GetMaximumLength() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
		
	private:
		uint8_t rex;
		uint8_t modRM;
		int regExpressionIndex;
		std::vector<uint8_t> opcodes;
	};
	
	class DynamicOpcodeRR : public Action
	{
	public:
		DynamicOpcodeRR(uint8_t aRex,
						uint8_t aModRM,
						int aRegExpressionIndex,
						int aRmExpressionIndex,
						const std::vector<uint8_t> &aOpcodes)
		: rex(aRex),
		  modRM(aModRM),
		  regExpressionIndex(aRegExpressionIndex),
		  rmExpressionIndex(aRmExpressionIndex),
		  opcodes(aOpcodes) { }
		
		virtual void Dump() const final;
		virtual size_t GetMinimumLength() const final;
		virtual size_t GetMaximumLength() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
		uint8_t rex;
		uint8_t modRM;
		int regExpressionIndex;
		int rmExpressionIndex;
		std::vector<uint8_t> opcodes;
	};
	
	class DynamicOpcodeRM : public Action
	{
	public:
		DynamicOpcodeRM(uint8_t aRex,
						uint8_t aModRM,
						uint8_t aSib,
						int aDisplacement,
						int aRegExpressionIndex,
						int aScaleExpressionIndex,
						int aIndexExpressionIndex,
						int aBaseExpressionIndex,
						int aDisplacementExpressionIndex,
						const std::vector<uint8_t> &aOpcodes)
		: rex(aRex),
		  modRM(aModRM),
		  sib(aSib),
		  displacement(aDisplacement),
		  regExpressionIndex(aRegExpressionIndex),
		  scaleExpressionIndex(aScaleExpressionIndex),
		  indexExpressionIndex(aIndexExpressionIndex),
		  baseExpressionIndex(aBaseExpressionIndex),
		  displacementExpressionIndex(aDisplacementExpressionIndex),
		  opcodes(aOpcodes) { }
		
		virtual void Dump() const final;
		virtual size_t GetMinimumLength() const final;
		virtual size_t GetMaximumLength() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
		uint8_t rex;
		uint8_t modRM;
		uint8_t sib;
		int displacement;
		int regExpressionIndex;
		int scaleExpressionIndex;
		int indexExpressionIndex;
		int baseExpressionIndex;
		int displacementExpressionIndex;
		std::vector<uint8_t> opcodes;
	};

	class DynamicOpcodeO : public Action
	{
	public:
		DynamicOpcodeO(uint8_t aRex, int aExpressionIndex, const std::vector<uint8_t> &aOpcodes)
		  : rex(aRex), expressionIndex(aExpressionIndex), opcodes(aOpcodes) { }

		virtual void Dump() const final;
		virtual size_t GetMinimumLength() const final;
		virtual size_t GetMaximumLength() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
		uint8_t rex;
		int expressionIndex;
		std::vector<uint8_t> opcodes;
	};
	
	class DynamicOpcodeRV : public Action
	{
	public:
		DynamicOpcodeRV(uint8_t aVex3B2,
						uint8_t aVex3B3,
						uint8_t aModRM,
						int aReg0ExpressionIndex,
						int aReg1ExpressionIndex,
						const std::vector<uint8_t> &aOpcodes)
		: vex3B2(aVex3B2),
		  vex3B3(aVex3B3),
		  modRM(aModRM),
		  reg0ExpressionIndex(aReg0ExpressionIndex),
		  reg1ExpressionIndex(aReg1ExpressionIndex),
		  opcodes(aOpcodes) { }
		
		virtual void Dump() const final;
		virtual size_t GetMinimumLength() const final;
		virtual size_t GetMaximumLength() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
		
	private:
		uint8_t vex3B2;
		uint8_t vex3B3;
		uint8_t modRM;
		int reg0ExpressionIndex;
		int reg1ExpressionIndex;
		std::vector<uint8_t> opcodes;
	};
	
	class DynamicOpcodeRVR : public Action
	{
	public:
		DynamicOpcodeRVR(uint8_t aVex3B2,
							 uint8_t aVex3B3,
							 uint8_t aModRM,
							 int aReg0ExpressionIndex,
							 int aReg1ExpressionIndex,
							 int aReg2ExpressionIndex,
							 const std::vector<uint8_t> &aOpcodes)
		: vex3B2(aVex3B2),
		  vex3B3(aVex3B3),
		  modRM(aModRM),
		  reg0ExpressionIndex(aReg0ExpressionIndex),
		  reg1ExpressionIndex(aReg1ExpressionIndex),
		  reg2ExpressionIndex(aReg2ExpressionIndex),
		  opcodes(aOpcodes) { }
		
		virtual void Dump() const final;
		virtual size_t GetMinimumLength() const final;
		virtual size_t GetMaximumLength() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
		
	private:
		uint8_t vex3B2;
		uint8_t vex3B3;
		uint8_t modRM;
		int reg0ExpressionIndex;
		int reg1ExpressionIndex;
		int reg2ExpressionIndex;
		std::vector<uint8_t> opcodes;
	};
	
	class DynamicOpcodeRVM : public Action
	{
	public:
		DynamicOpcodeRVM(uint8_t aVex3B2,
						 uint8_t aVex3B3,
						 uint8_t aModRM,
						 uint8_t aSib,
						 int aDisplacement,
						 int aReg0ExpressionIndex,
						 int aReg1ExpressionIndex,
						 int aScaleExpressionIndex,
						 int aIndexExpressionIndex,
						 int aBaseExpressionIndex,
						 int aDisplacementExpressionIndex,
						 const std::vector<uint8_t> &aOpcodes)
		: vex3B2(aVex3B2),
		  vex3B3(aVex3B3),
		  modRM(aModRM),
		  sib(aSib),
		  displacement(aDisplacement),
		  reg0ExpressionIndex(aReg0ExpressionIndex),
		  reg1ExpressionIndex(aReg1ExpressionIndex),
		  scaleExpressionIndex(aScaleExpressionIndex),
		  indexExpressionIndex(aIndexExpressionIndex),
		  baseExpressionIndex(aBaseExpressionIndex),
		  displacementExpressionIndex(aDisplacementExpressionIndex),
		  opcodes(aOpcodes) { }
		
		virtual void Dump() const final;
		virtual size_t GetMinimumLength() const final;
		virtual size_t GetMaximumLength() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
		uint8_t vex3B2;
		uint8_t vex3B3;
		uint8_t modRM;
		uint8_t sib;
		int displacement;
		int reg0ExpressionIndex;
		int reg1ExpressionIndex;
		int scaleExpressionIndex;
		int indexExpressionIndex;
		int baseExpressionIndex;
		int displacementExpressionIndex;
		std::vector<uint8_t> opcodes;
	};

	class AlternateAction : public Action
	{
	public:
		AlternateAction();
		~AlternateAction();

		virtual bool IsLiteral() const final;
		virtual void AppendToLiteral(LiteralAction *literal) const final;
		virtual void DelayAndConsolidate() final;

		virtual size_t GetMinimumLength() const final;
		virtual size_t GetMaximumLength() const final;

		virtual void Dump() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

		size_t GetNumberOfAlternates() const	{ return alternateList.size(); }

		Action *GetSingleAlternateAndClearAlternateList();
		
		void Add(Action *action, const AlternateActionCondition *condition);

	private:
		struct Alternate
		{
			const AlternateActionCondition*	condition;
			Action*				 			action;
		};
		std::vector<Alternate> alternateList;

		bool Simplify(ListAction *parent, size_t index) final;
		bool ResolveRelativeAddresses(ActionContext &context) final;
	};

	class ListAction : public Action
	{
	public:
		ListAction();
		~ListAction();
		
		virtual bool IsLiteral() const final;
		virtual void AppendToLiteral(LiteralAction *literal) const final;

		virtual size_t GetMinimumLength() const final;
		virtual size_t GetMaximumLength() const final;
		
		void ResolveRelativeAddresses();

		void Append(Action *action);
		virtual void Dump() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
		virtual const Action *GetLastAction() const final { return actionList.size() ? actionList.back() : nullptr; }

		virtual void DelayAndConsolidate() final;

		bool HasData() const { return !actionList.empty(); }
		
		Action *GetActionAtIndex(size_t i) const { return actionList[i]; }
		void RemoveActionAtIndex(size_t i) { actionList.erase(actionList.begin()+i); }
		void ReplaceActionAtIndex(size_t i, Action *a) { actionList[i] = a; }
		void ReplaceActionAtIndexWithList(size_t i, std::vector<Action*> &actionList);

	private:
		std::vector<Action*> 			actionList;
		std::unordered_set<int64_t>		numericLabelSet;
		std::unordered_set<std::string>	namedLabelSet;

		bool Simplify(ListAction *parent, size_t index) final;
		bool ResolveRelativeAddresses(ActionContext &context) final;

		void DelayActions(const std::vector<Action *> &delayList, int startIndex, int toIndex, int delay);
		void ConsolidateLiteralActions();
	};

//============================================================================
} // namespace Javelin::Assembler::x64
//============================================================================
