//============================================================================

#pragma once
#include "Javelin/Pattern/Pattern.h"

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

	class NoStackGrowthHandler : public Pattern::StackGrowthHandler
	{
	public:
		constexpr NoStackGrowthHandler() { }
		
		virtual void* Allocate(size_t size) const 	{ return nullptr; }
		virtual void Free(void* p) const			{ }
		
		static const NoStackGrowthHandler instance;
	};
		
//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
