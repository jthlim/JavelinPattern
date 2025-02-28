//==========================================================================

#pragma once
#include "Javelin/Container/BitTable.h"
#include "Javelin/Container/IntrusiveList.h"
#include "Javelin/Container/Table.h"
#include "Javelin/Container/Tuple.h"
#include "Javelin/Type/Character.h"
#include "Javelin/Type/String.h"
#include "Javelin/Type/Interval.h"
#include "Javelin/Pattern/Internal/PatternInstructionType.h"

//==========================================================================

namespace Javelin
{
//==========================================================================
	
	class ICharacterWriter;
	
	namespace PatternInternal
	{
//==========================================================================

		class ByteCodeBuilder;
		class NibbleMask;
		struct Instruction;

//==========================================================================

		struct InstructionTable : public Table<Instruction*, TableStorage_Dynamic<DynamicTableAllocatePolicy_NewDeleteWithInlineStorage<8>::Policy>>
		{
		public:
			bool ContainsInstruction(InstructionType type) const;
			bool HasStartAnchor() const;
			
			// Boolean is if there is at least one ie.. a+ rather than a*
			Tuple<Instruction*, bool> GetRepeatedByteInstruction() const;
			
			void Dump(ICharacterWriter& output) const;
		};
		
		struct InstructionReference
		{
			Instruction**	storage;
			Instruction*	instruction;
			
			static constexpr Instruction **const PREVIOUS_REFERENCE = nullptr;
			static Instruction **const SELF_REFERENCE;
			
			static Instruction* const START_REFERENCE;
			static Instruction* const SEARCH_OPTIMIZER_REFERENCE;
			
			bool operator==(Instruction** p) const 					{ return storage == p; }
			bool operator==(const InstructionReference& a) const 	{ return storage == a.storage && instruction == a.instruction; }
			bool operator!=(const InstructionReference& a) const 	{ return !(*this == a); }
			
			Instruction* GetInstruction(Instruction* current) const;
			
			static const InstructionReference PREVIOUS;
		};
		
		typedef Table<InstructionReference, TableStorage_Dynamic<DynamicTableAllocatePolicy_NewDeleteWithInlineStorage<8>::Policy>> ReferenceTable;
		
//==========================================================================

		struct Instruction
		{
			InstructionType			type;
			uint8_t					startReachable;
			bool					splitReachable;
			uint32_t 				index;
			ReferenceTable			referenceList;
			IntrusiveListNode		listNode;
			
			virtual Instruction* Clone() const = 0;
			virtual ~Instruction() { }
			
			virtual void 		BuildByteCode(ByteCodeBuilder& builder) const;
			virtual uint32_t	GetByteCodeData() const { return 0; }
			
			virtual bool		HasStartAnchor() const		{ return false; }
			virtual bool		IsEmptyWidth() const		{ return false; }
			virtual bool		IsByteConsumer() const		{ return IsSimpleByteConsumer(); }
			virtual bool		IsSimpleByteConsumer() const	{ return false; }
			virtual void		SetValidBytes(StaticBitTable<256>& mask) const;
			StaticBitTable<256> GetValidBytes() const 		{ StaticBitTable<256> result; SetValidBytes(result); return result; }
			
			virtual int				GetNumberOfPlanes() const											{ return 1; 			}
			virtual void			SetValidBytesForPlane(int plane, StaticBitTable<256>& mask) const	{ SetValidBytes(mask); 	}
			virtual Instruction*	GetTargetForPlane(int plane) const									{ return GetNext();		}
			StaticBitTable<256>		GetValidBytesForPlane(int plane) const								{ StaticBitTable<256> result; SetValidBytesForPlane(plane, result); return result; 	}
			
			bool				HasSingleReference() const		{ return referenceList.GetCount() == 1; }
			bool				HasMultipleReferences() const	{ return referenceList.GetCount() > 1; }
			
			Instruction*		GetPrevious() const			{ return listNode.GetPrevious<Instruction, &Instruction::listNode>();	}
			Instruction*		GetNext() const				{ return listNode.GetNext<Instruction, &Instruction::listNode>();		}
			
			bool IsSingleReference() const					{ return referenceList.GetCount() <= 1; }
			bool LeadsToMatch(bool direct) const;		// Returns true if this instruction *always* leads to a match. direct if it cannot go through dispatch or other loops
			bool CanLeadToMatch() const;
			
