//============================================================================

#pragma once
#include "JumpType.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

//============================================================================

namespace Javelin::Assembler::Common
{
//============================================================================

	class Action;
	class ListAction;
	class LiteralAction;
	
//============================================================================
	
	struct ActionOffset
	{
		// This is incremented every time a variable length instruction is encountered.
		int blockIndex;
		
		// Alignment, if any. A variable length instruction will reset this to 0.
		int alignment;
		
		//
		ssize_t offsetIntoBlock;
		
		ssize_t totalMinimumOffset;
		ssize_t totalMaximumOffset;
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
		ActionWriteContext(bool aIsDataSegment, int aAppendAssemblerReferenceSize, const std::string& variableName)
        : isDataSegment(aIsDataSegment), appendAssemblerReferenceSize(aAppendAssemblerReferenceSize) { SetVariableName(variableName); }

		bool isDataSegment;

        // The size of the structure used to collect the base instruction block information.
        //
        // Expressions are stored directly after this in memory.
        int appendAssemblerReferenceSize;
        
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
		Action *previousAction = nullptr;

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
        
        // Whether IsValid should be called at the end of the instruction sequence.
        // If false, IsValid will be called before advancing the alternate.
        virtual bool ShouldProcessIsValidAtEndOfAlternate() const { return true; }
		
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

		void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action, int opcode) const;
        void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action, int opcode, int bitCount) const;
	};

	class ZeroAlternateActionCondition : public ImmediateAlternateActionCondition
	{
	public:
		constexpr ZeroAlternateActionCondition(int aExpressionIndex) : ImmediateAlternateActionCondition(aExpressionIndex) { }

		virtual std::string GetDescription() const final;
	};
	
