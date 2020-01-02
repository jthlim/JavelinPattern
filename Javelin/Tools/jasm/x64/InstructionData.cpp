//============================================================================

#include "Javelin/Tools/jasm/x64/Instruction.h"

//============================================================================

using namespace Javelin::Assembler::x64;

//============================================================================

const InstructionMap InstructionMap::instance;

//============================================================================

#define UNWRAP(...) 		__VA_ARGS__
#define NUM_ELEMENTS(x)		(sizeof(x) / sizeof(x[0]))
#define NUM_OPERANDS(...)	(sizeof((uint64_t[]){__VA_ARGS__})/sizeof(uint64_t))
#define NUM_OPCODES(...)	(sizeof((uint8_t[]){__VA_ARGS__})/sizeof(uint8_t))

#define DECLARE_CANDIDATE(operand_list, op_width, enc, opcode_data, ops...) \
	{																	\
		.operandMatchMasks = { UNWRAP operand_list },					\
		.operandMatchMasksLength = NUM_OPERANDS(UNWRAP operand_list),	\
		.opcodeLength = NUM_OPCODES(ops),								\
		.opcodeData = opcode_data,										\
		.operationWidth = op_width,										\
		.encoder = Encoder::enc,										\
		.opcodes = { ops },												\
		.opcodePrefix = 0,												\
	}
	
#define DECLARE_PREFIXED_CANDIDATE(operand_list, op_width, enc, opcode_data, prefix, ops...) \
	{																	\
		.operandMatchMasks = { UNWRAP operand_list },					\
		.operandMatchMasksLength = NUM_OPERANDS(UNWRAP operand_list),	\
		.opcodeLength = NUM_OPCODES(ops),								\
		.opcodeData = opcode_data,										\
		.operationWidth = op_width,										\
		.encoder = Encoder::enc,										\
		.opcodes = { ops },												\
		.opcodePrefix = prefix,											\
	}

#define DECLARE_ENCODING_VARIANT(name, operand_list, op_width, enc, opcode_data, ops...) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] = \
	{																		\
		DECLARE_CANDIDATE(operand_list, op_width, enc, opcode_data, ops),	\
	}

#define DECLARE_INSTRUCTION(name, default_width) \
	constexpr Instruction name =											\
	{																		\
		.defaultWidth = default_width,										\
		.encodingVariantLength = NUM_ELEMENTS(name##_ENCODING_VARIANTS),	\
		.encodingVariants = name##_ENCODING_VARIANTS						\
	}

#define DECLARE_AVX_RVM_INSTRUCTION(name, ops...) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] = 									\
	{																						\
		DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmm, MatchXmmMem128), 128, RVM, 0, ops),	\
		DECLARE_PREFIXED_CANDIDATE((MatchYmm, MatchYmm, MatchYmmMem256), 256, RVM, 0, ops),	\
	};																						\
	DECLARE_INSTRUCTION(name, 128)

#define DECLARE_AVX_RM_INSTRUCTION(name, ops...) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] = 							\
	{																				\
		DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128), 128, vRM, 0, ops),	\
		DECLARE_PREFIXED_CANDIDATE((MatchYmm, MatchYmmMem256), 256, vRM, 0, ops),	\
	};																				\
	DECLARE_INSTRUCTION(name, 128)

#define DECLARE_SINGLE_CANDIDATE_INSTRUCTION(name, default_width, operand_list, op_width, enc, opcode_data, ops...) \
	DECLARE_ENCODING_VARIANT(name, operand_list, op_width, enc, opcode_data, ops);	\
	DECLARE_INSTRUCTION(name, default_width)

#define DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(name, default_width, operand_list, op_width, enc, opcode_data, ops...) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] =							\
	{																				\
		DECLARE_PREFIXED_CANDIDATE(operand_list, op_width, enc, opcode_data, ops),	\
	};																				\
	DECLARE_INSTRUCTION(name, default_width)

#define DECLARE_SSE_AVX_PSPD_INSTRUCTION(name, ops...) 	\
	DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(name##PS, 32, (MatchXmm, MatchXmmMem128), 32, RM, 0, 0, ops);		\
	DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(name##PD, 64, (MatchXmm, MatchXmmMem128), 32, RM, 0, 0x66, ops);	\
	DECLARE_AVX_RVM_INSTRUCTION(V##name##PS, 0, ops);   															\
	DECLARE_AVX_RVM_INSTRUCTION(V##name##PD, 0x66, ops); 															\

#define DECLARE_SSE_PSPDSSSD_INSTRUCTION(name, ops...) 	\
	DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(name##PS, 32, (MatchXmm, MatchXmmMem128), 32, RM, 0, 0, ops);		\
	DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(name##PD, 64, (MatchXmm, MatchXmmMem128), 32, RM, 0, 0x66, ops);	\
	DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(name##SS, 32, (MatchXmm, MatchXmmMem128), 32, RM, 0, 0xf3, ops);	\
	DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(name##SD, 64, (MatchXmm, MatchXmmMem128), 64, RM, 0, 0xf2, ops)

#define DECLARE_AVX_PSPDSSSD_INSTRUCTION(name, ops...) 	\
	DECLARE_AVX_RVM_INSTRUCTION(V##name##PS, 0, ops);   	\
	DECLARE_AVX_RVM_INSTRUCTION(V##name##PD, 0x66, ops); 	\
	DECLARE_AVX_RVM_INSTRUCTION(V##name##SS, 0xf3, ops); 	\
	DECLARE_AVX_RVM_INSTRUCTION(V##name##SD, 0xf2, ops)

#define DECLARE_SSE_AVX_PSPDSSSD_INSTRUCTION(name, ops...) 	\
	DECLARE_SSE_PSPDSSSD_INSTRUCTION(name, ops); \
	DECLARE_AVX_PSPDSSSD_INSTRUCTION(name, ops)

#define DECLARE_ALU_INSTRUCTION(name, opcode_data) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] = 								\
	{ 																					\
		DECLARE_CANDIDATE((MatchAL, MatchImm8), 8, I, 0, 8*opcode_data + 0x4), 			\
		DECLARE_CANDIDATE((MatchRM32, MatchImm8), 32, MI, opcode_data, 0x83),			\
		DECLARE_CANDIDATE((MatchRM64, MatchImm8), 64, MI, opcode_data, 0x83),			\
		DECLARE_CANDIDATE((MatchAX, MatchImm16), 16, I, 0, 0x66, 8*opcode_data + 0x5),	\
		DECLARE_CANDIDATE((MatchEAX, MatchImm32), 32, I, 0, 8*opcode_data + 0x5),		\
		DECLARE_CANDIDATE((MatchRAX, MatchImm32), 64, I, 0, 0x48, 8*opcode_data + 0x5),	\
		DECLARE_CANDIDATE((MatchRM8, MatchImm8), 8, MI, opcode_data, 0x80),				\
		DECLARE_CANDIDATE((MatchRM16, MatchImm8), 16, MI, opcode_data, 0x83),			\
		DECLARE_CANDIDATE((MatchRM16, MatchImm16), 16, MI, opcode_data, 0x81),			\
		DECLARE_CANDIDATE((MatchRM32, MatchImm32), 32, MI, opcode_data, 0x81),			\
		DECLARE_CANDIDATE((MatchRM64, MatchImm32), 64, MI, opcode_data, 0x81),			\
		DECLARE_CANDIDATE((MatchRM8, MatchReg8), 8, MR, 0, 8*opcode_data),				\
		DECLARE_CANDIDATE((MatchRM16, MatchReg16), 16, MR, 0, 8*opcode_data + 0x1),		\
		DECLARE_CANDIDATE((MatchRM32, MatchReg32), 32, MR, 0, 8*opcode_data + 0x1),		\
		DECLARE_CANDIDATE((MatchRM64, MatchReg64), 64, MR, 0, 8*opcode_data + 0x1),		\
		DECLARE_CANDIDATE((MatchReg8, MatchRM8), 8, RM, 0, 8*opcode_data + 0x2),		\
		DECLARE_CANDIDATE((MatchReg16, MatchRM16), 16, RM, 0, 8*opcode_data + 0x3),		\
		DECLARE_CANDIDATE((MatchReg32, MatchRM32), 32, RM, 0, 8*opcode_data + 0x3),		\
		DECLARE_CANDIDATE((MatchReg64, MatchRM64), 64, RM, 0, 8*opcode_data + 0x3),		\
	};																					\
	DECLARE_INSTRUCTION(name, 32)

#define DECLARE_SHIFT_INSTRUCTION(name, opcode_data) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] = 			\
	{ 																				\
		DECLARE_CANDIDATE((MatchRM8, MatchConstant1), 8, M, opcode_data, 0xd0), 	\
		DECLARE_CANDIDATE((MatchRM8, MatchCL), 8, M, opcode_data, 0xd2),		 	\
		DECLARE_CANDIDATE((MatchRM8, MatchImm8), 8, MI, opcode_data, 0xc0),	 		\
		DECLARE_CANDIDATE((MatchRM16, MatchConstant1), 16, M, opcode_data, 0xd1), 	\
		DECLARE_CANDIDATE((MatchRM16, MatchCL), 16, M, opcode_data, 0xd3),		 	\
		DECLARE_CANDIDATE((MatchRM16, MatchImm8), 16, MI, opcode_data, 0xc1),	 	\
		DECLARE_CANDIDATE((MatchRM32, MatchConstant1), 32, M, opcode_data, 0xd1), 	\
		DECLARE_CANDIDATE((MatchRM32, MatchCL), 32, M, opcode_data, 0xd3),		 	\
		DECLARE_CANDIDATE((MatchRM32, MatchImm8), 32, MI, opcode_data, 0xc1),	 	\
		DECLARE_CANDIDATE((MatchRM64, MatchConstant1), 64, M, opcode_data, 0xd1), 	\
		DECLARE_CANDIDATE((MatchRM64, MatchCL), 64, M, opcode_data, 0xd3),		 	\
		DECLARE_CANDIDATE((MatchRM64, MatchImm8), 64, MI, opcode_data, 0xc1),	 	\
	};																				\
	DECLARE_INSTRUCTION(name, 32)

#define DECLARE_MATCH_RM_INSTRUCTION(name, opcode_data, ops...) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] =						\
	{																			\
		DECLARE_CANDIDATE((MatchReg16, MatchRM16), 16, RM, opcode_data, ops),	\
		DECLARE_CANDIDATE((MatchReg32, MatchRM32), 32, RM, opcode_data, ops),	\
		DECLARE_CANDIDATE((MatchReg64, MatchRM64), 64, RM, opcode_data, ops),	\
	};																	\
	DECLARE_INSTRUCTION(name, 32)

#define DECLARE_SINGLE_RM_PARAMETER_INSTRUCTION(name, opcode_data, ops...) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] =				\
	{																	\
		DECLARE_CANDIDATE((MatchRM8), 8, M, opcode_data, ops),			\
		DECLARE_CANDIDATE((MatchRM16), 16, M, opcode_data, ops+1),		\
		DECLARE_CANDIDATE((MatchRM32), 32, M, opcode_data, ops+1),		\
		DECLARE_CANDIDATE((MatchRM64), 64, M, opcode_data, ops+1),		\
	};																	\
	DECLARE_INSTRUCTION(name, 32)

#define DECLARE_BT_INSTRUCTION(name, opcode_data, op) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] =							\
	{																				\
		DECLARE_CANDIDATE((MatchRM16, MatchReg16), 16, MR, 0, 0x0f, op),			\
		DECLARE_CANDIDATE((MatchRM32, MatchReg32), 32, MR, 0, 0x0f, op),			\
		DECLARE_CANDIDATE((MatchRM64, MatchReg64), 64, MR, 0, 0x0f, op),			\
		DECLARE_CANDIDATE((MatchRM16, MatchImm8), 16, MI, opcode_data, 0x0f, 0xba),	\
		DECLARE_CANDIDATE((MatchRM32, MatchImm8), 32, MI, opcode_data, 0x0f, 0xba),	\
		DECLARE_CANDIDATE((MatchRM64, MatchImm8), 64, MI, opcode_data, 0x0f, 0xba),	\
	};																				\
	DECLARE_INSTRUCTION(name, 32)

#define DECLARE_CMOVCC_INSTRUCTION(index) \
	constexpr EncodingVariant CMOVCC##index##_ENCODING_VARIANTS[] = 				\
	{ 																				\
		DECLARE_CANDIDATE((MatchReg16, MatchRM16), 16, RM, 0, 0x0f, 0x40+index), 	\
		DECLARE_CANDIDATE((MatchReg32, MatchRM32), 32, RM, 0, 0x0f, 0x40+index), 	\
		DECLARE_CANDIDATE((MatchReg64, MatchRM64), 64, RM, 0, 0x0f, 0x40+index), 	\
	};																				\
	DECLARE_INSTRUCTION(CMOVCC##index, 32)

#define DECLARE_JCC_INSTRUCTION(index) \
	constexpr EncodingVariant JCC##index##_ENCODING_VARIANTS[] =		\
	{																	\
		DECLARE_CANDIDATE((MatchRel8), 0, D, 0, 0x70 + index),			\
		DECLARE_CANDIDATE((MatchRel32), 0, D, 0, 0x0f, 0x80 + index),	\
	};																	\
	DECLARE_INSTRUCTION(JCC##index, 0)

#define DECLARE_SSE_AVX_RVM_INSTRUCTION(name, ops...) \
	DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(name, 128, (MatchXmm, MatchXmmMem128), 128, RM, 0, ops); \
	DECLARE_AVX_RVM_INSTRUCTION(V##name, ops)

#define DECLARE_SSE_AVX_RM_INSTRUCTION(name, ops...) \
	DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(name, 128, (MatchXmm, MatchXmmMem128), 128, RM, 0, ops); \
	DECLARE_AVX_RM_INSTRUCTION(V##name, ops)

#define DECLARE_SSE_AVX_SHIFT_INSTRUCTION(name, op_data, ops...) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] =															\
	{																												\
		DECLARE_CANDIDATE((MatchMm, MatchMmMem64), 0, RM, 0, ops+0x50+8*op_data),									\
		DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128), 0, RM, 0, 0x66, ops+0x50+8*op_data),					\
		DECLARE_CANDIDATE((MatchMm, MatchImm8), 0, MI, op_data, ops),												\
		DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchImm8), 0, MI, op_data, 0x66, ops),								\
	};																												\
	DECLARE_INSTRUCTION(name, 0);																					\
	constexpr EncodingVariant V##name##_ENCODING_VARIANTS[] =														\
	{																												\
		DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmm, MatchXmmMem128), 128, RVM, 0, 0x66, ops+0x50+8*op_data),	\
		DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmm, MatchImm8), 128, VMI, op_data, 0x66, ops),					\
		DECLARE_PREFIXED_CANDIDATE((MatchYmm, MatchYmm, MatchYmmMem256), 256, RVM, 0, 0x66, ops+0x50+8*op_data),	\
		DECLARE_PREFIXED_CANDIDATE((MatchYmm, MatchYmm, MatchImm8), 256, VMI, op_data, 0x66, ops),					\
	};																												\
	DECLARE_INSTRUCTION(V##name, 128)

#define DECLARE_AVX_VMI_INSTRUCTION(name, op_data, ops...) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] =										\
	{																							\
		DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmm, MatchImm8), 128, VMI, op_data, ops),	\
		DECLARE_PREFIXED_CANDIDATE((MatchYmm, MatchYmm, MatchImm8), 256, VMI, op_data, ops),	\
	};																							\
	DECLARE_INSTRUCTION(name, 128)

#define DECLARE_AVX_RVMI_INSTRUCTION(name, op_data, ops...) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] =														\
	{																											\
		DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmm, MatchXmmMem128, MatchImm8), 128, RVMI, op_data, ops),	\
		DECLARE_PREFIXED_CANDIDATE((MatchYmm, MatchYmm, MatchYmmMem256, MatchImm8), 256, RVMI, op_data, ops),	\
	};																											\
	DECLARE_INSTRUCTION(name, 128)

#define DECLARE_AVX_vRMI_INSTRUCTION(name, op_data, ops...) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] =														\
	{																											\
		DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128, MatchImm8), 128, vRMI, op_data, ops),	\
		DECLARE_PREFIXED_CANDIDATE((MatchYmm, MatchYmmMem256, MatchImm8), 256, vRMI, op_data, ops),	\
	};																											\
	DECLARE_INSTRUCTION(name, 128)

#define DECLARE_SSE_AVX_VMI_INSTRUCTION(name, op_data, ops...) \
	DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(name, 128, (MatchXmm, MatchImm8), 128, MI, op_data, ops); \
	DECLARE_AVX_VMI_INSTRUCTION(V##name, op_data, ops)

#define DECLARE_SSE_AVX_vRMI_INSTRUCTION(name, op_data, ops...) \
	DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(name, 128, (MatchXmm, MatchXmmMem128, MatchImm8), 128, RMI, op_data, ops); \
	DECLARE_AVX_vRMI_INSTRUCTION(V##name, op_data, ops)

#define DECLARE_MMX_SSE_MI_INSTRUCTION(name, op_data, ops...) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] =								\
	{                                                                        			\
		DECLARE_CANDIDATE((MatchMm, MatchImm8), 0, MI, op_data, ops),                	\
		DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchImm8), 0, MI, op_data, 0x66, ops), 	\
	};																					\
	DECLARE_INSTRUCTION(name, 0)

#define DECLARE_MMX_SSE_RMI_INSTRUCTION(name, op_data, ops...) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] =												\
	{                                                                        							\
		DECLARE_CANDIDATE((MatchMm, MatchMmMem64, MatchImm8), 0, RMI, op_data, ops),               		\
		DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128, MatchImm8), 0, RMI, op_data, 0x66, ops), 	\
	};																									\
	DECLARE_INSTRUCTION(name, 0)

#define DECLARE_MMX_SSE_AVX_VMI_INSTRUCTION(name, op_data, ops...) \
	DECLARE_MMX_SSE_MI_INSTRUCTION(name, op_data, ops); \
	DECLARE_AVX_VMI_INSTRUCTION(V##name, op_data, 0x66, ops)

#define DECLARE_MMX_SSE_AVX_RVMI_INSTRUCTION(name, op_data, ops...) \
	DECLARE_MMX_SSE_RMI_INSTRUCTION(name, op_data, ops); \
	DECLARE_AVX_RVMI_INSTRUCTION(V##name, op_data, 0x66, ops)

#define DECLARE_SSE_AVX_RVMI_INSTRUCTION(name, op_data, ops...) \
	DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(name, 128, (MatchXmm, MatchXmmMem128, MatchImm8), 128, RMI, op_data, ops); \
	DECLARE_AVX_RVMI_INSTRUCTION(V##name, op_data, ops)

#define DECLARE_MMX_SSE_RM_INSTRUCTION(name, op_data, ops...) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] = 							 	\
	{																				 	\
		DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128), 0, RM, 0, 0x66, ops),	\
		DECLARE_CANDIDATE((MatchMm, MatchMmMem64), 0, RM, 0, ops), 						\
	};																				 	\
	DECLARE_INSTRUCTION(name, 0)

#define DECLARE_MMX_SSE_AVX_INSTRUCTION(name, op_data, ops...) \
	DECLARE_MMX_SSE_RM_INSTRUCTION(name, op_data, ops); \
	DECLARE_AVX_RVM_INSTRUCTION(V##name, 0x66, ops)

#define DECLARE_MMX_SSE_AVX_RM_INSTRUCTION(name, op_data, ops...) \
	DECLARE_MMX_SSE_RM_INSTRUCTION(name, op_data, ops); \
	DECLARE_AVX_RM_INSTRUCTION(V##name, 0x66, ops)

#define DECLARE_SETCC_INSTRUCTION(index) \
	DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SETCC##index, 0, (MatchRM8), 8, M, 0, 0x0f, 0x90+index)

#define DECLARE_CC_INSTRUCTION(MACRO_NAME) \
	MACRO_NAME(0); \
	MACRO_NAME(1); \
	MACRO_NAME(2); \
	MACRO_NAME(3); \
	MACRO_NAME(4); \
	MACRO_NAME(5); \
	MACRO_NAME(6); \
	MACRO_NAME(7); \
	MACRO_NAME(8); \
	MACRO_NAME(9); \
	MACRO_NAME(10); \
	MACRO_NAME(11); \
	MACRO_NAME(12); \
	MACRO_NAME(13); \
	MACRO_NAME(14); \
	MACRO_NAME(15)

#define DECLARE_FPU_INSTRUCTION(name, data) \
	constexpr EncodingVariant F##name##_ENCODING_VARIANTS[] = 					\
	{																			\
		DECLARE_CANDIDATE((), 0, ZO, 0, 0xde, 0xc1 + 8*data),					\
		DECLARE_CANDIDATE((MatchMem32), 0, M, data, 0xd8),						\
		DECLARE_CANDIDATE((MatchMem64), 0, M, data, 0xdc),						\
		DECLARE_CANDIDATE((MatchST), 0, O, 0, 0xd8, 0xc0 + 8*data),				\
		DECLARE_CANDIDATE((MatchST0, MatchST), 0, O2, 0, 0xd8, 0xc0 + 8*data),	\
		DECLARE_CANDIDATE((MatchST, MatchST0), 0, O, 0, 0xdc, 0xc0 + 8*data),	\
	};																			\
	DECLARE_INSTRUCTION(F##name, 0);											\
	constexpr EncodingVariant F##name##P_ENCODING_VARIANTS[] = 					\
	{																			\
		DECLARE_CANDIDATE((), 0, ZO, 0, 0xde, 0xc1 + 8*data),					\
		DECLARE_CANDIDATE((MatchST), 0, O, 0, 0xde, 0xc0 + 8*data),				\
		DECLARE_CANDIDATE((MatchST, MatchST0), 0, O, 0, 0xde, 0xc0 + 8*data),	\
	};																			\
	DECLARE_INSTRUCTION(F##name##P, 0);											\
	constexpr EncodingVariant FI##name##_ENCODING_VARIANTS[] = 					\
	{																			\
		DECLARE_CANDIDATE((MatchMem32), 0, M, data, 0xda),						\
		DECLARE_CANDIDATE((MatchMem16), 0, M, data, 0xde),						\
	};																			\
	DECLARE_INSTRUCTION(FI##name, 0)

//============================================================================

DECLARE_ALU_INSTRUCTION(ADC, 2);

constexpr EncodingVariant ADCX_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchRM32), 32, RM, 0, 0x66, 0x0f, 0x38, 0xf6),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchRM64), 64, RM, 0, 0x66, 0x0f, 0x38, 0xf6),
};
DECLARE_INSTRUCTION(ADCX, 32);

DECLARE_ALU_INSTRUCTION(ADD, 0);
DECLARE_SSE_AVX_PSPDSSSD_INSTRUCTION(ADD, 0x0f, 0x58);
DECLARE_SSE_AVX_RVM_INSTRUCTION(ADDSUBPD, 0x66, 0x0f, 0xd0);
DECLARE_SSE_AVX_RVM_INSTRUCTION(ADDSUBPS, 0xf2, 0x0f, 0xd0);

constexpr EncodingVariant ADOX_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchRM32), 32, RM, 0, 0xf3, 0x0f, 0x38, 0xf6),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchRM64), 64, RM, 0, 0xf3, 0x0f, 0x38, 0xf6),
};
DECLARE_INSTRUCTION(ADOX, 32);

DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(AESDEC, 128, (MatchXmm, MatchXmmMem128), 128, RM, 0, 0x66, 0x0f, 0x38, 0xde);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(AESDECLAST, 128, (MatchXmm, MatchXmmMem128), 128, RM, 0, 0x66, 0x0f, 0x38, 0xdf);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(AESENC, 128, (MatchXmm, MatchXmmMem128), 128, RM, 0, 0x66, 0x0f, 0x38, 0xdc);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(AESENCLAST, 128, (MatchXmm, MatchXmmMem128), 128, RM, 0, 0x66, 0x0f, 0x38, 0xdd);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(AESIMC, 128, (MatchXmm, MatchXmmMem128), 128, RM, 0, 0x66, 0x0f, 0x38, 0xdb);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(AESKEYGENASSIST, 128, (MatchXmm, MatchXmmMem128, MatchImm8), 128, RMI, 0, 0x66, 0x0f, 0x3a, 0xdf);

DECLARE_ALU_INSTRUCTION(AND, 4);
DECLARE_SSE_AVX_PSPD_INSTRUCTION(AND, 0x0f, 0x54);

constexpr EncodingVariant ANDN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32, MatchReg32, MatchRM32), 32, RVM, 0, 0x0f, 0x38, 0xf2),
	DECLARE_CANDIDATE((MatchReg64, MatchReg64, MatchRM64), 64, RVM, 0x10, 0x0f, 0x38, 0xf2),
};
DECLARE_INSTRUCTION(ANDN, 32);

DECLARE_SSE_AVX_PSPD_INSTRUCTION(ANDN, 0x0f, 0x55);

constexpr EncodingVariant BEXTR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32, MatchRM32, MatchReg32), 32, RMV, 0, 0x0f, 0x38, 0xf7),
	DECLARE_CANDIDATE((MatchReg64, MatchRM64, MatchReg64), 64, RMV, 0x10, 0x0f, 0x38, 0xf7),
};
DECLARE_INSTRUCTION(BEXTR, 32);

constexpr EncodingVariant BLSI_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32, MatchRM32), 32, VM, 3, 0x0f, 0x38, 0xf3),
	DECLARE_CANDIDATE((MatchReg64, MatchRM64), 64, VM, 0x13, 0x0f, 0x38, 0xf3),
};
DECLARE_INSTRUCTION(BLSI, 32);

constexpr EncodingVariant BLSMSK_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32, MatchRM32), 32, VM, 2, 0x0f, 0x38, 0xf3),
	DECLARE_CANDIDATE((MatchReg64, MatchRM64), 64, VM, 0x12, 0x0f, 0x38, 0xf3),
};
DECLARE_INSTRUCTION(BLSMSK, 32);

constexpr EncodingVariant BLSR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32, MatchRM32), 32, VM, 1, 0x0f, 0x38, 0xf3),
	DECLARE_CANDIDATE((MatchReg64, MatchRM64), 64, VM, 0x11, 0x0f, 0x38, 0xf3),
};
DECLARE_INSTRUCTION(BLSR, 32);

DECLARE_MATCH_RM_INSTRUCTION(BSF, 0, 0x0f, 0xbc);
DECLARE_MATCH_RM_INSTRUCTION(BSR, 0, 0x0f, 0xbd);

constexpr EncodingVariant BSWAP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32), 32, O, 0, 0x0f, 0xc8),
	DECLARE_CANDIDATE((MatchReg64), 64, O, 0, 0x0f, 0xc8),
};
DECLARE_INSTRUCTION(BSWAP, 32);

DECLARE_BT_INSTRUCTION(BT, 4, 0xa3);
DECLARE_BT_INSTRUCTION(BTC, 7, 0xbb);
DECLARE_BT_INSTRUCTION(BTR, 6, 0xb3);
DECLARE_BT_INSTRUCTION(BTS, 5, 0xab);

constexpr EncodingVariant BZHI_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32, MatchRM32, MatchReg32), 32, RMV, 0, 0x0f, 0x38, 0xf5),
	DECLARE_CANDIDATE((MatchReg64, MatchRM64, MatchReg64), 64, RMV, 0x10, 0x0f, 0x38, 0xf5),
};
DECLARE_INSTRUCTION(BZHI, 32);

constexpr EncodingVariant CALL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchRel32), 0, D, 0, 0xe8),
	DECLARE_CANDIDATE((MatchRM64), 64, M, 2, 0xff),
};
DECLARE_INSTRUCTION(CALL, 64);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CBW, 0, (), 0, ZO, 0, 0x66, 0x98);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CDQ, 0, (), 0, ZO, 0, 0x99);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CDQE, 0, (), 0, ZO, 0, 0x48, 0x98);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CLC, 0, (), 0, ZO, 0, 0xf8);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CLFLUSH, 0, (MatchMem8), 8, M, 7, 0x0f, 0xae);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CLFLUSHOPT, 0, (MatchMem8), 8, M, 7, 0x66, 0x0f, 0xae);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CLD, 0, (), 0, ZO, 0, 0xfc);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CLI, 0, (), 0, ZO, 0, 0xfa);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CLWB, 0, (MatchMem8), 8, M, 6, 0x66, 0x0f, 0xae);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMC, 0, (), 0, ZO, 0, 0xf5);
DECLARE_CC_INSTRUCTION(DECLARE_CMOVCC_INSTRUCTION);
DECLARE_ALU_INSTRUCTION(CMP, 7);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPEQPD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 0, 0x66, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPEQSD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 0, 0xf2, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMPEQPS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 0, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPEQSS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 0, 0xf3, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPLEPD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 2, 0x66, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPLESD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 2, 0xf2, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMPLEPS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 2, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPLESS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 2, 0xf3, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPLTPD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 1, 0x66, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPLTSD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 1, 0xf2, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMPLTPS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 1, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPLTSS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 1, 0xf3, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPNEQPD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 4, 0x66, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPNEQSD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 4, 0xf2, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMPNEQPS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 4, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPNEQSS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 4, 0xf3, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPNLEPD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 6, 0x66, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPNLESD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 6, 0xf2, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMPNLEPS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 6, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPNLESS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 6, 0xf3, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPNLTPD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 5, 0x66, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPNLTSD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 5, 0xf2, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMPNLTPS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 5, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPNLTSS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 5, 0xf3, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPORDPD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 7, 0x66, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPORDSD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 7, 0xf2, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMPORDPS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 7, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPORDSS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 7, 0xf3, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPPD, 0, (MatchXmm, MatchXmmMem128, MatchImm8), 0, RMI, 0, 0x66, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMPPS, 0, (MatchXmm, MatchXmmMem128, MatchImm8), 0, RMI, 0, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMPSB, 0, (), 0, ZO, 0, 0xa6);

