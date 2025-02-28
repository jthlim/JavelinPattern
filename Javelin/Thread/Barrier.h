//============================================================================
//
// A barrier is a platform-efficient way to block a thread until
// signalled from another thread
//
//============================================================================

#pragma once
#include "Javelin/Thread/Mutex.h"

//===========================================================================

namespace Javelin
{
//===========================================================================
	
	// This is used to create a single thread barrier.
	class Barrier : public Mutex
	{
	public:
		JINLINE Barrier()				{ BeginLock();	}
		JINLINE ~Barrier()				{ EndLock(); 	}
		
		void Wait()						{ BeginLock();	}
		void Signal()					{ EndLock();	}
	};
	
//===========================================================================
} // namespace Javelin
//===========================================================================
