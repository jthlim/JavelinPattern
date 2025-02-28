//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

#if defined(JPLATFORM_APPLEOS)
	#include <libkern/OSAtomic.h>
#endif

//============================================================================

namespace Javelin
{
//============================================================================

    namespace Private
    {
        template<size_t n> class AtomicHelper;
    }

#if defined(JCOMPILER_MSVC)

	namespace Private
	{
        template<> class AtomicHelper<4>
		{
		public:
			typedef long Type;

			JINLINE static int PreIncrement(volatile Type* value)	 		{ return InterlockedIncrement(value);			}
			JINLINE static int PreDecrement(volatile Type* value)	 		{ return InterlockedDecrement(value); 			}
			JINLINE static int Add(volatile Type* value, Type a)			{ return InterlockedExchangeAdd(value, a)+a;	}
			JINLINE static int Subtract(volatile Type* value, Type a)		{ return InterlockedExchangeAdd(value, -a)-a;	}
			JINLINE static int PostAdd(volatile Type* value, Type a)		{ return InterlockedExchangeAdd(value, a);		}
			JINLINE static int PostSubtract(volatile Type* value, Type a)	{ return InterlockedExchangeAdd(value, -a); 	}
			JINLINE static int PostIncrement(volatile Type* value)			{ return PostAdd(value, 1);						}
			JINLINE static int PostDecrement(volatile Type* value)			{ return PostSubtract(value, 1);				}
		};
	}

#elif defined(JCOMPILER_GCC) || defined(JCOMPILER_CLANG)
    
    namespace Private
    {
        template<size_t n, typename TYPE> class AtomicHelperBase
        {
        public:
			typedef TYPE Type;
			
            template<typename T> JINLINE static T PreIncrement(volatile T* value)		{ return __sync_add_and_fetch((volatile Type*) value, 1); }
            template<typename T> JINLINE static T PreDecrement(volatile T* value)		{ return __sync_sub_and_fetch((volatile Type*) value, 1); }
            template<typename T> JINLINE static T Add(volatile T* value, T a)			{ return __sync_add_and_fetch((volatile Type*) value, a); }
            template<typename T> JINLINE static T Subtract(volatile T* value, T a)		{ return __sync_sub_and_fetch((volatile Type*) value, a); }
            template<typename T> JINLINE static T And(volatile T* value, T a)			{ return __sync_and_and_fetch((volatile Type*) value, a); }
            template<typename T> JINLINE static T Or(volatile T* value, T a)			{ return __sync_or_and_fetch((volatile Type*) value, a);  }
            template<typename T> JINLINE static T Xor(volatile T* value, T a)			{ return __sync_xor_and_fetch((volatile Type*) value, a); }
            template<typename T> JINLINE static T PostIncrement(volatile T* value)		{ return __sync_fetch_and_add((volatile Type*) value, 1); }
            template<typename T> JINLINE static T PostDecrement(volatile T* value)		{ return __sync_fetch_and_sub((volatile Type*) value, 1); }
            template<typename T> JINLINE static T PostAdd(volatile T* value, T a)		{ return __sync_fetch_and_add((volatile Type*) value, a); }
            template<typename T> JINLINE static T PostSubtract(volatile T* value, T a)	{ return __sync_fetch_and_sub((volatile Type*) value, a); }
			
			template<typename T> JINLINE static bool CompareAndSwap(volatile T* value, T oldValue, T newValue) { return __sync_bool_compare_and_swap((volatile Type*) value, (Type) oldValue, (Type) newValue); }
        };

		template<> class AtomicHelper<1> : public AtomicHelperBase<1, unsigned char>	{ };
		template<> class AtomicHelper<4> : public AtomicHelperBase<4, int> 				{ };
		template<> class AtomicHelper<8> : public AtomicHelperBase<8, long long>		{ };
	}
    
#else
	#warning "Atomic isn't threadsafe!"
	