			void TransferReferencesTo(Instruction* target);
			void TagReachable(int flags);
			
			virtual void Link();
			virtual void Unlink();
			
			virtual void Dump(ICharacterWriter& output) const = 0;
			friend bool operator==(const Instruction &a, const Instruction& b);
			friend bool operator!=(const Instruction &a, const Instruction& b) { return !(a == b); }
			
		protected:
			Instruction(InstructionType aType) : type(aType) { }
			Instruction(const Instruction& a);
			
			// Returns an instruction to process next to avoid deep recursion.
			virtual Instruction* ProcessReachable(int flags);
		};
		
		struct JumpInstruction : public Instruction
		{
			JumpInstruction() : Instruction(InstructionType::Jump) { }
			JumpInstruction(Instruction* aTarget);
			virtual Instruction* Clone() const final;
			
			virtual uint32_t GetByteCodeData() const;
			virtual void 	Dump(ICharacterWriter& output) const;
			virtual bool	HasStartAnchor() const		{ return target->HasStartAnchor(); }
			virtual Instruction* ProcessReachable(int flags);
			virtual void	Link() final;
			virtual void 	Unlink() final;

			Instruction*	target;
		};
		
		struct AdvanceByteInstruction : public Instruction
		{
			AdvanceByteInstruction() : Instruction(InstructionType::AdvanceByte) { }
			virtual Instruction* Clone() const final;
			
			virtual bool IsSimpleByteConsumer() const	{ return true; }
			virtual void SetValidBytes(StaticBitTable<256>& mask) const;
			virtual void Dump(ICharacterWriter& output) const;
		};
		
		struct AnyByteInstruction : public Instruction
		{
			AnyByteInstruction() : Instruction(InstructionType::AnyByte) { }
			virtual Instruction* Clone() const final;
			
			virtual bool IsSimpleByteConsumer() const	{ return true; }
			virtual void SetValidBytes(StaticBitTable<256>& mask) const;
			virtual void Dump(ICharacterWriter& output) const;
		};

		struct AssertStartOfInputInstruction : public Instruction
		{
			AssertStartOfInputInstruction() : Instruction(InstructionType::AssertStartOfInput) { }
			virtual Instruction* Clone() const final;
			virtual bool IsEmptyWidth() const final		{ return true; }
			virtual void Dump(ICharacterWriter& output) const;
			virtual bool HasStartAnchor() const			{ return true; }
		};
		
		struct AssertEndOfInputInstruction : public Instruction
		{
			AssertEndOfInputInstruction() : Instruction(InstructionType::AssertEndOfInput) { }
			virtual Instruction* Clone() const final;
			virtual bool IsEmptyWidth() const final		{ return true; }
			virtual void Dump(ICharacterWriter& output) const;
		};
		
		struct AssertStartOfLineInstruction : public Instruction
		{
			AssertStartOfLineInstruction() : Instruction(InstructionType::AssertStartOfLine) { }
			virtual Instruction* Clone() const final;
			virtual bool IsEmptyWidth() const final		{ return true; }
			virtual void Dump(ICharacterWriter& output) const;
		};
		
		struct AssertEndOfLineInstruction : public Instruction
		{
			AssertEndOfLineInstruction() : Instruction(InstructionType::AssertEndOfLine) { }
			virtual Instruction* Clone() const final;
			virtual bool IsEmptyWidth() const final		{ return true; }
			virtual void Dump(ICharacterWriter& output) const;
		};

		struct AssertWordBoundaryBaseInstruction : public Instruction
		{
			AssertWordBoundaryBaseInstruction(InstructionType aType)
			: Instruction(aType),
			  isPreviousWordOnly(false),
			  isPreviousNotWordOnly(false),
			  isNextWordOnly(false),
			  isNextNotWordOnly(false) { }
			
			bool isPreviousWordOnly;
			bool isPreviousNotWordOnly;
			bool isNextWordOnly;
			bool isNextNotWordOnly;

			virtual bool IsEmptyWidth() const final		{ return true; }
			virtual void Dump(ICharacterWriter& output) const final;
			virtual uint32_t GetByteCodeData() const final;
		};

