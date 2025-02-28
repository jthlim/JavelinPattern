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
		
		bool		IsValid() const			{ return isDataValid; }
        String		GetDescription() const;

		const char* GetFileName() const;
		int			GetLineNumber() const;
		
	private:
		// This is a IMAGEHLP_LINE64 type, but we don't include that header here.
		char		imageHlpLine64Buffer[40];
		unsigned	displacement;
		bool		isDataValid;
	};
	
//============================================================================

	class StackWalk final
	{
	public:
		StackWalk() { }
		StackWalk(size_t skipDepth) { Capture(skipDepth); }

		void Capture(size_t skipDepth=1);
		
		size_t GetCount() const					{ return addresses.GetCount();	}
		void*  operator[](size_t depth) const	{ return addresses[depth];		}

	private:
		static void InitializeSymbols();

		static const size_t MAXIMUM_STACK_DEPTH	= 8;

		StaticTable<void*, MAXIMUM_STACK_DEPTH>	addresses;

		static bool needToInitializeSymbols;
	};

//============================================================================
}
//============================================================================
