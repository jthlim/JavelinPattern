//============================================================================

#pragma once
#include <stdint.h>

//============================================================================

namespace Javelin::Assembler
{
//============================================================================

    class BitUtility
    {
    public:
        static bool IsValidSignedImmediate(int64_t value, int width);
        static bool IsValidUnsignedImmediate(uint64_t value, int width);
    };

//============================================================================
}
//============================================================================

