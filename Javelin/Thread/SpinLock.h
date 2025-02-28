//===========================================================================

#pragma once
#include "Javelin/System/Assert.h"
#include "Javelin/Type/Atomic.h"

//===========================================================================

namespace Javelin
{
//===========================================================================
	
	struct SpinLock
	{
	public:
		JINLINE SpinLock() : data(false)	{ }
		JINLINE ~SpinLock() 				{ JASSERT(data == false); }
		
		JINLINE void BeginLock() const		{ while(!data.Update(false, true)) { }		}
		JINLINE bool TryLock() const		{ return data.Update(false, true); 			}
		JINLINE void EndLock() const		{ JASSERT(data == true); data = false; 		}
		
		template<typename T>
		JINLINE void Synchronize(T&& callback) const
		{
			BeginLock();
			callback();
			EndLock();
		}
		
	private:
		mutable Atomic<bool>	data;
	};
	
//===========================================================================
} // namespace Javelin
//===========================================================================