constexpr EncodingVariant CMPSD_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((), 0, ZO, 0, 0xa7),
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128, MatchImm8), 32, MRI, 0, 0xf2, 0x0f, 0xc2),
};
DECLARE_INSTRUCTION(CMPSD, 32);

DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPSS, 0, (MatchXmm, MatchXmmMem128, MatchImm8), 0, RMI, 0, 0xf3, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMPSW, 0, (), 0, ZO, 0, 0x66, 0xa7);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMPSQ, 0, (), 0, ZO, 0, 0x48, 0xa7);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPUNORDPD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 3, 0x66, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPUNORDSD, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 3, 0xf2, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMPUNORDPS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 3, 0x0f, 0xc2);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CMPUNORDSS, 0, (MatchXmm, MatchXmmMem128), 0, RMX, 3, 0xf3, 0x0f, 0xc2);

constexpr EncodingVariant CMPXCHG_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchRM8, MatchReg8), 8, MR, 0, 0x0f, 0xb0),
	DECLARE_CANDIDATE((MatchRM16, MatchReg16), 16, MR, 0, 0x0f, 0xb1),
	DECLARE_CANDIDATE((MatchRM32, MatchReg32), 32, MR, 0, 0x0f, 0xb1),
	DECLARE_CANDIDATE((MatchRM64, MatchReg64), 64, MR, 0, 0x0f, 0xb1),
};
DECLARE_INSTRUCTION(CMPXCHG, 32);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMPXCHG8B, 64, (MatchMem64), 64, M, 1, 0x0f, 0xc7);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CMPXCHG16B, 32, (MatchMem128), 64, M, 1, 0x0f, 0xc7);

DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(COMISD, 128, (MatchXmm, MatchXmm|MatchMem64), 128, RM, 0, 0x66, 0x0f, 0x2f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(COMISS, 128, (MatchXmm, MatchXmm|MatchMem32), 128, RM, 0, 0x0f, 0x2f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CPUID, 0, (), 0, ZO, 0, 0x0f, 0xa2);

constexpr EncodingVariant CRC32_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchRM8), 8, RM, 0, 0xf2, 0x0f, 0x38, 0xf0),
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchRM16), 16, RM, 0, 0xf2, 0x0f, 0x38, 0xf1),
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchRM32), 32, RM, 0, 0xf2, 0x0f, 0x38, 0xf1),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchRM8), 8, RM, 0, 0xf2, 0x0f, 0x38, 0xf0),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchRM64), 64, RM, 0, 0xf2, 0x0f, 0x38, 0xf1),
};
DECLARE_INSTRUCTION(CRC32, 32);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CQO, 0, (), 0, ZO, 0, 0x48, 0x99);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CVTDQ2PD, 0, (MatchXmm, MatchXmm|MatchMem64), 0, RM, 0, 0xf3, 0x0f, 0xe6);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CVTDQ2PS, 0, (MatchXmm, MatchXmmMem128), 0, RM, 0, 0x0f, 0x5b );
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CVTPD2DQ, 0, (MatchXmm, MatchXmm|MatchMem128), 0, RM, 0, 0xf2, 0x0f, 0xe6);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CVTPD2PI, 0, (MatchMm, MatchXmmMem128), 0, RM, 0, 0x66, 0x0f, 0x2d);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CVTPD2PS, 0, (MatchXmm, MatchXmmMem128), 0, RM, 0, 0x66, 0x0f, 0x5a);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CVTPI2PD, 0, (MatchXmm, MatchMmMem64), 0, RM, 0, 0x66, 0x0f, 0x2a);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CVTPI2PS, 0, (MatchXmm, MatchMmMem64), 0, RM, 0, 0x0f, 0x2a);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CVTPS2DQ, 0, (MatchXmm, MatchXmmMem128), 0, RM, 0, 0x66, 0x0f, 0x5b);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CVTPS2PD, 0, (MatchXmm, MatchXmmMem128), 0, RM, 0, 0x0f, 0x5a);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CVTPS2PI, 0, (MatchMm, MatchXmm|MatchMem64), 0, RM, 0, 0x0f, 0x2d);

constexpr EncodingVariant CVTSD2SI_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchXmm|MatchMem64), 32, RM, 0, 0xf2, 0x0f, 0x2d),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchXmm|MatchMem64), 64, RM, 0, 0xf2, 0x0f, 0x2d),
};
DECLARE_INSTRUCTION(CVTSD2SI, 32);

DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CVTSD2SS, 0, (MatchXmm, MatchXmm|MatchMem64), 0, RM, 0, 0xf2, 0x0f, 0x5a);

constexpr EncodingVariant CVTSI2SD_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchRM32), 32, RM, 0, 0xf2, 0x0f, 0x2a),
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchRM64), 64, RM, 0, 0xf2, 0x0f, 0x2a),
};
DECLARE_INSTRUCTION(CVTSI2SD, 32);

constexpr EncodingVariant CVTSI2SS_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchRM32), 32, RM, 0, 0xf3, 0x0f, 0x2a),
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchRM64), 64, RM, 0, 0xf3, 0x0f, 0x2a),
};
DECLARE_INSTRUCTION(CVTSI2SS, 32);

DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(CVTSS2SD, 0, (MatchXmm, MatchXmm|MatchMem32), 0, RM, 0, 0xf3, 0x0f, 0x5a);

constexpr EncodingVariant CVTSS2SI_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchXmm|MatchMem32), 32, RM, 0, 0xf3, 0x0f, 0x2d),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchXmm|MatchMem64), 64, RM, 0, 0xf3, 0x0f, 0x2d),
};
DECLARE_INSTRUCTION(CVTSS2SI, 32);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CWD, 0, (), 0, ZO, 0, 0x66, 0x99);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CWDE, 0, (), 0, ZO, 0, 0x98);
DECLARE_SINGLE_RM_PARAMETER_INSTRUCTION(DEC, 1, 0xfe);
DECLARE_SINGLE_RM_PARAMETER_INSTRUCTION(DIV, 6, 0xf6);
DECLARE_SSE_AVX_PSPDSSSD_INSTRUCTION(DIV, 0x0f, 0x5E);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(EMMS, 0, (), 0, ZO, 0, 0x0f, 0x77);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(ENTER, 0, (MatchImm16, MatchImm8), 0, II, 0, 0xc8);
DECLARE_FPU_INSTRUCTION(ADD, 0);	// FADD variants
DECLARE_FPU_INSTRUCTION(DIV, 6);	// FDIV variants
DECLARE_FPU_INSTRUCTION(DIVR, 7);	// FDIVR variants
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(F2XM1, 0, (), 0, ZO, 0, 0xd9, 0xf0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FABS, 0, (), 0, ZO, 0, 0xd9, 0xe1);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FCHS, 0, (), 0, ZO, 0, 0xd9, 0xe0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FCLEX, 0, (), 0, ZO, 0, 0x9b, 0xdb, 0xe2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FCOS, 0, (), 0, ZO, 0, 0xd9, 0xff);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FDECSTP, 0, (), 0, ZO, 0, 0xd9, 0xf6);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FFREE, 0, (MatchST), 0, O, 0, 0xdd, 0xc0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FINCSTP, 0, (), 0, ZO, 0, 0xd9, 0xf7);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FINIT, 0, (), 0, ZO, 0, 0x9b, 0xdb, 0xe3);

constexpr EncodingVariant FLD_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchMem32), 32, M, 0, 0xd9),
	DECLARE_CANDIDATE((MatchMem64), 32, M, 0, 0xdd),
	DECLARE_CANDIDATE((MatchMem80), 32, M, 5, 0xdb),
	DECLARE_CANDIDATE((MatchST), 32, O, 0, 0xd9, 0xc0),
};
DECLARE_INSTRUCTION(FLD, 32);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FLD1, 0, (), 0, ZO, 0, 0xd9, 0xe8);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FLDL2T, 0, (), 0, ZO, 0, 0xd9, 0xe9);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FLDL2E, 0, (), 0, ZO, 0, 0xd9, 0xea);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FLDPI, 0, (), 0, ZO, 0, 0xd9, 0xeb);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FLDLG2, 0, (), 0, ZO, 0, 0xd9, 0xec);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FLDLN2, 0, (), 0, ZO, 0, 0xd9, 0xed);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FLDZ, 0, (), 0, ZO, 0, 0xd9, 0xee);
DECLARE_FPU_INSTRUCTION(MUL, 1);	// FMUL variants
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FNCLEX, 0, (), 0, ZO, 0, 0xdb, 0xe2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FNINIT, 0, (), 0, ZO, 0, 0xdb, 0xe3);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FNOP, 0, (), 0, ZO, 0, 0xd9, 0xd0);

constexpr EncodingVariant FNSTSW_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchAX), 32, ZO, 0, 0x9b, 0xdf, 0xe0),
	DECLARE_PREFIXED_CANDIDATE((MatchMem16), 32, M, 7, 0x9b, 0xdd),
};
DECLARE_INSTRUCTION(FNSTSW, 32);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FPATAN, 0, (), 0, ZO, 0, 0xd9, 0xf3);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FPREM, 0, (), 0, ZO, 0, 0xd9, 0xf8);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FPREM1, 0, (), 0, ZO, 0, 0xd9, 0xf5);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FPTAN, 0, (), 0, ZO, 0, 0xd9, 0xf2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FRNDINT, 0, (), 0, ZO, 0, 0xd9, 0xfc);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FSCALE, 0, (), 0, ZO, 0, 0xd9, 0xfd);

constexpr EncodingVariant FST_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchMem32), 32, M, 2, 0xd9),
	DECLARE_CANDIDATE((MatchMem64), 32, M, 2, 0xdd),
	DECLARE_CANDIDATE((MatchST), 32, O, 0, 0xdd, 0xd0),
};
DECLARE_INSTRUCTION(FST, 32);

constexpr EncodingVariant FSTP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchMem32), 32, M, 3, 0xd9),
	DECLARE_CANDIDATE((MatchMem64), 32, M, 3, 0xdd),
	DECLARE_CANDIDATE((MatchMem80), 32, M, 7, 0xdb),
	DECLARE_CANDIDATE((MatchST), 32, O, 0, 0xdd, 0xd8),
};
DECLARE_INSTRUCTION(FSTP, 32);

constexpr EncodingVariant FSTSW_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchAX), 32, ZO, 0, 0xdf, 0xe0),
	DECLARE_CANDIDATE((MatchMem16), 32, M, 7, 0xdd),
};
DECLARE_INSTRUCTION(FSTSW, 32);


DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FSIN, 0, (), 0, ZO, 0, 0xd9, 0xfe);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FSINCOS, 0, (), 0, ZO, 0, 0xd9, 0xfb);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FSQRT, 0, (), 0, ZO, 0, 0xd9, 0xfa);
DECLARE_FPU_INSTRUCTION(SUB, 4);	// FSUB variants
DECLARE_FPU_INSTRUCTION(SUBR, 5);	// FSUBR variants
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FTST, 0, (), 0, ZO, 0, 0xd9, 0xe4);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FWAIT, 0, (), 0, ZO, 0, 0x9b);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FXAM, 0, (), 0, ZO, 0, 0xd9, 0xe5);

constexpr EncodingVariant FXCH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((), 0, ZO, 0, 0xd9, 0xc9),
	DECLARE_CANDIDATE((MatchST), 0, O, 0, 0xd9, 0xc8),
};
DECLARE_INSTRUCTION(FXCH, 32);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FXTRACT, 0, (), 0, ZO, 0, 0xd9, 0xf4);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FYL2X, 0, (), 0, ZO, 0, 0xd9, 0xf1);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FYL2XP1, 0, (), 0, ZO, 0, 0xd9, 0xf9);
DECLARE_SSE_AVX_RVM_INSTRUCTION(HADDPD, 0x66, 0x0f, 0x7c);
DECLARE_SSE_AVX_RVM_INSTRUCTION(HADDPS, 0xf2, 0x0f, 0x7c);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(HLT, 0, (), 0, ZO, 0, 0xf4);
DECLARE_SSE_AVX_RVM_INSTRUCTION(HSUBPD, 0x66, 0x0f, 0x7d);
DECLARE_SSE_AVX_RVM_INSTRUCTION(HSUBPS, 0xf2, 0x0f, 0x7d);
DECLARE_SINGLE_RM_PARAMETER_INSTRUCTION(IDIV, 7, 0xf6);

constexpr EncodingVariant IMUL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchRM8), 8, M, 5, 0xf6),
	DECLARE_CANDIDATE((MatchRM16), 16, M, 5, 0xf7),
	DECLARE_CANDIDATE((MatchRM32), 32, M, 5, 0xf7),
	DECLARE_CANDIDATE((MatchRM64), 64, M, 5, 0xf7),
	DECLARE_CANDIDATE((MatchReg16, MatchRM16), 16, RM, 0, 0x0f, 0xaf),
	DECLARE_CANDIDATE((MatchReg32, MatchRM32), 32, RM, 0, 0x0f, 0xaf),
	DECLARE_CANDIDATE((MatchReg64, MatchRM64), 64, RM, 0, 0x0f, 0xaf),
	DECLARE_CANDIDATE((MatchReg16, MatchRM16, MatchImm8), 16, RMI, 0, 0x6b),
	DECLARE_CANDIDATE((MatchReg32, MatchRM32, MatchImm8), 32, RMI, 0, 0x6b),
	DECLARE_CANDIDATE((MatchReg64, MatchRM64, MatchImm8), 64, RMI, 0, 0x6b),
	DECLARE_CANDIDATE((MatchReg16, MatchRM16, MatchImm16), 16, RMI, 0, 0x69),
	DECLARE_CANDIDATE((MatchReg32, MatchRM32, MatchImm32), 32, RMI, 0, 0x69),
	DECLARE_CANDIDATE((MatchReg64, MatchRM64, MatchImm32), 64, RMI, 0, 0x69),
};
DECLARE_INSTRUCTION(IMUL, 32);

