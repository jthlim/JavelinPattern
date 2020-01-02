#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

//============================================================================

namespace Javelin
{
//===========================================================================

	/// Maps uint32_t label -> array of uint32_t offset
	class JitLabelOffsetQueue
	{
	public:
		JitLabelOffsetQueue() { }
		~JitLabelOffsetQueue() { free(hashData); }
		
		void Reserve(uint32_t numberOfLabels);
		size_t GetMemoryRequirements(uint32_t numberOfLabels);
		
		// Returns offset
		void	 QueueLabelOffset(uint32_t label, uint32_t offset);
		uint32_t GetFirstLabelOffset(uint32_t label) const;
		uint32_t GetLastLabelOffset(uint32_t label) const;
		void     DequeueLabel(uint32_t label);
		
		// returns NO_LABEL if it doesn't exist
		uint32_t GetLastLabelOffsetIfExists(uint32_t label) const;
		
		void StartUseBacking(JitLabelOffsetQueue &a)
		{
			lengthMask = a.lengthMask;
			hashData = a.hashData;
			
			numberOfEntries = a.numberOfEntries;
			entries = a.entries;
		}
		void StopUseBacking(JitLabelOffsetQueue &a) {
			assert(lengthMask == a.lengthMask);
			assert(hashData == a.hashData);			
			assert(entries == a.entries);

			a.numberOfEntries = numberOfEntries;

			hashData = nullptr;
		}

		static const uint32_t NO_LABEL = (uint32_t) -1;

	private:
		struct HashData
		{
			uint32_t label;
			uint32_t headIndex;
			uint32_t tailIndex;
		};
		struct EntryData
		{
			uint32_t offset;
			uint32_t nextIndex;
		};
		
		uint32_t 	lengthMask = 0;
		HashData*	hashData = nullptr;
		
		uint32_t	numberOfEntries = 0;
		EntryData	*entries = nullptr;
	};

//============================================================================
} // namespace Javelin
//============================================================================
