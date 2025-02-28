//============================================================================

#pragma once
#include "Javelin/Pattern/Internal/PatternProcessorBase.h"
#include "Javelin/Pattern/Internal/PatternData.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	class DataBlock;
	
//============================================================================
	
	namespace PatternInternal
	{
//============================================================================

		struct ByteCodeSearchData;
		class InstructionListBuilder;
		
		class PatternProcessor : public PatternProcessorBase
		{
		public:
			// When these find a match, they return a pointer to the END of the match.
			virtual const void* FullMatch(const void* data, size_t length) const = 0;
			virtual const void* FullMatch(const void* data, size_t length, const char **captures) const = 0;
			virtual const void* PartialMatch(const void* data, size_t length, size_t offset) const = 0;
			virtual const void* PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const = 0;
			virtual Interval<const void*> LocatePartialMatch(const void* data, size_t length, size_t offset) const = 0;
			
			// PopulateCaptures is used by scan processors that can't process captures.
			// It should use the full match program, starting at offset into data.
			virtual const void* PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const = 0;

			virtual bool ProvidesCaptures() const { return true; }
			virtual bool CanUseFullMatchProgram(size_t inputLength) const { return true; }
			virtual bool CanUsePartialMatchProgram(size_t inputLength) const { return true; }
			
			static PatternProcessor* CreateBackTrackingProcessor(DataBlock&& dataBlock);
			static PatternProcessor* CreateBackTrackingProcessor(const void* data, size_t length, bool makeCopy);
			static PatternProcessor* CreateConsistencyCheckProcessor(DataBlock&& dataBlock);
			static PatternProcessor* CreateConsistencyCheckProcessor(const void* data, size_t length, bool makeCopy);
			static PatternProcessor* CreateNfaProcessor(DataBlock&& dataBlock);
			static PatternProcessor* CreateNfaProcessor(const void* data, size_t length, bool makeCopy);
			static PatternProcessor* CreateNfaOrBitStateProcessor(DataBlock&& dataBlock);
			static PatternProcessor* CreateNfaOrBitStateProcessor(const void* data, size_t length, bool makeCopy);
			static PatternProcessor* CreateOnePassProcessor(DataBlock&& dataBlock);
			static PatternProcessor* CreateOnePassProcessor(const void* data, size_t length, bool makeCopy);
			static PatternProcessor* CreateScanAndCaptureProcessor(DataBlock&& dataBlock);
			static PatternProcessor* CreateScanAndCaptureProcessor(const void* data, size_t length, bool makeCopy);

			// These never make a copy of the data
			static PatternProcessor* CreateBitFieldGlushkovProcessor(const void* data, size_t length);
			static PatternProcessor* CreateBitStateProcessor(const void* data, size_t length);
			static PatternProcessor* CreateDfaProcessor(const void* data, size_t length);
			static PatternProcessor* CreateNfaOrDfaProcessor(const void* data, size_t length);
			static PatternProcessor* CreatePikeNfaProcessor(const void* data, size_t length);
			static PatternProcessor* CreateSimplePikeNfaProcessor(const void* data, size_t length);
			static PatternProcessor* CreateThompsonNfaProcessor(const void* data, size_t length);
			
			static bool CanUseBitFieldGlushkovNfaPatternProcessor(const void* data, size_t length);
			
			static const void* FindByte(const void* p, uint64_t v, const void* pEnd);
			static const void* FindBytePair(const void* p, uint64_t v, const void* pEnd);	// Lowest 16 bits of v contains 2 bytes to search for
			static const void* FindBytePair2(const void* p, uint64_t v, const void* pEnd);
			static const void* FindBytePair3(const void* p, uint64_t v, const void* pEnd);
			static const void* FindBytePair4(const void* p, uint64_t v, const void* pEnd);
			static const void* FindByteEitherOf2(const void* p, uint64_t v, const void* pEnd);	// Lowest 16 bits of v contains 2 bytes to search for
			static const void* FindByteEitherOf3(const void* p, uint64_t v, const void* pEnd);	// Lowest 24 bits of v contains 2 bytes to search for
			static const void* FindByteEitherOf4(const void* p, uint64_t v, const void* pEnd);
			static const void* FindByteEitherOf5(const void* p, uint64_t v, const void* pEnd);
			static const void* FindByteEitherOf6(const void* p, uint64_t v, const void* pEnd);
			static const void* FindByteEitherOf7(const void* p, uint64_t v, const void* pEnd);
			static const void* FindByteEitherOf8(const void* p, uint64_t v, const void* pEnd);
			static const void* FindByteRange(const void* p, uint64_t v, const void* pEnd);
			static const void* FindByteRangePair(const void* p, uint64_t v, const void* pEnd);
			static const void* FindByteTriplet(const void* p, uint64_t v, const void* pEnd);
			static const void* FindByteTriplet2(const void* p, uint64_t v, const void* pEnd);

			static const void* FindBoyerMoore(const void* p, const ByteCodeSearchData* data, const void* pEnd);
			static const void* FindShiftOr(const void* p, const ByteCodeSearchData* data, const void* pEnd);
		};

//============================================================================
	} // namespace PatternInternal
} // namespace Javelin
//============================================================================