DECLARE_SINGLE_RM_PARAMETER_INSTRUCTION(INC, 0, 0xfe);

constexpr EncodingVariant INT_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchConstant1), 0, ZO, 0, 0xf1),
	DECLARE_CANDIDATE((MatchConstant3), 0, ZO, 0, 0xcc),
	DECLARE_CANDIDATE((MatchImm8), 0, I, 0, 0xcd),
};
DECLARE_INSTRUCTION(INT, 0);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(INT1, 0, (), 0, ZO, 0, 0xf1);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(INT3, 0, (), 0, ZO, 0, 0xcc);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(INVD, 0, (), 0, ZO, 0, 0x0f, 0x08);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(INVLPG, 0, (MatchMemAll), 0, M, 7, 0x0f, 0x01);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(IRET, 0, (), 0, ZO, 0, 0x66, 0xcf);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(IRETD, 0, (), 0, ZO, 0, 0xcf);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(IRETQ, 0, (), 0, ZO, 0, 0x48, 0xcf);
DECLARE_CC_INSTRUCTION(DECLARE_JCC_INSTRUCTION);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(JECXZ, 0, (MatchRel8), 0, D, 0, 0x67, 0xe3);

constexpr EncodingVariant JMP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchRel8), 0, D, 0, 0xeb),
	DECLARE_CANDIDATE((MatchRel32), 0, D, 0, 0xe9),
	DECLARE_CANDIDATE((MatchRM64), 64, M, 4, 0xff),
};
DECLARE_INSTRUCTION(JMP, 64);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(JRCXZ, 0, (MatchRel8), 0, D, 0, 0xe3);

constexpr EncodingVariant LEA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg16, MatchMem16), 16, RM, 0, 0x8d),
	DECLARE_CANDIDATE((MatchReg32, MatchMem32), 32, RM, 0, 0x8d),
	DECLARE_CANDIDATE((MatchReg64, MatchMem64), 64, RM, 0, 0x8d),
};
DECLARE_INSTRUCTION(LEA, 32);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LEAVE, 0, (), 0, ZO, 0, 0xc9);
DECLARE_SSE_AVX_RM_INSTRUCTION(LDDQU, 0xf2, 0x0f, 0xf0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LFENCE, 0, (), 0, ZO, 0, 0x0f, 0xae, 0xe8);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LOCK, -1, (), 0, ZO, 0, 0xf0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LODSB, 0, (), 0, ZO, 0, 0xac);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LODSW, 0, (), 0, ZO, 0, 0x66, 0xad);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LODSD, 0, (), 0, ZO, 0, 0xad);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LODSQ, 0, (), 0, ZO, 0, 0x48, 0xad);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LOOP, 0, (MatchRel8), 0, D, 0, 0xe2);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LOOPE, 0, (MatchRel8), 0, D, 0, 0xe1);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LOOPNE, 0, (MatchRel8), 0, D, 0, 0xe0);

constexpr EncodingVariant LZCNT_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg16, MatchRM16), 16, RM, 0, 0xf3, 0x0f, 0xbd),
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchRM32), 32, RM, 0, 0xf3, 0x0f, 0xbd),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchRM64), 64, RM, 0, 0xf3, 0x0f, 0xbd),
};
DECLARE_INSTRUCTION(LZCNT, 32);

DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(MASKMOVDQU, 0, (MatchXmm, MatchXmm), 0, RM, 0, 0x66, 0x0f, 0xf7);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MASKMOVQ, 0, (MatchMm, MatchMm), 0, RM, 0, 0x0f, 0xf7);
DECLARE_SSE_AVX_PSPDSSSD_INSTRUCTION(MAX, 0x0f, 0x5F);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MFENCE, 0, (), 0, ZO, 0, 0x0f, 0xae, 0xf0);
DECLARE_SSE_AVX_PSPDSSSD_INSTRUCTION(MIN, 0x0f, 0x5D);

constexpr EncodingVariant MOV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg8, MatchRM8), 8, RM, 0, 0x8a),
	DECLARE_CANDIDATE((MatchReg16, MatchRM16), 16, RM, 0, 0x8b),
	DECLARE_CANDIDATE((MatchReg32, MatchRM32), 32, RM, 0, 0x8b),
	DECLARE_CANDIDATE((MatchReg64, MatchRM64), 64, RM, 0, 0x8b),
	DECLARE_CANDIDATE((MatchRM8, MatchReg8), 8, MR, 0, 0x88),
	DECLARE_CANDIDATE((MatchRM16, MatchReg16), 16, MR, 0, 0x89),
	DECLARE_CANDIDATE((MatchRM32, MatchReg32), 32, MR, 0, 0x89),
	DECLARE_CANDIDATE((MatchRM64, MatchReg64), 64, MR, 0, 0x89),
	DECLARE_CANDIDATE((MatchReg8, MatchImm8), 8, OI, 0, 0xb0),
	DECLARE_CANDIDATE((MatchReg16, MatchImm16), 16, OI, 0, 0xb8),
	DECLARE_CANDIDATE((MatchReg32, MatchImm32), 32, OI, 0, 0xb8),
	DECLARE_CANDIDATE((MatchReg64, MatchUImm32), 32, OI, 0, 0xb8),
	DECLARE_CANDIDATE((MatchRM8, MatchImm8), 8, MI, 0, 0xc6),
	DECLARE_CANDIDATE((MatchRM16, MatchImm16), 16, MI, 0, 0xc7),
	DECLARE_CANDIDATE((MatchRM32, MatchImm32), 32, MI, 0, 0xc7),
	DECLARE_CANDIDATE((MatchRM64, MatchImm32), 64, MI, 0, 0xc7),
	DECLARE_CANDIDATE((MatchReg64, MatchImm64), 64, OI, 0, 0xb8),
	DECLARE_CANDIDATE((MatchAL, MatchDirectMem8), 8, FD, 0, 0xa0),
	DECLARE_CANDIDATE((MatchAX, MatchDirectMem16), 16, FD, 0, 0xa1),
	DECLARE_CANDIDATE((MatchEAX, MatchDirectMem32), 32, FD, 0, 0xa1),
	DECLARE_CANDIDATE((MatchRAX, MatchDirectMem64), 64, FD, 0, 0xa1),
	DECLARE_CANDIDATE((MatchDirectMem8, MatchAL), 8, TD, 0, 0xa2),
	DECLARE_CANDIDATE((MatchDirectMem16, MatchAX), 16, TD, 0, 0xa3),
	DECLARE_CANDIDATE((MatchDirectMem32, MatchEAX), 32, TD, 0, 0xa3),
	DECLARE_CANDIDATE((MatchDirectMem64, MatchRAX), 64, TD, 0, 0xa3),
};
DECLARE_INSTRUCTION(MOV, 32);

constexpr EncodingVariant MOVAPS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchXmm, MatchXmmMem128), 32, RM, 0, 0x0f, 0x28),
	DECLARE_CANDIDATE((MatchXmmMem128, MatchXmm), 32, MR, 0, 0x0f, 0x29),
};
DECLARE_INSTRUCTION(MOVAPS, 32);

constexpr EncodingVariant MOVAPD_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128), 64, RM, 0, 0x66, 0x0f, 0x28),
	DECLARE_PREFIXED_CANDIDATE((MatchXmmMem128, MatchXmm), 64, MR, 0, 0x66, 0x0f, 0x29),
};
DECLARE_INSTRUCTION(MOVAPD, 64);

constexpr EncodingVariant MOVBE_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg16, MatchRM16), 16, RM, 0, 0x0f, 0x38, 0xf0),
	DECLARE_CANDIDATE((MatchReg32, MatchRM32), 32, RM, 0, 0x0f, 0x38, 0xf0),
	DECLARE_CANDIDATE((MatchReg64, MatchRM64), 64, RM, 0, 0x0f, 0x38, 0xf0),
	DECLARE_CANDIDATE((MatchRM16, MatchReg16), 16, MR, 0, 0x0f, 0x38, 0xf1),
	DECLARE_CANDIDATE((MatchRM32, MatchReg32), 32, MR, 0, 0x0f, 0x38, 0xf1),
	DECLARE_CANDIDATE((MatchRM64, MatchReg64), 64, MR, 0, 0x0f, 0x38, 0xf1),
};
DECLARE_INSTRUCTION(MOVBE, 32);

constexpr EncodingVariant MOVD_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchRM32), 32, RM, 0, 0x66, 0x0f, 0x6e),
	DECLARE_PREFIXED_CANDIDATE((MatchRM32, MatchXmm), 32, MR, 0, 0x66, 0x0f, 0x7e),
	DECLARE_CANDIDATE((MatchMm, MatchRM32), 32, RM, 0, 0x0f, 0x6e),
	DECLARE_CANDIDATE((MatchRM32, MatchMm), 32, MR, 0, 0x0f, 0x7e),
};
DECLARE_INSTRUCTION(MOVD, 32);

DECLARE_SSE_AVX_RM_INSTRUCTION(MOVDDUP, 0xf2, 0x0f, 0x12); // This should be Mem64 for Xmm theoretically
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(MOVDQ2Q, 128, (MatchMm, MatchXmm), 128, RM, 0, 0xf2, 0x0f, 0xd6);

constexpr EncodingVariant MOVDQA_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128), 128, RM, 0, 0x66, 0x0f, 0x6f),
	DECLARE_PREFIXED_CANDIDATE((MatchXmmMem128, MatchXmm), 128, MR, 0, 0x66, 0x0f, 0x7f),
};
DECLARE_INSTRUCTION(MOVDQA, 128);

constexpr EncodingVariant MOVDQU_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128), 128, RM, 0, 0xf3, 0x0f, 0x6f),
	DECLARE_PREFIXED_CANDIDATE((MatchXmmMem128, MatchXmm), 128, MR, 0, 0xf3, 0x0f, 0x7f),
};
DECLARE_INSTRUCTION(MOVDQU, 128);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MOVHLPS, 128, (MatchXmm, MatchXmm), 128, MR, 0, 0x0f, 0x12);

constexpr EncodingVariant MOVHPS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchXmm, MatchXmm|MatchMem64), 128, RM, 0, 0x0f, 0x16),
	DECLARE_CANDIDATE((MatchXmm|MatchMem64, MatchXmm), 128, MR, 0, 0x0f, 0x17),
};
DECLARE_INSTRUCTION(MOVHPS, 128);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MOVLHPS, 128, (MatchXmm, MatchXmm), 128, MR, 0, 0x0f, 0x16);

constexpr EncodingVariant MOVLPS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchXmm, MatchXmm|MatchMem64), 128, RM, 0, 0x0f, 0x12),
	DECLARE_CANDIDATE((MatchXmm|MatchMem64, MatchXmm), 128, MR, 0, 0x0f, 0x13),
};
DECLARE_INSTRUCTION(MOVLPS, 128);

DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(MOVMSKPD, 32, (MatchReg32, MatchXmm), 32, RM, 0, 0x66, 0x0f, 0x50);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MOVMSKPS, 32, (MatchReg32, MatchXmm), 32, RM, 0, 0x0f, 0x50);

constexpr EncodingVariant MOVNTI_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchMem32, MatchReg32), 32, MR, 0, 0x0f, 0xc3),
	DECLARE_CANDIDATE((MatchMem64, MatchReg64), 64, MR, 0, 0x0f, 0xc3),
};
DECLARE_INSTRUCTION(MOVNTI, 32);

DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(MOVNTDQ, 128, (MatchXmmMem128, MatchXmm), 128, MR, 0, 0x66, 0x0f, 0xe7);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MOVNTQ, 64, (MatchMem64, MatchMm), 64, MR, 0, 0x0f, 0xe7);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(MOVNTPD, 128, (MatchXmmMem128, MatchXmm), 128, MR, 0, 0x66, 0x0f, 0x2b);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MOVNTPS, 128, (MatchXmmMem128, MatchXmm), 128, MR, 0, 0x0f, 0x2b);

constexpr EncodingVariant MOVQ_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchRM64), 64, RM, 0, 0x66, 0x0f, 0x6e),
	DECLARE_PREFIXED_CANDIDATE((MatchRM64, MatchXmm), 64, MR, 0, 0x66, 0x0f, 0x7e),
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmm|MatchMem64), 0, RM, 0, 0xf3, 0x0f, 0x7e),
	DECLARE_PREFIXED_CANDIDATE((MatchXmm|MatchMem64, MatchXmm), 0, MR, 0, 0x66, 0x0f, 0xd6),
	DECLARE_CANDIDATE((MatchMm, MatchRM64), 64, RM, 0, 0x0f, 0x6e),
	DECLARE_CANDIDATE((MatchRM64, MatchMm), 64, MR, 0, 0x0f, 0x7e),
	DECLARE_CANDIDATE((MatchMm, MatchMmMem64), 0, RM, 0, 0x0f, 0x6f),
	DECLARE_CANDIDATE((MatchMmMem64, MatchMm), 0, MR, 0, 0x0f, 0x7f),
};
DECLARE_INSTRUCTION(MOVQ, 32);

DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(MOVQ2DQ, 128, (MatchXmm, MatchMm), 128, RM, 0, 0xf3, 0x0f, 0xd6);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MOVSB, 0, (), 0, ZO, 0, 0xa4);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MOVSD, 0, (), 0, ZO, 0, 0xa5);
DECLARE_SSE_AVX_RM_INSTRUCTION(MOVSHDUP, 0xf3, 0x0f, 0x16);
DECLARE_SSE_AVX_RM_INSTRUCTION(MOVSLDUP, 0xf3, 0x0f, 0x12);

constexpr EncodingVariant MOVSS_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128), 128, RM, 0, 0xf3, 0x0f, 0x10),
	DECLARE_PREFIXED_CANDIDATE((MatchXmmMem128, MatchXmm), 128, MR, 0, 0xf3, 0x0f, 0x11),
};
DECLARE_INSTRUCTION(MOVSS, 128);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MOVSQ, 0, (), 0, ZO, 0, 0x48, 0xa5);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MOVSW, 0, (), 0, ZO, 0, 0x66, 0xa5);

