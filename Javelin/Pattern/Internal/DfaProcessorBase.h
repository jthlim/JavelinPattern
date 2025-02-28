//============================================================================

#pragma once
#include "Javelin/Pattern/Internal/PatternProcessor.h"
#include "Javelin/Container/IntrusiveList.h"
#include "Javelin/Container/Map.h"
#include "Javelin/Thread/RecursiveMutex.h"
#include "Javelin/Thread/Semaphore.h"
#include "Javelin/Thread/SpinLock.h"
#include "Javelin/Type/Atomic.h"

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

	class NfaState;

//============================================================================

	class DfaProcessorBase
	{
	public:
		typedef const unsigned char* (*SearchHandler)(const unsigned char* p, const void* data, const unsigned char* pStop);

		DfaProcessorBase(const void* patternData, uint32_t numberOfInstructions);
		~DfaProcessorBase();

		class NfaStateKey;

	protected:
		enum class SearchHandlerEnum : unsigned char
		{
			Normal,
			SearchByte0,							// This is used if all states point back to this one!
			SearchByte,
			SearchByteEitherOf2,
			SearchByteEitherOf3,
			SearchByteEitherOf4,
			SearchByteEitherOf5,
			SearchByteEitherOf6,
			SearchByteEitherOf7,
			SearchByteEitherOf8,
			SearchBytePair,
			SearchBytePair2,
			SearchBytePair3,
			SearchBytePair4,
			SearchByteTriplet,
			SearchByteTriplet2,
			SearchByteRange,
			SearchByteRangePair,
			SearchBoyerMoore,
			SearchShiftOr,
			
			SearchByte0WithAssert,
			SearchByteWithAssert,
			SearchByteEitherOf2WithAssert,
			SearchByteEitherOf3WithAssert,
			SearchByteEitherOf4WithAssert,
			SearchByteEitherOf5WithAssert,
			SearchByteEitherOf6WithAssert,
			SearchByteEitherOf7WithAssert,
			SearchByteEitherOf8WithAssert,
			SearchBytePairWithAssert,
			SearchBytePair2WithAssert,
			SearchBytePair3WithAssert,
			SearchBytePair4WithAssert,
			SearchByteTripletWithAssert,
			SearchByteTriplet2WithAssert,
			SearchByteRangeWithAssert,
			SearchByteRangePairWithAssert,
			SearchBoyerMooreWithAssert,
			SearchShiftOrWithAssert,
		};
		
		// Reset counter replaces a SpinLock, bool isResetting and uint32_t activeThreadCount
		// The lower 31 bits represent the number of active threads in this pattern
		// The top bit (sign bit) represents if the states are resetting
		mutable Atomic<int32_t>		resetCounter 				= 0;
		mutable Atomic<uint64_t>	bytesProcessedSinceReset	= 0;
		SpinLock					resetLock;									// Used whenever flipping sign bit of resetCounter
		
		volatile bool				hasFailed					= false;		// Set to true when too many resets
		uint8_t						numberOfSelfOriginatedResets = 0;
		
		PatternData					patternData;
		uint32_t					numberOfInstructions;
		
		RecursiveMutex				populateLock;
		Semaphore					waitForResetBeginSemaphore;
		Semaphore					waitForResetEndSemaphore;
		size_t						stateMemoryAllocated = 0;
		
		class State;
		typedef Map<NfaStateKey, State*> NfaToDfaMap;
		mutable NfaToDfaMap			nfaToDfaMap;
		
		void FreeAllStates();
		
		void PopulateState(State* state) const;
		void ResetStates(bool selfOriginated);
		void HandleStateReset(State* state) const;

		virtual void ClearStartingStates() = 0;
		void ClearNfaToDfaMap();
		
		void BeginMatch() const;
		void EndMatch() const;
		void BeginPopulate() const;
		void EndPopulate() const;
		
		State* GetStateForNfaState(NfaState& state) const;
		
		static const unsigned char* NoSearchHandler(const unsigned char* p, const void* data, const unsigned char* pStop);
		static const unsigned char* SearchByte0Handler(const unsigned char* p, const void* data, const unsigned char* pStop);
		
		virtual SearchHandler GetSearchHandler(SearchHandlerEnum value, State* state) const = 0;
		
		virtual void ProcessNfaState(NfaState& result, const NfaState& state, const PatternData& patternData, CharacterRange& relevancyInterval, int flags, void* updateCache) const = 0;
		virtual const uint8_t* GetCharacterFlags() const = 0;
		
		enum class SearchMode
		{
			Full,
			Partial,
		};
		
	private:
		IntrusiveListNode			listNode;									// Used by DfaMemoryManager to track all DfaPatternProcessors
		
		friend class DfaMemoryManager;
	};

//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
