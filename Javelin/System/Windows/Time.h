//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Type/Duration.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class Time
	{
	public:
		enum TimeInitializer { NOW };

		Time() { }
		Time(TimeInitializer) { time = GetTickCount(); }

		bool operator<(const Time& a) const 	{ return time < a.time; 	}
		
		Duration operator-(const Time& a) const { return Duration((time-a.time) * 0.001); }
		double    AsDouble()		const 		{ return time * 1000.0;		}

	private:
		Time(DWORD aTime) : time(aTime) { }

		DWORD	time;
	};

//============================================================================
} // namespace Javelin
//============================================================================
