//============================================================================

// Match    Condition		Expression BitWidth
TAG(AL,      		Zero,			 8)
TAG(CL,      		Never,			 8)
TAG(AX,      		Zero,			 8)
TAG(EAX,     		Zero,			 8)
TAG(RAX,     		Zero,			 8)
TAG(RIP,     		Never,			 8)
TAG(ST0,			Zero,			 8)
TAG(Reg8,			Always,			 8)
TAG(Reg8Hi,		Never,			 0)
TAG(Reg16,			Always,			 8)
TAG(Reg32,			Always,			 8)
TAG(Reg64,			Always,			 8)
TAG(ST,				Always,			 8)
TAG(Mm,				Always,			 8)
TAG(Xmm,			Always,			 8)
TAG(XmmHi,			Always,			 8)		// For xmm16-31
TAG(MaskedXmm,		Always,			 8)		// For xmm0-31 k{1-7}{z}
TAG(Ymm,			Always,			 8)
TAG(YmmHi,			Always,			 8)		// For ymm16-31
TAG(MaskedYmm,		Always,			 8)		// For ymm0-31 k{1-7}{z}
TAG(Zmm,			Always,			 8)
TAG(MaskedZmm,		Always,			 8)
TAG(Mem8,			Always,			 0)
TAG(Mem16,			Always,			 0)
TAG(Mem32,			Always,			 0)
TAG(Mem64,			Always,			 0)
TAG(DirectMem8	,	Always,			 0)
TAG(DirectMem16,	Always,			 0)
TAG(DirectMem32,	Always,			 0)
TAG(DirectMem64,	Always,			 0)
TAG(Mem80,			Always,			 0)
TAG(Mem128,		Always,			 0)
TAG(Mem256,		Always,			 0)
TAG(Mem512,		Always,			 0)
TAG(Imm8,			Imm8,			 8)
TAG(Imm16,			Imm16,			16)
TAG(Imm32,			Imm32,			32)
TAG(UImm32,		UImm32,			32)
TAG(Imm64,			Always,			64)
TAG(Rel8,			Rel8,			 0)
TAG(Rel32,			Always,			 0)
TAG(Constant0,		Zero,			 8)
TAG(Constant1,		Never,			 8)
TAG(Constant3,		Never,			 8)

//============================================================================

