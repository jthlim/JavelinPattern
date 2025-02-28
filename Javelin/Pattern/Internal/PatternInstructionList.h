//==========================================================================

#pragma once
#include "Javelin/Pattern/Internal/PatternInstruction.h"
#include "Javelin/Pattern/Internal/PatternInstructionWalker.h"
#include "Javelin/Container/BitTable.h"
#include "Javelin/Container/EnumSet.h"
#include "Javelin/Container/Table.h"
#include "Javelin/Stream/DataBlockWriter.h"

//==========================================================================

#define DUMP_OPTIMIZATION_STATISTICS 	0

//==========================================================================

namespace Javelin
{
//==========================================================================
	
	class ICharacterWriter;
	
	namespace PatternInternal
	{
//==========================================================================

		class Compiler;
		enum class PatternProcessorType : uint16_t;
		struct IComponent;

//==========================================================================
		
		enum class AnyByteResult
		{
			False,
			Minimal,
			Maximal
		};
		
		class InstructionList
		{
		public:
			enum ScanDirection
			{
				Forwards,
				Reverse,
			};
			
			~InstructionList();
			
			void Build(uint32_t options, ScanDirection scanDirection, IComponent* headComponent, Compiler& compiler, bool needsProgressChecks, PatternProcessorType processorType);
			
			void AddInstruction(Instruction* instruction);
			void Patch(Instruction* instruction);
			Instruction* Back() 								{ return &instructionList.Back(); }
			void AddPatchReference(Instruction** patch) 		{ patchList.Append(patch); }
			void RemovePatchReference(Instruction** patch) 		{ patchList.Remove(patch); }
			static void RemoveInstruction(Instruction* instruction);
			static void ReplaceInstruction(Instruction* old, Instruction* newInstruction);
			static void InsertAfterInstruction(Instruction* instruction, Instruction* newInstruction);
			static void InsertBeforeInstruction(Instruction* instruction, Instruction* newInstruction);
			
			void Dump(ICharacterWriter& output);
			
			void Optimize();
			
			bool ShouldEmitSaveInstructions() const;
			void StopSaveInstructions()							{ ++saveCounter;				}
			void AllowSaveInstructions()						{ --saveCounter; 				}
			bool SaveNeverRecurses() const						{ return recurseCounter == 0;	}
			void IncrementSaveRecurse()							{ ++recurseCounter;				}
			void DecrementSaveRecurse()							{ --recurseCounter;				}

			uint32_t GetNextProgessCheckSlot() 					{ return progressCheckCounter++;	}
			
			bool ShouldEmitAssertInstructions() const			{ return allowAssertCounter == 0;	}
			void IncrementAssertCounter()						{ ++allowAssertCounter;				}
			void DecrementAssertCounter()						{ --allowAssertCounter; 			}
			uint32_t GetAssertCounter() const					{ return allowAssertCounter;		}
			void SetAssertCounter(uint32_t a) 					{ allowAssertCounter = a;			}

			bool IsForwards() const								{ return scanDirection == Forwards; }
			bool IsReverse() const								{ return scanDirection == Reverse; 	}
			bool RequiresAnyByteMinimalForPartialMatch(IComponent* headComponent) const;
			
			bool NeedsProgressChecks() const;

			void WriteByteCode(DataBlockWriter& writer, const String& pattern, uint32_t numberOfCaptures);
			void AppendByteCode(DataBlockWriter& writer);
			
			void SetHasResetCapture() 							{ hasResetCapture = true; }
			
			struct StateMap;
			
		private:
			typedef IntrusiveList<Instruction, &Instruction::listNode> LinkedInstructionList;
			LinkedInstructionList	instructionList;

			Table<Instruction**>	patchList;

			ScanDirection			scanDirection;
			bool					hasBackTrackingComponents;
			bool					hasStartAnchor;
			bool					hasEndAnchor;
			bool					isFixedLength;
			bool 					matchRequiresEndOfInput = false;
			bool					hasResetCapture = false;
			PatternProcessorType	processorType;
			uint32_t				options;
			uint32_t				minimumLength;
			uint32_t				maximumLength;
			uint32_t				totalNumberOfInstructions;
			uint32_t				allowAssertCounter = 0;
			uint32_t				saveCounter = 0;
			uint32_t				recurseCounter = 0;
			uint32_t				progressCheckCounter = 0;
#if DUMP_OPTIMIZATION_STATISTICS
			uint32_t				optimizationStepCounter = 0;
#endif
			Instruction* fullMatchInstruction;
			Instruction* partialMatchInstruction;
			
