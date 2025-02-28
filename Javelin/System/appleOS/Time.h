//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Type/Duration.h"
#include <mach/mach_time.h>

//============================================================================

namespace Javelin
{
//============================================================================
	
	class Time
	{
	public:
		enum TimeInitializer { NOW };

		Time() { }
		Time(TimeInitializer) { time = mach_absolute_time(); }

		bool operator<(const Time& a) const { return time < a.time; }
		
		Duration<> operator-(const Time& a) const { return Duration<>((time - a.time)*timeBaseConversion); }

		double    AsDouble() const { return time * timeBaseConversion; }
		long long AsCycles() const { return (long long) time; }

		void		Start()			{ time = -mach_absolute_time(); }
		Duration<>	Stop()			{ time += mach_absolute_time(); return Duration<>(time * timeBaseConversion); }
		
	private:
		uint64_t	time;
		
		static double timeBaseConversion;
		static double CalculateTimeBaseConversion();
	};

//============================================================================
} // namespace Javelin
//============================================================================
