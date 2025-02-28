//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class StandardAllocator
	{
	public:
		JINLINE static void* JCALL operator new(size_t size)	{ return ::operator new(size);		}
		JINLINE static void* JCALL operator new[](size_t size)	{ return ::operator new[](size);	}
		JINLINE static void  JCALL operator delete(void *p)		{ ::operator delete(p);				}
		JINLINE static void  JCALL operator delete[](void *p)	{ ::operator delete[](p);			}

		JDECLARE_ADDITIONAL_ALLOCATORS
	};

//============================================================================
} // namespace Javelin
//============================================================================
