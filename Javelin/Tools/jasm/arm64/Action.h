//============================================================================

#pragma once
#include "Javelin/Tools/jasm/arm64/Instruction.h"
#include "Javelin/Tools/jasm/Common/Action.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

//============================================================================

namespace Javelin::Assembler::arm64
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

//============================================================================

	class ZeroAlternateActionCondition : public Common::ZeroAlternateActionCondition
	{
	public:
        using Common::ZeroAlternateActionCondition::ZeroAlternateActionCondition;
        
		static const AlternateActionCondition* Create(const Operand *operand);

		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};
	
	class Delta21AlternateActionCondition : public Common::ImmediateAlternateActionCondition
	{
	public:
        Delta21AlternateActionCondition(int expressionIndex) : Common::ImmediateAlternateActionCondition(expressionIndex) { }
		
		virtual std::string GetDescription() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};
	
	class Delta26x4AlternateActionCondition : public Common::ImmediateAlternateActionCondition
	{
	public:
        Delta26x4AlternateActionCondition(int expressionIndex) : Common::ImmediateAlternateActionCondition(expressionIndex) { }

		virtual std::string GetDescription() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const final;
	};

	class AdrpAlternateActionCondition : public Common::ImmediateAlternateActionCondition
	{
	public:
        AdrpAlternateActionCondition(int expressionIndex) : Common::ImmediateAlternateActionCondition(expressionIndex) { }

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
		PatchExpressionAction(arm64Assembler::RelEncoding aRelEncoding, int offset, int expressionIndex, Assembler& assembler);
		
		virtual void Dump() const final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
        arm64Assembler::RelEncoding relEncoding;
		int expressionIndex;
	};
			
	class PatchOpcodeAction : public DelayableAction
	{
	public:
		enum Sign
		{
			Signed,
			Unsigned,
			Masked,
		};

		PatchOpcodeAction(Sign aSign, uint8_t aNumberOfBits, uint8_t aBitOffset, uint8_t aValueShift, int aExpressionIndex, Assembler &assembler);
		
		virtual void Dump() const override;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const override;
		virtual bool CanGroup() const final { return true; }
		virtual bool CanGroup(Action *other) const final;

	private:
		Sign sign;
		uint8_t numberOfBits;
		uint8_t bitOffset;
		uint8_t valueShift;
		int expressionIndex;
	};

	class PatchLogicalImmediateOpcodeAction : public DelayableAction
	{
	public:
		PatchLogicalImmediateOpcodeAction(uint8_t aNumberOfBits, int aExpressionIndex, Assembler &assembler);
		
		virtual void Dump() const override;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const override;
		
		virtual bool CanGroup() const final { return true; }
		virtual bool CanGroup(Action *other) const final;

	private:
		uint8_t numberOfBits;	// 32 or 64.
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
		PatchLabelAction(bool aGlobal, arm64Assembler::RelEncoding aRelEncoding, int offset, const LabelOperand &aLabelOperand)
		  : DelayableAction(offset), global(aGlobal), relEncoding(aRelEncoding), labelOperand(aLabelOperand) { }
		
	protected:
		bool    global;
		bool    blockLocal;		// True if this can be resolved within the block.
		bool	hasFoundTarget = false;
		bool 	hasFixedDelta = false;
        arm64Assembler::RelEncoding relEncoding;
		int64_t delta;
		LabelOperand labelOperand;

	private:
		virtual bool Simplify(Common::ListAction *parent, size_t index) final;
	};
	
	class PatchNameLabelAction : public PatchLabelAction
	{
	private:
		typedef PatchLabelAction Inherited;

	public:
		PatchNameLabelAction(bool global, arm64Assembler::RelEncoding relEncoding, int offset, std::string aValue, const LabelOperand &labelOperand)
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
		PatchNumericLabelAction(bool global, arm64Assembler::RelEncoding relEncoding, int offset, const LabelOperand &labelOperand)
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
		PatchExpressionLabelAction(arm64Assembler::RelEncoding relEncoding, int offset, const LabelOperand &aLabelOperand);
		
		virtual void Dump() const final;
		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};

//============================================================================

    using Common::AlignedAction;

//============================================================================

	// Special case opcode for arm64
	class MovExpressionImmediateAction : public Action
	{
	public:
		MovExpressionImmediateAction(int aBitWidth, int aRegisterIndex, int aExpressionIndex, Assembler& assembler);
		
		virtual void Dump() const final;
		virtual size_t GetMinimumLength() const override { return 4; }
		virtual size_t GetMaximumLength() const override { return 4; }
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;

	private:
		int bitWidth;
		int registerIndex;
		int expressionIndex;
	};
			
//============================================================================

	class AlignAction : public Common::AlignAction
	{
	public:
        using Common::AlignAction::AlignAction;

        bool Simplify(Common::ListAction *parent, size_t index) final;
		void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
	};

	class UnalignAction : public Common::UnalignAction
	{
	public:
        using Common::UnalignAction::UnalignAction;
		
        bool Simplify(Common::ListAction *parent, size_t index) final;
		void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
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

	private:
		void DelayActions(const std::vector<Action *> &delayList, int startIndex, int toIndex, int delay);
		void ConsolidateLiteralActions();
	};

//============================================================================
} // namespace Javelin::Assembler::arm64
//============================================================================
