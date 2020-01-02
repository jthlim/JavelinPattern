//============================================================================

TAG(Fixed)
TAG(LoadStoreOffsetRegister)
TAG(LoadStorePair)
TAG(ArithmeticExtendedRegister)
TAG(ArithmeticImmediate)
TAG(ArithmeticShiftedRegister)
TAG(LogicalImmediate)
TAG(Rel26)
TAG(RtRel19)
TAG(RdRel21HiLo)
TAG(RtImmRel14)				// For tbz, tbnz
TAG(RtRnImm9)
TAG(RtRnUImm12)
TAG(RdRnRmRa)
TAG(RdRnRmImm6)          	// Immr: Rm: 5 bits, Imm: 6 bits, Rn: 5 bits, Rd: 5 bits
TAG(RdHwImm16)           	// Immr: Rn: 5 bits, HwImm16: 2+16 bits
TAG(RdImm16Shift)
TAG(RdNot32HwImm16)      	// Immr: Rn: 5 bits, HwImm16: 2+16 bits
TAG(RdNot64HwImm16)      	// Immr: Rn: 5 bits, HwImm16: 2+16 bits
TAG(RdRnImmrImms)        	// Immr: 6 bits, Imms: 6 bits, Rn: 5 bits, Rd: 5 bits
TAG(RnImmNzcvCond)
TAG(RnRmNzcvCond)
TAG(RdRnRmCond)
TAG(RdExpressionImm)
TAG(FpRdRnIndex1Index2)		// For INS instruction
TAG(FpRdRnRmRa)				// encodingVariant.data controls how registerData is applied to the opcode.
TAG(FpRdRnFixedPointShift)	// For FCVTZS, FCVTZU
TAG(Fmov)					// encodingVariant.data controls how registerData is applied to the opcode.
TAG(RdRnImmSubfield)
TAG(VectorImmediate)		// For BIC, ORR, MOVI, MVNI
TAG(RdRnRmImm2)          	// Immr: Rm: 5 bits, Imm: 2 bits, Rn: 5 bits, Rd: 5 bits. For SM3TT* instructions

//============================================================================
