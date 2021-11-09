//============================================================================

/**
 * Return represents the end of the bytestream.
 */
TAG(Return)

TAG(Literal1)
TAG(Literal2)
TAG(Literal4)
TAG(Literal6)
TAG(Literal8)
TAG(Literal10)
TAG(Literal12)
TAG(Literal14)
TAG(Literal16)
TAG(Literal18)
TAG(Literal20)
TAG(Literal22)
TAG(Literal24)
TAG(Literal26)
TAG(Literal28)
TAG(Literal30)
TAG(Literal32)
/**
 * LiteralBlock contains 2-byte length, followed by
 * literal bytes to copy.
 **/
TAG(LiteralBlock)
	
/**
 * Alternate block
 * Followed by:
 *  u8 maximum instruction size
 *
 * Each alternate is then a series of:
 *  <Condition>
 *  <instructions>
 *
 * EndAlternate is used by the first pass to manage the offset stack.
 */
TAG(Alternate)
TAG(EndAlternate)

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
TAG(CIValuePatchB1Opcode)
TAG(RepeatPatch2BOpcode)
TAG(RepeatPatch4BOpcode)

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
//TAG(RvcRegisterNumberCondition) // 8-15 inclusive
TAG(SimmCondition)
TAG(UimmCondition)

TAG(DeltaLabelCondition)
TAG(DeltaLabelForwardCondition)
TAG(DeltaLabelBackwardCondition)
TAG(DeltaExpressionLabelCondition)
TAG(DeltaExpressionLabelForwardCondition)
TAG(DeltaExpressionLabelBackwardCondition)

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

//============================================================================
