//============================================================================

#include "Javelin/Tools/jasm/x64/Register.h"

//============================================================================

namespace Javelin::Assembler::x64::Register
{
//============================================================================

	constexpr RegisterOperand AL(0, MatchAL | MatchReg8 | MatchRM8);
	constexpr RegisterOperand AH(4, MatchReg8Hi | MatchReg8 | MatchRM8);
	constexpr RegisterOperand AX(0, MatchAX | MatchReg16 | MatchRM16);
	constexpr RegisterOperand EAX(0, MatchEAX | MatchReg32 | MatchRM32);
	constexpr RegisterOperand RAX(0, MatchRAX | MatchReg64 | MatchRM64);

	constexpr RegisterOperand BL(3, MatchReg8 | MatchRM8);
	constexpr RegisterOperand BH(7, MatchReg8Hi | MatchReg8 | MatchRM8);
	constexpr RegisterOperand BX(3, MatchReg16 | MatchRM16);
	constexpr RegisterOperand EBX(3, MatchReg32 | MatchRM32);
	constexpr RegisterOperand RBX(3, MatchReg64 | MatchRM64);

	constexpr RegisterOperand CL(1, MatchCL | MatchReg8 | MatchRM8);
	constexpr RegisterOperand CH(5, MatchReg8Hi | MatchReg8 | MatchRM8);
	constexpr RegisterOperand CX(1, MatchReg16 | MatchRM16);
	constexpr RegisterOperand ECX(1, MatchReg32 | MatchRM32);
	constexpr RegisterOperand RCX(1, MatchReg64 | MatchRM64);

	constexpr RegisterOperand DL(2, MatchReg8 | MatchRM8);
	constexpr RegisterOperand DH(6, MatchReg8Hi | MatchReg8 | MatchRM8);
	constexpr RegisterOperand DX(2, MatchReg16 | MatchRM16);
	constexpr RegisterOperand EDX(2, MatchReg32 | MatchRM32);
	constexpr RegisterOperand RDX(2, MatchReg64 | MatchRM64);
	
	constexpr RegisterOperand SIL(6, MatchReg8 | MatchRM8);
	constexpr RegisterOperand SI(6, MatchReg16 | MatchRM16);
	constexpr RegisterOperand ESI(6, MatchReg32 | MatchRM32);
	constexpr RegisterOperand RSI(6, MatchReg64 | MatchRM64);
	
	constexpr RegisterOperand DIL(7, MatchReg8 | MatchRM8);
	constexpr RegisterOperand DI(7, MatchReg16 | MatchRM16);
	constexpr RegisterOperand EDI(7, MatchReg32 | MatchRM32);
	constexpr RegisterOperand RDI(7, MatchReg64 | MatchRM64);
	
	constexpr RegisterOperand BPL(5, MatchReg8 | MatchRM8);
	constexpr RegisterOperand BP(5, MatchReg16 | MatchRM16);
	constexpr RegisterOperand EBP(5, MatchReg32 | MatchRM32);
	constexpr RegisterOperand RBP(5, MatchReg64 | MatchRM64);
	
	constexpr RegisterOperand SPL(4, MatchReg8 | MatchRM8);
	constexpr RegisterOperand SP(4, MatchReg16 | MatchRM16);
	constexpr RegisterOperand ESP(4, MatchReg32 | MatchRM32);
	constexpr RegisterOperand RSP(4, MatchReg64 | MatchRM64);
	
	constexpr RegisterOperand R8B(8, MatchReg8 | MatchRM8);
	constexpr RegisterOperand R8W(8, MatchReg16 | MatchRM16);
	constexpr RegisterOperand R8D(8, MatchReg32 | MatchRM32);
	constexpr RegisterOperand R8(8, MatchReg64 | MatchRM64);

	constexpr RegisterOperand R9B(9, MatchReg8 | MatchRM8);
	constexpr RegisterOperand R9W(9, MatchReg16 | MatchRM16);
	constexpr RegisterOperand R9D(9, MatchReg32 | MatchRM32);
	constexpr RegisterOperand R9(9, MatchReg64 | MatchRM64);
	
	constexpr RegisterOperand R10B(10, MatchReg8 | MatchRM8);
	constexpr RegisterOperand R10W(10, MatchReg16 | MatchRM16);
	constexpr RegisterOperand R10D(10, MatchReg32 | MatchRM32);
	constexpr RegisterOperand R10(10, MatchReg64 | MatchRM64);
	
