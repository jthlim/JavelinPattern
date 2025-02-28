//============================================================================
//
// Optional<T> is used to represent a value that may be T, or not.
// If it's not T, then it remains unconstructed.
//
// OptionalStorage_NullPointerMember uses a nullptr in a member of the provided
// class to determine what value should be considered empty.
//
// This should only be used when nullptr is NOT a valid member value.
//
//============================================================================

#pragma once
#include "Javelin/System/Assert.h"
#include "Javelin/Template/Utility.h"

//============================================================================

namespace Javelin
{
//============================================================================

	template<typename CLASS, typename R, R CLASS::*M> class OptionalStorage_NullPointerMember
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
				
				JINLINE bool HasValue() const	{ return value.*M != nullptr;	}
				JINLINE void SetEmpty()			{ value.*M = nullptr;			}
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
