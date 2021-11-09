//============================================================================

#pragma once
#include "Javelin/Tools/jasm/x64/Instruction.h"
#include "Javelin/Tools/jasm/Common/Action.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

//============================================================================

namespace Javelin::Assembler::x64
{
//============================================================================

    using Common::ActionOffset;
    using Common::ActionContext;
    using Common::ActionWriteContext;

//============================================================================

    using Common::AlternateActionCondition;
    using Common::AndAlternateActionCondition;

    class AlwaysAlternateActionCondition : public Common::AlwaysAlternateActionCondition
    {
    public:
        static const AlternateActionCondition* Create(const Operand *operand) { return &instance; }
    };

    class NeverAlternateActionCondition : public Common::NeverAlternateActionCondition
    {
    public:
        static const AlternateActionCondition* Create(const Operand *operand) { return &instance; }
    };

	class ZeroAlternateActionCondition : public Common::ZeroAlternateActionCondition
	{
    private:
        typedef Common::ZeroAlternateActionCondition Inherited;
        
	public:
		constexpr ZeroAlternateActionCondition(int aExpressionIndex) : Inherited(aExpressionIndex) { }
        
		static const AlternateActionCondition* Create(const Operand *operand);

        virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};

	class Imm8AlternateActionCondition : public Common::ImmediateAlternateActionCondition
	{
	public:
        using Common::ImmediateAlternateActionCondition::ImmediateAlternateActionCondition;
        
		static const AlternateActionCondition* Create(const Operand *operand);

		virtual std::string GetDescription() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};

	class Imm16AlternateActionCondition : public Common::ImmediateAlternateActionCondition
	{
	public:
        using Common::ImmediateAlternateActionCondition::ImmediateAlternateActionCondition;

        static const AlternateActionCondition* Create(const Operand *operand);

		virtual std::string GetDescription() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};

	class Imm32AlternateActionCondition : public Common::ImmediateAlternateActionCondition
	{
	public:
        using Common::ImmediateAlternateActionCondition::ImmediateAlternateActionCondition;

        static const AlternateActionCondition* Create(const Operand *operand);

		virtual std::string GetDescription() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};

	class UImm32AlternateActionCondition : public Common::ImmediateAlternateActionCondition
	{
	public:
        using Common::ImmediateAlternateActionCondition::ImmediateAlternateActionCondition;

        static const AlternateActionCondition* Create(const Operand *operand);

		virtual std::string GetDescription() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};
	
	// Calculates a relative address from the end of action, and returns true if it fits into a SImm8 value.
	class Rel8AlternateActionCondition : public Common::AlternateActionCondition
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
		Result IsValidForJumpType(ActionContext &context, JumpType jumpType) const;
		
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
		Result IsValidForJumpType(ActionContext &context, JumpType jumpType) const;
		
		static Result IsValid(const ActionOffset &anchorOffset, const ActionOffset &destinationOffset);
	};
	
	class Delta32AlternateActionCondition : public Common::ImmediateAlternateActionCondition
	{
	public:
        Delta32AlternateActionCondition(int expressionIndex) : Common::ImmediateAlternateActionCondition(expressionIndex) { }

		virtual std::string GetDescription() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};
	
//============================================================================

    using Common::Action;
    using Common::EmptyAction;
    using Common::SetAssemblerVariableNameAction;
	
	class LiteralAction : public Common::LiteralAction
	{
	public:
        using Common::LiteralAction::LiteralAction;
		
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};

    class DelayableAction : public Common::DelayableAction
    {
    public:
        DelayableAction(int initialDelay) : Common::DelayableAction(initialDelay, 256) { }
    };

	class PatchAbsoluteAddressAction : public DelayableAction
	{
	public:
		PatchAbsoluteAddressAction() : DelayableAction(8) { }
		
		virtual void Dump() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};
			
	class ExpressionAction : public Common::ExpressionAction
	{
	public:
        using Common::ExpressionAction::ExpressionAction;

		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const override;
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
	
	class NamedLabelAction : public Common::NamedLabelAction
	{
	public:
        using Common::NamedLabelAction::NamedLabelAction;

        virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};
	
	class NumericLabelAction : public Common::NumericLabelAction
	{
	public:
        using Common::NumericLabelAction::NumericLabelAction;

		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};
	
class ExpressionLabelAction : public Common::ExpressionLabelAction
	{
	public:
        using Common::ExpressionLabelAction::ExpressionLabelAction;

		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
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
		bool Simplify(Common::ListAction *parent, size_t index) final;
		
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

    using Common::AlignedAction;

	class AlignAction : public Common::AlignAction
	{
	public:
        using Common::AlignAction::AlignAction;

        virtual bool Simplify(Common::ListAction *parent, size_t index) final;
        virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};

	class UnalignAction : public Common::UnalignAction
	{
	public:
        using Common::UnalignAction::UnalignAction;

        virtual bool Simplify(Common::ListAction *parent, size_t index) final;
        virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};
	
	class DynamicOpcodeR : public Common::Action
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
	
	class DynamicOpcodeRR : public Common::Action
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

    class AlternateAction : public Common::AlternateAction
	{
	public:
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};

	class ListAction : public Common::ListAction
	{
    public:
	};

//============================================================================
} // namespace Javelin::Assembler::x64
//============================================================================
