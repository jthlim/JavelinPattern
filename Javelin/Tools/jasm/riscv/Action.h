//============================================================================

#pragma once
#include "Javelin/Tools/jasm/riscv/Instruction.h"
#include "Javelin/Tools/jasm/Common/Action.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

//============================================================================

namespace Javelin::Assembler::riscv
{
//============================================================================

	class Assembler;

//============================================================================

    using Common::ActionOffset;
    using Common::ActionContext;
    using Common::ActionWriteContext;

//============================================================================

    using Common::AlternateActionCondition;
    using Common::AndAlternateActionCondition;
    using Common::AlwaysAlternateActionCondition;
    using Common::NeverAlternateActionCondition;
	
	class ZeroAlternateActionCondition : public Common::ZeroAlternateActionCondition
	{
	public:
        using Common::ZeroAlternateActionCondition::ZeroAlternateActionCondition;

		static const AlternateActionCondition* Create(const Operand *operand);

		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};

    class SimmAlternateActionCondition : public Common::ImmediateAlternateActionCondition
    {
    public:
        SimmAlternateActionCondition(int expressionIndex, int aBitCount) : Common::ImmediateAlternateActionCondition(expressionIndex), bitCount(aBitCount) { }
        
        virtual std::string GetDescription() const final;
        virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
        virtual bool Equals(const AlternateActionCondition *other) const;

    private:
        int bitCount;
    };

    class UimmAlternateActionCondition : public Common::ImmediateAlternateActionCondition
    {
    public:
        UimmAlternateActionCondition(int expressionIndex, int aBitCount) : Common::ImmediateAlternateActionCondition(expressionIndex), bitCount(aBitCount) { }
        
        virtual std::string GetDescription() const final;
        virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
        virtual bool Equals(const AlternateActionCondition *other) const;

    private:
        int bitCount;
    };

    // Calculates a relative address from the end of action, and returns true if it fits into a the bit width.
    class DeltaAlternateActionCondition : public Common::AlternateActionCondition
    {
    public:
        DeltaAlternateActionCondition(int bitWidth, const LabelOperand &aLabelOperand);
                
        virtual Result IsValid(ActionContext &context, const Action *action) const final;
        virtual bool Equals(const AlternateActionCondition *other) const;
        
        bool ShouldProcessIsValidAtEndOfAlternate() const { return false; }

        virtual std::string GetDescription() const;
        virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;

    protected:
        // This is the actual bit range supported.
        // e.g. if the delta supports ±1kb, this requires 11 bits, and bitWidth would be 11,
        // even if the lowest bit (bit 0) isn't stored.
        int bitWidth;
        
        LabelOperand labelOperand;
        
    private:
        Result IsValidForJumpType(ActionContext &context) const;
        Result IsValid(const ActionOffset &anchorOffset, const ActionOffset &destinationOffset) const;
    };

    // Calculates a relative address from the end of action, and returns true if it fits into a CBDelta value.
    class CBDeltaAlternateActionCondition : public DeltaAlternateActionCondition
    {
    public:
        virtual std::string GetDescription() const final;

        static const AlternateActionCondition* Create(const Operand *operand);
        
    private:
        CBDeltaAlternateActionCondition(const LabelOperand &labelOperand)
        : DeltaAlternateActionCondition(9, labelOperand) { }
    };

    class CJDeltaAlternateActionCondition : public DeltaAlternateActionCondition
    {
    public:
        virtual std::string GetDescription() const final;

        static const AlternateActionCondition* Create(const Operand *operand);
        
    private:
        CJDeltaAlternateActionCondition(const LabelOperand &labelOperand)
        : DeltaAlternateActionCondition(12, labelOperand) { }
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
        DelayableAction(int initialDelay) : Common::DelayableAction(initialDelay, 32768) { }
    };
			
	class PatchAbsoluteAddressAction : public DelayableAction
	{
	public:
		PatchAbsoluteAddressAction() : DelayableAction(8) { }
		
