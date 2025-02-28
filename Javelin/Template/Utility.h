//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

namespace Javelin
{
//============================================================================

	template<typename T> JINLINE T& Minimum(T& a, T& b)									{ return (a < b) ?  a : b;	}
	template<typename T> JINLINE const T& Minimum(const T& a, const T& b)				{ return (a < b) ?  a : b;	}
    template<typename T> JINLINE void SetMinimum(T &a, const T &b)    					{ if(b < a) a = b;          }
	
	template<typename T> JINLINE T& Maximum(T& a, T& b)									{ return (a < b) ?  b : a;	}
	template<typename T> JINLINE const T& Maximum(const T& a, const T& b)				{ return (a < b) ?  b : a;	}
    template<typename T> JINLINE void SetMaximum(T &a, const T &b)    					{ if(b > a) a = b;          }
	
	template<typename T> JINLINE T Clamp(const T& a, const T& min, const T& max)		{ return Minimum(Maximum(a, min), max); }
	
	template<typename T> JINLINE T Lerp(const T& a, const T& b, float t)				{ return (1.0f-t) * a + t * b; }
	
	template<typename T> JINLINE T* OffsetPointerByByteCount(T* base, size_t offset)	{ return reinterpret_cast<T*>(reinterpret_cast<size_t>(base)+offset); }
	template<typename T> JINLINE T* OffsetPointerByByteCount(T* base, int offset)		{ return reinterpret_cast<T*>(reinterpret_cast<size_t>(base)+offset); }
	
	template<typename T> JINLINE T AlignUp(T value, size_t alignment)					{ return (T) ((size_t(value) + (alignment-1)) & ~(alignment-1)); }
    template<typename T> JINLINE T* AlignUp(T* value)                                   { return AlignUp(value, alignof(T)); }
    template<typename T> JINLINE T AlignDown(T value, size_t alignment)                 { return (T) (size_t(value) & ~(alignment-1)); }
    template<typename T> JINLINE T* AlignDown(T* value)                                 { return AlignDown(value, alignof(T)); }
    
	template<typename T> JINLINE T&& Move(T& a)											{ return (T&&) a; }

	template<typename T> T MakeValue();
	
	// Handles 3+ parameters
	// We no longer return a reference, since some results (eg. Minimum(Point2<T>) are created
	template<typename T, typename... U> JINLINE T Minimum(const T& a, const T& b, const T& c, const U&... u)	{ return Minimum(a, Minimum(b, c, u...)); }
	template<typename T, typename... U> JINLINE T Maximum(const T& a, const T& b, const T& c, const U&... u)	{ return Maximum(a, Maximum(b, c, u...)); }

	template<typename T, size_t N> constexpr size_t JNUMBER_OF_ELEMENTS(const T (&a)[N]) { return N; }
	
//============================================================================
}
//============================================================================

