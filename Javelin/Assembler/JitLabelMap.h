#pragma once
#include <assert.h>
#include <stdint.h>

//============================================================================

namespace Javelin
{
//===========================================================================

	/// Maps uint32_t label -> void*
	class JitLabelMap
	{
	public:
		JitLabelMap() { }
		~JitLabelMap() { delete [] data; }
		
		void Reserve(uint32_t size);
		
		void* Get(uint32_t label) const;
		void* GetIfExists(uint32_t label) const;
		bool Contains(uint32_t label) const		{ return GetIfExists(label) != nullptr; }

		void Set(uint32_t label, void *p);
		
		void Clear();
		
		void StartUseBacking(JitLabelMap &a)
		{
			lengthMask = a.lengthMask;
			data = a.data;
		}
		void StopUseBacking(JitLabelMap &a) {
			assert(lengthMask == a.lengthMask);
			assert(data == a.data);

			data = nullptr;
		}
		
	private:
#pragma pack(push, 4)
		struct Data
		{
			uint32_t label;
			void *p;
		};
#pragma pack(pop)

		static const uint32_t NO_LABEL = (uint32_t) -1;
		
		uint32_t lengthMask = 0;
		Data*	 data = nullptr;
	};

//============================================================================
} // namespace Javelin
//============================================================================
