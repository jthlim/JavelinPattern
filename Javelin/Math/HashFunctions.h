//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	JINLINE size_t	GetHash(const void* p)			{ return (size_t) p;			}
	JINLINE size_t	GetHash(char c)					{ return (size_t) c;			}
	JINLINE size_t	GetHash(unsigned char c)		{ return (size_t) c;			}
	JINLINE size_t	GetHash(short s)				{ return (size_t) s;			}
	JINLINE size_t	GetHash(unsigned short s)		{ return (size_t) s;			}
	JINLINE size_t	GetHash(int a) 					{ return (size_t) a;			}
	JINLINE size_t	GetHash(unsigned int a) 		{ return (size_t) a; 			}
	JINLINE size_t	GetHash(long a) 				{ return (size_t) a;			}
	JINLINE size_t	GetHash(unsigned long a) 		{ return (size_t) a;			}
	JINLINE size_t	GetHash(long long a) 			{ return (size_t) (a ^ (a>32)); }
	JINLINE size_t	GetHash(unsigned long long a) 	{ return (size_t) (a ^ (a>32)); }
	JINLINE size_t	GetHash(float f)				{ union { float f; uint32_t u; } u; u.f = f; return (size_t) u.u; }
	JINLINE size_t	GetHash(double d)				{ union { double d; uint64_t u; } u; u.d = d; return GetHash(u.u); }

//============================================================================
	
	template<typename T, typename A> struct CheckEqualHash
	{
		JINLINE static bool IsEqual(size_t aHash, const A& value) { return aHash == GetHash(T(value)); }
	};
	
	template<typename T> struct CheckEqualHash<T, T>
	{
		JINLINE static bool IsEqual(size_t aHash, const T& value) { return true; }
	};

//============================================================================
} // namespace Javelin
//===========================================================================e