	namespace Private
	{
		template<size_t n, typename TYPE> class AtomicHelperBase
		{
		public:
			typedef TYPE Type;
			
			template<typename T> JINLINE static T PreIncrement(volatile T* value)		{ return ++*value;		}
			template<typename T> JINLINE static T PreDecrement(volatile T* value)		{ return --*value;		}
			template<typename T> JINLINE static T Add(volatile T* value, T a)			{ return *value += a;	}
			template<typename T> JINLINE static T Subtract(volatile T* value, T a)		{ return *value -= a;	}
			template<typename T> JINLINE static T And(volatile T* value, T a)			{ return *value &= a;						}
			template<typename T> JINLINE static T Or(volatile T* value, T a)			{ return *value |= a;						}
			template<typename T> JINLINE static T Xor(volatile T* value, T a)			{ return *value ^= a;						}
			template<typename T> JINLINE static T PostIncrement(volatile T* value)		{ return PostAdd(value, 1);					}
			template<typename T> JINLINE static T PostDecrement(volatile T* value)		{ return PostSubtract(value, 1);			}
			template<typename T> JINLINE static T PostAdd(volatile T* value, T a)		{ T v = *value; *value += a; return v; 		}
			template<typename T> JINLINE static T PostSubtract(volatile T* value, T a)	{ T v = *value; *value -= a; return v; 		}
			
			template<typename T> JINLINE static bool CompareAndSwap(volatile T* value, T oldValue, T newValue)
																						{
																							if(*(volatile Type*) value != (Type) oldValue) return false;
																							*(volatile Type*) value = (Type) newValue;
																							return true;
																						}
		};
		
		template<> class AtomicHelper<1> : public AtomicHelperBase<1, unsigned char>	{ };
		template<> class AtomicHelper<4> : public AtomicHelperBase<4, int> 				{ };
		template<> class AtomicHelper<8> : public AtomicHelperBase<8, long long>		{ };
	}
#endif
    
//============================================================================
	
	namespace AtomicUpdate
	{
		template<typename T> JINLINE static T PreIncrement(volatile T& value)		{ return (T) Private::AtomicHelper<sizeof(T)>::PreIncrement( (typename Private::AtomicHelper<sizeof(T)>::Type*) &value); }
		template<typename T> JINLINE static T PreDecrement(volatile T& value)		{ return (T) Private::AtomicHelper<sizeof(T)>::PreDecrement( (typename Private::AtomicHelper<sizeof(T)>::Type*) &value); }
		template<typename T> JINLINE static T Add(volatile T& value, T a)			{ return (T) Private::AtomicHelper<sizeof(T)>::Add( (typename Private::AtomicHelper<sizeof(T)>::Type*) &value, (typename Private::AtomicHelper<sizeof(T)>::Type) a); }
		template<typename T> JINLINE static T Subtract(volatile T& value, T a)		{ return (T) Private::AtomicHelper<sizeof(T)>::Subtract( (typename Private::AtomicHelper<sizeof(T)>::Type*) &value, (typename Private::AtomicHelper<sizeof(T)>::Type) a); }
		template<typename T> JINLINE static T And(volatile T& value, T a)			{ return (T) Private::AtomicHelper<sizeof(T)>::And( (typename Private::AtomicHelper<sizeof(T)>::Type*) &value, (typename Private::AtomicHelper<sizeof(T)>::Type) a); }
		template<typename T> JINLINE static T Or(volatile T& value, T a)			{ return (T) Private::AtomicHelper<sizeof(T)>::Or( (typename Private::AtomicHelper<sizeof(T)>::Type*) &value, (typename Private::AtomicHelper<sizeof(T)>::Type) a); }
		template<typename T> JINLINE static T Xor(volatile T& value, T a)			{ return (T) Private::AtomicHelper<sizeof(T)>::Xor( (typename Private::AtomicHelper<sizeof(T)>::Type*) &value, (typename Private::AtomicHelper<sizeof(T)>::Type) a); }
		template<typename T> JINLINE static T PostIncrement(volatile T& value)		{ return (T) Private::AtomicHelper<sizeof(T)>::PostIncrement( (typename Private::AtomicHelper<sizeof(T)>::Type*) &value); }
		template<typename T> JINLINE static T PostDecrement(volatile T& value)		{ return (T) Private::AtomicHelper<sizeof(T)>::PostDecrement( (typename Private::AtomicHelper<sizeof(T)>::Type*) &value); }
		template<typename T> JINLINE static T PostAdd(volatile T& value, T a)		{ return (T) Private::AtomicHelper<sizeof(T)>::PostAdd( (typename Private::AtomicHelper<sizeof(T)>::Type*) &value, (typename Private::AtomicHelper<sizeof(T)>::Type) a); }
		template<typename T> JINLINE static T PostSubtract(volatile T& value, T a)	{ return (T) Private::AtomicHelper<sizeof(T)>::PostSubtract( (typename Private::AtomicHelper<sizeof(T)>::Type*) &value, (typename Private::AtomicHelper<sizeof(T)>::Type) a); }
		
		// Returns true if the write is successful.
		template<typename T> JINLINE static bool CompareAndSwap(volatile T& value, T oldValue, T newValue) { return Private::AtomicHelper<sizeof(T)>::CompareAndSwap(&value, oldValue, newValue); }
	}
	
//============================================================================
} // namespace Javelin
//============================================================================