	constexpr RegisterOperand R11B(11, MatchReg8 | MatchRM8);
	constexpr RegisterOperand R11W(11, MatchReg16 | MatchRM16);
	constexpr RegisterOperand R11D(11, MatchReg32 | MatchRM32);
	constexpr RegisterOperand R11(11, MatchReg64 | MatchRM64);
	
	constexpr RegisterOperand R12B(12, MatchReg8 | MatchRM8);
	constexpr RegisterOperand R12W(12, MatchReg16 | MatchRM16);
	constexpr RegisterOperand R12D(12, MatchReg32 | MatchRM32);
	constexpr RegisterOperand R12(12, MatchReg64 | MatchRM64);
	
	constexpr RegisterOperand R13B(13, MatchReg8 | MatchRM8);
	constexpr RegisterOperand R13W(13, MatchReg16 | MatchRM16);
	constexpr RegisterOperand R13D(13, MatchReg32 | MatchRM32);
	constexpr RegisterOperand R13(13, MatchReg64 | MatchRM64);
	
	constexpr RegisterOperand R14B(14, MatchReg8 | MatchRM8);
	constexpr RegisterOperand R14W(14, MatchReg16 | MatchRM16);
	constexpr RegisterOperand R14D(14, MatchReg32 | MatchRM32);
	constexpr RegisterOperand R14(14, MatchReg64 | MatchRM64);
	
	constexpr RegisterOperand R15B(15, MatchReg8 | MatchRM8);
	constexpr RegisterOperand R15W(15, MatchReg16 | MatchRM16);
	constexpr RegisterOperand R15D(15, MatchReg32 | MatchRM32);
	constexpr RegisterOperand R15(15, MatchReg64 | MatchRM64);

	constexpr RegisterOperand RIP(5, MatchRIP);

	constexpr RegisterOperand ST0(0, MatchST | MatchST0);
	constexpr RegisterOperand ST1(1, MatchST);
	constexpr RegisterOperand ST2(2, MatchST);
	constexpr RegisterOperand ST3(3, MatchST);
	constexpr RegisterOperand ST4(4, MatchST);
	constexpr RegisterOperand ST5(5, MatchST);
	constexpr RegisterOperand ST6(6, MatchST);
	constexpr RegisterOperand ST7(7, MatchST);

	constexpr RegisterOperand MM0(0, MatchMm);
	constexpr RegisterOperand MM1(1, MatchMm);
	constexpr RegisterOperand MM2(2, MatchMm);
	constexpr RegisterOperand MM3(3, MatchMm);
	constexpr RegisterOperand MM4(4, MatchMm);
	constexpr RegisterOperand MM5(5, MatchMm);
	constexpr RegisterOperand MM6(6, MatchMm);
	constexpr RegisterOperand MM7(7, MatchMm);
	