constexpr EncodingVariant MOVSX_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg16, MatchRM8), 16, RM, 0, 0x0f, 0xbe),
	DECLARE_CANDIDATE((MatchReg32, MatchRM8), 32, RM, 0, 0x0f, 0xbe),
	DECLARE_CANDIDATE((MatchReg64, MatchRM8), 64, RM, 0, 0x0f, 0xbe),
	DECLARE_CANDIDATE((MatchReg32, MatchRM16), 32, RM, 0, 0x0f, 0xbf),
	DECLARE_CANDIDATE((MatchReg64, MatchRM16), 64, RM, 0, 0x0f, 0xbf),
	DECLARE_CANDIDATE((MatchReg64, MatchRM32), 64, RM, 0, 0x63),
};
DECLARE_INSTRUCTION(MOVSX, 32);

constexpr EncodingVariant MOVUPS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchXmm, MatchXmmMem128), 128, RM, 0, 0x0f, 0x10),
	DECLARE_CANDIDATE((MatchXmmMem128, MatchXmm), 128, MR, 0, 0x0f, 0x11),
};
DECLARE_INSTRUCTION(MOVUPS, 128);

constexpr EncodingVariant MOVZX_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg16, MatchRM8), 16, RM, 0, 0x0f, 0xb6),
	DECLARE_CANDIDATE((MatchReg32, MatchRM8), 32, RM, 0, 0x0f, 0xb6),
	DECLARE_CANDIDATE((MatchReg64, MatchRM8), 32, RM, 0, 0x0f, 0xb6),	// Intentionally 32, smaller encoding for equivalent results.
	DECLARE_CANDIDATE((MatchReg32, MatchRM16), 32, RM, 0, 0x0f, 0xb7),
	DECLARE_CANDIDATE((MatchReg64, MatchRM16), 32, RM, 0, 0x0f, 0xb7),	// Intentionally 32, smaller encoding for equivalent results.
};
DECLARE_INSTRUCTION(MOVZX, 32);

DECLARE_SINGLE_RM_PARAMETER_INSTRUCTION(MUL, 4, 0xf6);
DECLARE_SSE_AVX_PSPDSSSD_INSTRUCTION(MUL, 0x0f, 0x59);

constexpr EncodingVariant MULX_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchReg32, MatchRM32), 32, RVM, 0, 0xf2, 0x0f, 0x38, 0xf6),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchReg64, MatchRM64), 64, RVM, 0x10, 0xf2, 0x0f, 0x38, 0xf6),
};
DECLARE_INSTRUCTION(MULX, 32);

DECLARE_SINGLE_RM_PARAMETER_INSTRUCTION(NEG, 3, 0xf6);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(NOP, 0, (), 0, ZO, 0, 0x90);
DECLARE_SINGLE_RM_PARAMETER_INSTRUCTION(NOT, 2, 0xf6);
DECLARE_ALU_INSTRUCTION(OR, 1);
DECLARE_SSE_AVX_PSPD_INSTRUCTION(OR, 0x0f, 0x56);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PACKSSDW, 0, 0x0f, 0x6b);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PACKSSWB, 0, 0x0f, 0x63);
DECLARE_SSE_AVX_RVM_INSTRUCTION(PACKUSDW, 0x66, 0x0f, 0x38, 0x2b);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PACKUSWB, 0, 0x0f, 0x67);
DECLARE_MMX_SSE_AVX_RM_INSTRUCTION(PABSB, 0, 0x0f, 0x38, 0x1c);
DECLARE_MMX_SSE_AVX_RM_INSTRUCTION(PABSD, 0, 0x0f, 0x38, 0x1e);
DECLARE_MMX_SSE_AVX_RM_INSTRUCTION(PABSW, 0, 0x0f, 0x38, 0x1d);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PADDB, 0, 0x0f, 0xfc);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PADDD, 0, 0x0f, 0xfe);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PADDSB, 0, 0x0f, 0xec);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PADDSW, 0, 0x0f, 0xed);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PADDW, 0, 0x0f, 0xfd);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PADDQ, 0, 0x0f, 0xd4);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PADDUSB, 0, 0x0f, 0xdc);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PADDUSW, 0, 0x0f, 0xdd);
DECLARE_MMX_SSE_AVX_RVMI_INSTRUCTION(PALIGNR, 0, 0x0f, 0x3a, 0x0f);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PAND, 0, 0x0f, 0xdb);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PANDN, 0, 0x0f, 0xdf);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PAVGB, 0, 0x0f, 0xe0);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PAVGW, 0, 0x0f, 0xe3);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PAUSE, 0, (), 0, ZO, 0, 0xf3, 0x90);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PCMPEQB, 0, 0x0f, 0x74);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PCMPEQD, 0, 0x0f, 0x76);
DECLARE_SSE_AVX_RVM_INSTRUCTION(PCMPEQQ, 0x66, 0x0f, 0x38, 0x29);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PCMPEQW, 0, 0x0f, 0x75);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(PCMPESTRI, 128, (MatchXmm, MatchXmmMem128, MatchImm8), 128, RMI, 0, 0x66, 0x0f, 0x3a, 0x61);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(PCMPESTRM, 128, (MatchXmm, MatchXmmMem128, MatchImm8), 128, RMI, 0, 0x66, 0x0f, 0x3a, 0x60);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PCMPGTB, 0, 0x0f, 0x64);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PCMPGTD, 0, 0x0f, 0x66);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(PCMPGTQ, 128, (MatchXmm, MatchXmmMem128), 128, RM, 0, 0x66, 0x0f, 0x38, 0x37);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PCMPGTW, 0, 0x0f, 0x65);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(PCMPISTRI, 128, (MatchXmm, MatchXmmMem128, MatchImm8), 128, RMI, 0, 0x66, 0x0f, 0x3a, 0x63);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(PCMPISTRM, 128, (MatchXmm, MatchXmmMem128, MatchImm8), 128, RMI, 0, 0x66, 0x0f, 0x3a, 0x62);

constexpr EncodingVariant PDEP_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchReg32, MatchRM32), 32, RVM, 0, 0xf2, 0x0f, 0x38, 0xf5),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchReg64, MatchRM64), 64, RVM, 0x10, 0xf2, 0x0f, 0x38, 0xf5),
};
DECLARE_INSTRUCTION(PDEP, 32);

constexpr EncodingVariant PEXT_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchReg32, MatchRM32), 32, RVM, 0, 0xf3, 0x0f, 0x38, 0xf5),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchReg64, MatchRM64), 64, RVM, 0x10, 0xf3, 0x0f, 0x38, 0xf5),
};
DECLARE_INSTRUCTION(PEXT, 32);

DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(PEXTRB, 32, (MatchReg32|MatchReg64|MatchMem8, MatchXmm, MatchImm8), 8, MRI, 0, 0x66, 0x0f, 0x3a, 0x14);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(PEXTRD, 32, (MatchRM32, MatchXmm, MatchImm8), 32, MRI, 0, 0x66, 0x0f, 0x3a, 0x16);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(PEXTRQ, 32, (MatchRM64, MatchXmm, MatchImm8), 64, MRI, 0, 0x66, 0x0f, 0x3a, 0x16);

constexpr EncodingVariant PEXTRW_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg64, MatchMm, MatchImm8), 32, RMI, 0, 0x0f, 0xc5),
	DECLARE_PREFIXED_CANDIDATE((MatchReg32|MatchReg64, MatchXmm, MatchImm8), 32, RMI, 0, 0x66, 0x0f, 0xc5),
	DECLARE_PREFIXED_CANDIDATE((MatchReg32|MatchReg64|MatchMem16, MatchXmm, MatchImm8), 32, MRI, 0, 0x66, 0x0f, 0x3a, 0x15),
};
DECLARE_INSTRUCTION(PEXTRW, 32);

DECLARE_MMX_SSE_AVX_INSTRUCTION(PHADDD, 0, 0x0f, 0x38, 0x02);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PHADDSW, 0, 0x0f, 0x38, 0x03);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PHADDW, 0, 0x0f, 0x38, 0x01);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PHSUBD, 0, 0x0f, 0x38, 0x06);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PHSUBSW, 0, 0x0f, 0x38, 0x07);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PHSUBW, 0, 0x0f, 0x38, 0x05);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(PINSRB, 32, (MatchXmm, MatchRegAll|MatchMem8, MatchImm8), 8, RMI, 0, 0x66, 0x0f, 0x3a, 0x20);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(PINSRD, 32, (MatchXmm, MatchRM32, MatchImm8), 32, RMI, 0, 0x66, 0x0f, 0x3a, 0x22);
DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(PINSRQ, 32, (MatchXmm, MatchRM64, MatchImm8), 64, RMI, 0, 0x66, 0x0f, 0x3a, 0x22);

constexpr EncodingVariant PINSRW_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchMm, MatchReg32|MatchMem16, MatchImm8), 32, RMI, 0, 0x0f, 0xc4),
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchReg32|MatchMem16, MatchImm8), 32, RMI, 0, 0x66, 0x0f, 0xc4),
};
DECLARE_INSTRUCTION(PINSRW, 32);

DECLARE_MMX_SSE_AVX_INSTRUCTION(PMADDWD, 0, 0x0f, 0xf5);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PMADDUBSW, 0, 0x0f, 0x38, 0x04);
DECLARE_SSE_AVX_RVM_INSTRUCTION(PMAXSB, 0x66, 0x0f, 0x38, 0x3c);
DECLARE_SSE_AVX_RVM_INSTRUCTION(PMAXSD, 0x66, 0x0f, 0x38, 0x3d);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PMAXSW, 0, 0x0f, 0xee);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PMAXUB, 0, 0x0f, 0xde);
DECLARE_SSE_AVX_RVM_INSTRUCTION(PMAXUD, 0x66, 0x0f, 0x38, 0x3f);
DECLARE_SSE_AVX_RVM_INSTRUCTION(PMAXUW, 0x66, 0x0f, 0x38, 0x3e);
DECLARE_SSE_AVX_RVM_INSTRUCTION(PMINSB, 0x66, 0x0f, 0x38, 0x38);
DECLARE_SSE_AVX_RVM_INSTRUCTION(PMINSD, 0x66, 0x0f, 0x38, 0x39);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PMINSW, 0, 0x0f, 0xea);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PMINUB, 0, 0x0f, 0xda);
DECLARE_SSE_AVX_RVM_INSTRUCTION(PMINUD, 0x66, 0x0f, 0x38, 0x3b);
DECLARE_SSE_AVX_RVM_INSTRUCTION(PMINUW, 0x66, 0x0f, 0x38, 0x3a);

constexpr EncodingVariant PMOVMSKB_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg32|MatchReg64, MatchXmm), 0, RM, 0, 0x66, 0x0f, 0xd7),
	DECLARE_CANDIDATE((MatchReg32|MatchReg64, MatchMm), 0, RM, 0, 0x0f, 0xd7),
};
DECLARE_INSTRUCTION(PMOVMSKB, 0);

DECLARE_MMX_SSE_AVX_INSTRUCTION(PMULHRSW, 0, 0x0f, 0x38, 0x0b);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PMULHW, 0, 0x0f, 0xe5);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PMULHUW, 0, 0x0f, 0xe4);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PMULLW, 0, 0x0f, 0xd5);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PMULUDQ, 0, 0x0f, 0xf4);

constexpr EncodingVariant POP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg64), 64, O, 0, 0x58),
	DECLARE_CANDIDATE((MatchRM64), 64, M, 0, 0x8f),
	DECLARE_CANDIDATE((MatchReg16), 16, O, 0, 0x58),
	DECLARE_CANDIDATE((MatchRM16), 16, M, 0, 0x8f),
};
DECLARE_INSTRUCTION(POP, 64);

constexpr EncodingVariant POPCNT_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg16, MatchRM16), 16, RM, 0, 0xf3, 0x0f, 0xb8),
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchRM32), 32, RM, 0, 0xf3, 0x0f, 0xb8),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchRM64), 64, RM, 0, 0xf3, 0x0f, 0xb8),
};
DECLARE_INSTRUCTION(POPCNT, 32);

DECLARE_MMX_SSE_AVX_INSTRUCTION(POR, 0, 0x0f, 0xeb);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PREFETCHT0, 0, (MatchMem8), 8, M, 1, 0x0f, 0x18);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PREFETCHT1, 0, (MatchMem8), 8, M, 2, 0x0f, 0x18);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PREFETCHT2, 0, (MatchMem8), 8, M, 3, 0x0f, 0x18);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PREFETCHTNTA, 0, (MatchMem8), 8, M, 0, 0x0f, 0x18);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PSADBW, 0, 0x0f, 0xf6);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PSHUFB, 0, 0x0f, 0x38, 0x00);
DECLARE_SSE_AVX_vRMI_INSTRUCTION(PSHUFD, 0, 0x66, 0x0f, 0x70);
DECLARE_SSE_AVX_vRMI_INSTRUCTION(PSHUFHW, 0, 0xf3, 0x0f, 0x70);
DECLARE_SSE_AVX_vRMI_INSTRUCTION(PSHUFLW, 0, 0xf2, 0x0f, 0x70);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PSHUFW, 64, (MatchMm, MatchMmMem64, MatchImm8), 64, RMI, 0, 0x0f, 0x70);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PSIGNB, 0, 0x0f, 0x38, 0x08);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PSIGND, 0, 0x0f, 0x38, 0x0a);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PSIGNW, 0, 0x0f, 0x38, 0x09);
DECLARE_SSE_AVX_SHIFT_INSTRUCTION(PSLLD, 6, 0x0f, 0x72);
DECLARE_SSE_AVX_VMI_INSTRUCTION(PSLLDQ, 7, 0x66, 0x0f, 0x73);
DECLARE_SSE_AVX_SHIFT_INSTRUCTION(PSLLW, 6, 0x0f, 0x71);
DECLARE_SSE_AVX_SHIFT_INSTRUCTION(PSLLQ, 6, 0x0f, 0x73);
DECLARE_SSE_AVX_SHIFT_INSTRUCTION(PSRAD, 4, 0x0f, 0x72);
DECLARE_SSE_AVX_SHIFT_INSTRUCTION(PSRAW, 4, 0x0f, 0x71);
DECLARE_SSE_AVX_SHIFT_INSTRUCTION(PSRLD, 2, 0x0f, 0x72);
DECLARE_SSE_AVX_VMI_INSTRUCTION(PSRLDQ, 3, 0x66, 0x0f, 0x73);
DECLARE_SSE_AVX_SHIFT_INSTRUCTION(PSRLW, 2, 0x0f, 0x71);
DECLARE_SSE_AVX_SHIFT_INSTRUCTION(PSRLQ, 2, 0x0f, 0x73);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PSUBB, 0, 0x0f, 0xf8);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PSUBD, 0, 0x0f, 0xfa);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PSUBQ, 0, 0x0f, 0xfb);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PSUBSB, 0, 0x0f, 0xe8);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PSUBSW, 0, 0x0f, 0xe9);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PSUBW, 0, 0x0f, 0xf9);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PSUBUSB, 0, 0x0f, 0xd8);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PSUBUSW, 0, 0x0f, 0xd9);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PUNPCKHBW, 0, 0x0f, 0x68);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PUNPCKHWD, 0, 0x0f, 0x69);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PUNPCKHDQ, 0, 0x0f, 0x6a);
DECLARE_SSE_AVX_RVM_INSTRUCTION(PUNPCKHQDQ, 0x66, 0x0f, 0x6d);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PUNPCKLBW, 0, 0x0f, 0x60);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PUNPCKLWD, 0, 0x0f, 0x61);
DECLARE_MMX_SSE_AVX_INSTRUCTION(PUNPCKLDQ, 0, 0x0f, 0x62);
DECLARE_SSE_AVX_RVM_INSTRUCTION(PUNPCKLQDQ, 0x66, 0x0f, 0x6c);

