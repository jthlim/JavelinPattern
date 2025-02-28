//============================================================================

#pragma once
#include "Javelin/Container/IntrusiveList.h"
#include "Javelin/Pattern/Internal/PatternTypes.h"
#include "Javelin/Type/String.h"
#include "Javelin/Container/Table.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class ICharacterWriter;
	
	namespace PatternInternal
	{
//============================================================================

		class Instruction;
		class InstructionList;
		class RecurseInstruction;
		struct AlternationComponent;
		struct CaptureComponent;

//============================================================================

		enum class AssertType : unsigned char
		{
			StartOfInput,
			EndOfInput,
			StartOfLine,
			EndOfLine,
			WordBoundary,
			NotWordBoundary,
			StartOfSearch,
		};
		
//============================================================================

		struct IComponent
		{
			virtual ~IComponent() { }
			virtual void Dump(ICharacterWriter& output, int depth);

			virtual bool IsEmpty() const					{ return false; }
			virtual bool IsTerminal() const					{ return false;	}
			virtual const IComponent* GetFront() const		{ return this; }
			virtual const IComponent* GetBack() const		{ return this; }
			virtual bool CanCoalesce() const				{ return true;  }
			virtual bool HasStartAnchor() const				{ return false; }
			virtual bool HasEndAnchor() const				{ return false; }
			virtual bool IsByte() const 					{ return false; }
			virtual bool IsCounter() const					{ return false; }
			virtual bool IsConcatenate() const				{ return false; }
			virtual void AddToCharacterRangeList(CharacterRangeList&) const { }
			virtual void BuildInstructions(InstructionList &instructionList) const = 0;
			virtual bool HasSpecificRepeatInstruction() const { return false; }
			virtual void BuildRepeatInstructions(InstructionList &instructionList) const { BuildInstructions(instructionList); }
			virtual uint32_t GetMinimumLength() const = 0;
			virtual uint32_t GetMaximumLength() const 		{ return GetMinimumLength(); }
			virtual bool IsFixedLength() const				{ return true; }
			virtual bool IsEqual(const IComponent* other) const = 0;
			virtual bool RequiresAnyByteMinimalForPartialMatch() const { return true; }
			
		protected:
			bool HasSameClass(const IComponent* other) const;
		};

//============================================================================

		struct EmptyComponent : public IComponent
		{
			virtual void BuildInstructions(InstructionList &instructionList) const final;
			virtual uint32_t GetMinimumLength() const final	{ return 0; }
			virtual bool IsEqual(const IComponent* other) const;
			virtual bool IsEmpty() const					{ return true; }
		};
		
//============================================================================

		struct BackReferenceComponent : public IComponent
		{
			BackReferenceComponent(CaptureComponent* aCaptureComponent) : captureComponent(aCaptureComponent)		{ }

			virtual uint32_t GetMinimumLength() const final;
			virtual uint32_t GetMaximumLength() const final;
			virtual void BuildInstructions(InstructionList &instructionList) const final;
			virtual bool IsFixedLength() const;
			virtual bool IsEqual(const IComponent* other) const final;

			CaptureComponent* captureComponent;
		};

		struct ByteComponent : public IComponent
		{
			ByteComponent(unsigned char aC) : c(aC) 		{ }
			virtual bool IsByte() const final 				{ return true; }
			virtual void AddToCharacterRangeList(CharacterRangeList& rangeList) const;
			virtual uint32_t GetMinimumLength() const final 	{ return 1; }
			virtual uint32_t GetMaximumLength() const final 	{ return 1; }
			virtual void BuildInstructions(InstructionList &instructionList) const final;
			virtual bool IsEqual(const IComponent* other) const final;

			unsigned char c;
			virtual void Dump(ICharacterWriter& output, int depth);
		};

//============================================================================
		
		struct AssertComponent : public IComponent
		{
			AssertComponent(AssertType aAssertType) : assertType(aAssertType) { }
			virtual void Dump(ICharacterWriter& output, int depth);
			virtual uint32_t GetMinimumLength() const final 											{ return 0; }
			virtual void BuildInstructions(InstructionList &instructionList) const final;
			virtual bool HasStartAnchor() const final;
			virtual bool HasEndAnchor() const final;
			virtual bool IsEqual(const IComponent* other) const final;
			virtual bool RequiresAnyByteMinimalForPartialMatch() const final;

			AssertType	assertType;
		};
		
//============================================================================

		struct TerminalComponent : public IComponent
		{
			TerminalComponent(bool aIsSuccess) : isSuccess(aIsSuccess) { }
			virtual bool IsTerminal() const																{ return true;	}
			virtual void Dump(ICharacterWriter& output, int depth);
			virtual uint32_t GetMinimumLength() const final 											{ return 0; }
			virtual void BuildInstructions(InstructionList &instructionList) const final;
			virtual bool IsEqual(const IComponent* other) const final;
			virtual bool RequiresAnyByteMinimalForPartialMatch() const final;

			bool isSuccess;
		};

//============================================================================

		struct ContentComponent : public IComponent
		{
			ContentComponent(IComponent* aContent) : content(aContent) { }
			~ContentComponent();
			virtual void Dump(ICharacterWriter& output, int depth) override;
			virtual uint32_t GetMinimumLength() const override;
			virtual uint32_t GetMaximumLength() const override;
			virtual bool IsFixedLength() const override;
			virtual bool RequiresAnyByteMinimalForPartialMatch() const override;
			
			IComponent* content;
		};

		struct LookAheadComponent : public ContentComponent
		{
			LookAheadComponent(IComponent* body, bool aExpectedResult) : ContentComponent(body), expectedResult(aExpectedResult) { }
			virtual void Dump(ICharacterWriter& output, int depth) final;
			virtual uint32_t GetMinimumLength() const override	{ return 0; }
			virtual uint32_t GetMaximumLength() const override	{ return 0; }
			virtual bool IsFixedLength() const override			{ return true; }
			virtual void BuildInstructions(InstructionList &instructionList) const override;
			virtual bool IsEqual(const IComponent* other) const final;
			virtual bool RequiresAnyByteMinimalForPartialMatch() const final { return true; }
		
			bool expectedResult;
		};

		struct LookBehindComponent : public LookAheadComponent
		{
			LookBehindComponent(IComponent* body, bool expectedResult) : LookAheadComponent(body, expectedResult) { }
			virtual void BuildInstructions(InstructionList &instructionList) const final;
		};

		struct ConditionalComponent : public IComponent
		{
			ConditionalComponent(TokenType aConditionType, IComponent* aCondition, IComponent* aFalseComponent, IComponent* aTrueComponent) : conditionType(aConditionType), condition(aCondition), falseComponent(aFalseComponent), trueComponent(aTrueComponent) { }
			~ConditionalComponent();
																																	 
			virtual void Dump(ICharacterWriter& output, int depth) final;
			virtual uint32_t GetMinimumLength() const override;
			virtual uint32_t GetMaximumLength() const override;
			virtual bool IsFixedLength() const override;
			virtual void BuildInstructions(InstructionList &instructionList) const override;
			virtual bool IsEqual(const IComponent* other) const final;
			
			TokenType 	conditionType;
			IComponent* condition;
			IComponent* falseComponent;
			IComponent* trueComponent;
		};

		struct CounterComponent : public ContentComponent
		{
			enum Mode
			{
				Maximal,
				Minimal,
				Possessive,
			};
			
			CounterComponent(uint32_t aMinimum, uint32_t aMaximum, Mode aMode, IComponent* content);

			virtual void Dump(ICharacterWriter& output, int depth) final;
			virtual uint32_t GetMinimumLength() const final;
			virtual uint32_t GetMaximumLength() const final;
			virtual bool IsFixedLength() const;
			virtual bool IsCounter() const					{ return true; }
			virtual bool IsEqual(const IComponent* other) const final;
			virtual bool RequiresAnyByteMinimalForPartialMatch() const final;
			
			virtual void BuildInstructions(InstructionList &instructionList) const final;
			Instruction* BuildMinimumInstructions(InstructionList &instructionList) const;
			void BuildMinimalInstructions(InstructionList &instructionList) const;
			void BuildMaximalInstructions(InstructionList &instructionList) const;

			uint32_t	minimum;
			uint32_t	maximum;
			Mode		mode;
		};

		struct CaptureComponent : public ContentComponent
		{
			CaptureComponent(uint32_t aCaptureIndex, IComponent* content) : ContentComponent(content), captureIndex(aCaptureIndex) { }
			virtual void Dump(ICharacterWriter& output, int depth);
			virtual bool CanCoalesce() const				{ return false; }
			virtual void BuildInstructions(InstructionList &instructionList) const final;
			virtual bool HasStartAnchor() const final;
			virtual bool HasEndAnchor() const final;
			virtual bool IsEqual(const IComponent* other) const final;
			
			uint32_t 				captureIndex;
			bool					emitSaveInstructions = true;
			bool					emitCaptureStart = true;
			bool					isRecurseTarget = false;
			bool					isBackReferenceTarget = false;
			mutable bool			captured = false;
			mutable Instruction*	firstInstruction = nullptr;
		};

//============================================================================

		struct GroupComponent : public IComponent
		{
			virtual ~GroupComponent();
			
			virtual void Dump(ICharacterWriter& output, int depth) override;
			virtual bool CanCoalesce() const override;
			virtual bool IsEqual(const IComponent* other) const override;
			
			Table<IComponent*> componentList;
		};

		struct ConcatenateComponent final : public GroupComponent
		{
			virtual uint32_t GetMinimumLength() const override;
			virtual uint32_t GetMaximumLength() const override;
			virtual const IComponent* GetFront() const final;
			virtual const IComponent* GetBack() const final;
			virtual void BuildInstructions(InstructionList &instructionList) const final;
			virtual bool HasStartAnchor() const final;
			virtual bool HasEndAnchor() const final;
			virtual bool IsFixedLength() const final;
			virtual bool IsConcatenate() const final			{ return true; }
			virtual bool RequiresAnyByteMinimalForPartialMatch() const final;
		};

		struct AlternationComponent : public GroupComponent
		{
			virtual uint32_t GetMinimumLength() const override;
			virtual uint32_t GetMaximumLength() const override;
			virtual void BuildInstructions(InstructionList &instructionList) const override;
			virtual bool HasStartAnchor() const override;
			virtual bool HasEndAnchor() const override;
			virtual bool IsFixedLength() const override;
			virtual bool RequiresAnyByteMinimalForPartialMatch() const override;

			void Optimize();
			void Optimize_MergeBytesToChracterSets();
			void Optimize_LeftFactor();
			void Optimize_RightFactor();
		};

		struct CharacterRangeListComponent : public AlternationComponent
		{
			CharacterRangeListComponent(bool aUseUtf8, const CharacterRangeList& aCharacterRangeList);
			CharacterRangeListComponent(bool aUseUtf8, const CharacterRange& range);
			CharacterRangeListComponent(bool aUseUtf8) : useUtf8(aUseUtf8) { }
			
			virtual bool IsByte() const;
			virtual void AddToCharacterRangeList(CharacterRangeList&) const;
			virtual void Dump(ICharacterWriter& output, int depth);
			virtual bool CanCoalesce() const final				{ return true;  }
			virtual uint32_t GetMinimumLength() const final;
			virtual uint32_t GetMaximumLength() const final;
			virtual bool HasStartAnchor() const final			{ return false; }
			virtual bool HasEndAnchor() const final				{ return false; }
			virtual bool IsFixedLength() const;
			virtual bool RequiresAnyByteMinimalForPartialMatch() const final;
			virtual bool IsEqual(const IComponent* other) const final;
			virtual bool HasSpecificRepeatInstruction() const final;
			virtual void BuildRepeatInstructions(InstructionList &instructionList) const;
			
			virtual void BuildInstructions(InstructionList &instructionList) const final;
			
			bool				useUtf8;
			CharacterRangeList 	characterRangeList;
			
			void BuildUtf8Components(InstructionList &instructionList) const;
			void BuildUtf8Components(AlternationComponent* alternation, CharacterRange interval, int bitShift, int offset) const;
			void BuildUtf8Components(AlternationComponent* alternation,IComponent* firstConcatenateInstruction, CharacterRange interval, int bitShift) const;
		};

//============================================================================

	struct RecurseComponent : public IComponent
	{
		RecurseComponent(int32_t aCaptureIndex) : captureIndex(aCaptureIndex) { }
		virtual void Dump(ICharacterWriter& output, int depth) final;
		virtual void BuildInstructions(InstructionList &instructionList) const final;
		virtual bool IsEqual(const IComponent* other) const final;
		virtual uint32_t GetMinimumLength() const final;
		virtual uint32_t GetMaximumLength() const final;
		virtual bool IsFixedLength() const final;
		
		mutable bool				isRecursing = false;
		int32_t 					captureIndex;
		CaptureComponent* 			target = nullptr;
		IntrusiveListNode			listNode;
		mutable Table<RecurseInstruction*>	recurseInstructionList;
	};

//============================================================================

	struct ResetCaptureComponent : public IComponent
	{
		ResetCaptureComponent(CaptureComponent* aCaptureComponent) : captureComponent(aCaptureComponent) { }
		virtual void Dump(ICharacterWriter& output, int depth) final;
		virtual void BuildInstructions(InstructionList &instructionList) const final;
		virtual bool IsEqual(const IComponent* other) const final;
		virtual uint32_t GetMinimumLength() const final;
		
		CaptureComponent* captureComponent;
	};

//============================================================================
	} // namespace PatternInternal
} // namespace Javelin
//============================================================================
