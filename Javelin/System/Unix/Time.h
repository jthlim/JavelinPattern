//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Type/Duration.h"
#include <sys/time.h>

//============================================================================

namespace Javelin
{
//============================================================================

	class Time : private timeval
	{
	public:
		enum TimeInitializer { NOW };

		Time() { }
		Time(TimeInitializer) { gettimeofday(this, nullptr); }

		bool operator<(const Time& a) const { return tv_sec < a.tv_sec || (tv_sec == a.tv_sec && tv_usec < a.tv_usec); }
		
		Duration<> operator-(const Time& a) const { return Duration<>(AsDouble()-a.AsDouble()); }

		double    AsDouble()	   const { return tv_sec + tv_usec * 1e-6; }
	};

//============================================================================
} // namespace Javelin
//============================================================================