		struct AssertWordBoundaryInstruction : public AssertWordBoundaryBaseInstruction
		{
			AssertWordBoundaryInstruction() : AssertWordBoundaryBaseInstruction(InstructionType::AssertWordBoundary) { }
			virtual Instruction* Clone() const final;
		};
		
		struct AssertNotWordBoundaryInstruction : public AssertWordBoundaryBaseInstruction
		{
			AssertNotWordBoundaryInstruction() : AssertWordBoundaryBaseInstruction(InstructionType::AssertNotWordBoundary) { }
			virtual Instruction* Clone() const final;
		};
		
		struct AssertStartOfSearchInstruction : public Instruction
		{
			AssertStartOfSearchInstruction() : Instruction(InstructionType::AssertStartOfSearch) { }
			virtual Instruction* Clone() const final;
			virtual bool IsEmptyWidth() const final		{ return true; }
			virtual void Dump(ICharacterWriter& output) const;
		};

		struct AssertRecurseValueInstruction : public Instruction
		{
			AssertRecurseValueInstruction() : Instruction(InstructionType::AssertRecurseValue) { }
			virtual Instruction* Clone() const final;
			virtual bool IsEmptyWidth() const final					{ return true; }
			virtual void Dump(ICharacterWriter& output) const;
			virtual uint32_t GetByteCodeData() 	const 				{ return index; }

			uint8_t recurseValue;
		};

		struct BackReferenceInstruction : public Instruction
		{
			BackReferenceInstruction(uint32_t aIndex) : Instruction(InstructionType::BackReference), index(aIndex) { }
			virtual Instruction* Clone() const final;
			virtual uint32_t GetByteCodeData() const;
			virtual void Dump(ICharacterWriter& output) const;
			
			uint32_t index;
		};
		
		struct ByteInstruction : public Instruction
		{
			ByteInstruction(unsigned char aC) : Instruction(InstructionType::Byte), c(aC) { }
			virtual Instruction* Clone() const;
			virtual uint32_t GetByteCodeData() const;

			virtual bool IsSimpleByteConsumer() const	{ return true; }
			virtual void SetValidBytes(StaticBitTable<256>& mask) const;
			virtual void Dump(ICharacterWriter& output) const;
			
			unsigned char c;
		};
		
		struct ByteEitherOf2Instruction : public Instruction
		{
			ByteEitherOf2Instruction() : Instruction(InstructionType::ByteEitherOf2) { }
			ByteEitherOf2Instruction(unsigned char c0, unsigned char c1) : Instruction(InstructionType::ByteEitherOf2) { c[0] = c0; c[1] = c1; }
			virtual Instruction* Clone() const;
			virtual uint32_t GetByteCodeData() const;

			virtual bool IsSimpleByteConsumer() const	{ return true; }
			virtual void SetValidBytes(StaticBitTable<256>& mask) const;
			virtual void Dump(ICharacterWriter& output) const;
			
			unsigned char c[2];
		};

		struct ByteEitherOf3Instruction : public Instruction
		{
			ByteEitherOf3Instruction() : Instruction(InstructionType::ByteEitherOf3) { }
			virtual Instruction* Clone() const;
			virtual uint32_t GetByteCodeData() const;
			
			virtual bool IsSimpleByteConsumer() const	{ return true; }
			virtual void SetValidBytes(StaticBitTable<256>& mask) const;
			virtual void Dump(ICharacterWriter& output) const;
			
			unsigned char c[3];
		};
		
		struct ByteRangeInstruction : public Instruction
		{
			ByteRangeInstruction() : Instruction(InstructionType::ByteRange) { }
			ByteRangeInstruction(unsigned char min, unsigned char max) : Instruction(InstructionType::ByteRange), byteRange(min, max) { }
			virtual Instruction* Clone() const;
			virtual uint32_t GetByteCodeData() const;

			virtual bool IsSimpleByteConsumer() const	{ return true; }
			virtual void SetValidBytes(StaticBitTable<256>& mask) const;
			virtual void Dump(ICharacterWriter& output) const;

			Interval<unsigned char, true> byteRange;
			
			bool Contains(unsigned char c) const { return byteRange.Contains(c); }
		};