			struct SearchOptimizer
			{
				bool						isMaximal;
				Instruction*				original;
				InstructionWalker			instructionWalker;
				IntrusiveListNode			listNode;
				
				bool						canSkipBackwardsPropagate;
				StaticBitTable<256>			backwardsPropagate;
				
				void ReplaceOriginal(Instruction* searcher, bool replaceByteJumpTableWithAdvanceByte, bool hasZeroOffsetCheck, Instruction*& partialMatchInstruction);
			};
			
			typedef IntrusiveList<SearchOptimizer, &SearchOptimizer::listNode> SearchOptimizerList;
			SearchOptimizerList	searchOptimizerList;
			
			static const int AVOID_STATES_REQUIRE_CONSECUTIVE_GROWTH_COUNT = 3;
			typedef OpenHashSet<InstructionTable> StateSet;
			
			void Optimize_RemoveAssertEndOfInput();
			void Optimize_CaptureSearch();
			void Optimize_AccelerateSearch();
			void Optimize_SplitToSplit();
			void Optimize_LeftFactor();
			void Optimize_RightFactor();
			void Optimize_CollapseSplit();
			void Optimize_CollapseSplit(StateMap& stateMap, StateSet* growthStateList, SplitInstruction* split);
			void Optimize_RemoveZeroReference();
			void Optimize_RemoveJumpToNext();
			void Optimize_SplitToDispatch();
			void Optimize_DelaySave()											{ Optimize_DelaySave(true); }
			void Optimize_DelaySave(bool allowSplittingMultipleReferences);
			void Optimize_DelaySave(SaveInstruction* saveInstruction, bool allowSplittingMultipleReferences);
			void Optimize_SimplifyWordBoundaryAsserts();
			void Optimize_SplitToByteConsumers();
			void Optimize_SplitToMatchReorder();
			Instruction* Optimize_SimplifyByteJumpTable(ByteJumpTableInstruction* byteJumpTableInstruction);
			void Optimize_DispatchTargetsToAdvanceByte();
			void Optimize_ByteJumpTableToFindByte();
			void Optimize_ForwardJumpTargets();
			void Optimize_SimpleByteConsumersToAdvanceByte();
			void Optimize_RemoveLeadingAsserts();
			
			void RunOptimizeStep(void (InstructionList::*)(), const char* name);
			
			void CleanReverseProgram();
			
			void IndexInstructions();
			void UpdateReferencesForInstructions();
			bool VerifyReferencesForInstructions(const char* optimizationPass);
			void DumpReferenceMismatch(const ReferenceTable& before, size_t instructionIndex, const char* optimizationPass);
			
			void InsertPartialMatchAnyByteMinimal();
			
			void Optimize_SplitToSplit(SplitInstruction* split);
			void Optimize_SplitToByteConsumers(LinkedInstructionList::Iterator& insertAfter, SplitInstruction* split, StateMap& stateMap);
			void Optimize_SplitToByteFilters(LinkedInstructionList::Iterator& insertAfter, SplitInstruction* split);
			
			void AddToSearchOptimizer(Instruction* split, Instruction* afterAnyByte, AnyByteResult mode, bool canSkipBackwardsPropagate, const StaticBitTable<256>& backwardsPropagate);
			
			Instruction* GetByteFilterStartingFrom(Instruction* instruction) const;
			Instruction* GetAfterByteFilterStartingFrom(Instruction* instruction) const;
			Instruction* GetMatchStartingFrom(Instruction* instruction) const;
			Instruction* ReplaceWithFail(Instruction* old);
			void PropagateFail(Instruction* instruction, Instruction* fail);
			
			void RemoveAllInstructionsOfType(const EnumSet<InstructionType, uint64_t>& typeSet);
		
			void ConvertToFail(Instruction* &startingInstruction);
			
			static uint32_t CalculateSearchEffectivenessValue(const uint32_t* data, size_t offset);
		};

//==========================================================================
	} // namespace PatternInternal
} // namespace Javelin
//==========================================================================
