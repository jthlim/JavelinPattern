//============================================================================

#pragma once
#include "Javelin/Cryptography/Crc64.h"
#include "Javelin/Type/String.h"

//============================================================================

namespace Javelin
{
//============================================================================

	template<typename T> struct ObjectWithHash
	{
		T			value;
		size_t		hash;
		
		ObjectWithHash() 											{ hash = GetHash(value); }
		explicit ObjectWithHash(const T& aValue) : value(aValue)	{ hash = GetHash(value); }
		explicit ObjectWithHash(const T& aValue, size_t aHash) : value(aValue), hash(aHash) { }

		JINLINE ObjectWithHash& operator=(const T& aValue)			{ value = aValue; hash = GetHash(value); }
		JINLINE ObjectWithHash& operator=(const ObjectWithHash& a)	{ value = a.value; hash = a.hash; }
		
		JINLINE bool operator==(const T& a) const					{ return value == a;    }
		JINLINE bool operator!=(const T& a) const					{ return value != a;    }

		JINLINE bool operator==(const ObjectWithHash& a) const		{ return hash == a.hash && value == a.value; }
		JINLINE bool operator< (const ObjectWithHash& a) const		{ return hash != a.hash ? hash < a.hash : value < a.value; }
		JINLINE bool operator!=(const ObjectWithHash& a) const		{ return !(*this == a);	}
		JINLINE bool operator> (const ObjectWithHash& a) const		{ return  (a < *this);	}
		JINLINE bool operator<=(const ObjectWithHash& a) const		{ return !(a < *this);	}
		JINLINE bool operator>=(const ObjectWithHash& a) const		{ return !(*this < a);	}
		
		friend size_t GetHash(const ObjectWithHash& a) 				{ return a.hash; 		}
		
		operator T&() 												{ return value; 		}
		operator const T&() const									{ return value;			}
	};
	
//============================================================================

	// This definition of JSH has the following properties:
	//	No memory allocations required
	//  Release builds collapse the entire function, with no global guards for static variable accesses
	//  The StringData is correctly defined in read-write memory so that reference count increments/decrements are valid
	//  The string contents is defined right next to the structure so that cache misses are minimized.
	
	namespace Private
	{
		template<int N> struct RawStringWithHashInline
		{
			int*		stringData;
			size_t		hash;
			mutable int	counter;
			const char*	data;
			size_t		count;
//			size_t		capacity;			// Don't bother storing capacity. It'll never be inspected for compile-time strings
			char		storage[N];
		};
	}
	
	#define JSH(x)	([]() -> const Javelin::ObjectWithHash<Javelin::String>&						\
	{ 																								\
		static constexpr Javelin::Private::RawStringWithHashInline<sizeof(x)> rawString 			\
		{ &rawString.counter, (size_t) JHASH_CRC64(x), 1, &rawString.storage[0], sizeof(x)-1, x };	\
		return *reinterpret_cast<const Javelin::ObjectWithHash<Javelin::String>*>(&rawString); 		\
	})()

//============================================================================
} // namespace Javelin
//============================================================================