		struct ByteBitMaskInstruction : public Instruction
		{
			ByteBitMaskInstruction() : Instruction(InstructionType::ByteBitMask) { }
			ByteBitMaskInstruction(const StaticBitTable<256> &aBitMask) : Instruction(InstructionType::ByteBitMask), bitMask(aBitMask) { }
			virtual Instruction* Clone() const;
			virtual void BuildByteCode(ByteCodeBuilder& builder) const final;

			virtual bool IsSimpleByteConsumer() const	{ return true; }
			virtual void SetValidBytes(StaticBitTable<256>& mask) const;
			virtual void Dump(ICharacterWriter& output) const;

			StaticBitTable<256>	bitMask;
		};
		
		struct JumpTableInstruction : public Instruction
		{
			JumpTableInstruction(InstructionType type) : Instruction(type) { ClearMemory(targetTable); }
			JumpTableInstruction(const JumpTableInstruction& a);
			virtual void Dump(ICharacterWriter& output) const;
			virtual Instruction* ProcessReachable(int flags);
			virtual void Link() final;
			virtual void Unlink() final;
			virtual const char* GetName() const = 0;
			
			virtual void BuildByteCode(ByteCodeBuilder& builder) const final;
			virtual void SetValidBytes(StaticBitTable<256>& mask) const;
			virtual int				GetNumberOfPlanes() const;
			virtual void			SetValidBytesForPlane(int plane, StaticBitTable<256>& mask) const;
			virtual Instruction*	GetTargetForPlane(int plane) const;
			
			uint8_t				targetTable[256];
			InstructionTable	targetList;
		};
		
		struct ByteJumpTableInstruction : public JumpTableInstruction
		{
			ByteJumpTableInstruction() : JumpTableInstruction(InstructionType::ByteJumpTable) { }
			virtual Instruction* Clone() const;
			virtual const char* GetName() const { return "byte-jump-table"; }
			virtual bool IsByteConsumer() const		{ return true; }
			virtual bool IsSimpleByteConsumer() const	{ return false; }
		};
		
		struct DispatchTableInstruction : public JumpTableInstruction
		{
			DispatchTableInstruction() : JumpTableInstruction(InstructionType::DispatchTable) { }
			virtual Instruction* Clone() const;
			virtual const char* GetName() const { return "dispatch-table"; }
		};
		
		struct ByteNotInstruction : public Instruction
		{
			ByteNotInstruction(unsigned char aC) : Instruction(InstructionType::ByteNot), c(aC) { }
			virtual Instruction* Clone() const;
			virtual uint32_t GetByteCodeData() const;

			virtual bool IsSimpleByteConsumer() const	{ return true; }
			virtual void SetValidBytes(StaticBitTable<256>& mask) const;
			virtual void Dump(ICharacterWriter& output) const;
			
			unsigned char c;
		};
		
		struct ByteNotEitherOf2Instruction : public Instruction
		{
			ByteNotEitherOf2Instruction(unsigned char c0, unsigned char c1) : Instruction(InstructionType::ByteNotEitherOf2) { c[0] = c0; c[1] = c1; }
			virtual Instruction* Clone() const;
			virtual uint32_t GetByteCodeData() const;

			virtual bool IsSimpleByteConsumer() const	{ return true; }
			virtual void SetValidBytes(StaticBitTable<256>& mask) const;
			virtual void Dump(ICharacterWriter& output) const;
			
			unsigned char c[2];
		};
		
		struct ByteNotEitherOf3Instruction : public Instruction
		{
			ByteNotEitherOf3Instruction(unsigned char c0, unsigned char c1, unsigned char c2) : Instruction(InstructionType::ByteNotEitherOf3) { c[0] = c0; c[1] = c1; c[2] = c2; }
			virtual Instruction* Clone() const;
			virtual uint32_t GetByteCodeData() const;

			virtual bool IsSimpleByteConsumer() const	{ return true; }
			virtual void SetValidBytes(StaticBitTable<256>& mask) const;
			virtual void Dump(ICharacterWriter& output) const;
			
			unsigned char c[3];
		};
		
		struct ByteNotRangeInstruction : public Instruction
		{
			ByteNotRangeInstruction() : Instruction(InstructionType::ByteNotRange) { }
			virtual Instruction* Clone() const;
			virtual uint32_t GetByteCodeData() const;

