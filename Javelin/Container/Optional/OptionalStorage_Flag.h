//============================================================================
//
// Optional<T> is used to represent a value that may be T, or not.
// If it's not T, then it remains unconstructed.
//
// OptionalStorage_Flag uses an external bitfield to determine if the value is
// present or not
//
//============================================================================

#pragma once
#include "Javelin/System/Assert.h"

//============================================================================

namespace Javelin
{
//============================================================================

	template<typename U, ssize_t FLAG_OFFSET, int FLAG_VALUE> class OptionalStorage_Flag
	{
	public:
		template<typename T> class Policy
		{
		public:
			struct Type
			{
				Type() { *GetFlagPointer(*this) &= ~FLAG_VALUE; }
				template<typename... A> JINLINE Type(A&&... a) : value((A&&) a...) { *GetFlagPointer(*this) |= FLAG_VALUE; }
				~Type() { }
				
				union
				{
					T 	value;
				};
			};
			
			JINLINE static void DestroyValue(Type& a)				{ a.value.~T(); }
			
			template<typename... A>
			JINLINE static void Replace(Type &a, A&&... b)			{ a.value.~T(); new(Placement(&a.value)) Type((A&&) b...); *GetFlagPointer(a) |= FLAG_VALUE; }
			
//			JINLINE static void CopyFrom(Type& a, Type& b)			{ a.hasValue = b.hasValue; if(b.hasValue) new(Placement(&a.value)) T(b.value); }
//			JINLINE static void CopyFrom(Type& a, const Type& b)	{ a.hasValue = b.hasValue; if(b.hasValue) new(Placement(&a.value)) T(b.value); }
//			JINLINE static void MoveFrom(Type& a, Type&& b)			{ a.hasValue = b.hasValue; if(b.hasValue) new(Placement(&a.value)) T((T&&) b.value); }
			
			template<typename... A>
			JINLINE static T& CopyFrom(Type& a, A&&... b)			{ return (new(Placement(&a.value)) Type((A&&) b...))->value; }
			
			JINLINE static bool HasValue(const Type& a) 			{ return (*GetFlagPointer(a) & FLAG_VALUE) != 0; }
			JINLINE static void SetEmpty(Type& a) 					{ return &GetFlagPointer(a) &= ~FLAG_VALUE; }
			
			JINLINE static T& GetValue(Type& a) 					{ JASSERT(HasValue(a)); return a.value; }
			JINLINE static const T& GetValue(const Type& a)			{ JASSERT(HasValue(a)); return a.value; }
			
		private:
			JINLINE static U* GetFlagPointer(Type& a)			 	{ return reinterpret_cast<U*>(reinterpret_cast<unsigned char*>(&a) + FLAG_OFFSET); 				}
			JINLINE static const U* GetFlagPointer(const Type& a) 	{ return reinterpret_cast<const U*>(reinterpret_cast<const unsigned char*>(&a) + FLAG_OFFSET);	}
		};
	};
	
//============================================================================
} // namespace Javelin
//============================================================================