		virtual void Dump() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};
			
	class PatchExpressionAction : public DelayableAction
	{
	public:
		PatchExpressionAction(riscvAssembler::RelEncoding aRelEncoding, int offset, int expressionIndex, Assembler& assembler);
		
		virtual void Dump() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
        riscvAssembler::RelEncoding relEncoding;
		int expressionIndex;
	};
			
	class PatchOpcodeAction : public DelayableAction
	{
	public:
		enum Format
		{
			Signed,
			Unsigned,
			Masked,
            CIValue,
		};

		PatchOpcodeAction(Format aFormat, uint8_t opcodeSize, uint8_t aNumberOfBits, uint8_t aBitOffset, uint8_t aValueShift, int aExpressionIndex, Assembler &assembler);
		
		virtual void Dump() const override;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const override;
		virtual bool CanGroup() const final { return true; }
		virtual bool CanGroup(Action *other) const final;

	private:
		Format format;
		uint8_t numberOfBits;
		uint8_t bitOffset;
		uint8_t valueShift;
		int expressionIndex;
	};

    class ExpressionAction : public Common::ExpressionAction
    {
    public:
        using Common::ExpressionAction::ExpressionAction;
        
        virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const override;
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
	
	class PatchLabelAction : public DelayableAction
	{
	public:
		PatchLabelAction(bool aGlobal, riscvAssembler::RelEncoding aRelEncoding, int offset, const LabelOperand &aLabelOperand)
		  : DelayableAction(offset), global(aGlobal), relEncoding(aRelEncoding), labelOperand(aLabelOperand) { }
		
	protected:
		bool    global;
		bool    blockLocal;		// True if this can be resolved within the block.
		bool	hasFoundTarget = false;
		bool 	hasFixedDelta = false;
        riscvAssembler::RelEncoding relEncoding;
		int64_t delta;
		LabelOperand labelOperand;

	private:
		virtual bool Simplify(Common::ListAction *parent, size_t index) final;

        friend class DeltaAlternateActionCondition;
    };
	
	class PatchNameLabelAction : public PatchLabelAction
	{
	private:
		typedef PatchLabelAction Inherited;

	public:
		PatchNameLabelAction(bool global, riscvAssembler::RelEncoding relEncoding, int offset, std::string aValue, const LabelOperand &labelOperand)
		: PatchLabelAction(global, relEncoding, offset, labelOperand), value(aValue) { }
		
		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
		virtual void Dump() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
		std::string value;
		JumpType jumpType = JumpType::Name;
	};
	
	class PatchNumericLabelAction : public PatchLabelAction
	{
	private:
		typedef PatchLabelAction Inherited;

	public:
		PatchNumericLabelAction(bool global, riscvAssembler::RelEncoding relEncoding, int offset, const LabelOperand &labelOperand)
		  : PatchLabelAction(global, relEncoding, offset, labelOperand), jumpType(labelOperand.jumpType), value(labelOperand.labelValue) { }
		
		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
		virtual void Dump() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
		int64_t value;		// Numeric label value.
		JumpType jumpType;
	};

	
	class PatchExpressionLabelAction : public PatchLabelAction
	{
	private:
		typedef PatchLabelAction Inherited;

	public:
		PatchExpressionLabelAction(riscvAssembler::RelEncoding relEncoding, int offset, const LabelOperand &aLabelOperand);
		
		virtual void Dump() const final;
		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};

//============================================================================

    using Common::AlignedAction;

//============================================================================

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
	
	class AlternateAction : public Common::AlternateAction
	{
	public:
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};

	class ListAction : public Common::ListAction
	{
	public:
		void AppendOpcode(uint32_t opcode);
        void AppendCOpcode(uint32_t opcode);

	private:
		void DelayActions(const std::vector<Action *> &delayList, int startIndex, int toIndex, int delay);
		void ConsolidateLiteralActions();
	};

//============================================================================
} // namespace Javelin::Assembler::riscv
//============================================================================
