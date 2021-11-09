//============================================================================

#pragma once
#include <stdint.h>

//============================================================================

namespace Javelin::riscvAssembler {

//============================================================================

    enum class RelEncoding : uint8_t
    {
        BDelta,               // e.g. beq
        JDelta,               // e.g. jal
        Auipc,                // For auipc
        Imm12,                // For add following auipc.
        CBDelta,              // e.g. c.beq
        CJDelta,              // e.g. c.jal
        
        Rel64,                // For .quad labels
    };

//============================================================================

} // Javelin::riscVAssembler

// //============================================================================