			virtual bool IsSimpleByteConsumer() const	{ return true; }
			virtual void SetValidBytes(StaticBitTable<256>& mask) const;
			virtual void Dump(ICharacterWriter& output) const;
			
			Interval<unsigned char, true> byteRange;
		};

		struct FailInstruction : public Instruction
		{
			FailInstruction() : Instruction(InstructionType::Fail) { }
			virtual Instruction* Clone() const;
			
			// Fail is a byte consumer where nothing matches.
			virtual bool IsSimpleByteConsumer() const					{ return true; }
			virtual int	 GetNumberOfPlanes() const						{ return 0; }
			virtual void SetValidBytes(StaticBitTable<256>& mask) const { }
			virtual void Dump(ICharacterWriter& output) const;
			virtual Instruction* ProcessReachable(int flags)			{ return nullptr; }
			virtual void Link() final									{ }
			virtual void Unlink() final									{ }
		};

		struct ReturnIfRecurseValueInstruction : public Instruction
		{
			ReturnIfRecurseValueInstruction(uint32_t aRecurseValue) : Instruction(InstructionType::ReturnIfRecurseValue), recurseValue(aRecurseValue) { }
			virtual Instruction* Clone() const;
			
			virtual void 	 Dump(ICharacterWriter& output) const;
			virtual uint32_t GetByteCodeData() 	const 					{ return recurseValue; }
			
			uint32_t recurseValue;
		};

		struct SuccessInstruction : public Instruction
		{
			SuccessInstruction() : Instruction(InstructionType::Success) 	{ }
			virtual Instruction* Clone() const;
			
			virtual void Dump(ICharacterWriter& output) const;
			virtual Instruction* ProcessReachable(int flags)				{ return nullptr; }
			virtual void Link() final										{ }
			virtual void Unlink() final										{ }
		};

		struct CallInstruction : public Instruction
		{
			CallInstruction() : Instruction(InstructionType::Call) { }
			virtual Instruction* Clone() const;
			virtual void Dump(ICharacterWriter& output) const;
			virtual void BuildByteCode(ByteCodeBuilder& builder) const;
			virtual Instruction* ProcessReachable(int flags);
			virtual void Link() final;
			virtual void Unlink() final;
		
			Instruction* callTarget;
			Instruction* falseTarget	= nullptr;
			Instruction* trueTarget		= nullptr;
		};

		struct PossessInstruction : public Instruction
		{
			PossessInstruction() : Instruction(InstructionType::Possess) { }
			virtual Instruction* Clone() const;
			virtual void Dump(ICharacterWriter& output) const;
			virtual void BuildByteCode(ByteCodeBuilder& builder) const;
			virtual Instruction* ProcessReachable(int flags);
			virtual void Link() final;
			virtual void Unlink() final;
		
			Instruction* callTarget;
			Instruction* trueTarget;
		};

		struct RecurseInstruction : public Instruction
		{
			RecurseInstruction(int32_t aRecurseValue) : Instruction(InstructionType::Recurse), recurseValue(aRecurseValue) { }
			virtual Instruction* Clone() const;
			virtual void Dump(ICharacterWriter& output) const;
			virtual uint32_t GetByteCodeData() const;
			virtual Instruction* ProcessReachable(int flags);
			virtual void Link() final;
			virtual void Unlink() final;
			
			Instruction*	callTarget;
			int32_t		 	recurseValue;
		};

		struct MatchInstruction : public Instruction
		{
			MatchInstruction() : Instruction(InstructionType::Match) { }
			virtual Instruction* Clone() const;
			virtual void Dump(ICharacterWriter& output) const;
			virtual Instruction* ProcessReachable(int flags)	{ return nullptr; }
			virtual void Link() final							{ }
			virtual void Unlink() final							{ }
		};

		struct ProgressCheckInstruction : public Instruction
		{
			ProgressCheckInstruction(uint32_t aIndex) : Instruction(InstructionType::ProgressCheck), index(aIndex) { }
			virtual Instruction* Clone() const;
			virtual bool IsEmptyWidth() const			{ return true; }
			virtual uint32_t GetByteCodeData() const 	{ return index; }
			virtual void Dump(ICharacterWriter& output) const;
			virtual bool HasStartAnchor() const			{ return GetNext()->HasStartAnchor(); }
			
			uint32_t index;
		};