constexpr EncodingVariant PUSH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg64), 64, O, 0, 0x50),
	DECLARE_CANDIDATE((MatchRM64), 64, M, 6, 0xff),
	DECLARE_CANDIDATE((MatchReg16), 16, O, 0, 0x50),
	DECLARE_CANDIDATE((MatchRM16), 16, M, 6, 0xff),
	DECLARE_CANDIDATE((MatchImm8), 64, I, 0, 0x6a),
	DECLARE_CANDIDATE((MatchImm32), 64, I, 0, 0x68),
};
DECLARE_INSTRUCTION(PUSH, 64);

DECLARE_MMX_SSE_AVX_INSTRUCTION(PXOR, 0, 0x0f, 0xef);
DECLARE_SHIFT_INSTRUCTION(RCL, 2);
DECLARE_SSE_AVX_RVM_INSTRUCTION(RCPPS, 0, 0x0f, 0x53);
DECLARE_SSE_AVX_RVM_INSTRUCTION(RCPSS, 0xf3, 0x0f, 0x53);
DECLARE_SHIFT_INSTRUCTION(RCR, 3);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(RDMSR, 0, (), 0, ZO, 0, 0x0f, 0x32);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(RDTSC, 0, (), 0, ZO, 0, 0x0f, 0x31);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(RDTSCP, 0, (), 0, ZO, 0, 0x0f, 0x01, 0xf9);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(REP, -1, (), 0, ZO, 0, 0xf3);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(REPE, -1, (), 0, ZO, 0, 0xf3);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(REPNE, -1, (), 0, ZO, 0, 0xf2);

constexpr EncodingVariant RET_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((), 0, ZO, 0, 0xc3),
	DECLARE_CANDIDATE((MatchImm16), 0, I, 0, 0xc2),
};
DECLARE_INSTRUCTION(RET, 64);

DECLARE_SHIFT_INSTRUCTION(ROL, 0);
DECLARE_SHIFT_INSTRUCTION(ROR, 1);

constexpr EncodingVariant RORX_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchRM32, MatchImm8), 32, vRMI, 0, 0xf2, 0x0f, 0x3a, 0xf0),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchRM64, MatchImm8), 64, vRMI, 0x10, 0xf2, 0x0f, 0x3a, 0xf0),
};
DECLARE_INSTRUCTION(RORX, 32);

DECLARE_SSE_AVX_RVM_INSTRUCTION(RSQRTPS, 0, 0x0f, 0x52);
DECLARE_SSE_AVX_RVM_INSTRUCTION(RSQRTSS, 0xf3, 0x0f, 0x52);
DECLARE_SHIFT_INSTRUCTION(SAR, 7);

constexpr EncodingVariant SARX_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchRM32, MatchReg32), 32, RMV, 0, 0xf3, 0x0f, 0x38, 0xf7),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchRM64, MatchReg64), 64, RMV, 0x10, 0xf3, 0x0f, 0x38, 0xf7),
};
DECLARE_INSTRUCTION(SARX, 32);

DECLARE_ALU_INSTRUCTION(SBB, 3);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SCASB, 0, (), 0, ZO, 0, 0xae);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SCASW, 0, (), 0, ZO, 0, 0x66, 0xaf);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SCASD, 0, (), 0, ZO, 0, 0xaf);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SCASQ, 0, (), 0, ZO, 0, 0x48, 0xaf);
DECLARE_CC_INSTRUCTION(DECLARE_SETCC_INSTRUCTION);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SFENCE, 0, (), 0, ZO, 0, 0x0f, 0xae, 0xf8);
DECLARE_SHIFT_INSTRUCTION(SHL, 4);

constexpr EncodingVariant SHLD_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchRM16, MatchReg16, MatchImm8), 16, MRI, 0, 0x0f, 0xa4),
	DECLARE_CANDIDATE((MatchRM32, MatchReg32, MatchImm8), 32, MRI, 0, 0x0f, 0xa4),
	DECLARE_CANDIDATE((MatchRM64, MatchReg64, MatchImm8), 64, MRI, 0, 0x0f, 0xa4),
	DECLARE_CANDIDATE((MatchRM16, MatchReg16, MatchCL), 16, MR, 0, 0x0f, 0xa5),
	DECLARE_CANDIDATE((MatchRM32, MatchReg32, MatchCL), 32, MR, 0, 0x0f, 0xa5),
	DECLARE_CANDIDATE((MatchRM64, MatchReg64, MatchCL), 64, MR, 0, 0x0f, 0xa5),
};
DECLARE_INSTRUCTION(SHLD, 32);

constexpr EncodingVariant SHLX_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchRM32, MatchReg32), 32, RMV, 0, 0x66, 0x0f, 0x38, 0xf7),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchRM64, MatchReg64), 64, RMV, 0x10, 0x66, 0x0f, 0x38, 0xf7),
};
DECLARE_INSTRUCTION(SHLX, 32);

DECLARE_SHIFT_INSTRUCTION(SHR, 5);

constexpr EncodingVariant SHRD_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchRM16, MatchReg16, MatchImm8), 16, MRI, 0, 0x0f, 0xac),
	DECLARE_CANDIDATE((MatchRM32, MatchReg32, MatchImm8), 32, MRI, 0, 0x0f, 0xac),
	DECLARE_CANDIDATE((MatchRM64, MatchReg64, MatchImm8), 64, MRI, 0, 0x0f, 0xac),
	DECLARE_CANDIDATE((MatchRM16, MatchReg16, MatchCL), 16, MR, 0, 0x0f, 0xad),
	DECLARE_CANDIDATE((MatchRM32, MatchReg32, MatchCL), 32, MR, 0, 0x0f, 0xad),
	DECLARE_CANDIDATE((MatchRM64, MatchReg64, MatchCL), 64, MR, 0, 0x0f, 0xad),
};
DECLARE_INSTRUCTION(SHRD, 32);

constexpr EncodingVariant SHRX_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchRM32, MatchReg32), 32, RMV, 0, 0xf2, 0x0f, 0x38, 0xf7),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchRM64, MatchReg64), 64, RMV, 0x10, 0xf2, 0x0f, 0x38, 0xf7),
};
DECLARE_INSTRUCTION(SHRX, 32);

DECLARE_SSE_AVX_RVMI_INSTRUCTION(SHUFPD, 0, 0x66, 0x0f, 0xc6);
DECLARE_SSE_AVX_RVMI_INSTRUCTION(SHUFPS, 0, 0, 0x0f, 0xc6);
DECLARE_SSE_PSPDSSSD_INSTRUCTION(SQRT, 0x0f, 0x51);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STC, 0, (), 0, ZO, 0, 0xf9);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STD, 0, (), 0, ZO, 0, 0xfd);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STI, 0, (), 0, ZO, 0, 0xfb);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STOSB, 0, (), 0, ZO, 0, 0xaa);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STOSW, 0, (), 0, ZO, 0, 0x66, 0xab);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STOSD, 0, (), 0, ZO, 0, 0xab);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STOSQ, 0, (), 0, ZO, 0, 0x48, 0xab);
DECLARE_ALU_INSTRUCTION(SUB, 5);
DECLARE_SSE_AVX_PSPDSSSD_INSTRUCTION(SUB, 0x0f, 0x5c);

constexpr EncodingVariant TEST_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchAL, MatchImm8), 8, I, 0, 0xa8),
	DECLARE_CANDIDATE((MatchAX, MatchImm16), 16, I, 0, 0x66, 0xa9),
	DECLARE_CANDIDATE((MatchEAX, MatchImm32|MatchUImm32), 32, I, 0, 0xa9),
	DECLARE_CANDIDATE((MatchRAX, MatchImm32), 64, I, 0, 0x48, 0xa9),  // REX byte must be include for encoding "I"
	DECLARE_CANDIDATE((MatchRM8, MatchImm8), 8, MI, 0, 0xf6),
	DECLARE_CANDIDATE((MatchRM16, MatchImm16), 16, MI, 0, 0xf7),
	DECLARE_CANDIDATE((MatchRM32, MatchImm32|MatchUImm32), 32, MI, 0, 0xf7),
	DECLARE_CANDIDATE((MatchRM64, MatchImm32), 64, MI, 0, 0xf7),
	DECLARE_CANDIDATE((MatchRM8, MatchReg8), 8, MR, 0, 0x84),
	DECLARE_CANDIDATE((MatchRM16, MatchReg16), 16, MR, 0, 0x85),
	DECLARE_CANDIDATE((MatchRM32, MatchReg32), 32, MR, 0, 0x85),
	DECLARE_CANDIDATE((MatchRM64, MatchReg64), 64, MR, 0, 0x85),
	DECLARE_CANDIDATE((MatchReg8, MatchMem8), 8, RM, 0, 0x84),
	DECLARE_CANDIDATE((MatchReg16, MatchMem16), 16, RM, 0, 0x85),
	DECLARE_CANDIDATE((MatchReg32, MatchMem32), 32, RM, 0, 0x85),
	DECLARE_CANDIDATE((MatchReg64, MatchMem64), 64, RM, 0, 0x85),
};
DECLARE_INSTRUCTION(TEST, 32);

constexpr EncodingVariant TZCNT_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg16, MatchRM16), 16, RM, 0, 0xf3, 0x0f, 0xbc),
	DECLARE_PREFIXED_CANDIDATE((MatchReg32, MatchRM32), 32, RM, 0, 0xf3, 0x0f, 0xbc),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchRM64), 64, RM, 0, 0xf3, 0x0f, 0xbc),
};
DECLARE_INSTRUCTION(TZCNT, 32);

DECLARE_SINGLE_CANDIDATE_PREFIXED_INSTRUCTION(UCOMISD, 128, (MatchXmm, MatchXmm|MatchMem64), 128, RM, 0, 0x66, 0x0f, 0x2e);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UCOMISS, 128, (MatchXmm, MatchXmm|MatchMem32), 128, RM, 0, 0x0f, 0x2e);
DECLARE_SSE_AVX_RVM_INSTRUCTION(UNPCKHPS, 0, 0x0f, 0x15);
DECLARE_SSE_AVX_RVM_INSTRUCTION(UNPCKLPS, 0, 0x0f, 0x14);

constexpr EncodingVariant VMOVD_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchRM32), 32, vRM, 0, 0x66, 0x0f, 0x6e),
	DECLARE_PREFIXED_CANDIDATE((MatchRM32, MatchXmm), 32, vMR, 0, 0x66, 0x0f, 0x7e),
};
DECLARE_INSTRUCTION(VMOVD, 32);

constexpr EncodingVariant VMOVDQA_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128), 128, vRM, 0, 0x66, 0x0f, 0x6f),
	DECLARE_PREFIXED_CANDIDATE((MatchXmmMem128, MatchXmm), 128, vMR, 0, 0x66, 0x0f, 0x7f),
	DECLARE_PREFIXED_CANDIDATE((MatchYmm, MatchYmmMem256), 256, vRM, 0, 0x66, 0x0f, 0x6f),
	DECLARE_PREFIXED_CANDIDATE((MatchYmmMem256, MatchYmm), 256, vMR, 0, 0x66, 0x0f, 0x7f),
};
DECLARE_INSTRUCTION(VMOVDQA, 128);

constexpr EncodingVariant VMOVDQU_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128), 128, vRM, 0, 0xf3, 0x0f, 0x6f),
	DECLARE_PREFIXED_CANDIDATE((MatchXmmMem128, MatchXmm), 128, vMR, 0, 0xf3, 0x0f, 0x7f),
	DECLARE_PREFIXED_CANDIDATE((MatchYmm, MatchYmmMem256), 256, vRM, 0, 0xf3, 0x0f, 0x6f),
	DECLARE_PREFIXED_CANDIDATE((MatchYmmMem256, MatchYmm), 256, vMR, 0, 0xf3, 0x0f, 0x7f),
};
DECLARE_INSTRUCTION(VMOVDQU, 128);

constexpr EncodingVariant VMOVQ_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchRM64), 64, vRM, 0x10, 0x66, 0x0f, 0x6e),
	DECLARE_PREFIXED_CANDIDATE((MatchRM64, MatchXmm), 64, vMR, 0x10, 0x66, 0x0f, 0x7e),
};
DECLARE_INSTRUCTION(VMOVQ, 32);

constexpr EncodingVariant VPMOVMSKB_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchReg32|MatchReg64, MatchXmm), 128, vRM, 0, 0x66, 0x0f, 0xd7),
	DECLARE_PREFIXED_CANDIDATE((MatchReg64, MatchYmm), 256, vRM, 0, 0x66, 0x0f, 0xd7),
};
DECLARE_INSTRUCTION(VPMOVMSKB, 128);

constexpr EncodingVariant VSQRTPD_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128), 128, vRM, 0, 0x66, 0x0f, 0x51),
	DECLARE_PREFIXED_CANDIDATE((MatchYmm, MatchYmmMem256), 256, vRM, 0, 0x66, 0x0f, 0x51),
};
DECLARE_INSTRUCTION(VSQRTPD, 128);

