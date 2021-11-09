//============================================================================

#pragma once
#include <stdint.h>

//============================================================================

namespace Javelin::Assembler
{
//============================================================================

    class JumpType
    {
    public:
        enum Value : uint8_t
        {
            Name,
            Backward,
            Forward,
            BackwardOrForward,
        };
        
        JumpType() = default;
        constexpr JumpType(Value aValue) : value(aValue) { }
        
        const char* ToString() const        { return JUMP_TYPES[(int) value]; }
        
        // Allow switch and comparisons.
        constexpr operator Value() const    { return value; }
        
        // Prevent usage: if(jumpType)
        explicit operator bool() = delete;
        
    private:
        Value value;

        static const char* JUMP_TYPES[];
    };

//============================================================================
} // namespace Javelin::Assembler
//============================================================================