	constexpr RegisterOperand XMM0(0, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM1(1, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM2(2, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM3(3, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM4(4, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM5(5, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM6(6, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM7(7, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM8(8, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM9(9, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM10(10, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM11(11, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM12(12, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM13(13, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM14(14, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM15(15, MatchXmm | MatchXmmHi);
	constexpr RegisterOperand XMM16(16, MatchXmmHi);
	constexpr RegisterOperand XMM17(17, MatchXmmHi);
	constexpr RegisterOperand XMM18(18, MatchXmmHi);
	constexpr RegisterOperand XMM19(19, MatchXmmHi);
	constexpr RegisterOperand XMM20(20, MatchXmmHi);
	constexpr RegisterOperand XMM21(21, MatchXmmHi);
	constexpr RegisterOperand XMM22(22, MatchXmmHi);
	constexpr RegisterOperand XMM23(23, MatchXmmHi);
	constexpr RegisterOperand XMM24(24, MatchXmmHi);
	constexpr RegisterOperand XMM25(25, MatchXmmHi);
	constexpr RegisterOperand XMM26(26, MatchXmmHi);
	constexpr RegisterOperand XMM27(27, MatchXmmHi);
	constexpr RegisterOperand XMM28(28, MatchXmmHi);
	constexpr RegisterOperand XMM29(29, MatchXmmHi);
	constexpr RegisterOperand XMM30(30, MatchXmmHi);
	constexpr RegisterOperand XMM31(31, MatchXmmHi);

	constexpr RegisterOperand YMM0(0, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM1(1, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM2(2, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM3(3, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM4(4, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM5(5, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM6(6, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM7(7, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM8(8, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM9(9, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM10(10, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM11(11, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM12(12, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM13(13, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM14(14, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM15(15, MatchYmm | MatchYmmHi);
	constexpr RegisterOperand YMM16(16, MatchYmmHi);
	constexpr RegisterOperand YMM17(17, MatchYmmHi);
	constexpr RegisterOperand YMM18(18, MatchYmmHi);
	constexpr RegisterOperand YMM19(19, MatchYmmHi);
	constexpr RegisterOperand YMM20(20, MatchYmmHi);
	constexpr RegisterOperand YMM21(21, MatchYmmHi);
	constexpr RegisterOperand YMM22(22, MatchYmmHi);
	constexpr RegisterOperand YMM23(23, MatchYmmHi);
	constexpr RegisterOperand YMM24(24, MatchYmmHi);
	constexpr RegisterOperand YMM25(25, MatchYmmHi);
	constexpr RegisterOperand YMM26(26, MatchYmmHi);
	constexpr RegisterOperand YMM27(27, MatchYmmHi);
	constexpr RegisterOperand YMM28(28, MatchYmmHi);
	constexpr RegisterOperand YMM29(29, MatchYmmHi);
	constexpr RegisterOperand YMM30(30, MatchYmmHi);
	constexpr RegisterOperand YMM31(31, MatchYmmHi);

	constexpr RegisterOperand ZMM0(0, MatchZmm);
	constexpr RegisterOperand ZMM1(1, MatchZmm);
	constexpr RegisterOperand ZMM2(2, MatchZmm);
	constexpr RegisterOperand ZMM3(3, MatchZmm);
	constexpr RegisterOperand ZMM4(4, MatchZmm);
	constexpr RegisterOperand ZMM5(5, MatchZmm);
	constexpr RegisterOperand ZMM6(6, MatchZmm);
	constexpr RegisterOperand ZMM7(7, MatchZmm);
	constexpr RegisterOperand ZMM8(8, MatchZmm);
	constexpr RegisterOperand ZMM9(9, MatchZmm);
	constexpr RegisterOperand ZMM10(10, MatchZmm);
	constexpr RegisterOperand ZMM11(11, MatchZmm);
	constexpr RegisterOperand ZMM12(12, MatchZmm);
	constexpr RegisterOperand ZMM13(13, MatchZmm);
	constexpr RegisterOperand ZMM14(14, MatchZmm);
	constexpr RegisterOperand ZMM15(15, MatchZmm);
	constexpr RegisterOperand ZMM16(16, MatchZmm);
	constexpr RegisterOperand ZMM17(17, MatchZmm);
	constexpr RegisterOperand ZMM18(18, MatchZmm);
	constexpr RegisterOperand ZMM19(19, MatchZmm);
	constexpr RegisterOperand ZMM20(20, MatchZmm);
	constexpr RegisterOperand ZMM21(21, MatchZmm);
	constexpr RegisterOperand ZMM22(22, MatchZmm);
	constexpr RegisterOperand ZMM23(23, MatchZmm);
	constexpr RegisterOperand ZMM24(24, MatchZmm);
	constexpr RegisterOperand ZMM25(25, MatchZmm);
	constexpr RegisterOperand ZMM26(26, MatchZmm);
	constexpr RegisterOperand ZMM27(27, MatchZmm);
	constexpr RegisterOperand ZMM28(28, MatchZmm);
	constexpr RegisterOperand ZMM29(29, MatchZmm);
	constexpr RegisterOperand ZMM30(30, MatchZmm);
	constexpr RegisterOperand ZMM31(31, MatchZmm);

//============================================================================
} // namespace Javelin::Assembler::x64::Register
//============================================================================

namespace Javelin::Assembler::x64
{
//============================================================================

	const RegisterMap RegisterMap::instance;
	const ParameterizedRegisterMap ParameterizedRegisterMap::instance;

//============================================================================

	RegisterMap::RegisterMap()
	{
		using namespace Register;
		
		(*this)["al"] = &AL;
		(*this)["ah"] = &AH;
		(*this)["ax"] = &AX;
		(*this)["eax"] = &EAX;
		(*this)["rax"] = &RAX;
		
		(*this)["bl"] = &BL;
		(*this)["bh"] = &BH;
		(*this)["bx"] = &BX;
		(*this)["ebx"] = &EBX;
		(*this)["rbx"] = &RBX;
		
		(*this)["cl"] = &CL;
		(*this)["ch"] = &CH;
		(*this)["cx"] = &CX;
		(*this)["ecx"] = &ECX;
		(*this)["rcx"] = &RCX;

		(*this)["dl"] = &DL;
		(*this)["dh"] = &DH;
		(*this)["dx"] = &DX;
		(*this)["edx"] = &EDX;
		(*this)["rdx"] = &RDX;

		(*this)["sil"] = &SIL;
		(*this)["si"] = &SI;
		(*this)["esi"] = &ESI;
		(*this)["rsi"] = &RSI;

		(*this)["dil"] = &DIL;
		(*this)["di"] = &DI;
		(*this)["edi"] = &EDI;
		(*this)["rdi"] = &RDI;

		(*this)["bpl"] = &BPL;
		(*this)["bp"] = &BP;
		(*this)["ebp"] = &EBP;
		(*this)["rbp"] = &RBP;

		(*this)["spl"] = &SPL;
		(*this)["sp"] = &SP;
		(*this)["esp"] = &ESP;
		(*this)["rsp"] = &RSP;

		(*this)["r8b"] = &R8B;
		(*this)["r8w"] = &R8W;
		(*this)["r8d"] = &R8D;
		(*this)["r8"] = &R8;

		(*this)["r9b"] = &R9B;
		(*this)["r9w"] = &R9W;
		(*this)["r9d"] = &R9D;
		(*this)["r9"] = &R9;

		(*this)["r10b"] = &R10B;
		(*this)["r10w"] = &R10W;
		(*this)["r10d"] = &R10D;
		(*this)["r10"] = &R10;

		(*this)["r11b"] = &R11B;
		(*this)["r11w"] = &R11W;
		(*this)["r11d"] = &R11D;
		(*this)["r11"] = &R11;

		(*this)["r12b"] = &R12B;
		(*this)["r12w"] = &R12W;
		(*this)["r12d"] = &R12D;
		(*this)["r12"] = &R12;

		(*this)["r13b"] = &R13B;
		(*this)["r13w"] = &R13W;
		(*this)["r13d"] = &R13D;
		(*this)["r13"] = &R13;

		(*this)["r14b"] = &R14B;
		(*this)["r14w"] = &R14W;
		(*this)["r14d"] = &R14D;
		(*this)["r14"] = &R14;

		(*this)["r15b"] = &R15B;
		(*this)["r15w"] = &R15W;
		(*this)["r15d"] = &R15D;
		(*this)["r15"] = &R15;

		(*this)["rip"] = &RIP;
	
		(*this)["st0"] = &ST0;
		(*this)["st1"] = &ST1;
		(*this)["st2"] = &ST2;
		(*this)["st3"] = &ST3;
		(*this)["st4"] = &ST4;
		(*this)["st5"] = &ST5;
		(*this)["st6"] = &ST6;
		(*this)["st7"] = &ST7;
		
		(*this)["mm0"] = &MM0;
		(*this)["mm1"] = &MM1;
		(*this)["mm2"] = &MM2;
		(*this)["mm3"] = &MM3;
		(*this)["mm4"] = &MM4;
		(*this)["mm5"] = &MM5;
		(*this)["mm6"] = &MM6;
		(*this)["mm7"] = &MM7;
		
		(*this)["xmm0"] = &XMM0;
		(*this)["xmm1"] = &XMM1;
		(*this)["xmm2"] = &XMM2;
		(*this)["xmm3"] = &XMM3;
		(*this)["xmm4"] = &XMM4;
		(*this)["xmm5"] = &XMM5;
		(*this)["xmm6"] = &XMM6;
		(*this)["xmm7"] = &XMM7;
		(*this)["xmm8"] = &XMM8;
		(*this)["xmm9"] = &XMM9;
		(*this)["xmm10"] = &XMM10;
		(*this)["xmm11"] = &XMM11;
		(*this)["xmm12"] = &XMM12;
		(*this)["xmm13"] = &XMM13;
		(*this)["xmm14"] = &XMM14;
		(*this)["xmm15"] = &XMM15;
		(*this)["xmm16"] = &XMM16;
		(*this)["xmm17"] = &XMM17;
		(*this)["xmm18"] = &XMM18;
		(*this)["xmm19"] = &XMM19;
		(*this)["xmm20"] = &XMM20;
		(*this)["xmm21"] = &XMM21;
		(*this)["xmm22"] = &XMM22;
		(*this)["xmm23"] = &XMM23;
		(*this)["xmm24"] = &XMM24;
		(*this)["xmm25"] = &XMM25;
		(*this)["xmm26"] = &XMM26;
		(*this)["xmm27"] = &XMM27;
		(*this)["xmm28"] = &XMM28;
		(*this)["xmm29"] = &XMM29;
		(*this)["xmm30"] = &XMM30;
		(*this)["xmm31"] = &XMM31;

		(*this)["ymm0"] = &YMM0;
		(*this)["ymm1"] = &YMM1;
		(*this)["ymm2"] = &YMM2;
		(*this)["ymm3"] = &YMM3;
		(*this)["ymm4"] = &YMM4;
		(*this)["ymm5"] = &YMM5;
		(*this)["ymm6"] = &YMM6;
		(*this)["ymm7"] = &YMM7;
		(*this)["ymm8"] = &YMM8;
		(*this)["ymm9"] = &YMM9;
		(*this)["ymm10"] = &YMM10;
		(*this)["ymm11"] = &YMM11;
		(*this)["ymm12"] = &YMM12;
		(*this)["ymm13"] = &YMM13;
		(*this)["ymm14"] = &YMM14;
		(*this)["ymm15"] = &YMM15;
		(*this)["ymm16"] = &YMM16;
		(*this)["ymm17"] = &YMM17;
		(*this)["ymm18"] = &YMM18;
		(*this)["ymm19"] = &YMM19;
		(*this)["ymm20"] = &YMM20;
		(*this)["ymm21"] = &YMM21;
		(*this)["ymm22"] = &YMM22;
		(*this)["ymm23"] = &YMM23;
		(*this)["ymm24"] = &YMM24;
		(*this)["ymm25"] = &YMM25;
		(*this)["ymm26"] = &YMM26;
		(*this)["ymm27"] = &YMM27;
		(*this)["ymm28"] = &YMM28;
		(*this)["ymm29"] = &YMM29;
		(*this)["ymm30"] = &YMM30;
		(*this)["ymm31"] = &YMM31;

		(*this)["zmm0"] = &ZMM0;
		(*this)["zmm1"] = &ZMM1;
		(*this)["zmm2"] = &ZMM2;
		(*this)["zmm3"] = &ZMM3;
		(*this)["zmm4"] = &ZMM4;
		(*this)["zmm5"] = &ZMM5;
		(*this)["zmm6"] = &ZMM6;
		(*this)["zmm7"] = &ZMM7;
		(*this)["zmm8"] = &ZMM8;
		(*this)["zmm9"] = &ZMM9;
		(*this)["zmm10"] = &ZMM10;
		(*this)["zmm11"] = &ZMM11;
		(*this)["zmm12"] = &ZMM12;
		(*this)["zmm13"] = &ZMM13;
		(*this)["zmm14"] = &ZMM14;
		(*this)["zmm15"] = &ZMM15;
		(*this)["zmm16"] = &ZMM16;
		(*this)["zmm17"] = &ZMM17;
		(*this)["zmm18"] = &ZMM18;
		(*this)["zmm19"] = &ZMM19;
		(*this)["zmm20"] = &ZMM20;
		(*this)["zmm21"] = &ZMM21;
		(*this)["zmm22"] = &ZMM22;
		(*this)["zmm23"] = &ZMM23;
		(*this)["zmm24"] = &ZMM24;
		(*this)["zmm25"] = &ZMM25;
		(*this)["zmm26"] = &ZMM26;
		(*this)["zmm27"] = &ZMM27;
		(*this)["zmm28"] = &ZMM28;
		(*this)["zmm29"] = &ZMM29;
		(*this)["zmm30"] = &ZMM30;
		(*this)["zmm31"] = &ZMM31;
	}

//============================================================================

	ParameterizedRegisterMap::ParameterizedRegisterMap()
	{
		(*this)["reg8"] = MatchBitfield(MatchAL | MatchReg8);
		(*this)["reg16"] = MatchBitfield(MatchAX | MatchReg16);
		(*this)["reg32"] = MatchBitfield(MatchEAX | MatchReg32);
		(*this)["reg64"] = MatchBitfield(MatchRAX | MatchReg64);
		(*this)["st"] = MatchBitfield(MatchST0 | MatchST);
		(*this)["mm"] = MatchMm;
		(*this)["xmm"] = MatchXmm;
		(*this)["ymm"] = MatchYmm;
		(*this)["zmm"] = MatchZmm;
	}
	
//============================================================================
} // Javelin::Assembler::x64
//============================================================================