constexpr EncodingVariant VSQRTPS_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128), 128, vRM, 0, 0, 0x0f, 0x51),
	DECLARE_PREFIXED_CANDIDATE((MatchYmm, MatchYmmMem256), 256, vRM, 0, 0, 0x0f, 0x51),
};
DECLARE_INSTRUCTION(VSQRTPS, 128);

constexpr EncodingVariant VSQRTSD_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128), 128, vRM, 0, 0xf2, 0x0f, 0x51),
	DECLARE_PREFIXED_CANDIDATE((MatchYmm, MatchYmmMem256), 256, vRM, 0, 0xf2, 0x0f, 0x51),
};
DECLARE_INSTRUCTION(VSQRTSD, 128);

constexpr EncodingVariant VSQRTSS_ENCODING_VARIANTS[] =
{
	DECLARE_PREFIXED_CANDIDATE((MatchXmm, MatchXmmMem128), 128, vRM, 0, 0xf3, 0x0f, 0x51),
	DECLARE_PREFIXED_CANDIDATE((MatchYmm, MatchYmmMem256), 256, vRM, 0, 0xf3, 0x0f, 0x51),
};
DECLARE_INSTRUCTION(VSQRTSS, 128);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(VZEROALL, 0, (), 0, ZO, 0, 0xc5, 0xfc, 0x77);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(VZEROUPPER, 0, (), 0, ZO, 0, 0xc5, 0xf8, 0x77);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(WAIT, 0, (), 0, ZO, 0, 0x9b);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(WBINVD, 0, (), 0, ZO, 0, 0x0f, 0x09);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(WRMSR, 0, (), 0, ZO, 0, 0x0f, 0x30);

constexpr EncodingVariant XADD_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchRM8, MatchReg8), 8, MR, 0, 0x0f, 0xc0),
	DECLARE_CANDIDATE((MatchRM16, MatchReg16), 16, MR, 0, 0x0f, 0xc1),
	DECLARE_CANDIDATE((MatchRM32, MatchReg32), 32, MR, 0, 0x0f, 0xc1),
	DECLARE_CANDIDATE((MatchRM64, MatchReg64), 64, MR, 0, 0x0f, 0xc1),
};
DECLARE_INSTRUCTION(XADD, 32);

constexpr EncodingVariant XCHG_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchAX, MatchReg16), 16, O2, 0, 0x90),
	DECLARE_CANDIDATE((MatchReg16, MatchAX), 16, O, 0, 0x90),
	DECLARE_CANDIDATE((MatchEAX, MatchReg32), 32, O2, 0, 0x90),
	DECLARE_CANDIDATE((MatchReg32, MatchEAX), 32, O, 0, 0x90),
	DECLARE_CANDIDATE((MatchRAX, MatchReg64), 64, O2, 0, 0x90),
	DECLARE_CANDIDATE((MatchReg64, MatchRAX), 64, O, 0,  0x90),
	DECLARE_CANDIDATE((MatchRM8, MatchReg8), 8, MR, 0, 0x86),
	DECLARE_CANDIDATE((MatchRM16, MatchReg16), 16, MR, 0, 0x87),
	DECLARE_CANDIDATE((MatchRM32, MatchReg32), 32, MR, 0, 0x87),
	DECLARE_CANDIDATE((MatchRM64, MatchReg64), 64, MR, 0, 0x87),
	DECLARE_CANDIDATE((MatchReg8, MatchRM8), 8, RM, 0, 0x86),
	DECLARE_CANDIDATE((MatchReg16, MatchRM16), 16, RM, 0, 0x87),
	DECLARE_CANDIDATE((MatchReg32, MatchRM32), 32, RM, 0, 0x87),
	DECLARE_CANDIDATE((MatchReg64, MatchRM64), 64, RM, 0, 0x87),
};
DECLARE_INSTRUCTION(XCHG, 32);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(XLATB, 0, (), 0, ZO, 0, 0xd7);
DECLARE_ALU_INSTRUCTION(XOR, 6);
DECLARE_SSE_AVX_PSPD_INSTRUCTION(XOR, 0x0f, 0x57);

//============================================================================

#define INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, suffix, index) \
	(*this)[#name_prefix #suffix] = &instr_prefix##index
#define INSERT_CC_INSTRUCTIONS(name_prefix, instr_prefix) \
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, o, 0);	\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, no, 1);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, b, 2);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, nb, 3);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, e, 4);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, ne, 5);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, be, 6);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, nbe, 7);	\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, s, 8);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, ns, 9);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, p, 10);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, np, 11);	\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, l, 12);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, nl, 13);	\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, le, 14);	\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, nle, 15);	\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, c, 2);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, nae, 2);	\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, nc, 3);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, ae, 3);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, z, 4);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, nz, 5);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, na, 6);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, a, 7);		\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, pe, 10);	\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, po, 11);	\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, nge, 12);	\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, ge, 13);	\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, ng, 14);	\
	INSERT_SINGLE_CC_INSTRUCTION(name_prefix, instr_prefix, g, 15)

#define INSERT_SSE_AVX_PSPDSSSD_INSTRUCTION(name, op) 	\
	(*this)[name "ps"] = &op##PS;			\
	(*this)[name "pd"] = &op##PD;			\
	(*this)[name "ss"] = &op##SS;			\
	(*this)[name "sd"] = &op##SD;			\
	(*this)["v" name "ps"] = &V##op##PS;	\
	(*this)["v" name "pd"] = &V##op##PD;	\
	(*this)["v" name "ss"] = &V##op##SS;	\
	(*this)["v" name "sd"] = &V##op##SD;

#define INSERT_SSE_AVX_PSPD_INSTRUCTION(name, op) 	\
	(*this)[name "ps"] = &op##PS;			\
	(*this)[name "pd"] = &op##PD;			\
	(*this)["v" name "ps"] = &V##op##PS;	\
	(*this)["v" name "pd"] = &V##op##PD

#define INSERT_MMX_SSE_AVX_INSTRUCTION(name, op) \
	(*this)[name] = &op; \
	(*this)["v" name] = &V##op

#define INSERT_SSE_AVX_INSTRUCTION(name, op) \
	(*this)[name] = &op; \
	(*this)["v" name] = &V##op

#define INSERT_FPU_INSTRUCTION(name, op) \
	(*this)["f" name] = &F##op; 			\
	(*this)["f" name "p"] = &F##op##P; 		\
	(*this)["fi" name] = &FI##op;

