#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

//============================================================================

namespace Javelin
{
//===========================================================================

	struct JitForwardReferenceData
	{
		uint32_t label;
		uint32_t data;
		uint8_t* p;
		JitForwardReferenceData *next;
	};

	struct JitForwardReferenceMapLookupResult
	{
		JitForwardReferenceData* reference;
		uint32_t keyIndex;
	};
	
	/// Maps to an array of indices into pool.
	class JitForwardReferenceMap
	{
	public:
		JitForwardReferenceMap() { }
		~JitForwardReferenceMap() { free(pool); }
		
		void Reserve(uint32_t size);
		
		// This version is provided because it has lower register pressure,
		// and the arm64 result no longer needs to save/restore registers.
		JitForwardReferenceData* Add(uint32_t label);
		
		void Add(uint32_t label, uint8_t *p, uint32_t data);
		void Remove(JitForwardReferenceMapLookupResult result, JitForwardReferenceData* tail);
		JitForwardReferenceMapLookupResult Find(uint32_t label);

		bool HasData() const;
		
		void StartUseBacking(JitForwardReferenceMap &a)
		{
			lengthMask = a.lengthMask;
			poolIndices = a.poolIndices;
			poolFreeIndex = a.poolFreeIndex;
			pool = a.pool;
			headFree = a.headFree;
		}
		void StopUseBacking(JitForwardReferenceMap &a) {
			assert(lengthMask == a.lengthMask);
			assert(poolIndices == a.poolIndices);
			assert(pool == a.pool);

			a.poolFreeIndex = poolFreeIndex;
			a.headFree = headFree;
			
			pool = nullptr;
		}

		static const uint32_t NO_LABEL = (uint32_t) -1;

	private:
		uint32_t lengthMask = 0;
		uint32_t *poolIndices = nullptr;
		
		// Everything after this index is free.
		uint32_t					poolFreeIndex = 0;
		JitForwardReferenceData*	pool = nullptr;
		JitForwardReferenceData*	headFree = nullptr;
	};

//============================================================================
} // namespace Javelin
//============================================================================
