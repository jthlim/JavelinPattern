//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Container/Table.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	class String;

	class StackWalkInformation final
	{
	public:
		StackWalkInformation(void* address);
		~StackWalkInformation();
		
		bool		IsValid() const			{ return functionName != nullptr;	}
        String		GetDescription() const;

		const char* GetFunctionName() const { return functionName;	}
		size_t		GetOffset() const		{ return offset;		}

	private:
		void DemangleFunctionNameToBuffer(const char* name);
		
		size_t	offset;
		char*	functionName	= nullptr;
	};
	
//============================================================================

	static const size_t MAXIMUM_STACK_DEPTH	= 8;

	class StackWalk final : private StaticTable<void*, MAXIMUM_STACK_DEPTH>
	{
	private:
		typedef StaticTable<void*, MAXIMUM_STACK_DEPTH> Inherited;
		
	public:
		StackWalk() { }
		StackWalk(size_t skipDepth) { Capture(skipDepth); }

		void Capture(size_t skipDepth=1);

		using Inherited::Begin;
		using Inherited::End;
		using Inherited::GetCount;
		using Inherited::GetDomain;
		using Inherited::operator[];
	};

//============================================================================
}
//============================================================================
