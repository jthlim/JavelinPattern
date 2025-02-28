//============================================================================

#pragma once
#include "Javelin/Pattern/Internal/PatternProcessorBase.h"
#include "Javelin/Pattern/Internal/PatternData.h"

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

		struct ByteCodeSearchData;
		class InstructionListBuilder;
		
		class ReverseProcessor : public PatternProcessorBase
		{
		public:
			// Returns null if captures has been populated
			virtual const void* Match(const void* data,			// Start of the entire data block
									  size_t length,			// data+length represents the end of the search
									  size_t startOffset,		// data+startOffset is the leftmost allowed match
									  const void* matchEnd,		// matchEnd is where a previous forward processor ended.
									  const char **captures,	// Pointer to capture data.
									  bool matchIsAnchored) const = 0;

			virtual bool ProvidesCaptures() const { return false; }

			// These never make a copy of the data
			static ReverseProcessor* CreateAnchoredReverseProcessor();
			static ReverseProcessor* CreateDfaReverseProcessor(const void* data, size_t length);
			static ReverseProcessor* CreateNfaReverseProcessor(const void* data, size_t length);
			static ReverseProcessor* CreateBitFieldGlushkovNfaReverseProcessor(const void* data, size_t length);
			static ReverseProcessor* CreateThompsonNfaReverseProcessor(const void* data, size_t length);
			static ReverseProcessor* CreateNfaOrDfaReverseProcessor(const void* data, size_t length);
			static ReverseProcessor* CreateOnePassReverseProcessor(const void* data, size_t length);
			static ReverseProcessor* CreateBackTrackingReverseProcessor(const void* data, size_t length);
			static ReverseProcessor* CreateFixedLengthReverseProcessor(const void* data, size_t length);

			static bool CanUseBitFieldGlushkovNfaReverseProcessor(const void* data, size_t length);

			static ReverseProcessor* Create(const void* data, size_t length, bool allowCaptures);
			
			static const void* FindByteReverse(const void* p, uint64_t v, const void* pEnd);
		};

//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
