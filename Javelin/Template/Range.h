//============================================================================
//
// This is a convenience class that allow code such as:
//
//   for(auto a : Range<int>(5))
//   {
//      // a is 0..4 inclusive
//   }
//
// When a max is specified, it is exclusive
//
//   for(auto a : Range<long>(3,5))
//   {
//      // a is 3, 4
//   }
//
//
//============================================================================

#pragma once
#include "Javelin/Type/Interval.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	template<typename T> class RangeIterator
	{
	public:
		JINLINE RangeIterator(T aN) : n(aN) { }
		
		JINLINE T operator*() 		{ return n; }
		JINLINE void operator++() 	{ ++n;		}
		
		JINLINE bool operator!=(const RangeIterator &a) const { return n != a.n; }
		
	private:
		T n;
	};

//============================================================================

	template<typename T> JINLINE const Interval<RangeIterator<T>> Range(T count)		{ return Interval<RangeIterator<T>>(0, count); 		}
	template<typename T> JINLINE const Interval<RangeIterator<T>> Range(T min, T max)	{ return Interval<RangeIterator<T>>(min, max); 		}
	
//============================================================================
}
//============================================================================
