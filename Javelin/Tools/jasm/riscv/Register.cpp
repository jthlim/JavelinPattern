//============================================================================

#include "Javelin/Tools/jasm/riscv/Register.h"

//============================================================================

namespace Javelin::Assembler::riscv::Register
{
//============================================================================

	constexpr RegisterOperand X0(0, MatchRegX|MatchRegZero);
	constexpr RegisterOperand X1(1, MatchRegX|MatchRegReturnAddress);
	constexpr RegisterOperand X2(2, MatchRegX|MatchRegStackPointer);
	constexpr RegisterOperand X3(3, MatchRegX);
	constexpr RegisterOperand X4(4, MatchRegX);
	constexpr RegisterOperand X5(5, MatchRegX);
	constexpr RegisterOperand X6(6, MatchRegX);
	constexpr RegisterOperand X7(7, MatchRegX);
	constexpr RegisterOperand X8(8, MatchRegX|MatchRegCX);
	constexpr RegisterOperand X9(9, MatchRegX|MatchRegCX);
	constexpr RegisterOperand X10(10, MatchRegX|MatchRegCX);
	constexpr RegisterOperand X11(11, MatchRegX|MatchRegCX);
	constexpr RegisterOperand X12(12, MatchRegX|MatchRegCX);
	constexpr RegisterOperand X13(13, MatchRegX|MatchRegCX);
	constexpr RegisterOperand X14(14, MatchRegX|MatchRegCX);
	constexpr RegisterOperand X15(15, MatchRegX|MatchRegCX);
	constexpr RegisterOperand X16(16, MatchRegX);
	constexpr RegisterOperand X17(17, MatchRegX);
	constexpr RegisterOperand X18(18, MatchRegX);
	constexpr RegisterOperand X19(19, MatchRegX);
	constexpr RegisterOperand X20(20, MatchRegX);
	constexpr RegisterOperand X21(21, MatchRegX);
	constexpr RegisterOperand X22(22, MatchRegX);
	constexpr RegisterOperand X23(23, MatchRegX);
	constexpr RegisterOperand X24(24, MatchRegX);
	constexpr RegisterOperand X25(25, MatchRegX);
	constexpr RegisterOperand X26(26, MatchRegX);
	constexpr RegisterOperand X27(27, MatchRegX);
	constexpr RegisterOperand X28(28, MatchRegX);
	constexpr RegisterOperand X29(29, MatchRegX);
	constexpr RegisterOperand X30(30, MatchRegX);
	constexpr RegisterOperand X31(31, MatchRegX);

    constexpr RegisterOperand F0(0, MatchRegF);
    constexpr RegisterOperand F1(1, MatchRegF);
    constexpr RegisterOperand F2(2, MatchRegF);
    constexpr RegisterOperand F3(3, MatchRegF);
    constexpr RegisterOperand F4(4, MatchRegF);
    constexpr RegisterOperand F5(5, MatchRegF);
    constexpr RegisterOperand F6(6, MatchRegF);
    constexpr RegisterOperand F7(7, MatchRegF);
    constexpr RegisterOperand F8(8, MatchRegF|MatchRegCF);
    constexpr RegisterOperand F9(9, MatchRegF|MatchRegCF);
    constexpr RegisterOperand F10(10, MatchRegF|MatchRegCF);
    constexpr RegisterOperand F11(11, MatchRegF|MatchRegCF);
    constexpr RegisterOperand F12(12, MatchRegF|MatchRegCF);
    constexpr RegisterOperand F13(13, MatchRegF|MatchRegCF);
    constexpr RegisterOperand F14(14, MatchRegF|MatchRegCF);
    constexpr RegisterOperand F15(15, MatchRegF|MatchRegCF);
    constexpr RegisterOperand F16(16, MatchRegF);
    constexpr RegisterOperand F17(17, MatchRegF);
    constexpr RegisterOperand F18(18, MatchRegF);
    constexpr RegisterOperand F19(19, MatchRegF);
    constexpr RegisterOperand F20(20, MatchRegF);
    constexpr RegisterOperand F21(21, MatchRegF);
    constexpr RegisterOperand F22(22, MatchRegF);
    constexpr RegisterOperand F23(23, MatchRegF);
    constexpr RegisterOperand F24(24, MatchRegF);
    constexpr RegisterOperand F25(25, MatchRegF);
    constexpr RegisterOperand F26(26, MatchRegF);
    constexpr RegisterOperand F27(27, MatchRegF);
    constexpr RegisterOperand F28(28, MatchRegF);
    constexpr RegisterOperand F29(29, MatchRegF);
    constexpr RegisterOperand F30(30, MatchRegF);
    constexpr RegisterOperand F31(31, MatchRegF);

//============================================================================
} // namespace Javelin::Assembler::riscv::Register
//============================================================================

namespace Javelin::Assembler::riscv
{
//============================================================================

	const RegisterMap RegisterMap::instance;
	const ParameterizedRegisterMap ParameterizedRegisterMap::instance;

//============================================================================

