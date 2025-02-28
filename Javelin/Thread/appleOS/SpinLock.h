//===========================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/System/Assert.h"
#include <libkern/OSAtomic.h>

//===========================================================================

namespace Javelin
{
//===========================================================================
	
	struct SpinLock
	{
	public:
		JINLINE SpinLock() : data{OS_SPINLOCK_INIT} { }
		
		JINLINE void BeginLock() const	{ OSSpinLockLock(&data);		}
		JINLINE bool TryLock() const	{ return OSSpinLockTry(&data);	}
		JINLINE void EndLock() const	{ OSSpinLockUnlock(&data); 		}
		
		template<typename T>
		JINLINE void Synchronize(T&& callback) const
		{
			BeginLock();
			callback();
			EndLock();
		}
	
	private:
		mutable volatile OSSpinLock	data;
	};
	
//===========================================================================
} // namespace Javelin
//===========================================================================