InstructionMap::InstructionMap()
{
	(*this)["adc"] = &ADC;
	(*this)["add"] = &ADD;
	(*this)["and"] = &AND;
	(*this)["bsf"] = &BSF;
	(*this)["bsr"] = &BSR;
	(*this)["bswap"] = &BSWAP;
	(*this)["bt"] = &BT;
	(*this)["btc"] = &BTC;
	(*this)["btr"] = &BTR;
	(*this)["bts"] = &BTS;
	(*this)["call"] = &CALL;
	(*this)["cbw"] = &CBW;
	(*this)["cdq"] = &CDQ;
	(*this)["cdqe"] = &CDQE;
	(*this)["clc"] = &CLC;
	(*this)["cld"] = &CLD;
	(*this)["clflush"] = &CLFLUSH;
	(*this)["clflushopt"] = &CLFLUSHOPT;
	(*this)["clwb"] = &CLWB;
	(*this)["cli"] = &CLI;
	(*this)["cmc"] = &CMC;
	(*this)["cmp"] = &CMP;
	(*this)["cmpsb"] = &CMPSB;
	(*this)["cmpsw"] = &CMPSW;
	(*this)["cmpsd"] = &CMPSD;
	(*this)["cmpsq"] = &CMPSQ;
	(*this)["cmpxchg"] = &CMPXCHG;
	(*this)["cmpxchg8b"] = &CMPXCHG8B;
	(*this)["cmpxchg16b"] = &CMPXCHG16B;
	(*this)["cpuid"] = &CPUID;
	(*this)["cqo"] = &CQO;
	(*this)["cwd"] = &CWD;
	(*this)["cwde"] = &CWDE;
	(*this)["dec"] = &DEC;
	(*this)["div"] = &DIV;
	(*this)["enter"] = &ENTER;
	(*this)["hlt"] = &HLT;
	(*this)["idiv"] = &IDIV;
	(*this)["imul"] = &IMUL;
	(*this)["inc"] = &INC;
	(*this)["int"] = &INT;
	(*this)["int1"] = &INT1;
	(*this)["int3"] = &INT3;
	(*this)["invd"] = &INVD;
	(*this)["invlpg"] = &INVLPG;
	(*this)["iret"] = &IRET;
	(*this)["iretd"] = &IRETD;
	(*this)["iretq"] = &IRETQ;
	(*this)["jecxz"] = &JECXZ;
	(*this)["jmp"] = &JMP;
	(*this)["jrcxz"] = &JRCXZ;
	(*this)["lea"] = &LEA;
	(*this)["leave"] = &LEAVE;
	(*this)["lfence"] = &LFENCE;
	(*this)["lock"] = &LOCK;
	(*this)["lodsb"] = &LODSB;
	(*this)["lodsw"] = &LODSW;
	(*this)["lodsd"] = &LODSD;
	(*this)["lodsq"] = &LODSQ;
	(*this)["loop"] = &LOOP;
	(*this)["loope"] = &LOOPE;
	(*this)["loopz"] = &LOOPE;
	(*this)["loopne"] = &LOOPNE;
	(*this)["loopnz"] = &LOOPNE;
	(*this)["mfence"] = &MFENCE;
	(*this)["mov"] = &MOV;
	(*this)["movbe"] = &MOVBE;
	(*this)["movnti"] = &MOVNTI;
	(*this)["movsb"] = &MOVSB;
	(*this)["movsw"] = &MOVSW;
	(*this)["movsd"] = &MOVSD;
	(*this)["movsq"] = &MOVSQ;
	(*this)["movsx"] = &MOVSX;
	(*this)["movzx"] = &MOVZX;
	(*this)["mul"] = &MUL;
	(*this)["neg"] = &NEG;
	(*this)["nop"] = &NOP;
	(*this)["not"] = &NOT;
	(*this)["or"] = &OR;
	(*this)["pause"] = &PAUSE;
	(*this)["pop"] = &POP;
	(*this)["push"] = &PUSH;
	(*this)["rcl"] = &RCL;
	(*this)["rcr"] = &RCR;
	(*this)["rdmsr"] = &RDMSR;
	(*this)["rdtsc"] = &RDTSC;
	(*this)["rdtscp"] = &RDTSCP;
	(*this)["rep"] = &REP;
	(*this)["repe"] = &REPE;
	(*this)["repne"] = &REPNE;
	(*this)["ret"] = &RET;
	(*this)["rol"] = &ROL;
	(*this)["ror"] = &ROR;
	(*this)["sal"] = &SHL;		// Alias sal -> shl.
	(*this)["sar"] = &SAR;
	(*this)["sbb"] = &SBB;
	(*this)["scasb"] = &SCASB;
	(*this)["scasw"] = &SCASW;
	(*this)["scasd"] = &SCASD;
	(*this)["scasq"] = &SCASQ;
	(*this)["sfence"] = &SFENCE;
	(*this)["shl"] = &SHL;
	(*this)["shld"] = &SHLD;
	(*this)["shr"] = &SHR;
	(*this)["shrd"] = &SHRD;
	(*this)["stc"] = &STC;
	(*this)["std"] = &STD;
	(*this)["sti"] = &STI;
	(*this)["stosb"] = &STOSB;
	(*this)["stosw"] = &STOSW;
	(*this)["stosd"] = &STOSD;
	(*this)["stosq"] = &STOSQ;
	(*this)["sub"] = &SUB;
	(*this)["test"] = &TEST;
	(*this)["wait"] = &WAIT;
	(*this)["wbinvd"] = &WBINVD;
	(*this)["wrmsr"] = &WRMSR;
	(*this)["xadd"] = &XADD;
	(*this)["xchg"] = &XCHG;
	(*this)["xlatb"] = &XLATB;
	(*this)["xor"] = &XOR;
	
	INSERT_CC_INSTRUCTIONS(cmov, CMOVCC);
	INSERT_CC_INSTRUCTIONS(set, SETCC);
	INSERT_CC_INSTRUCTIONS(j, JCC);
	
	// x87
	(*this)["f2xm1"] = &F2XM1;
	(*this)["fabs"] = &FABS;
	(*this)["fchs"] = &FCHS;
	(*this)["fclex"] = &FCLEX;
	(*this)["fcos"] = &FCOS;
	(*this)["fdecstp"] = &FDECSTP;
	(*this)["fincstp"] = &FINCSTP;
	(*this)["ffree"] = &FFREE;
	(*this)["finit"] = &FINIT;
	(*this)["fld"] = &FLD;
	(*this)["fld1"] = &FLD1;
	(*this)["fldl2t"] = &FLDL2T;
	(*this)["fldl2e"] = &FLDL2E;
	(*this)["fldpi"] = &FLDPI;
	(*this)["fldlg2"] = &FLDLG2;
	(*this)["fldln2"] = &FLDLN2;
	(*this)["fldz"] = &FLDZ;
	(*this)["fnclex"] = &FNCLEX;
	(*this)["fninit"] = &FNINIT;
	(*this)["fnop"] = &FNOP;
	(*this)["fnstsw"] = &FNSTSW;
	(*this)["fpatan"] = &FPATAN;
	(*this)["fprem"] = &FPREM;
	(*this)["fprem1"] = &FPREM1;
	(*this)["fptan"] = &FPTAN;
	(*this)["frndint"] = &FRNDINT;
	(*this)["fscale"] = &FSCALE;
	(*this)["fsin"] = &FSIN;
	(*this)["fsincos"] = &FSINCOS;
	(*this)["fsqrt"] = &FSQRT;
	(*this)["fst"] = &FST;
	(*this)["fstp"] = &FSTP;
	(*this)["fstsw"] = &FSTSW;
	(*this)["ftst"] = &FTST;
	(*this)["fwait"] = &FWAIT;
	(*this)["fxam"] = &FXAM;
	(*this)["fxch"] = &FXCH;
	(*this)["fxtract"] = &FXTRACT;
	(*this)["fyl2x"] = &FYL2X;
	(*this)["fyl2xp1"] = &FYL2XP1;

	INSERT_FPU_INSTRUCTION("add", ADD);
	INSERT_FPU_INSTRUCTION("div", DIV);
	INSERT_FPU_INSTRUCTION("divr", DIVR);
	INSERT_FPU_INSTRUCTION("mul", MUL);
	INSERT_FPU_INSTRUCTION("sub", SUB);
	INSERT_FPU_INSTRUCTION("subr", SUBR);
	// TODO: fcom, fucom

	// ADX
	(*this)["adcx"] = &ADCX;
	(*this)["adox"] = &ADOX;

	// MMX (and SSE variants)
	(*this)["emms"] = &EMMS;
	(*this)["maskmovq"] = &MASKMOVQ;
	(*this)["movd"] = &MOVD;
	(*this)["movntq"] = &MOVNTQ;
	(*this)["movq"] = &MOVQ;
	(*this)["pshufw"] = &PSHUFW;

	INSERT_MMX_SSE_AVX_INSTRUCTION("packssdw", PACKSSDW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("packsswb", PACKSSWB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("packuswb", PACKUSWB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("paddb", PADDB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("paddd", PADDD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("paddsb", PADDSB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("paddsw", PADDSW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("paddw", PADDW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("paddq", PADDQ);
	INSERT_MMX_SSE_AVX_INSTRUCTION("paddusb", PADDUSB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("paddusw", PADDUSW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pand", PAND);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pandn", PANDN);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pavgb", PAVGB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pavgw", PAVGW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pcmpeqb", PCMPEQB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pcmpeqd", PCMPEQD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pcmpeqw", PCMPEQW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pcmpgtb", PCMPGTB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pcmpgtd", PCMPGTD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pcmpgtw", PCMPGTW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pmaddwd", PMADDWD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pmaxsb", PMAXSB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pmaxsw", PMAXSW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pmaxub", PMAXUB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pmaxud", PMAXUD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pmaxuw", PMAXUW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pminsb", PMINSB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pminsw", PMINSW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pminub", PMINUB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pminud", PMINUD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pminuw", PMINUW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pmovmskb", PMOVMSKB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pmulhw", PMULHW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pmulhuw", PMULHUW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pmullw", PMULLW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pmuludq", PMULUDQ);
	INSERT_MMX_SSE_AVX_INSTRUCTION("por", POR);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psadbw", PSADBW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pslld", PSLLD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pslldq", PSLLDQ);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psllq", PSLLQ);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psllw", PSLLW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psrad", PSRAD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psraw", PSRAW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psrld", PSRLD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psrldq", PSRLDQ);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psrlq", PSRLQ);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psrlw", PSRLW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psubb", PSUBB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psubw", PSUBW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psubd", PSUBD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psubq", PSUBQ);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psubsb", PSUBSB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psubsw", PSUBSW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psubusb", PSUBUSB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psubusw", PSUBUSW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("punpckhbw", PUNPCKHBW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("punpckhwd", PUNPCKHWD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("punpckhdq", PUNPCKHDQ);
	INSERT_MMX_SSE_AVX_INSTRUCTION("punpcklbw", PUNPCKLBW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("punpcklwd", PUNPCKLWD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("punpckldq", PUNPCKLDQ);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pxor", PXOR);

	// SSE
	(*this)["cmpps"] = &CMPPS;
	(*this)["cmpss"] = &CMPSS;
	(*this)["cmpeqps"] = &CMPEQPS;
	(*this)["cmpeqss"] = &CMPEQSS;
	(*this)["cmpleps"] = &CMPLEPS;
	(*this)["cmpless"] = &CMPLESS;
	(*this)["cmpltps"] = &CMPLTPS;
	(*this)["cmpltss"] = &CMPLTSS;
	(*this)["cmpneqps"] = &CMPNEQPS;
	(*this)["cmpneqss"] = &CMPNEQSS;
	(*this)["cmpnleps"] = &CMPNLEPS;
	(*this)["cmpnless"] = &CMPNLESS;
	(*this)["cmpnltps"] = &CMPNLTPS;
	(*this)["cmpnltss"] = &CMPNLTSS;
	(*this)["cmpordps"] = &CMPORDPS;
	(*this)["cmpordss"] = &CMPORDSS;
	(*this)["cmpunordps"] = &CMPUNORDPS;
	(*this)["cmpunordss"] = &CMPUNORDSS;
	(*this)["comiss"] = &COMISS;
	(*this)["cvtpi2ps"] = &CVTPI2PS;
	(*this)["cvtps2pi"] = &CVTPS2PI;
	(*this)["cvtsi2ss"] = &CVTSI2SS;
	(*this)["cvtss2si"] = &CVTSS2SI;
	(*this)["movaps"] = &MOVAPS;
	(*this)["movhlps"] = &MOVHLPS;
	(*this)["movhps"] = &MOVHPS;
	(*this)["movlhps"] = &MOVLHPS;
	(*this)["movlps"] = &MOVLPS;
	(*this)["movmskps"] = &MOVMSKPS;
	(*this)["movntps"] = &MOVNTPS;
	(*this)["movss"] = &MOVSS;
	(*this)["movups"] = &MOVUPS;
	(*this)["prefetcht0"] = &PREFETCHT0;
	(*this)["prefetcht1"] = &PREFETCHT1;
	(*this)["prefetcht2"] = &PREFETCHT2;
	(*this)["prefetchtnta"] = &PREFETCHTNTA;
	(*this)["ucomiss"] = &UCOMISS;

	INSERT_SSE_AVX_INSTRUCTION("pshufd", PSHUFD);
	INSERT_SSE_AVX_INSTRUCTION("pshufhw", PSHUFHW);
	INSERT_SSE_AVX_INSTRUCTION("pshuflw", PSHUFLW);
	INSERT_SSE_AVX_INSTRUCTION("rcpps", RCPPS);
	INSERT_SSE_AVX_INSTRUCTION("rcpss", RCPSS);
	INSERT_SSE_AVX_INSTRUCTION("rsqrtps", RSQRTPS);
	INSERT_SSE_AVX_INSTRUCTION("rsqrtss", RSQRTSS);
	INSERT_SSE_AVX_INSTRUCTION("shufps", SHUFPS);
	INSERT_SSE_AVX_INSTRUCTION("unpckhps", UNPCKHPS);
	INSERT_SSE_AVX_INSTRUCTION("unpcklps", UNPCKLPS);

	INSERT_SSE_AVX_PSPD_INSTRUCTION("and", AND);
	INSERT_SSE_AVX_PSPD_INSTRUCTION("andn", ANDN);
	INSERT_SSE_AVX_PSPD_INSTRUCTION("or", OR);
	INSERT_SSE_AVX_PSPD_INSTRUCTION("xor", XOR);

	INSERT_SSE_AVX_PSPDSSSD_INSTRUCTION("add", ADD);
	INSERT_SSE_AVX_PSPDSSSD_INSTRUCTION("div", DIV);
	INSERT_SSE_AVX_PSPDSSSD_INSTRUCTION("max", MAX);
	INSERT_SSE_AVX_PSPDSSSD_INSTRUCTION("min", MIN);
	INSERT_SSE_AVX_PSPDSSSD_INSTRUCTION("mul", MUL);
	INSERT_SSE_AVX_PSPDSSSD_INSTRUCTION("sqrt", SQRT);
	INSERT_SSE_AVX_PSPDSSSD_INSTRUCTION("sub", SUB);

	// SSE2
	(*this)["cmppd"] = &CMPPD;
	(*this)["cmpsd"] = &CMPSD;
	(*this)["cmppd"] = &CMPPD;
	(*this)["cmpeqpd"] = &CMPEQPD;
	(*this)["cmpeqsd"] = &CMPEQSD;
	(*this)["cmplepd"] = &CMPLEPD;
	(*this)["cmplesd"] = &CMPLESD;
	(*this)["cmpltpd"] = &CMPLTPD;
	(*this)["cmpltsd"] = &CMPLTSD;
	(*this)["cmpneqpd"] = &CMPNEQPD;
	(*this)["cmpneqsd"] = &CMPNEQSD;
	(*this)["cmpnlepd"] = &CMPNLEPD;
	(*this)["cmpnlesd"] = &CMPNLESD;
	(*this)["cmpnltpd"] = &CMPNLTPD;
	(*this)["cmpnltsd"] = &CMPNLTSD;
	(*this)["cmpordpd"] = &CMPORDPD;
	(*this)["cmpordsd"] = &CMPORDSD;
	(*this)["cmpunordpd"] = &CMPUNORDPD;
	(*this)["cmpunordsd"] = &CMPUNORDSD;
	(*this)["comisd"] = &COMISD;
	(*this)["cvtdq2pd"] = &CVTDQ2PD;
	(*this)["cvtdq2ps"] = &CVTDQ2PS;
	(*this)["cvtpd2dq"] = &CVTPD2DQ;
	(*this)["cvtpd2pi"] = &CVTPD2PI;
	(*this)["cvtpd2ps"] = &CVTPD2PS;
	(*this)["cvtpi2pd"] = &CVTPI2PD;
	(*this)["cvtps2dq"] = &CVTPS2DQ;
	(*this)["cvtps2pd"] = &CVTPS2PD;
	(*this)["cvtsd2si"] = &CVTSD2SI;
	(*this)["cvtsd2ss"] = &CVTSD2SS;
	(*this)["cvtsi2sd"] = &CVTSI2SD;
	(*this)["cvtss2sd"] = &CVTSS2SD;
	(*this)["maskmovdqu"] = &MASKMOVDQU;
	(*this)["movapd"] = &MOVAPD;
	(*this)["movdq2q"] = &MOVDQ2Q;
	(*this)["movdqa"] = &MOVDQA;
	(*this)["movdqu"] = &MOVDQU;
	(*this)["movmskpd"] = &MOVMSKPD;
	(*this)["movntdq"] = &MOVNTDQ;
	(*this)["movntpd"] = &MOVNTPD;
	(*this)["movq2dq"] = &MOVQ2DQ;
	(*this)["pextrw"] = &PEXTRW;			// Includes SSE4.1 variation
	(*this)["pinsrw"] = &PINSRW;			// Includes SSE4.1 variation
	(*this)["ucomisd"] = &UCOMISD;
	INSERT_SSE_AVX_INSTRUCTION("punpckhqdq", PUNPCKHQDQ);
	INSERT_SSE_AVX_INSTRUCTION("punpcklqdq", PUNPCKLQDQ);
	INSERT_SSE_AVX_INSTRUCTION("shufpd", SHUFPD);

	// SSE3
	INSERT_SSE_AVX_INSTRUCTION("addsubpd", ADDSUBPD);
	INSERT_SSE_AVX_INSTRUCTION("addsubps", ADDSUBPS);
	INSERT_SSE_AVX_INSTRUCTION("haddpd", HADDPD);
	INSERT_SSE_AVX_INSTRUCTION("haddps", HADDPS);
	INSERT_SSE_AVX_INSTRUCTION("hsubpd", HSUBPD);
	INSERT_SSE_AVX_INSTRUCTION("hsubps", HSUBPS);
	INSERT_SSE_AVX_INSTRUCTION("lddqu", LDDQU);
	INSERT_SSE_AVX_INSTRUCTION("movddup", MOVDDUP);
	INSERT_SSE_AVX_INSTRUCTION("movshdup", MOVSHDUP);
	INSERT_SSE_AVX_INSTRUCTION("movsldup", MOVSLDUP);

	// SSSE3
	INSERT_MMX_SSE_AVX_INSTRUCTION("pabsb", PABSB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pabsw", PABSW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pabsd", PABSD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("palignr", PALIGNR);
	INSERT_MMX_SSE_AVX_INSTRUCTION("phaddw", PHADDW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("phaddsw", PHADDSW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("phaddd", PHADDD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("phsubw", PHSUBW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("phsubsw", PHSUBSW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("phsubd", PHSUBD);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pmaddubsw", PMADDUBSW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pmulhrsw", PMULHRSW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("pshufb", PSHUFB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psignb", PSIGNB);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psignw", PSIGNW);
	INSERT_MMX_SSE_AVX_INSTRUCTION("psignd", PSIGND);

	// SSE4.1
	INSERT_SSE_AVX_INSTRUCTION("packusdw", PACKUSDW);
	INSERT_SSE_AVX_INSTRUCTION("pcmpeqq", PCMPEQQ);
	(*this)["pextrb"] = &PEXTRB;
	(*this)["pextrd"] = &PEXTRD;
	(*this)["pextrq"] = &PEXTRQ;
	(*this)["pinsrb"] = &PINSRB;
	(*this)["pinsrd"] = &PINSRD;
	(*this)["pinsrq"] = &PINSRQ;
	INSERT_SSE_AVX_INSTRUCTION("pmaxsd", PMAXSD);
	INSERT_SSE_AVX_INSTRUCTION("pminsd", PMINSD);
	// TODO: <lots missing>

	// SSE4.2
	(*this)["crc32"] = &CRC32;
	(*this)["pcmpestri"] = &PCMPESTRI;
	(*this)["pcmpestrm"] = &PCMPESTRM;
	(*this)["pcmpgtq"] = &PCMPGTQ;
	(*this)["pcmpistri"] = &PCMPISTRI;
	(*this)["pcmpistrm"] = &PCMPISTRM;

	// AES
	(*this)["aesdec"] = &AESDEC;
	(*this)["aesdeclast"] = &AESDECLAST;
	(*this)["aesenc"] = &AESENC;
	(*this)["aesenclast"] = &AESENCLAST;
	(*this)["aesimc"] = &AESIMC;
	(*this)["aeskeygenassist"] = &AESKEYGENASSIST;
	
	// AVX
	(*this)["vmovd"] = &VMOVD;
	(*this)["vmovdqa"] = &VMOVDQA;
	(*this)["vmovdqu"] = &VMOVDQU;
	(*this)["vmovq"] = &VMOVQ;
	(*this)["vzeroall"] = &VZEROALL;
	(*this)["vzeroupper"] = &VZEROUPPER;

	// ABM
	(*this)["lzcnt"] = &LZCNT;
	(*this)["popcnt"] = &POPCNT;

	// BMI1
	(*this)["andn"] = &ANDN;
	(*this)["bextr"] = &BEXTR;
	(*this)["blsi"] = &BLSI;
	(*this)["blsmsk"] = &BLSMSK;
	(*this)["blsr"] = &BLSR;
	(*this)["tzcnt"] = &TZCNT;

	// BMI2
	(*this)["bzhi"] = &BZHI;
	(*this)["mulx"] = &MULX;
	(*this)["pdep"] = &PDEP;
	(*this)["pext"] = &PEXT;
	(*this)["rorx"] = &RORX;
	(*this)["sarx"] = &SARX;
	(*this)["shrx"] = &SHRX;
	(*this)["shlx"] = &SHLX;
};

//============================================================================
