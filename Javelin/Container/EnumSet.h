//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	template<typename ENUM_CLASS, typename STORAGE>
	class EnumSet
	{
	public:
		constexpr EnumSet(STORAGE aValue) : value(aValue) { }
		template<typename... E> constexpr EnumSet(E... e) : EnumSet(MakeValue(0, e...)) { }
		
		JINLINE bool Contains(ENUM_CLASS e) const { return size_t(e) < 8*sizeof(STORAGE) && (value & (STORAGE(1) << (int) e)) != 0; }
		
	private:
		STORAGE	value;
		
		static constexpr STORAGE MakeValue(STORAGE result) { return result; }
		template<typename... E> static constexpr STORAGE MakeValue(STORAGE result, ENUM_CLASS e0, E... en) { return MakeValue(result | (STORAGE(1) << (int) e0), en...); }
	};

//============================================================================
} // namespace Javelin
//============================================================================
