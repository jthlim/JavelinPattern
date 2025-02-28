//============================================================================

#include "Javelin/Pattern/Internal/PatternProcessor.h"
#if defined(JASM_GNUC_ARM_NEON) || defined(JASM_GNUC_ARM64)
#include <arm_neon.h>

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================
		
	namespace ArmNeonFindMethods
	{
		const void* InternalFindBytePairPath2(const void* pIn, uint32_t v, const void* pEnd);

		const void* InternalFindNibbleMask(const void* pIn, const uint8x16_t* nibbleMasks, const void* pEnd);
		const void* InternalFindPairNibbleMask(const void* pIn, const uint8x16_t* nibbleMasks, const void* pEnd);
		const void* InternalFindPairNibbleMaskPath(const void* pIn, const uint8x16_t* nibbleMasks, const void* pEnd);
		const void* InternalFindTripletNibbleMask(const void* pIn, const uint8x16_t* nibbleMasks, const void* pEnd);
		const void* InternalFindTripletNibbleMaskPath(const void* pIn, const uint8x16_t* nibbleMasks, const void* pEnd);
	}
	
//============================================================================
}

//============================================================================
#endif
//============================================================================
