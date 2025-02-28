//============================================================================
//
// Optional<T> is used to represent a value that may be T, or not.
// If it's not T, then it remains unconstructed.
//
// NullPointer uses a union of the storage type with void* and considers the
// value empty if the pointer is null
//
//============================================================================

#pragma once
#include "Javelin/System/Assert.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class OptionalStorage_NullPointer
	{
	public:
		template<typename T> class Policy
		{
		public:
			struct Type
			{
				Type() : p(nullptr) { }
				template<typename... A> JINLINE Type(A&&... a) : value((A&&) a...) { }			
				~Type()	{ }
				
				// We use a union of the original type so that Optional gains alignment information of T
				union
				{
					T 		value;
					void*	p;
				};
			};
			
			JINLINE static void DestroyValue(Type& a)				{ a.value.~T(); }
			
			template<typename... A>
			JINLINE static void Replace(Type &a, A&&... b)			{ a.value.~T(); new(Placement(&a.value)) Type((A&&) b...); }
			
			JINLINE static void CopyFrom(Type& a, Type& b)			{ if(HasValue(b)) new(Placement(&a.value)) T(b.value); else SetEmpty(a); }
			JINLINE static void CopyFrom(Type& a, const Type& b)	{ if(HasValue(b)) new(Placement(&a.value)) T(b.value); else SetEmpty(a); }
			JINLINE static void MoveFrom(Type& a, Type&& b)			{ if(HasValue(b)) new(Placement(&a.value)) T((T&&) b.value); else SetEmpty(a); }
			
			template<typename... A>
			JINLINE static T& CopyFrom(Type& a, A&&... b)			{ return *new(Placement(&a.value)) T((A&&) b...); }
			
			JINLINE static bool HasValue(const Type& a)				{ return a.p != nullptr; 	}
			JINLINE static void SetEmpty(Type& a) 					{ a.p = nullptr; 			}
			
			JINLINE static T& GetValue(Type& a) 					{ JASSERT(HasValue(a)); return a.value; }
			JINLINE static const T& GetValue(const Type& a)			{ JASSERT(HasValue(a)); return a.value; }
		};
	};

//============================================================================
} // namespace Javelin
//============================================================================
