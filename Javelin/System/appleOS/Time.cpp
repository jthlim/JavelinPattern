//============================================================================

#include "Time.h"

//============================================================================

using namespace Javelin;

//============================================================================

double Time::timeBaseConversion = Time::CalculateTimeBaseConversion();

//============================================================================

double Time::CalculateTimeBaseConversion()
{
	struct mach_timebase_info tb;
	mach_timebase_info(&tb);
	return 1e-9 * tb.numer / tb.denom;
}

//============================================================================
