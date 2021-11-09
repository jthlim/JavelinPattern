//============================================================================

#include "Javelin/Tools/jasm/riscv/RoundingMode.h"

//============================================================================

namespace Javelin::Assembler::riscv::RoundingMode
{
//============================================================================

	constexpr ImmediateOperand RNE(0, MatchRoundingMode);
    constexpr ImmediateOperand RTZ(1, MatchRoundingMode);
    constexpr ImmediateOperand RDN(2, MatchRoundingMode);
    constexpr ImmediateOperand RUP(3, MatchRoundingMode);
    constexpr ImmediateOperand RMM(4, MatchRoundingMode);
    constexpr ImmediateOperand DYN(7, MatchRoundingMode);

//============================================================================
} // namespace Javelin::Assembler::riscv::Register
//============================================================================

namespace Javelin::Assembler::riscv
{
//============================================================================

const RoundingModeMap RoundingModeMap::instance;

//============================================================================

RoundingModeMap::RoundingModeMap()
{
    using namespace RoundingMode;
    
    (*this)["rne"] = &RNE;
    (*this)["rtz"] = &RTZ;
    (*this)["rdn"] = &RDN;
    (*this)["rup"] = &RUP;
    (*this)["rmm"] = &RMM;
    (*this)["dyn"] = &DYN;
}

//============================================================================
} // Javelin::Assembler::riscv
//============================================================================