	RegisterMap::RegisterMap()
	{
		using namespace Register;
		
		(*this)["x0"] = &X0;
		(*this)["x1"] = &X1;
		(*this)["x2"] = &X2;
		(*this)["x3"] = &X3;
		(*this)["x4"] = &X4;
		(*this)["x5"] = &X5;
		(*this)["x6"] = &X6;
		(*this)["x7"] = &X7;
		(*this)["x8"] = &X8;
		(*this)["x9"] = &X9;
		(*this)["x10"] = &X10;
		(*this)["x11"] = &X11;
		(*this)["x12"] = &X12;
		(*this)["x13"] = &X13;
		(*this)["x14"] = &X14;
		(*this)["x15"] = &X15;
		(*this)["x16"] = &X16;
		(*this)["x17"] = &X17;
		(*this)["x18"] = &X18;
		(*this)["x19"] = &X19;
		(*this)["x20"] = &X20;
		(*this)["x21"] = &X21;
		(*this)["x22"] = &X22;
		(*this)["x23"] = &X23;
		(*this)["x24"] = &X24;
		(*this)["x25"] = &X25;
		(*this)["x26"] = &X26;
		(*this)["x27"] = &X27;
		(*this)["x28"] = &X28;
		(*this)["x29"] = &X29;
        (*this)["x30"] = &X30;
		(*this)["x31"] = &X31;
        
        (*this)["zero"] = &X0;
        (*this)["ra"] = &X1;
        (*this)["sp"] = &X2;
        (*this)["gp"] = &X3;
        (*this)["tp"] = &X4;
        (*this)["t0"] = &X5;
        (*this)["t1"] = &X6;
        (*this)["t2"] = &X7;
        (*this)["s0"] = &X8;
        (*this)["fp"] = &X8;
        (*this)["s1"] = &X9;
        (*this)["a0"] = &X10;
        (*this)["a1"] = &X11;
        (*this)["a2"] = &X12;
        (*this)["a3"] = &X13;
        (*this)["a4"] = &X14;
        (*this)["a5"] = &X15;
        (*this)["a6"] = &X16;
        (*this)["a7"] = &X17;
        (*this)["s2"] = &X18;
        (*this)["s3"] = &X19;
        (*this)["s4"] = &X20;
        (*this)["s5"] = &X21;
        (*this)["s6"] = &X22;
        (*this)["s7"] = &X23;
        (*this)["s8"] = &X24;
        (*this)["s9"] = &X25;
        (*this)["s10"] = &X26;
        (*this)["s11"] = &X27;
        (*this)["t3"] = &X28;
        (*this)["t4"] = &X29;
        (*this)["t5"] = &X30;
        (*this)["t6"] = &X31;
        (*this)["ip0"] = &X16;
        (*this)["ip1"] = &X17;

        (*this)["f0"] = &F0;
        (*this)["f1"] = &F1;
        (*this)["f2"] = &F2;
        (*this)["f3"] = &F3;
        (*this)["f4"] = &F4;
        (*this)["f5"] = &F5;
        (*this)["f6"] = &F6;
        (*this)["f7"] = &F7;
        (*this)["f8"] = &F8;
        (*this)["f9"] = &F9;
        (*this)["f10"] = &F10;
        (*this)["f11"] = &F11;
        (*this)["f12"] = &F12;
        (*this)["f13"] = &F13;
        (*this)["f14"] = &F14;
        (*this)["f15"] = &F15;
        (*this)["f16"] = &F16;
        (*this)["f17"] = &F17;
        (*this)["f18"] = &F18;
        (*this)["f19"] = &F19;
        (*this)["f20"] = &F20;
        (*this)["f21"] = &F21;
        (*this)["f22"] = &F22;
        (*this)["f23"] = &F23;
        (*this)["f24"] = &F24;
        (*this)["f25"] = &F25;
        (*this)["f26"] = &F26;
        (*this)["f27"] = &F27;
        (*this)["f28"] = &F28;
        (*this)["f29"] = &F29;
        (*this)["f30"] = &F30;
        (*this)["f31"] = &F31;

        (*this)["ft0"] = &F0;
        (*this)["ft1"] = &F1;
        (*this)["ft2"] = &F2;
        (*this)["ft3"] = &F3;
        (*this)["ft4"] = &F4;
        (*this)["ft5"] = &F5;
        (*this)["ft6"] = &F6;
        (*this)["ft7"] = &F7;
        (*this)["fs0"] = &F8;
        (*this)["fs1"] = &F9;
        (*this)["fa0"] = &F10;
        (*this)["fa1"] = &F11;
        (*this)["fa2"] = &F12;
        (*this)["fa3"] = &F13;
        (*this)["fa4"] = &F14;
        (*this)["fa5"] = &F15;
        (*this)["fa6"] = &F16;
        (*this)["fa7"] = &F17;
        (*this)["fs2"] = &F18;
        (*this)["fs3"] = &F19;
        (*this)["fs4"] = &F20;
        (*this)["fs5"] = &F21;
        (*this)["fs6"] = &F22;
        (*this)["fs7"] = &F23;
        (*this)["fs8"] = &F24;
        (*this)["fs9"] = &F25;
        (*this)["fs10"] = &F26;
        (*this)["fs11"] = &F27;
        (*this)["ft8"] = &F28;
        (*this)["ft9"] = &F29;
        (*this)["ft10"] = &F30;
        (*this)["ft11"] = &F31;
    }

	ParameterizedRegisterMap::ParameterizedRegisterMap()
	{
        (*this)["regx"] = { 0, MatchRegX };
		(*this)["regf"] = { 0, MatchRegF };
	}
	
//============================================================================
} // Javelin::Assembler::riscv
//============================================================================
