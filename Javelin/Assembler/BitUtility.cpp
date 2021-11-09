//============================================================================

#include "BitUtility.h"

//============================================================================

using namespace Javelin::Assembler;

//============================================================================

bool BitUtility::IsValidSignedImmediate(int64_t value, int width)
{
    int64_t shiftedValue = value >> (width - 1);
    return shiftedValue == 0 || shiftedValue == -1;
}

bool BitUtility::IsValidUnsignedImmediate(uint64_t value, int width)
{
    return (value >> width) == 0;
}

//============================================================================
