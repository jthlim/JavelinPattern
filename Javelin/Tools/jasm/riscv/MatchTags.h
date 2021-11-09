//============================================================================

// Match    	  			Description

// Reserve 8 bits to determine operand targets.
// These must be the first matches
TAG(Op0,					"",             Always)
TAG(Op1,					"",             Always)
TAG(Op2,					"",             Always)
TAG(Op3,					"",             Always)
TAG(Op4,					"",             Always)
TAG(Op5,					"",             Always)
TAG(Op6,					"",             Always)
TAG(Op7,					"",             Always)

TAG(RegX,					"Xn",           Always)
TAG(RegZero,                "X0",           Always)
TAG(RegReturnAddress,       "XA",           Always)
TAG(RegStackPointer,        "SP",           Always)
TAG(RegCX,                  "CXn",          Always)
TAG(ParenthesizedRegX,      "(Xn)",         Always)
TAG(RegF,					"Fn",           Always)
TAG(RegCF,                  "CFn",          Always)
TAG(RoundingMode,           "RoundingMode", Always)
TAG(RepeatOp0,              "RepOp0",       Always)
TAG(RepeatOp1,              "RepOp1",       Always)

TAG(Imm,                    "imm",          Always)
TAG(FloatImm,               "fimm",         Always)
TAG(Uimm5,                  "uimm5",        Uimm5)
TAG(Imm6,                   "imm6",         Simm6)
TAG(Imm12,                  "imm12",        Simm12)
TAG(Uimm12,                 "uimm12",       Uimm12)
TAG(Imm20,                  "imm20",        Simm20)
TAG(Uimm20,                 "uimm20",       Uimm20)
TAG(ShiftImm,               "shiftimm",     Always)
TAG(CShiftImm,              "cshiftimm",    Always)

TAG(BRel,                   "BRel",         Always)
TAG(JRel,                   "JRel",         Always)
TAG(CBRel,                  "CBRel",        CBDelta)
TAG(CJRel,                  "CJRel",        CJDelta)

TAG(Condition,				"Condition",    Always)

TAG(Comma,					", ",           Always)

//============================================================================

