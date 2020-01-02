//============================================================================

/**
 * Return represents the end of the bytestream.
 */
TAG(Return)

TAG(Literal4)
TAG(Literal8)
TAG(Literal12)
TAG(Literal16)
TAG(Literal20)
TAG(Literal24)
TAG(Literal28)
TAG(Literal32)
/**
 * LiteralBlock contains var-length encoded length, followed by
 * literal bytes to copy.
 **/
TAG(LiteralBlock)
		
/**
 * Unconditional jump in instruction stream.
 */
TAG(Jump)

/**
 * Align expression, followed by byte representing the power of
 * 2 byte alignment.
 */
TAG(Align)
		
/**
 * Unalign command.
 */
TAG(Unalign)

/**
 * Patch.
 */
TAG(MaskedPatchB1Opcode)
TAG(SignedPatchB1Opcode)
TAG(MaskedPatchB2Opcode)
TAG(SignedPatchB2Opcode)
TAG(MaskedPatchB4Opcode)
TAG(SignedPatchB4Opcode)
TAG(UnsignedPatchB1Opcode)
TAG(UnsignedPatchB2Opcode)
TAG(UnsignedPatchB4Opcode)
TAG(RepeatPatchOpcode)
TAG(LogicalImmediatePatchB4Opcode)
TAG(LogicalImmediatePatchB8Opcode)

/**
 * Expression.
 * Writes an expression value into the output.
 * u16 encoded offset into the expression pool.
 */
TAG(B1Expression)
TAG(B2Expression)
TAG(B4Expression)
TAG(B8Expression)

TAG(DataBlock)
TAG(DataPointer)

/**
 * Condition
 * u16 offset into the expression pool.
 * u8 skip if false.
 */
TAG(Imm0B1Condition)
TAG(Imm0B2Condition)
TAG(Imm0B4Condition)
TAG(Imm0B8Condition)
TAG(Delta21Condition)
TAG(Delta26x4Condition)
TAG(AdrpCondition)

/**
 * Labels.
 */
TAG(Label)
TAG(ExpressionLabel)

TAG(PatchLabel)
TAG(PatchLabelForward)
TAG(PatchLabelBackward)
TAG(PatchExpressionLabel)
TAG(PatchExpressionLabelForward)
TAG(PatchExpressionLabelBackward)
TAG(PatchExpression)
TAG(PatchAbsoluteAddress)

/**
 * Custom Opcode Handling
 */
TAG(MovReg32Expression)
TAG(MovReg64Expression)

//============================================================================
