//============================================================================
//
// Optional<T> is used to represent a value that may be T, or not.
// If it's not T, then it remains unconstructed.
//
// OptionalStorage_EmptyValue uses a provided value as an empty magic value
//
//============================================================================

#pragma once
#include "Javelin/Container/Optional/OptionalStorage_EmptyProvider.h"

//============================================================================

namespace Javelin
{
//============================================================================

	namespace Private
	{
		template<typename T, T EMPTY_VALUE> struct EmptyValueHelper
		{
			JINLINE static T GetEmptyValue() { return EMPTY_VALUE; }
		};
	}

//============================================================================
	
	template<typename U, U EMPTY_VALUE>
		class OptionalStorage_EmptyValue : public OptionalStorage_EmptyProvider< Private::EmptyValueHelper<U, EMPTY_VALUE> >
	{
	};
	
//============================================================================
} // namespace Javelin
//============================================================================
