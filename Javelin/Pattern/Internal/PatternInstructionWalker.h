//==========================================================================

#pragma once
#include "Javelin/Container/Map.h"
#include "Javelin/Container/Table.h"
#include "Javelin/Container/Tuple.h"
#include "Javelin/Pattern/Internal/PatternInstruction.h"

//==========================================================================

namespace Javelin::PatternInternal
{
//==========================================================================
	
	class NibbleMask;
	
//==========================================================================
	
	class InstructionWalker
	{
	public:
		struct Presence
		{
			uint32_t				mask;
			uint32_t				masks[256];
			StaticBitTable<256>		bitField;
			
			Presence() { mask = 0; ClearMemory(masks); }
			void Update(const StaticBitTable<256>& presenceBits, uint32_t pathMask);
		};
		
		InstructionWalker();
		~InstructionWalker();
		
		bool ShouldContinue() const;
		void AdvanceAllInstructions();
		void AddInitialInstruction(const Instruction* instruction);
		
		void Dump(ICharacterWriter& output) const;
		
		size_t GetPresenceListCount() const							{ return presenceList.GetCount(); }
		const StaticBitTable<256>& GetPresenceBits(size_t i) const	{ return presenceList[i].bitField; }
		const Presence& GetPresence(size_t i) const					{ return presenceList[i]; }

		void UpdatePresenceMasks(size_t i);

		Table<NibbleMask> BuildNibbleMaskList(size_t i, size_t length, bool allowSinglePath);
		
	private:
		Map<const Instruction*, uint32_t> activeInstructions;
		
		Table<Presence>	presenceList;

		bool childMasksUpdated = false;
		
		uint32_t pathIndex;
		uint32_t parentMasks[32];			// mapping of where branches came from
		uint32_t childMasks[32];

		uint32_t GetNewPathMask(uint32_t pathMask);

		bool HasMatch() const;
		void UpdateChildMasks();
		
		uint32_t GetFullMask(uint32_t mask, uint32_t stateMask) const;
		void ProcessInstruction(OpenHashSet<const Instruction*> &progressCheckSet, Map<const Instruction*, uint32_t>& nextState, Presence& presence, const Instruction* instruction, uint32_t pathMask);
		
		Table<uint32_t> GetPathMaskList(size_t index, size_t length) const;
		void BuildNibbleMaskListFromPathMaskList(Table<NibbleMask>& nibbleMaskList, const Table<uint32_t>& pathMaskList, size_t index, size_t length) const;
		
		static bool AddToNibbleMaskPathList(Table<NibbleMask>& result, const NibbleMask* newNibbleMaskPath);
		static bool AddToNibbleMaskPathList(Table<NibbleMask>& result, const NibbleMask* newNibbleMaskPath, int* newNibbleBitIndexList, int index);
		static bool AddSingleNibbleMaskListToResult(Table<NibbleMask>& result, const NibbleMask* singleNibbleMaskList);
		static uint32_t CalculateMergeCost(const NibbleMask* first, int firstBit, const NibbleMask* second, int secondBit, int length);
	};
	
//==========================================================================
} // namespace Javelin::PatternInternal
//==========================================================================
