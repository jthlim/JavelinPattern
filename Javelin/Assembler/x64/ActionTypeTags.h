//============================================================================

/**
 * Return represents the end of the bytestream.
 */
TAG(Return)

/**
 * Literal1-16 are optimizations, and opcode == bytes to copy.
 */
TAG(Literal1)
TAG(Literal2)
TAG(Literal3)
TAG(Literal4)
TAG(Literal5)
TAG(Literal6)
TAG(Literal7)
TAG(Literal8)
TAG(Literal9)
TAG(Literal10)
TAG(Literal11)
TAG(Literal12)
TAG(Literal13)
TAG(Literal14)
TAG(Literal15)
TAG(Literal16)

/**
 * LiteralBlock contains u16 encoded length, followed by
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
 * Expression.
 * Writes an expression value into the output.
 * u16 encoded offset into the expression pool.
 */
TAG(B1Expression)
TAG(B2Expression)
TAG(B4Expression)
TAG(B8Expression)

// Delta and Add patch the previous bytes.
TAG(B1DeltaExpression)
TAG(B4DeltaExpression)
TAG(B4AddExpression)

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

TAG(Imm8B2Condition)
TAG(Imm8B4Condition)
TAG(Imm8B8Condition)

TAG(Imm16B4Condition)
TAG(Imm16B8Condition)

TAG(Imm32B8Condition)
TAG(UImm32B8Condition)

TAG(Delta32Condition)

TAG(Rel8LabelCondition)
TAG(Rel8LabelForwardCondition)
TAG(Rel8LabelBackwardCondition)
TAG(Rel8ExpressionLabelCondition)
TAG(Rel8ExpressionLabelForwardCondition)
TAG(Rel8ExpressionLabelBackwardCondition)

/**
 * Special case opcode handlers
 */
TAG(DynamicOpcodeO1)			// 1 byte "o" encoding.
TAG(DynamicOpcodeO2)			// 2 byte "o" encoding.
TAG(DynamicOpcodeR)
TAG(DynamicOpcodeRR)
TAG(DynamicOpcodeRM)
TAG(DynamicOpcodeRV)
TAG(DynamicOpcodeRVR)
TAG(DynamicOpcodeRVM)

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
TAG(PatchAbsoluteAddress)

//============================================================================
