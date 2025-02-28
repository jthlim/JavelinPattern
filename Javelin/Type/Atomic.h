//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Thread/Atomic.h"
#include "Javelin/Type/TypeData.h"

//============================================================================

namespace Javelin
{
//============================================================================

	template<typename T> class Atomic
	{
	public:
		JINLINE constexpr Atomic() : value() { }
		JINLINE constexpr Atomic(T aValue) : value(aValue) { }

		JINLINE operator volatile T&() 				{ return value;	}
		JINLINE operator const volatile T&() const	{ return value;	}

		JINLINE T operator->() const				{ return value; }
		
		JINLINE Atomic& operator=(const Atomic& a)	{ value = a.value; return *this; }
		template<typename U>
		JINLINE Atomic& operator=(U&& a)			{ value = a; return *this;	}

		JINLINE T operator++(int)					{ return AtomicUpdate::PostIncrement(value);	}
		JINLINE T operator--(int)					{ return AtomicUpdate::PostDecrement(value);	}
		JINLINE T operator++()						{ return AtomicUpdate::PreIncrement(value); 	}
		JINLINE T operator--()						{ return AtomicUpdate::PreDecrement(value);		}
		JINLINE T operator+=(T a)					{ return AtomicUpdate::Add(value, a);			}
		JINLINE T operator-=(T a)					{ return AtomicUpdate::Subtract(value, a);		}

		JINLINE T operator&=(T a)					{ return AtomicUpdate::And(value, a);			}
		JINLINE T operator|=(T a)					{ return AtomicUpdate::Or(value, a);			}
		JINLINE T operator^=(T a)					{ return AtomicUpdate::Xor(value, a);			}

        JINLINE bool Update(T oldValue, T newValue) { return AtomicUpdate::CompareAndSwap(value, oldValue, newValue); }
        
		JINLINE T& GetValue() volatile				{ return const_cast<T&>(value); }
		JINLINE const T& GetValue() const volatile	{ return const_cast<const T&>(value); }
		
	private:
		volatile T	value;
	};
  
//============================================================================
} // namespace Javelin
//============================================================================