//============================================================================

	class Action
	{
	public:
		virtual ~Action() { }
		
		virtual bool IsLiteral() const { return false; }
		virtual bool IsLiteralAction() const { return false; }
        virtual std::vector<uint8_t>& GetLiteralData();
		virtual void Dump() const = 0;
		virtual void AppendToLiteral(LiteralAction *literal) const { }
		virtual const Action *GetLastAction() const { return this; }
		virtual size_t GetMinimumLength() const = 0;
		virtual size_t GetMaximumLength() const = 0;
		virtual bool CanDelay(int amount) const { return false; }
		virtual void Delay(int amount) { }
		virtual void DelayAndConsolidate() { }
		virtual void Group() { }
		virtual bool CanGroup() const { return false; }
		virtual bool CanGroup(Action *other) const { return false; }

		virtual bool ResolveRelativeAddresses(ActionContext &context);
		virtual bool Simplify(ListAction *parent, size_t index) { return false; }

		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const = 0;

		bool CanDelay() const { return CanDelay(0); }
		
		static void WriteSigned16(std::vector<uint8_t> &result, int16_t value);
        static void WriteSigned32(std::vector<uint8_t> &result, int32_t value) { WriteUnsigned32(result, (uint32_t) value); }
		static void WriteUnsigned32(std::vector<uint8_t> &result, uint32_t value);
		static void WriteExpressionOffset(std::vector<uint8_t> &result, const ActionWriteContext &context, int expressionIndex, int additionalOffset=0);
		
	protected:
		Action() = default;
		ActionOffset actionOffset;

	private:
		Action(const Action&) = delete;
		void operator=(const Action&) = delete;

        void UpdateActionOffset(bool forwards, ActionOffset &offset) const;
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
        SetAssemblerVariableNameAction(const std::string &aVariableName);

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
		virtual bool IsLiteralAction() const final { return true; }
        virtual std::vector<uint8_t>& GetLiteralData() final { return bytes; }
		virtual void Dump() const final;
		bool Simplify(ListAction *parent, size_t index) final;

		virtual void AppendToLiteral(LiteralAction *literal) const final;
		
		virtual size_t GetMinimumLength() const final { return bytes.size(); }
		virtual size_t GetMaximumLength() const final { return bytes.size(); }

		int GetNumberOfBytes() const { return int(bytes.size()); }
		void AppendBytes(const std::vector<uint8_t>& literalBytes);
		void AppendBytes(const uint8_t *bytes, size_t length);

	protected:
		std::vector<uint8_t> bytes;
	};

	class DelayableAction : public EmptyAction
	{
	public:
		DelayableAction(int initialDelay, int aMaxDelay) : delay(initialDelay), maxDelay(aMaxDelay) { }

		virtual bool CanDelay(int amount) const final { return amount + delay < maxDelay; }
		virtual void Delay(int amount) final { delay += amount; }
			
	protected:
		int delay;
        int maxDelay;
	};
			
	class ExpressionAction : public Action
	{
	public:
		ExpressionAction(uint8_t aNumberOfBytes, int aExpressionIndex)
		: numberOfBytes(aNumberOfBytes), expressionIndex(aExpressionIndex) { }
		
		virtual void Dump() const override;
		virtual size_t GetMinimumLength() const override { return numberOfBytes; }
		virtual size_t GetMaximumLength() const override { return numberOfBytes; }
		
	protected:
		uint8_t numberOfBytes;
		int expressionIndex;
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

	protected:
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

    protected:
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

    protected:
		int expressionIndex;
	};
	
    // Marks a point in the code where the code is known to be aligned to a particular boundary.
	class AlignedAction : public EmptyAction
	{
	public:
		AlignedAction(int aAlignment) : alignment(aAlignment) { }
		
		virtual void Dump() const final;
		
		virtual void WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const final;
		
	private:
		int alignment;
		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
	};

	class AlignAction : public Action
	{
	public:
		AlignAction(int aAlignment) : alignment(aAlignment) { }

		virtual void Dump() const final;

		virtual size_t GetMinimumLength() const final;
		virtual size_t GetMaximumLength() const final;
		virtual bool Simplify(ListAction *parent, size_t index) override = 0;

	protected:
		int alignment;
		bool isFixed = false;
		int fixedLength = 0;

		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
	};

	class UnalignAction : public Action
	{
	public:
		UnalignAction(int aAlignment) : alignment(aAlignment) { }
		
		virtual void Dump() const final;
		
		virtual size_t GetMinimumLength() const final;
		virtual size_t GetMaximumLength() const final;
		virtual bool Simplify(ListAction *parent, size_t index) override = 0;
		
    protected:
		int alignment;
		bool isFixed = false;
		int fixedLength = 0;
		
		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
	};
	
	class AlternateAction : public Action
	{
	public:
		AlternateAction();
		~AlternateAction();

		virtual bool IsLiteral() const final;
		virtual void AppendToLiteral(LiteralAction *literal) const final;
		virtual void DelayAndConsolidate() final;
		virtual void Group() final;

		virtual size_t GetMinimumLength() const final;
		virtual size_t GetMaximumLength() const final;

		virtual void Dump() const final;

		size_t GetNumberOfAlternates() const	{ return alternateList.size(); }

		Action *GetSingleAlternateAndClearAlternateList();
		
		void Add(Action *action, const AlternateActionCondition *condition);

	protected:
		struct Alternate
		{
			const AlternateActionCondition*	condition;
			Action*				 			action;
		};
		std::vector<Alternate> alternateList;

		virtual bool Simplify(ListAction *parent, size_t index) final;
		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
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
		virtual void Group() final;

		bool HasData() const { return !actionList.empty(); }
		
		Action *GetActionAtIndex(size_t i) const { return actionList[i]; }
		void RemoveActionAtIndex(size_t i) { actionList.erase(actionList.begin()+i); }
		void ReplaceActionAtIndex(size_t i, Action *a) { actionList[i] = a; }
		void ReplaceActionAtIndexWithList(size_t i, std::vector<Action*> &actionList);

	protected:
		std::vector<Action*> 			actionList;
		std::unordered_set<int64_t>		numericLabelSet;
		std::unordered_set<std::string>	namedLabelSet;

		virtual bool Simplify(ListAction *parent, size_t index) final;
		virtual bool ResolveRelativeAddresses(ActionContext &context) final;
		
		void DelayActions(const std::vector<Action *> &delayList, int startIndex, int toIndex, int delay);
		void ConsolidateLiteralActions();
	};

//============================================================================
} // namespace Javelin::Assembler::Common
//============================================================================
