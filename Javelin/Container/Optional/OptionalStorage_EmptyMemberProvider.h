//============================================================================
//
// Optional<T> is used to represent a value that may be T, or not.
// If it's not T, then it remains unconstructed.
//
// OptionalStorage_EmptyMemberProvider uses a provided class to determine what
// value should be considered empty
//
//============================================================================

#pragma once
#include "Javelin/System/Assert.h"
#include "Javelin/Template/Utility.h"

//============================================================================

namespace Javelin
{
//============================================================================

	// The EmptyProvider/EmptyValue use a magic value to represent an empty store.
	// EMPTY_VALUE_CLASS::GetEmptyValue() is a static method that provides this magic value.
	template<typename CLASS, typename R, R CLASS::*M, typename EMPTY_VALUE_CLASS> class OptionalStorage_EmptyMemberProvider
	{
	public:
		template<typename T> class Policy
		{
		public:
			struct Type
			{
				Type() { SetEmpty(); }																									
				template<typename... A>	JINLINE Type(A&&... a) : value((A&&) a...) { }
				~Type()	{ }
				
				// Union will prevent the constructor of T being called.
				union
				{
					T	value;
				};
				
				JINLINE bool HasValue() const	{ return value.*M != EMPTY_VALUE_CLASS::GetEmptyValue();	}
				JINLINE void SetEmpty()			{ value.*M = EMPTY_VALUE_CLASS::GetEmptyValue();			}
			};
			
			JINLINE static void DestroyValue(Type& a)				{ a.value.~T(); }
			
			template<typename... A>
			JINLINE static void Replace(Type &a, A&&... b)			{ a.value.~T(); new(Placement(&a.value)) Type((A&&) b...); }
			
			JINLINE static void CopyFrom(Type& a, Type& b)			{ a.value = b.value; }
			JINLINE static void CopyFrom(Type& a, const Type& b)	{ a.value = b.value; }
			JINLINE static void MoveFrom(Type& a, Type&& b)			{ a.value = (T&&) b.value; }
			
			template<typename... A>
			JINLINE static T& CopyFrom(Type& a, A&&... b)			{ return *new(Placement(&a.value)) T((A&&) b...); }
			
			JINLINE static bool HasValue(const Type& a)				{ return a.HasValue(); 	}
			JINLINE static void SetEmpty(Type& a) 					{ a.SetEmpty(); 		}
			
			JINLINE static T& GetValue(Type& a) 					{ JASSERT(HasValue(a)); return a.value; }
			JINLINE static const T& GetValue(const Type& a)			{ JASSERT(HasValue(a)); return a.value; }
		};
	};
	
//============================================================================
} // namespace Javelin
//============================================================================
