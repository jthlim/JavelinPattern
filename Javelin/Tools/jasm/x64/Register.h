//============================================================================

#pragma once
#include "Javelin/Tools/jasm/x64/Instruction.h"

#include <string>
#include <unordered_map>

//============================================================================

namespace Javelin::Assembler::x64
{
//============================================================================

	namespace Register
	{
		extern const RegisterOperand AL;
		extern const RegisterOperand AH;
		extern const RegisterOperand AX;
		extern const RegisterOperand EAX;
		extern const RegisterOperand RAX;

		extern const RegisterOperand BL;
		extern const RegisterOperand BH;
		extern const RegisterOperand BX;
		extern const RegisterOperand EBX;
		extern const RegisterOperand RBX;

		extern const RegisterOperand CL;
		extern const RegisterOperand CH;
		extern const RegisterOperand CX;
		extern const RegisterOperand ECX;
		extern const RegisterOperand RCX;
		
		extern const RegisterOperand DL;
		extern const RegisterOperand DH;
		extern const RegisterOperand DX;
		extern const RegisterOperand EDX;
		extern const RegisterOperand RDX;
		
		extern const RegisterOperand SIL;
		extern const RegisterOperand SI;
		extern const RegisterOperand ESI;
		extern const RegisterOperand RSI;
		
		extern const RegisterOperand DIL;
		extern const RegisterOperand DI;
		extern const RegisterOperand EDI;
		extern const RegisterOperand RDI;
		
		extern const RegisterOperand BPL;
		extern const RegisterOperand BP;
		extern const RegisterOperand EBP;
		extern const RegisterOperand RBP;
		
		extern const RegisterOperand SPL;
		extern const RegisterOperand SP;
		extern const RegisterOperand ESP;
		extern const RegisterOperand RSP;

		extern const RegisterOperand R8B;
		extern const RegisterOperand R8W;
		extern const RegisterOperand R8D;
		extern const RegisterOperand R8;

		extern const RegisterOperand R9B;
		extern const RegisterOperand R9W;
		extern const RegisterOperand R9D;
		extern const RegisterOperand R9;
		
		extern const RegisterOperand R10B;
		extern const RegisterOperand R10W;
		extern const RegisterOperand R10D;
		extern const RegisterOperand R10;
		
		extern const RegisterOperand R11B;
		extern const RegisterOperand R11W;
		extern const RegisterOperand R11D;
		extern const RegisterOperand R11;
		
		extern const RegisterOperand R12B;
		extern const RegisterOperand R12W;
		extern const RegisterOperand R12D;
		extern const RegisterOperand R12;
		
		extern const RegisterOperand R13B;
		extern const RegisterOperand R13W;
		extern const RegisterOperand R13D;
		extern const RegisterOperand R13;
		
		extern const RegisterOperand R14B;
		extern const RegisterOperand R14W;
		extern const RegisterOperand R14D;
		extern const RegisterOperand R14;
		
		extern const RegisterOperand R15B;
		extern const RegisterOperand R15W;
		extern const RegisterOperand R15D;
		extern const RegisterOperand R15;
		
		extern const RegisterOperand RIP;

		extern const RegisterOperand ST0;
		extern const RegisterOperand ST1;
		extern const RegisterOperand ST2;
		extern const RegisterOperand ST3;
		extern const RegisterOperand ST4;
		extern const RegisterOperand ST5;
		extern const RegisterOperand ST6;
		extern const RegisterOperand ST7;

		extern const RegisterOperand MM0;
		extern const RegisterOperand MM1;
		extern const RegisterOperand MM2;
		extern const RegisterOperand MM3;
		extern const RegisterOperand MM4;
		extern const RegisterOperand MM5;
		extern const RegisterOperand MM6;
		extern const RegisterOperand MM7;

		extern const RegisterOperand XMM0;
		extern const RegisterOperand XMM1;
		extern const RegisterOperand XMM2;
		extern const RegisterOperand XMM3;
		extern const RegisterOperand XMM4;
		extern const RegisterOperand XMM5;
		extern const RegisterOperand XMM6;
		extern const RegisterOperand XMM7;
		extern const RegisterOperand XMM8;
		extern const RegisterOperand XMM9;
		extern const RegisterOperand XMM10;
		extern const RegisterOperand XMM11;
		extern const RegisterOperand XMM12;
		extern const RegisterOperand XMM13;
		extern const RegisterOperand XMM14;
		extern const RegisterOperand XMM15;
		extern const RegisterOperand XMM16;
		extern const RegisterOperand XMM17;
		extern const RegisterOperand XMM18;
		extern const RegisterOperand XMM19;
		extern const RegisterOperand XMM20;
		extern const RegisterOperand XMM21;
		extern const RegisterOperand XMM22;
		extern const RegisterOperand XMM23;
		extern const RegisterOperand XMM24;
		extern const RegisterOperand XMM25;
		extern const RegisterOperand XMM26;
		extern const RegisterOperand XMM27;
		extern const RegisterOperand XMM28;
		extern const RegisterOperand XMM29;
		extern const RegisterOperand XMM30;
		extern const RegisterOperand XMM31;

		extern const RegisterOperand YMM0;
		extern const RegisterOperand YMM1;
		extern const RegisterOperand YMM2;
		extern const RegisterOperand YMM3;
		extern const RegisterOperand YMM4;
		extern const RegisterOperand YMM5;
		extern const RegisterOperand YMM6;
		extern const RegisterOperand YMM7;
		extern const RegisterOperand YMM8;
		extern const RegisterOperand YMM9;
		extern const RegisterOperand YMM10;
		extern const RegisterOperand YMM11;
		extern const RegisterOperand YMM12;
		extern const RegisterOperand YMM13;
		extern const RegisterOperand YMM14;
		extern const RegisterOperand YMM15;
		
		extern const RegisterOperand ZMM0;
		extern const RegisterOperand ZMM1;
		extern const RegisterOperand ZMM2;
		extern const RegisterOperand ZMM3;
		extern const RegisterOperand ZMM4;
		extern const RegisterOperand ZMM5;
		extern const RegisterOperand ZMM6;
		extern const RegisterOperand ZMM7;
		extern const RegisterOperand ZMM8;
		extern const RegisterOperand ZMM9;
		extern const RegisterOperand ZMM10;
		extern const RegisterOperand ZMM11;
		extern const RegisterOperand ZMM12;
		extern const RegisterOperand ZMM13;
		extern const RegisterOperand ZMM14;
		extern const RegisterOperand ZMM15;
		
//============================================================================
	} // namespace Register
//============================================================================

	class RegisterMap : public std::unordered_map<std::string, const RegisterOperand*>
	{
	public:
		static const RegisterMap& GetInstance() { return instance; }
		
	private:
		RegisterMap();
		
		static const RegisterMap instance;
	};

//============================================================================

	class ParameterizedRegisterMap : public std::unordered_map<std::string, MatchBitfield>
	{
	public:
		ParameterizedRegisterMap();
		
		static const ParameterizedRegisterMap instance;
	};

//============================================================================
} // namespace Javelin::Assembler::x64
//============================================================================
