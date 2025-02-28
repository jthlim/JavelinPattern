//============================================================================
//
// Optional<T> is used to represent a value that may be T, or not.
// If it's not T, then it remains unconstructed.
//
// InstrusiveLinked storage is used to keep track of the order in which items
// are created in a linked hash set
//
//============================================================================

#pragma once
#include "Javelin/System/Assert.h"
#include "Javelin/Container/IntrusiveList.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class OptionalStorage_IntrusiveLinked
	{
	public:
		template<typename T> class Policy
		{
		public:
			struct Type
			{
				Type() { }
				template<typename... A> JINLINE Type(A&&... a) : value((A&&) a...) { }			
				~Type()	{ }

				IntrusiveListNode	list;
				
				// Put inside a union to prevent T's constructor from being called
				union
				{
					T 	value;
				};
			};
			
			JINLINE static void DestroyValue(Type& a)				{ JASSERT(HasValue(a)); a.value.~T(); a.list.Remove(); }
			
			template<typename... A>
			JINLINE static void Replace(Type &a, A&&... b)			{ a.value.~T(); new(Placement(&a.value)) Type((A&&) b...); }
			
//			JINLINE static void CopyFrom(Type& a, Type& b)			{ if(HasValue(b)) new(Placement(&a.value)) T(b.value); else SetEmpty(a); }
//			JINLINE static void CopyFrom(Type& a, const Type& b)	{ if(HasValue(b)) new(Placement(&a.value)) T(b.value); else SetEmpty(a); }
			JINLINE static void MoveFrom(Type& a, Type&& b)			{
																		JASSERT(!HasValue(a) && HasValue(b));																		
																		a.list.AdoptLinks(b.list);
																		a.value = (T&&) b.value;
																	}
			
			template<typename... A>
			JINLINE static T& CopyFrom(Type& a, A&&... b)			{ return *new(Placement(&a.value)) T((A&&) b...); }
			
			
			JINLINE static bool HasValue(const Type& a)				{ return !a.list.IsIsolated(); 	}
			JINLINE static void SetEmpty(Type& a) 					{ a.list.Remove(); 				}
			
			JINLINE static T& GetValue(Type& a) 					{ JASSERT(HasValue(a)); return a.value; }
			JINLINE static const T& GetValue(const Type& a)			{ JASSERT(HasValue(a)); return a.value; }
		};
	};

//============================================================================
} // namespace Javelin
//============================================================================
