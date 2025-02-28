//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include <unistd.h>

//============================================================================

namespace Javelin
{
//============================================================================

	class Machine
	{
	public:
		static size_t GetPageSize();
		static size_t GetNumberOfProcessors();

#if defined(JASM_GNUC_X86_64)
		static bool SupportsAvx();
		static bool SupportsAvx2();
        static bool SupportsAvx512BW();
#elif defined(JASM_GNUC_ARM)
		static bool SupportsNeon();
#elif defined(JASM_GNUC_ARM64)
        static bool SupportsNeon() { return true; }
#endif
	};
	
//============================================================================
} // namespace Javelin
//============================================================================
