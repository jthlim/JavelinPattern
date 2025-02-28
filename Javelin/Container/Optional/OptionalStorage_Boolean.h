//============================================================================
//
// Optional<T> is used to represent a value that may be T, or not.
// If it's not T, then it remains unconstructed.
//
// OptionalStorage_Boolean uses a bool hasValue field to determine if the
// value is present
//
//============================================================================

#pragma once
#include "Javelin/System/Assert.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class OptionalStorage_Boolean
	{
	public:
		template<typename T> class Policy
		{
		public:
			struct Type
			{
				Type() : hasValue(false) { }
				template<typename... A> JINLINE Type(A&&... a) : hasValue(true), value((A&&) a...) { }
				~Type() { }
				
				// Put inside a union to prevent T's constructor from being called
				union
				{
					T 			value;
				};
				bool			hasValue;
			};

			JINLINE static void DestroyValue(Type& a)				{ a.value.~T(); }
			
			template<typename... A>
			JINLINE static void Replace(Type &a, A&&... b)			{ a.value.~T(); new(Placement(&a.value)) Type((A&&) b...); }
			
			JINLINE static void CopyFrom(Type& a, Type& b)			{ a.hasValue = b.hasValue; if(b.hasValue) new(Placement(&a.value)) T(b.value); }
			JINLINE static void CopyFrom(Type& a, const Type& b)	{ a.hasValue = b.hasValue; if(b.hasValue) new(Placement(&a.value)) T(b.value); }
			JINLINE static void MoveFrom(Type& a, Type&& b)			{ a.hasValue = b.hasValue; if(b.hasValue) new(Placement(&a.value)) T((T&&) b.value); }
			
			template<typename... A>
			JINLINE static T& CopyFrom(Type& a, A&&... b)			{ return (new(Placement(&a.value)) Type((A&&) b...))->value; }
			
			JINLINE static bool HasValue(const Type& a) 			{ return a.hasValue; }
			JINLINE static void SetEmpty(Type& a) 					{ a.hasValue = false; }
			
			JINLINE static T& GetValue(Type& a) 					{ JASSERT(a.hasValue); return a.value; }
			JINLINE static const T& GetValue(const Type& a)			{ JASSERT(a.hasValue); return a.value; }

		};
	};

//============================================================================
} // namespace Javelin
//============================================================================
