//============================================================================

// Match    	  			Description

// Reserve 8 bits to determine operand targets.
// These must be the first matches
TAG(Op0,					""				)
TAG(Op1,					""				)
TAG(Op2,					""				)
TAG(Op3,					""				)
TAG(Op4,					""				)
TAG(Op5,					""				)
TAG(Op6,					""				)
TAG(Op7,					""				)

TAG(Reg32,					"Wn"			)
TAG(Reg32Z,					"WZR"			)
TAG(Reg32SP,				"WSP"			)
TAG(Reg64,					"Xn"			)
TAG(Reg64Z,					"XZR"			)
TAG(Reg64SP,				"SP"			)
TAG(RegNP1,					"RegN+1"		)

TAG(B,						"B"				)
TAG(H,						"H"				)
TAG(S,						"S"				)
TAG(D,						"D"				)
TAG(Q,						"Q"				)
TAG(VB,						"VB"			)
TAG(V4B,					"V4B"			)
TAG(V8B,					"V8B"			)
TAG(V16B,					"V16B"			)
TAG(VH,						"VH"			)
TAG(V2H,					"V2H"			)
TAG(V4H,					"V4H"			)
TAG(V8H,					"V8H"			)
TAG(VS,						"VS"			)
TAG(V2S,					"V2S"			)
TAG(V4S,					"V4S"			)
TAG(VD,						"VD"			)
TAG(V1D,					"V1D"			)
TAG(V2D,					"V2D"			)
TAG(V1Q,					"V1Q"			)

// Repeat of previous operand mask
TAG(Rep,					"Rep"			)

TAG(Rel,					"Rel"			)

TAG(ExpressionImm,			"{Imm}"			)
TAG(Imm0,					"#0"			)
TAG(HwImm16,				"#HwImm16"		)
TAG(UImm12,					"#uimm12"		)
TAG(UImm16,					"#uimm16"		)
TAG(Not32HwImm16,			"#~32HmImm16"	)
TAG(Not64HwImm16,			"#~64HwImm16"	)
TAG(Imm,					"#imm"			)
TAG(Logical32Imm,			"#l32imm"		)
TAG(Logical64Imm,			"#l64imm"		)
TAG(Number,					"Number"		)
TAG(FloatImm,				"#fimm"			)

TAG(LSL,					"LSL"			)
TAG(Shift,					"Shift"			)
TAG(ROR,					"ROR"			)

TAG(ExtendReg32,			"XT"			)
TAG(ExtendLoadStoreReg32,	"XT32"			)
TAG(ExtendReg64,			"XT64"			)

TAG(Condition,				"Condition"		)

TAG(Colon,					":"				)
TAG(Comma,					", "			)
TAG(ExclamationMark,		"!"				)
TAG(LeftSquareBracket,		"["				)
TAG(RightSquareBracket,		"]"				)
TAG(LeftBrace,				"{"				)
TAG(RightBrace,				"}"				)

//============================================================================