		struct FindByteInstruction : public Instruction
		{
			FindByteInstruction(unsigned char aC, Instruction* aTarget) : Instruction(InstructionType::FindByte), c(aC), target(aTarget) { }

			virtual Instruction* Clone() const;
			virtual uint32_t GetByteCodeData() const;
			virtual void Dump(ICharacterWriter& output) const;
			virtual void Link() final;
			virtual void Unlink() final;

			unsigned char c;
			Instruction* target;
		};

		struct SearchByteInstruction : public Instruction
		{
			SearchByteInstruction(unsigned char aC, unsigned aOffset);
			SearchByteInstruction(unsigned char numberOfBytes, unsigned char aBytes[8], unsigned aOffset);
			
			// For range instructions
			SearchByteInstruction(InstructionType type, unsigned char aBytes[8], unsigned aOffset);
			
			// For pair instructions
			SearchByteInstruction(InstructionType type, unsigned char aBytes[8], unsigned aOffset, const Table<NibbleMask>& nibbleMaskList);
			
			virtual Instruction* Clone() const;
			virtual void BuildByteCode(ByteCodeBuilder& builder) const;
			virtual void Dump(ICharacterWriter& output) const;

			unsigned char		bytes[8];
			unsigned 			offset;
			Table<NibbleMask>	nibbleMaskList;
		};

		struct SearchDataInstruction : public Instruction
		{
			SearchDataInstruction(InstructionType type, uint32_t length, uint32_t data[256]);
			SearchDataInstruction(InstructionType type, uint32_t length, uint32_t data[256], const Table<NibbleMask>& nibbleMaskList);
			
			virtual Instruction* Clone() const;
			virtual void BuildByteCode(ByteCodeBuilder& builder) const;
			virtual void Dump(ICharacterWriter& output) const;

			uint32_t	length;
			uint32_t 	data[256];
			Table<NibbleMask>	nibbleMaskList;
		};

		struct SaveInstruction : public Instruction
		{
			SaveInstruction(bool aNeverRecurse, uint32_t aSaveIndex, uint32_t aSaveOffset = 0) : Instruction(InstructionType::Save), neverRecurse(aNeverRecurse), saveIndex(aSaveIndex), saveOffset(aSaveOffset) { }
			virtual Instruction* Clone() const;
			virtual bool IsEmptyWidth() const			{ return true; }
			virtual void BuildByteCode(ByteCodeBuilder& builder) const;
			virtual void Dump(ICharacterWriter& output) const;
			virtual bool HasStartAnchor() const			{ return GetNext()->HasStartAnchor(); }
			
			bool		neverRecurse;
			uint32_t	saveIndex;
			uint32_t	saveOffset;
		};

		struct SplitInstruction : public Instruction
		{
			SplitInstruction() : Instruction(InstructionType::Split) { }
			virtual Instruction* Clone() const final;
			virtual void BuildByteCode(ByteCodeBuilder& builder) const final;
			virtual void Dump(ICharacterWriter& output) const;
			virtual Instruction* ProcessReachable(int flags);
			virtual void Link() final;
			virtual void Unlink() final;
			
			InstructionTable targetList;
			
			enum class OptimizationPhase
			{
				None,
				SplitToByteFiltersDone
			};
			OptimizationPhase	optimizationPhase = OptimizationPhase::None;
		};

		struct StepBackInstruction : public Instruction
		{
			StepBackInstruction(uint32_t aStep) : Instruction(InstructionType::StepBack), step(aStep) { }
			virtual Instruction* Clone() const;
			virtual void Dump(ICharacterWriter& output) const;
			
			virtual uint32_t GetByteCodeData() const { return step; }
			uint32_t step;
		};

		struct PropagateBackwardsInstruction : public Instruction
		{
			PropagateBackwardsInstruction(const StaticBitTable<256>& aBitMask) : Instruction(InstructionType::PropagateBackwards), bitMask(aBitMask) { }
			virtual Instruction* Clone() const;
			virtual void Dump(ICharacterWriter& output) const;
			
			virtual void BuildByteCode(ByteCodeBuilder& builder) const final;
			
			StaticBitTable<256> bitMask;
		};

//==========================================================================
	} // namespace PatternInternal
} // namespace Javelin
//==========================================================================
