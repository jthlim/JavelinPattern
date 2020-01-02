//============================================================================

#include "Javelin/Tools/jasm/arm64/Instruction.h"

//============================================================================

using namespace Javelin::Assembler::arm64;

//============================================================================

const InstructionMap InstructionMap::instance;

//============================================================================

#define UNWRAP(...) 		__VA_ARGS__
#define NUM_ELEMENTS(x)		(sizeof(x) / sizeof(x[0]))
#define NUM_OPERANDS(...)	(sizeof((MatchBitfield[]){__VA_ARGS__})/sizeof(MatchBitfield))

#define DECLARE_CANDIDATE(operand_list, enc, enc_data, op) \
	{																	\
		.encoder = Encoder::enc,										\
		.operandMatchMasksLength = NUM_OPERANDS(UNWRAP operand_list),	\
		.data = enc_data,                                              	\
		.opcode = op,													\
		.operandMatchMasks = (const MatchBitfield[]){ UNWRAP operand_list }, \
	}

#define DECLARE_ENCODING_VARIANT(name, operand_list, enc, enc_data, op) \
	constexpr EncodingVariant name##_ENCODING_VARIANTS[] = \
	{														\
		DECLARE_CANDIDATE(operand_list, enc, enc_data, op),	\
	}

#define DECLARE_INSTRUCTION(name) \
	constexpr Instruction name =											\
	{																		\
		.encodingVariantLength = NUM_ELEMENTS(name##_ENCODING_VARIANTS),	\
		.encodingVariants = name##_ENCODING_VARIANTS						\
	}

#define DECLARE_SINGLE_CANDIDATE_INSTRUCTION(name, operand_list, enc, enc_data, op) \
	DECLARE_ENCODING_VARIANT(name, operand_list, enc, enc_data, op);	\
	DECLARE_INSTRUCTION(name)

#define DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchOp0Mask, scale, op) \
	/* LDR (Wn|Xn), [Xn|SP] */ \
	DECLARE_CANDIDATE((MatchOp0Mask|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnUImm12, scale, op | 0x1000000), \
	/* LDR (Wn|Xn), [XP, #UImm12] */ \
	DECLARE_CANDIDATE((MatchOp0Mask|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnUImm12, scale, op | 0x1000000), \
	/* LDR (Wn|Xn), [XP, #imm9]! */ \
	DECLARE_CANDIDATE((MatchOp0Mask|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket, MatchExclamationMark), RtRnImm9, 1, op | 0x00000c00), \
	/* LDR (Wn|Xn), [XP], #imm9 */ \
	DECLARE_CANDIDATE((MatchOp0Mask|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp2), RtRnImm9, 1, op | 0x00000400)

#define DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchOp0Mask, shift_value, op) \
	/* LDR <Xt>, [<Xn|SP>, <Xm>] */ \
	DECLARE_CANDIDATE((MatchOp0Mask|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchRightSquareBracket), LoadStoreOffsetRegister, shift_value, op | 0x6000), \
	/* LDR <Xt>, [<Xn|SP>, <Xm>, <extend>] */ \
	DECLARE_CANDIDATE((MatchOp0Mask|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg64|MatchOp3, MatchRightSquareBracket), LoadStoreOffsetRegister, shift_value, op), \
	/* LDR <Xt>, [<Xn|SP>, <Xm>, <extend> #<amount>] */ \
	DECLARE_CANDIDATE((MatchOp0Mask|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg64|MatchOp3, MatchImm|MatchOp4, MatchRightSquareBracket), LoadStoreOffsetRegister, shift_value, op), \
	/* LDR <Xt>, [<Xn|SP>, <Wm>, <extend>] */ \
	DECLARE_CANDIDATE((MatchOp0Mask|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchExtendLoadStoreReg32|MatchOp3, MatchRightSquareBracket), LoadStoreOffsetRegister, shift_value, op), \
	/* LDR <Xt>, [<Xn|SP>, <Wm>, <extend> #<amount>] */ \
	DECLARE_CANDIDATE((MatchOp0Mask|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchExtendLoadStoreReg32|MatchOp3, MatchImm|MatchOp4, MatchRightSquareBracket), LoadStoreOffsetRegister, shift_value, op)

//============================================================================

constexpr EncodingVariant ABS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1), FpRdRnRmRa, 0, 0x5ee0b800),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 3, 0x0e20b800),
};
DECLARE_INSTRUCTION(ABS);

constexpr EncodingVariant ADC_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x1a000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), RdRnRmImm6, 0, 0x9a000000),
};
DECLARE_INSTRUCTION(ADC);

constexpr EncodingVariant ADCS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x3a000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), RdRnRmImm6, 0, 0xba000000),
};
DECLARE_INSTRUCTION(ADCS);

constexpr EncodingVariant ADD_ENCODING_VARIANTS[] =
{
	// ADD <Wd|WSP>, <Wn|WSP>, #<imm>, <shift>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchImm|MatchOp2), ArithmeticImmediate, 0, 0x11000000),
	// ADD <Wd|WSP>, <Wn|WSP>, #<imm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchUImm12|MatchOp2, MatchComma, MatchLSL, MatchImm|MatchOp3), ArithmeticImmediate, 0, 0x11000000),
	// ADD Wd, <Wn>, <Wm>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x0b000000),
	// ADD Wd, <Wn>, <Wm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x0b000000),
	// ADD <Wd|WSP>, <Wn|WSP>, <Wm>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticExtendedRegister, 0, 0x0b204000),
	// ADD <Wd|WSP>, <Wn|WSP>, <Wm>, <extend>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchExtendReg32|MatchOp3), ArithmeticExtendedRegister, 0, 0x0b200000),
	// ADD <Wd|WSP>, <Wn|WSP>, <Wm>, <extend> #<amount>}
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg32|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0x0b200000),

	// ADD <Xd|SP>, <Xn|SP>, #<imm>, <shift>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2), ArithmeticImmediate, 0, 0x91000000),
	// ADD <Xd|SP>, <Xn|SP>, #<imm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchUImm12|MatchOp2, MatchComma, MatchLSL, MatchImm|MatchOp3), ArithmeticImmediate, 0, 0x91000000),
	// ADD Xd, <Xn>, <Xm>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x8b000000),
	// ADD Xd, <Xn>, <Xm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x8b000000),
	// ADD <Xd|SP>, <Xn|WSP>, <Wm>, <extend>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchExtendReg32|MatchOp3), ArithmeticExtendedRegister, 0, 0x8b200000),
	// ADD <Xd|SP>, <Xn|WSP>, <Wm>, <extend> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg32|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0x8b200000),
	// ADD <Xd|SP>, <Xn|WSP>, <Xm>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticExtendedRegister, 0, 0x8b206000),
	// ADD <Xd|SP>, <Xn|WSP>, <Xm>, <extend>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchExtendReg64|MatchOp3), ArithmeticExtendedRegister, 0, 0x8b200000),
	// ADD <Xd|SP>, <Xn|WSP>, <Xm>, <extend> #<amount>}
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg64|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0x8b200000),
	
	// ADD Dd, Dn, Dm
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchD|MatchOp2), FpRdRnRmRa, 0, 0x5ee08400),

	// ADD <Vd>.T, <Vn>.T, <Vm>.T
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e208400),
	
	// ADD Rd, Rn, {target}[highbit:lowbit]
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchNumber|MatchImm|MatchRel|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchColon, MatchNumber|MatchOp4, MatchRightSquareBracket), RdRnImmSubfield, 0x0c0a, 0x11000000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchNumber|MatchImm|MatchRel|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchColon, MatchNumber|MatchOp4, MatchRightSquareBracket), RdRnImmSubfield, 0x0c0a, 0x91000000),
};
DECLARE_INSTRUCTION(ADD);

constexpr EncodingVariant ADDHN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 1, 0x0e204000),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 1, 0x0e204000),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV2D|MatchOp2), FpRdRnRmRa, 1, 0x0e204000),
};
DECLARE_INSTRUCTION(ADDHN);

constexpr EncodingVariant ADDHN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 1, 0x4e204000),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 1, 0x4e204000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV2D|MatchOp2), FpRdRnRmRa, 1, 0x4e204000),
};
DECLARE_INSTRUCTION(ADDHN2);

constexpr EncodingVariant ADDP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 0, 0x5ef1b800),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e20bc00),
};
DECLARE_INSTRUCTION(ADDP);

constexpr EncodingVariant ADDS_ENCODING_VARIANTS[] =
{
	// ADDS Wd, <Wn|WSP>, #<imm>, <shift>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchImm|MatchOp2), ArithmeticImmediate, 0, 0x31000000),
	// ADDS Wd, <Wn|WSP>, #<imm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchUImm12|MatchOp2, MatchComma, MatchLSL, MatchImm|MatchOp3), ArithmeticImmediate, 0, 0x31000000),
	// ADDS Wd, <Wn>, <Wm>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x2b000000),
	// ADDS Wd, <Wn>, <Wm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x2b000000),
	// ADDS Wd, <Wn|WSP>, <Wm>, <extend>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchExtendReg32|MatchOp3), ArithmeticExtendedRegister, 0, 0x2b200000),
	// ADDS Wd, <Wn|WSP>, <Wm>, <extend> #<amount>}
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg32|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0x2b200000),
	
	// ADDS Xd, <Xn|SP>, #<imm>, <shift>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2), ArithmeticImmediate, 0, 0xb1000000),
	// ADDS Xd, <Xn|SP>, #<imm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchUImm12|MatchOp2, MatchComma, MatchLSL, MatchImm|MatchOp3), ArithmeticImmediate, 0, 0xb1000000),
	// ADDS Xd, <Xn>, <Xm>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xab000000),
	// ADDS Xd, <Xn>, <Xm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xab000000),
	// ADDS Xd, <Xn|WSP>, <Wm>, <extend>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchExtendReg32|MatchOp3), ArithmeticExtendedRegister, 0, 0xab200000),
	// ADDS Xd, <Xn|WSP>, <Wm>, <extend> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg32|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0xab200000),
	// ADDS Xd, <Xn|WSP>, <Xm>, <extend>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchExtendReg64|MatchOp3), ArithmeticExtendedRegister, 0, 0xab200000),
	// ADDS Xd, <Xn|WSP>, <Xm>, <extend> #<amount>}
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg64|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0xab200000),
};
DECLARE_INSTRUCTION(ADDS);

constexpr EncodingVariant ADDV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0, MatchComma, MatchV8B16B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e31b800),
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchV4H8H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e31b800),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e31b800),
};
DECLARE_INSTRUCTION(ADDV);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(ADR, (MatchReg64|MatchOp0, MatchComma, MatchRel|MatchOp1), RdRel21HiLo, 0, 0x10000000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(ADRP, (MatchReg64|MatchOp0, MatchComma, MatchRel|MatchOp1), RdRel21HiLo, 1, 0x90000000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AESD, (MatchV16B|MatchOp0, MatchComma, MatchV16B|MatchOp1), FpRdRnRmRa, 0, 0x4e285800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AESE, (MatchV16B|MatchOp0, MatchComma, MatchV16B|MatchOp1), FpRdRnRmRa, 0, 0x4e284800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AESIMC, (MatchV16B|MatchOp0, MatchComma, MatchV16B|MatchOp1), FpRdRnRmRa, 0, 0x4e287800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AESMC, (MatchV16B|MatchOp0, MatchComma, MatchV16B|MatchOp1), FpRdRnRmRa, 0, 0x4e286800);

constexpr EncodingVariant AND_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchImm|MatchOp2), LogicalImmediate, 32, 0x12000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchImm|MatchOp2), LogicalImmediate, 64, 0x92000000),

	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x0a000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x8a000000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x0a000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x8a000000),
	
	// AND <Vd>.T, <Vn>.T, <Vm>.T
	DECLARE_CANDIDATE((MatchV8B16B|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0e201c00),
};
DECLARE_INSTRUCTION(AND);

constexpr EncodingVariant ANDS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchImm|MatchOp2), LogicalImmediate, 32, 0x72000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchImm|MatchOp2), LogicalImmediate, 64, 0xf2000000),
	
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x6a000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xea000000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x6a000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xea000000),
};
DECLARE_INSTRUCTION(ANDS);

constexpr EncodingVariant ASR_ENCODING_VARIANTS[] =
{
	// Immediate
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchImm|MatchOp2), RdRnImmrImms, 0, 0x13007c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchImm|MatchOp2), RdRnImmrImms, 0, 0x9340fc00),

	// Register
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2), RdRnRmImm6, 0, 0x1ac02800),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchOp2), RdRnRmImm6, 0, 0x9ac02800),
};
DECLARE_INSTRUCTION(ASR);

constexpr EncodingVariant ASRV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2), RdRnRmImm6, 0, 0x1ac02800),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchOp2), RdRnRmImm6, 0, 0x9ac02800),
};
DECLARE_INSTRUCTION(ASRV);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUTDA, (MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1), RdRnRmRa, 0, 0xdac11800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUTDZA, (MatchReg64|MatchOp0), RdRnRmRa, 0, 0xdac13be0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUTDB, (MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1), RdRnRmRa, 0, 0xdac11c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUTDZB, (MatchReg64|MatchOp0), RdRnRmRa, 0, 0xdac13fe0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUTIA, (MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1), RdRnRmRa, 0, 0xdac11000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUTIA1716, (), Fixed, 0, 0xd503219f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUTIASP, (), Fixed, 0, 0xd50323bf);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUTIAZ, (), Fixed, 0, 0xd503239f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUTIZA, (MatchReg64|MatchOp0), RdRnRmRa, 0, 0xdac133e0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUTIB, (MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1), RdRnRmRa, 0, 0xdac11400);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUTIB1716, (), Fixed, 0, 0xd50321df);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUTIBSP, (), Fixed, 0, 0xd50323ff);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUTIBZ, (), Fixed, 0, 0xd50323df);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUTIZB, (MatchReg64|MatchOp0), RdRnRmRa, 0, 0xdac137e0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B, (MatchRel|MatchOp0), Rel26, 0, 0x14000000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B_CC, (MatchRel|MatchOp1), RtRel19, 0, 0x54000003);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B_CS, (MatchRel|MatchOp1), RtRel19, 0, 0x54000002);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B_EQ, (MatchRel|MatchOp1), RtRel19, 0, 0x54000000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B_GE, (MatchRel|MatchOp1), RtRel19, 0, 0x5400000a);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B_GT, (MatchRel|MatchOp1), RtRel19, 0, 0x5400000c);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B_HI, (MatchRel|MatchOp1), RtRel19, 0, 0x54000008);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B_LE, (MatchRel|MatchOp1), RtRel19, 0, 0x5400000d);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B_LS, (MatchRel|MatchOp1), RtRel19, 0, 0x54000009);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B_LT, (MatchRel|MatchOp1), RtRel19, 0, 0x5400000b);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B_MI, (MatchRel|MatchOp1), RtRel19, 0, 0x54000004);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B_NE, (MatchRel|MatchOp1), RtRel19, 0, 0x54000001);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B_PL, (MatchRel|MatchOp1), RtRel19, 0, 0x54000005);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B_VC, (MatchRel|MatchOp1), RtRel19, 0, 0x54000007);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(B_VS, (MatchRel|MatchOp1), RtRel19, 0, 0x54000006);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BCAX, (MatchV16B|MatchOp0, MatchComma, MatchV16B|MatchOp1, MatchComma, MatchV16B|MatchOp2, MatchComma, MatchV16B|MatchOp3), FpRdRnRmRa, 0, 0xce200000);

constexpr EncodingVariant BFC_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0x21, 0x330003e0),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0x31, 0xb34003e0),
};
DECLARE_INSTRUCTION(BFC);

constexpr EncodingVariant BFI_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0x21, 0x33000000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0x31, 0xb3400000),
};
DECLARE_INSTRUCTION(BFI);

constexpr EncodingVariant BFM_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0, 0x33000000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0, 0xb3400000),
};
DECLARE_INSTRUCTION(BFM);

constexpr EncodingVariant BFXIL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0x4, 0x33000000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0x4, 0xb3400000),
};
DECLARE_INSTRUCTION(BFXIL);

constexpr EncodingVariant BIC_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x0a200000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x8a200000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x0a200000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x8a200000),

	// BIC <Vd>.T, #imm
	DECLARE_CANDIDATE((MatchV4H8H|MatchV2S4S|MatchOp0, MatchComma, MatchImm|MatchOp1), VectorImmediate, 0, 0x2f001400),
	DECLARE_CANDIDATE((MatchV4H8H|MatchV2S4S|MatchOp0, MatchComma, MatchImm|MatchOp1, MatchComma, MatchLSL, MatchImm|MatchOp3), VectorImmediate, 0, 0x2f001400),

	// BIC <Vd>.T, <Vn>.T, <Vm>.T
	DECLARE_CANDIDATE((MatchV8B16B|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0e601c00),
};
DECLARE_INSTRUCTION(BIC);

constexpr EncodingVariant BICS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x6a200000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xea200000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x6a200000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xea200000),
};
DECLARE_INSTRUCTION(BICS);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BIF, (MatchV8B|MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2ee01c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BIT, (MatchV8B|MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2ea01c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BL, (MatchRel|MatchOp0), Rel26, 0, 0x94000000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BLR, (MatchReg64|MatchOp1), RdRnRmImm6, 0, 0xd63f0000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BLRAA, (MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchReg64SP|MatchOp0), RdRnRmRa, 0, 0xd73f0800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BLRAAZ, (MatchReg64|MatchOp1), RdRnRmRa, 0, 0xd63f081f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BLRAB, (MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchReg64SP|MatchOp0), RdRnRmRa, 0, 0xd73f0c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BLRABZ, (MatchReg64|MatchOp1), RdRnRmRa, 0, 0xd63f081f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BR, (MatchReg64|MatchOp1), RdRnRmImm6, 0, 0xd61f0000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BRAA, (MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchReg64SP|MatchOp0), RdRnRmRa, 0, 0xd71f0800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BRAAZ, (MatchReg64|MatchOp1), RdRnRmRa, 0, 0xd61f081f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BRAB, (MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchReg64SP|MatchOp0), RdRnRmRa, 0, 0xd71f0c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BRABZ, (MatchReg64|MatchOp1), RdRnRmRa, 0, 0xd61f081f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BRK, (MatchUImm16|MatchOp1), RdImm16Shift, 0, 0xd4200000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BSL, (MatchV8B|MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2e601c00);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CASAB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x08e07c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CASALB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x08e0fc00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CASB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x08a07c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CASLB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x08a0fc00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CASAH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x48e07c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CASALH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x48e0fc00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CASH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x48a07c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CASLH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x48a0fc00);

constexpr EncodingVariant CASP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchRegNP1, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchRegNP1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x08207c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchRegNP1, MatchComma, MatchReg64|MatchOp0, MatchComma, MatchRegNP1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x48207c00),
};
DECLARE_INSTRUCTION(CASP);

constexpr EncodingVariant CASPA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchRegNP1, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchRegNP1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x08607c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchRegNP1, MatchComma, MatchReg64|MatchOp0, MatchComma, MatchRegNP1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x48607c00),
};
DECLARE_INSTRUCTION(CASPA);

constexpr EncodingVariant CASPAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchRegNP1, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchRegNP1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x0860fc00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchRegNP1, MatchComma, MatchReg64|MatchOp0, MatchComma, MatchRegNP1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x4860fc00),
};
DECLARE_INSTRUCTION(CASPAL);

constexpr EncodingVariant CASPL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchRegNP1, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchRegNP1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x0820fc00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchRegNP1, MatchComma, MatchReg64|MatchOp0, MatchComma, MatchRegNP1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x4820fc00),
};
DECLARE_INSTRUCTION(CASPL);

constexpr EncodingVariant CASA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x88e07c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0xc8e0fc00),
};
DECLARE_INSTRUCTION(CASA);

constexpr EncodingVariant CASAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x88e0fc00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0xc8e0fc00),
};
DECLARE_INSTRUCTION(CASAL);

constexpr EncodingVariant CAS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x88a07c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0xc8a07c00),
};
DECLARE_INSTRUCTION(CAS);

constexpr EncodingVariant CASL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0x88a0fc00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmImm6, 0, 0xc8a0fc00),
};
DECLARE_INSTRUCTION(CASL);

constexpr EncodingVariant CBNZ_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchRel|MatchOp1), RtRel19, 0, 0x35000000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchRel|MatchOp1), RtRel19, 0, 0xb5000000),
};
DECLARE_INSTRUCTION(CBNZ);

constexpr EncodingVariant CBZ_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchRel|MatchOp1), RtRel19, 0, 0x34000000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchRel|MatchOp1), RtRel19, 0, 0xb4000000),
};
DECLARE_INSTRUCTION(CBZ);

constexpr EncodingVariant CCMN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchImm|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchCondition|MatchOp3), RnImmNzcvCond, 0, 0x3a400800),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchImm|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchCondition|MatchOp3), RnImmNzcvCond, 0, 0xba400800),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchCondition|MatchOp3), RnRmNzcvCond, 0, 0x3a400000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchCondition|MatchOp3), RnRmNzcvCond, 0, 0xba400000),
};
DECLARE_INSTRUCTION(CCMN);

constexpr EncodingVariant CCMP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchImm|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchCondition|MatchOp3), RnImmNzcvCond, 0, 0x7a400800),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchImm|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchCondition|MatchOp3), RnImmNzcvCond, 0, 0xfa400800),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchCondition|MatchOp3), RnRmNzcvCond, 0, 0x7a400000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchCondition|MatchOp3), RnRmNzcvCond, 0, 0xfa400000),
};
DECLARE_INSTRUCTION(CCMP);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CFINV, (), Fixed, 0, 0xd500401f);

constexpr EncodingVariant CINC_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0x100, 0x1a800400),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0x100, 0x9a800400),
};
DECLARE_INSTRUCTION(CINC);

constexpr EncodingVariant CINV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0x100, 0x5a800000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0x100, 0xda800000),
};
DECLARE_INSTRUCTION(CINV);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CLREX, (), Fixed, 0, 0xd503305f);

constexpr EncodingVariant CLS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1), RdRnRmRa, 0, 0x5ac01400),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1), RdRnRmRa, 0, 0xdac01400),
	
	// CLS <Vd>.T, <Vn>.T
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 3, 0x0e204800),

};
DECLARE_INSTRUCTION(CLS);

constexpr EncodingVariant CLZ_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1), RdRnRmRa, 0, 0x5ac01000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1), RdRnRmRa, 0, 0xdac01000),

	// CLZ <Vd>.T, <Vn>.T
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 3, 0x2e204800),
};
DECLARE_INSTRUCTION(CLZ);

constexpr EncodingVariant CMEQ_ENCODING_VARIANTS[] =
{
	// Scalar: CMEQ <V><d>, <V><n>, <V><m>
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 0, 0x7ee08c00),

	// Vector: CMEQ <Vd>.<T>, <Vn>.<T>, <Vm>.<T>
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e208c00),

	// Scalar Zero: CMEQ <V><d>, <V><n>, #0
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 0, 0x5ee09800),

	// Vector Zero: CMEQ <Vd>.<T>, <Vn>.<T>, #0
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 3, 0x0e209800),
};
DECLARE_INSTRUCTION(CMEQ);

constexpr EncodingVariant CMGE_ENCODING_VARIANTS[] =
{
	// Scalar: CMGE <V><d>, <V><n>, <V><m>
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 0, 0x5ee03c00),
	
	// Vector: CMGE <Vd>.<T>, <Vn>.<T>, <Vm>.<T>
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e203c00),
	
	// Scalar Zero: CMGE <V><d>, <V><n>, #0
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 0, 0x7ee08800),
	
	// Vector Zero: CMGE <Vd>.<T>, <Vn>.<T>, #0
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 3, 0x2e208800),
};
DECLARE_INSTRUCTION(CMGE);

constexpr EncodingVariant CMGT_ENCODING_VARIANTS[] =
{
	// Scalar: CMGT <V><d>, <V><n>, <V><m>
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 0, 0x5ee03400),
	
	// Vector: CMGT <Vd>.<T>, <Vn>.<T>, <Vm>.<T>
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e203400),
	
	// Scalar Zero: CMGT <V><d>, <V><n>, #0
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 0, 0x5ee08800),
	
	// Vector Zero: CMGT <Vd>.<T>, <Vn>.<T>, #0
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 3, 0x0e208800),
};
DECLARE_INSTRUCTION(CMGT);

constexpr EncodingVariant CMHI_ENCODING_VARIANTS[] =
{
	// Scalar: CMHI <V><d>, <V><n>, <V><m>
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 0, 0x7ee03400),
	
	// Vector: CMHI <Vd>.<T>, <Vn>.<T>, <Vm>.<T>
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e203400),
};
DECLARE_INSTRUCTION(CMHI);

constexpr EncodingVariant CMHS_ENCODING_VARIANTS[] =
{
	// Scalar: CMHS <V><d>, <V><n>, <V><m>
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 0, 0x7ee03c00),
	
	// Vector: CMHS <Vd>.<T>, <Vn>.<T>, <Vm>.<T>
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e203c00),
};
DECLARE_INSTRUCTION(CMHS);

constexpr EncodingVariant CMLE_ENCODING_VARIANTS[] =
{
	// Scalar: CMGE <V><d>, <V><m>, <V><n>
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchRep|MatchOp2, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 0, 0x5ee03c00),
	
	// Vector: CMGE <Vd>.<T>, <Vm>.<T>, <Vn>.<T>
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp2, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 3, 0x0e203c00),
	
	// Scalar Zero: CMLE <V><d>, <V><n>, #0
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 0, 0x7ee09800),
	
	// Vector Zero: CMLE <Vd>.<T>, <Vn>.<T>, #0
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 3, 0x2e209800),
};
DECLARE_INSTRUCTION(CMLE);

constexpr EncodingVariant CMLT_ENCODING_VARIANTS[] =
{
	// Scalar: CMGT <V><d>, <V><m>, <V><n>
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchRep|MatchOp2, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 0, 0x5ee03400),
	
	// Vector: CMGT <Vd>.<T>, <Vm>.<T>, <Vn>.<T>
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp2, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 3, 0x0e203400),
	
	// Scalar Zero: CMLE <V><d>, <V><n>, #0
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 0, 0x5ee0a800),
	
	// Vector Zero: CMLE <Vd>.<T>, <Vn>.<T>, #0
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 3, 0x0e20a800),
};
DECLARE_INSTRUCTION(CMLT);

constexpr EncodingVariant CMN_ENCODING_VARIANTS[] =
{
	// CMN <Wn|WSP>, #<imm>, <shift>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchImm|MatchOp2), ArithmeticImmediate, 0, 0x3100001f),
	// CMN <Wn|WSP>, #<imm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchUImm12|MatchOp2, MatchComma, MatchLSL, MatchImm|MatchOp3), ArithmeticImmediate, 0, 0x3100001f),
	// CMN <Wn>, <Wm>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x2b00001f),
	// CMN <Wn>, <Wm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x2b00001f),
	// CMN <Wn|WSP>, <Wm>, <extend>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchExtendReg32|MatchOp3), ArithmeticExtendedRegister, 0, 0x2b20001f),
	// CMN <Wn|WSP>, <Wm>, <extend> #<amount>}
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg32|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0x2b20001f),
	
	// CMN <Xn|SP>, #<imm>, <shift>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2), ArithmeticImmediate, 0, 0xb100001f),
	// CMN <Xn|SP>, #<imm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchUImm12|MatchOp2, MatchComma, MatchLSL, MatchImm|MatchOp3), ArithmeticImmediate, 0, 0xb100001f),
	// CMN <Xn>, <Xm>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xab00001f),
	// CMN <Xn>, <Xm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xab00001f),
	// CMN <Xn|WSP>, <Wm>, <extend>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchExtendReg32|MatchOp3), ArithmeticExtendedRegister, 0, 0xab20001f),
	// CMN <Xn|WSP>, <Wm>, <extend> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg32|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0xab20001f),
	// CMN <Xn|WSP>, <Xm>, <extend>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchExtendReg64|MatchOp3), ArithmeticExtendedRegister, 0, 0xab20001f),
	// CMN <Xn|WSP>, <Xm>, <extend> #<amount>}
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg64|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0xab20001f),
};
DECLARE_INSTRUCTION(CMN);

constexpr EncodingVariant CMP_ENCODING_VARIANTS[] =
{
	// CMN <Wn|WSP>, #<imm>, <shift>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchImm|MatchOp2), ArithmeticImmediate, 0, 0x7100001f),
	// CMN <Wn|WSP>, #<imm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchUImm12|MatchOp2, MatchComma, MatchLSL, MatchImm|MatchOp3), ArithmeticImmediate, 0, 0x7100001f),
	// CMN <Wn>, <Wm>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x6b00001f),
	// CMN <Wn>, <Wm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x6b00001f),
	// CMN <Wn|WSP>, <Wm>, <extend>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchExtendReg32|MatchOp3), ArithmeticExtendedRegister, 0, 0x6b20001f),
	// CMN <Wn|WSP>, <Wm>, <extend> #<amount>}
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg32|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0x6b20001f),
	
	// CMN <Xn|SP>, #<imm>, <shift>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2), ArithmeticImmediate, 0, 0xf100001f),
	// CMN <Xn|SP>, #<imm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchUImm12|MatchOp2, MatchComma, MatchLSL, MatchImm|MatchOp3), ArithmeticImmediate, 0, 0xf100001f),
	// CMN <Xn>, <Xm>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xeb00001f),
	// CMN <Xn>, <Xm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xeb00001f),
	// CMN <Xn|WSP>, <Wm>, <extend>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchExtendReg32|MatchOp3), ArithmeticExtendedRegister, 0, 0xeb20001f),
	// CMN <Xn|WSP>, <Wm>, <extend> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg32|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0xeb20001f),
	// CMN <Xn|WSP>, <Xm>, <extend>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchExtendReg64|MatchOp3), ArithmeticExtendedRegister, 0, 0xeb20001f),
	// CMN <Xn|WSP>, <Xm>, <extend> #<amount>}
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg64|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0xeb20001f),
};
DECLARE_INSTRUCTION(CMP);

constexpr EncodingVariant CMTST_ENCODING_VARIANTS[] =
{
	// Scalar: CMTST <V><d>, <V><n>, <V><m>
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 0, 0x5ee08c00),
	
	// Vector: CMTST <Vd>.<T>, <Vm>.<T>, <Vn>.<T>
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e208c00),
};
DECLARE_INSTRUCTION(CMTST);

constexpr EncodingVariant CNEG_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0x100, 0x5a800400),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0x100, 0xda800400),
};
DECLARE_INSTRUCTION(CNEG);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CNT, (MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 3, 0x0e205800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CRC32B, (MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x1ac04000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CRC32H, (MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x1ac04400);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CRC32W, (MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x1ac04800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CRC32X, (MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), RdRnRmImm6, 0, 0x9ac04c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CRC32CB, (MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x1ac05000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CRC32CH, (MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x1ac05400);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CRC32CW, (MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x1ac05800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CRC32CX, (MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), RdRnRmImm6, 0, 0x9ac05c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CSDB, (), Fixed, 0, 0xd503229f);

constexpr EncodingVariant CSEL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0, 0x1a800000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0, 0x9a800000),
};
DECLARE_INSTRUCTION(CSEL);

constexpr EncodingVariant CSET_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0x100, 0x1a9f07e0),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0x100, 0x9a9f07e0),
};
DECLARE_INSTRUCTION(CSET);

constexpr EncodingVariant CSETM_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0x100, 0x5a9f03e0),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0x100, 0xda9f03e0),
};
DECLARE_INSTRUCTION(CSETM);

constexpr EncodingVariant CSINC_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0, 0x1a800400),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0, 0x9a800400),
};
DECLARE_INSTRUCTION(CSINC);

constexpr EncodingVariant CSINV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0, 0x5a800000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0, 0xda800000),
};
DECLARE_INSTRUCTION(CSINV);

constexpr EncodingVariant CSNEG_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0, 0x5a800400),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0, 0xda800400),
};
DECLARE_INSTRUCTION(CSNEG);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(DCPS1, (MatchUImm16|MatchOp1), RdHwImm16, 0, 0xd4a00001);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(DCPS2, (MatchUImm16|MatchOp1), RdHwImm16, 0, 0xd4a00002);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(DCPS3, (MatchUImm16|MatchOp1), RdHwImm16, 0, 0xd4a00003);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(DRPS, (), Fixed, 0, 0xd6bf03e0);

constexpr EncodingVariant DUP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0|MatchOp7, MatchComma, MatchVB|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x5e000400),
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchVH|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x5e000400),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchVS|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x5e000400),
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchVD|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x5e000400),

	DECLARE_CANDIDATE((MatchV8B16B|MatchOp0|MatchOp7, MatchComma, MatchVB|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 2, 0x0e000400),
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchVH|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 2, 0x0e000400),
	DECLARE_CANDIDATE((MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchVS|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 2, 0x0e000400),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0|MatchOp7, MatchComma, MatchVD|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 2, 0x0e000400),
};
DECLARE_INSTRUCTION(DUP);

constexpr EncodingVariant EON_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x4a200000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xca200000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x4a200000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xca200000),
};
DECLARE_INSTRUCTION(EON);

constexpr EncodingVariant EOR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchImm|MatchOp2), LogicalImmediate, 32, 0x52000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchImm|MatchOp2), LogicalImmediate, 64, 0xd2000000),
	
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x4a000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xca000000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x4a000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xca000000),

	// EOR <Vd>.T, <Vn>.T, <Vm>.T
	DECLARE_CANDIDATE((MatchV8B16B|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2e201c00),
};
DECLARE_INSTRUCTION(EOR);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(EOR3, (MatchV16B|MatchOp0, MatchComma, MatchV16B|MatchOp1, MatchComma, MatchV16B|MatchOp2, MatchComma, MatchV16B|MatchOp3), FpRdRnRmRa, 0, 0xce000000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(ERET, (), Fixed, 0, 0xd69f03e0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(ERETAA, (), Fixed, 0, 0xd69f0bff);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(ERETAB, (), Fixed, 0, 0xd69f0fff);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(ESB, (), Fixed, 0, 0xd503221f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(EXT, (MatchV8B16B|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x102, 0x2e000000);

constexpr EncodingVariant EXTR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnRmImm6, 0, 0x13800000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnRmImm6, 0, 0x93c00000),
};
DECLARE_INSTRUCTION(EXTR);

constexpr EncodingVariant FABD_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 0, 0x7ec01400),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 4, 0x7ea0d400),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2ec01400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x2ea0d400),
};
DECLARE_INSTRUCTION(FABD);

constexpr EncodingVariant FABS_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 8, 0x1e20c000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x0ef8f800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x0ea0f800),
};
DECLARE_INSTRUCTION(FABS);

constexpr EncodingVariant FACGE_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 0, 0x7e402c00),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 4, 0x7e20ec00),

	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2e402c00),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x2e20ec00),
};
DECLARE_INSTRUCTION(FACGE);

constexpr EncodingVariant FACGT_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 0, 0x7ec02c00),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 4, 0x7ea0ec00),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2ec02c00),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x2ea0ec00),
};
DECLARE_INSTRUCTION(FACGT);

constexpr EncodingVariant FADD_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 8, 0x1e202800),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0e401400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x0e20d400),
};
DECLARE_INSTRUCTION(FADD);

constexpr EncodingVariant FADDP_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchV2H|MatchOp1), FpRdRnRmRa, 0, 0x5e30d800),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchV2S|MatchOp1), FpRdRnRmRa, 0, 0x7e30d800),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 0, 0x7e70d800),

	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2e401400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x2e20d400),
};
DECLARE_INSTRUCTION(FADDP);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FCCMP, (MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchCondition|MatchOp3), RnRmNzcvCond, 8, 0x1e200400);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FCCMPE, (MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchCondition|MatchOp3), RnRmNzcvCond, 8, 0x1e200410);

constexpr EncodingVariant FCMEQ_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1, MatchComma, MatchH|MatchOp2), FpRdRnRmRa, 0, 0x5e402400),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 4, 0x5e20e400),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0e402400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x0e20e400),
	
	// Scalar Zero
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 0, 0x5ef8d800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 4, 0x5ea0d800),
	
	// Vector Zero
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 2, 0x0ef8d800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 6, 0x0ea0d800),
};
DECLARE_INSTRUCTION(FCMEQ);

constexpr EncodingVariant FCMGE_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1, MatchComma, MatchH|MatchOp2), FpRdRnRmRa, 0, 0x7e402400),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 4, 0x7e20e400),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2e402400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x2e20e400),
	
	// Scalar Zero
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 0, 0x7ef8c800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 4, 0x7ea0c800),
	
	// Vector Zero
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 2, 0x2ef8c800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 6, 0x2ea0c800),
};
DECLARE_INSTRUCTION(FCMGE);

constexpr EncodingVariant FCMGT_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1, MatchComma, MatchH|MatchOp2), FpRdRnRmRa, 0, 0x7ec02400),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 4, 0x7ea0e400),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2ec02400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x2ea0e400),
	
	// Scalar Zero
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 0, 0x5ef8c800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 4, 0x5ea0c800),
	
	// Vector Zero
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 2, 0x0ef8c800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 6, 0x0ea0c800),
};
DECLARE_INSTRUCTION(FCMGT);

constexpr EncodingVariant FCMLE_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp2, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x7e402400),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp2, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x7e20e400),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp2, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2e402400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp2, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x2e20e400),
	
	// Scalar Zero
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 0, 0x7ef8d800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 4, 0x7ea0d800),
	
	// Vector Zero
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 2, 0x2ef8d800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 6, 0x2ea0d800),
};
DECLARE_INSTRUCTION(FCMLE);

constexpr EncodingVariant FCMLT_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp2, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x7ec02400),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp2, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x7ea0e400),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp2, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2ec02400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp2, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x2ea0e400),
	
	// Scalar Zero
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 0, 0x5ef8e800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 4, 0x5ea0e800),
	
	// Vector Zero
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 2, 0x0ef8e800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm0), FpRdRnRmRa, 6, 0x0ea0e800),
};
DECLARE_INSTRUCTION(FCMLT);

constexpr EncodingVariant FCMP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchFpAll|MatchOp1|MatchOp7, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 8, 0x1e202000),
	DECLARE_CANDIDATE((MatchFpAll|MatchOp1|MatchOp7, MatchComma, MatchImm0), FpRdRnRmRa, 8, 0x1e202008),
};
DECLARE_INSTRUCTION(FCMP);

constexpr EncodingVariant FCMPE_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchFpAll|MatchOp1|MatchOp7, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 8, 0x1e202010),
	DECLARE_CANDIDATE((MatchFpAll|MatchOp1|MatchOp7, MatchComma, MatchImm0), FpRdRnRmRa, 8, 0x1e202018),
};
DECLARE_INSTRUCTION(FCMPE);

constexpr EncodingVariant FCSEL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1, MatchComma, MatchH|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0, 0x1ee00c00),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchS|MatchOp1, MatchComma, MatchS|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0, 0x1e200c00),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchD|MatchOp1, MatchComma, MatchD|MatchOp2, MatchComma, MatchCondition|MatchOp3), RdRnRmCond, 0, 0x1e600c00),
};
DECLARE_INSTRUCTION(FCSEL);

constexpr EncodingVariant FCVT_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x1ee24000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x1ee2c000),
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchS|MatchOp1), FpRdRnRmRa, 0, 0x1e23c000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchS|MatchOp1), FpRdRnRmRa, 0, 0x1e22c000),
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchD|MatchOp1), FpRdRnRmRa, 0, 0x1e63c000),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchD|MatchOp1), FpRdRnRmRa, 0, 0x1e624000),
};
DECLARE_INSTRUCTION(FCVT);

constexpr EncodingVariant FCVTAS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x5e79c800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x5e21c800),
	
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x0e79c800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x0e21c800),
	
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x1e240000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x9e240000),
};
DECLARE_INSTRUCTION(FCVTAS);

constexpr EncodingVariant FCVTAU_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x7e79c800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x7e21c800),
	
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2e79c800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x2e21c800),
	
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x1e250000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x9e250000),
};
DECLARE_INSTRUCTION(FCVTAU);

constexpr EncodingVariant FCVTL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1), FpRdRnRmRa, 0, 0x0e217800),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1), FpRdRnRmRa, 0, 0x0e617800),
};
DECLARE_INSTRUCTION(FCVTL);

constexpr EncodingVariant FCVTL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1), FpRdRnRmRa, 0, 0x4e217800),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1), FpRdRnRmRa, 0, 0x4e617800),
};
DECLARE_INSTRUCTION(FCVTL2);

constexpr EncodingVariant FCVTMS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x5e79b800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x5e21b800),
	
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x0e79b800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x0e21b800),
	
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x1e300000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x9e300000),
};
DECLARE_INSTRUCTION(FCVTMS);

constexpr EncodingVariant FCVTMU_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x7e79b800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x7e21b800),
	
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2e79b800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x2e21b800),
	
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x1e310000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x9e310000),
};
DECLARE_INSTRUCTION(FCVTMU);

constexpr EncodingVariant FCVTN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4H|MatchOp0, MatchComma, MatchV4S|MatchOp1), FpRdRnRmRa, 0, 0x0e216800),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 0, 0x0e616800),
};
DECLARE_INSTRUCTION(FCVTN);

constexpr EncodingVariant FCVTN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV4S|MatchOp1), FpRdRnRmRa, 0, 0x4e216800),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 0, 0x4e616800),
};
DECLARE_INSTRUCTION(FCVTN2);

constexpr EncodingVariant FCVTNS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x5e79a800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x5e21a800),
	
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x0e79a800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x0e21a800),
	
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x1e200000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x9e200000),
};
DECLARE_INSTRUCTION(FCVTNS);

constexpr EncodingVariant FCVTNU_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x7e79a800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x7e21a800),
	
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2e79a800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x2e21a800),
	
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x1e210000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x9e210000),
};
DECLARE_INSTRUCTION(FCVTNU);

constexpr EncodingVariant FCVTPS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x5ef9a800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x5ea1a800),
	
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x0ef9a800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x0ea1a800),
	
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x1e280000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x9e280000),
};
DECLARE_INSTRUCTION(FCVTPS);

constexpr EncodingVariant FCVTPU_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x7ef9a800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x7ea1a800),
	
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2ef9a800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x2ea1a800),
	
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x1e290000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchFpAll|MatchOp1|MatchOp7), FpRdRnRmRa, 8, 0x9e290000),
};
DECLARE_INSTRUCTION(FCVTPU);

constexpr EncodingVariant FCVTXN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchD|MatchOp1|MatchOp7), FpRdRnRmRa, 4, 0x7e216800),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0, MatchComma, MatchV2D|MatchOp1|MatchOp7), FpRdRnRmRa, 4, 0x2e216800),
};
DECLARE_INSTRUCTION(FCVTXN);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FCVTXN2, (MatchV4S|MatchOp0, MatchComma, MatchV2D|MatchOp1|MatchOp7), FpRdRnRmRa, 4, 0x6e216800);

constexpr EncodingVariant FCVTZS_ENCODING_VARIANTS[] =
{
	// Vector, Fixed-point
	DECLARE_CANDIDATE((MatchH|MatchS|MatchD|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp2), FpRdRnFixedPointShift, 1, 0x5f00fc00),
	DECLARE_CANDIDATE((MatchV4H8H|MatchV2S4S2D|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp2), FpRdRnFixedPointShift, 5, 0x0f00fc00),

	// Vector, Integer
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x5ef9b800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x5ea1b800),
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x0ef9b800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x0ea1b800),

	// Scalar, Fixed-Point
	DECLARE_CANDIDATE((MatchReg32|MatchReg64|MatchOp0, MatchComma, MatchH|MatchS|MatchD|MatchOp1, MatchComma, MatchImm|MatchOp2), FpRdRnFixedPointShift, 0x1a, 0x1e180000),

	// Scalar, Integer
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x1ef80000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x9ef80000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchS|MatchOp1), FpRdRnRmRa, 0, 0x1e380000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchS|MatchOp1), FpRdRnRmRa, 0, 0x9e380000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchD|MatchOp1), FpRdRnRmRa, 0, 0x1e780000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchD|MatchOp1), FpRdRnRmRa, 0, 0x9e780000),
};
DECLARE_INSTRUCTION(FCVTZS);

constexpr EncodingVariant FCVTZU_ENCODING_VARIANTS[] =
{
	// Vector, Fixed-point
	DECLARE_CANDIDATE((MatchH|MatchS|MatchD|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp2), FpRdRnFixedPointShift, 1, 0x7f00fc00),
	DECLARE_CANDIDATE((MatchV4H8H|MatchV2S4S2D|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp2), FpRdRnFixedPointShift, 5, 0x2f00fc00),
	
	// Vector, Integer
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x7ef9b800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x7ea1b800),
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2ef9b800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x2ea1b800),
	
	// Scalar, Fixed-Point
	DECLARE_CANDIDATE((MatchReg32|MatchReg64|MatchOp0, MatchComma, MatchH|MatchS|MatchD|MatchOp1, MatchComma, MatchImm|MatchOp2), FpRdRnFixedPointShift, 0x1a, 0x1e190000),
	
	// Scalar, Integer
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x1ef90000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x9ef90000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchS|MatchOp1), FpRdRnRmRa, 0, 0x1e390000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchS|MatchOp1), FpRdRnRmRa, 0, 0x9e390000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchD|MatchOp1), FpRdRnRmRa, 0, 0x1e790000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchD|MatchOp1), FpRdRnRmRa, 0, 0x9e790000),
};
DECLARE_INSTRUCTION(FCVTZU);

constexpr EncodingVariant FDIV_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 8, 0x1e201800),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2e403c00),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x2e20fc00),
};
DECLARE_INSTRUCTION(FDIV);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FJCVTZS, (MatchReg32|MatchOp0, MatchComma, MatchD|MatchOp1), RdRnRmRa, 0, 0x1e7e0000);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FMADD, (MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2, MatchComma, MatchRep|MatchOp3), FpRdRnRmRa, 8, 0x1f000000);

constexpr EncodingVariant FMAX_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 8, 0x1e204800),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0e403400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x0e20f400),
};
DECLARE_INSTRUCTION(FMAX);

constexpr EncodingVariant FMAXNM_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 8, 0x1e206800),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0e400400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x0e20c400),
};
DECLARE_INSTRUCTION(FMAXNM);

constexpr EncodingVariant FMAXNMP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchV2H|MatchOp1), FpRdRnRmRa, 0, 0x5e30c800),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchV2S|MatchOp1), FpRdRnRmRa, 4, 0x7e30c800),
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 4, 0x7e30c800),
	
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2e400400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x2e20c400),
};
DECLARE_INSTRUCTION(FMAXNMP);

constexpr EncodingVariant FMAXNMV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchV4H8H|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x0e30c800),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x2e30c800),
};
DECLARE_INSTRUCTION(FMAXNMV);

constexpr EncodingVariant FMAXP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchV2H|MatchOp1), FpRdRnRmRa, 0, 0x5e30f800),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchV2S|MatchOp1), FpRdRnRmRa, 4, 0x7e30f800),
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 4, 0x7e30f800),

	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2e403400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x2e20f400),
};
DECLARE_INSTRUCTION(FMAXP);

constexpr EncodingVariant FMAXV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchV4H8H|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x0e30f800),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x2e30f800),
};
DECLARE_INSTRUCTION(FMAXV);

constexpr EncodingVariant FMIN_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 8, 0x1e205800),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0ec03400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x0ea0f400),
};
DECLARE_INSTRUCTION(FMIN);

constexpr EncodingVariant FMINNM_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 8, 0x1e207800),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0ec00400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x0ea0c400),
};
DECLARE_INSTRUCTION(FMINNM);

constexpr EncodingVariant FMINNMP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchV2H|MatchOp1), FpRdRnRmRa, 0, 0x5eb0c800),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchV2S|MatchOp1), FpRdRnRmRa, 4, 0x7eb0c800),
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 4, 0x7eb0c800),
	
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2ec00400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x2ea0c400),
};
DECLARE_INSTRUCTION(FMINNMP);

constexpr EncodingVariant FMINNMV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchV4H8H|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x0eb0c800),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x2eb0c800),
};
DECLARE_INSTRUCTION(FMINNMV);

constexpr EncodingVariant FMINP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchV2H|MatchOp1), FpRdRnRmRa, 0, 0x5eb0f800),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchV2S|MatchOp1), FpRdRnRmRa, 4, 0x7eb0f800),
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 4, 0x7eb0f800),
	
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2ec03400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x2ea0f400),
};
DECLARE_INSTRUCTION(FMINP);

constexpr EncodingVariant FMINV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchV4H8H|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x0eb0f800),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x2eb0f800),
};
DECLARE_INSTRUCTION(FMINV);

constexpr EncodingVariant FMLA_ENCODING_VARIANTS[] =
{
	// Scalar Element
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x5f001000),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x5f801000),
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVD|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x5f801000),

	// Vector Element
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x0f001000),
	DECLARE_CANDIDATE((MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x0f801000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVD|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x0f801000),

	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0e400c00),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x0e20cc00),
};
DECLARE_INSTRUCTION(FMLA);

constexpr EncodingVariant FMLAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2H|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x0f800000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV4H|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x0f800000),
	
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2H|MatchOp1, MatchComma, MatchV2H|MatchOp2), FpRdRnRmRa, 2, 0x0e20ec00),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV4H|MatchOp1, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 2, 0x0e20ec00),
};
DECLARE_INSTRUCTION(FMLAL);

constexpr EncodingVariant FMLAL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2H|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x2f808000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV4H|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x2f808000),
	
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2H|MatchOp1, MatchComma, MatchV2H|MatchOp2), FpRdRnRmRa, 2, 0x0e20cc00),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV4H|MatchOp1, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 2, 0x0e20cc00),
};
DECLARE_INSTRUCTION(FMLAL2);

constexpr EncodingVariant FMLS_ENCODING_VARIANTS[] =
{
	// Scalar Element
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x5f005000),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x5f805000),
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVD|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x5f805000),
	
	// Vector Element
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x0f005000),
	DECLARE_CANDIDATE((MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x0f805000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVD|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x0f805000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0ec00c00),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x0ea0cc00),
};
DECLARE_INSTRUCTION(FMLS);

constexpr EncodingVariant FMLSL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2H|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x0f804000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV4H|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x0f800000),
	
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2H|MatchOp1, MatchComma, MatchV2H|MatchOp2), FpRdRnRmRa, 2, 0x0ea0ec00),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV4H|MatchOp1, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 2, 0x0ea0ec00),
};
DECLARE_INSTRUCTION(FMLSL);

constexpr EncodingVariant FMLSL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2H|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x2f80c000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV4H|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x2f80c000),
	
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2H|MatchOp1, MatchComma, MatchV2H|MatchOp2), FpRdRnRmRa, 2, 0x0ea0cc00),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV4H|MatchOp1, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 2, 0x0ea0cc00),
};
DECLARE_INSTRUCTION(FMLSL2);

constexpr EncodingVariant FMOV_ENCODING_VARIANTS[] =
{
	// Vector, Immediate
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchImm|MatchFloatImm|MatchOp1), Fmov, 0x22, 0x0f00fc00),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchImm|MatchFloatImm|MatchOp1), Fmov, 0x26, 0x0f00f400),

	// Register
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 8, 0x1e204000),

	// General
	DECLARE_CANDIDATE((MatchReg32|MatchReg64|MatchOp0|MatchOp6, MatchComma, MatchH|MatchOp2|MatchOp7), Fmov, 9, 0x1e260000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0|MatchOp6, MatchComma, MatchS|MatchOp2|MatchOp7), Fmov, 9, 0x1e260000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0|MatchOp6, MatchComma, MatchD|MatchOp2|MatchOp7), Fmov, 9, 0x1e260000),
	DECLARE_CANDIDATE((MatchH|MatchS|MatchOp0|MatchOp7, MatchComma, MatchReg32|MatchOp2|MatchOp6), Fmov, 9, 0x1e270000),
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchReg64|MatchOp2|MatchOp6), Fmov, 9, 0x1e270000),

	DECLARE_CANDIDATE((MatchVD|MatchOp0, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), Fmov, 0x40, 0x9eaf0000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchVD|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchRightSquareBracket), Fmov, 0x40, 0x9eae0000),

	// Scalar, Immediate
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchImm|MatchFloatImm|MatchOp1), Fmov, 0x11, 0x1e201000),
};
DECLARE_INSTRUCTION(FMOV);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FMSUB, (MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2, MatchComma, MatchRep|MatchOp3), FpRdRnRmRa, 8, 0x1f008000);

constexpr EncodingVariant FMUL_ENCODING_VARIANTS[] =
{
	// Scalar Element
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x5f009000),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x5f809000),
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVD|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x5f809000),
	
	// Vector Element
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x0f009000),
	DECLARE_CANDIDATE((MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x0f809000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVD|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x0f809000),
	
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 8, 0x1e200800),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2e401c00),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x2e20dc00),
};
DECLARE_INSTRUCTION(FMUL);

constexpr EncodingVariant FMULX_ENCODING_VARIANTS[] =
{
	// Scalar Element
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x7f009000),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x7f809000),
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVD|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x7f809000),
	
	// Vector Element
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x802, 0x2f009000),
	DECLARE_CANDIDATE((MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x2f809000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVD|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x806, 0x2f809000),

	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x5e401c00),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x5e20dc00),

	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0e401c00),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x0e20dc00),
};
DECLARE_INSTRUCTION(FMULX);

constexpr EncodingVariant FNEG_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 8, 0x1e214000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2ef8f800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x2ea0f800),
};
DECLARE_INSTRUCTION(FNEG);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FNMADD, (MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2, MatchComma, MatchRep|MatchOp3), FpRdRnRmRa, 8, 0x1f200000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FNMSUB, (MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2, MatchComma, MatchRep|MatchOp3), FpRdRnRmRa, 8, 0x1f208000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FNMUL, (MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 8, 0x1e208800);

constexpr EncodingVariant FRECPE_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x5ef9d800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x5ea1d800),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x0ef9d800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x0ea1d800),
};
DECLARE_INSTRUCTION(FRECPE);

constexpr EncodingVariant FRECPS_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1, MatchComma, MatchH|MatchOp2), FpRdRnRmRa, 0, 0x5e403c00),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 4, 0x5e20fc00),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0e403c00),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x0e20fc00),
};
DECLARE_INSTRUCTION(FRECPS);

constexpr EncodingVariant FRECPX_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x5ef9f800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x5ea1f800),
};
DECLARE_INSTRUCTION(FRECPX);

constexpr EncodingVariant FRINTA_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 8, 0x1e264000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2e798800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x2e218800),
};
DECLARE_INSTRUCTION(FRINTA);

constexpr EncodingVariant FRINTI_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 8, 0x1e27c000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2ef99800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x2ea19800),
};
DECLARE_INSTRUCTION(FRINTI);

constexpr EncodingVariant FRINTM_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 8, 0x1e254000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x0e799800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x0e219800),
};
DECLARE_INSTRUCTION(FRINTM);

constexpr EncodingVariant FRINTN_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 8, 0x1e244000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x0e798800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x0e218800),
};
DECLARE_INSTRUCTION(FRINTN);

constexpr EncodingVariant FRINTP_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 8, 0x1e24c000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x0ef98800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x0ea18800),
};
DECLARE_INSTRUCTION(FRINTP);

constexpr EncodingVariant FRINTX_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 8, 0x1e274000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2e799800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x2e219800),
};
DECLARE_INSTRUCTION(FRINTX);

constexpr EncodingVariant FRINTZ_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 8, 0x1e25c000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x0ef99800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x0ea19800),
};
DECLARE_INSTRUCTION(FRINTZ);

constexpr EncodingVariant FRSQRTE_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x7ef9d800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x7ea1d800),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2ef9d800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x2ea1d800),
};
DECLARE_INSTRUCTION(FRSQRTE);

constexpr EncodingVariant FRSQRTS_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchH|MatchOp1, MatchComma, MatchH|MatchOp2), FpRdRnRmRa, 0, 0x5ec03c00),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 4, 0x5ea0fc00),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0ec03c00),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x0ea0fc00),
};
DECLARE_INSTRUCTION(FRSQRTS);

constexpr EncodingVariant FSQRT_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 8, 0x1e21c000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2ef9f800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x2ea1f800),
};
DECLARE_INSTRUCTION(FSQRT);

constexpr EncodingVariant FSUB_ENCODING_VARIANTS[] =
{
	// Scalar
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 8, 0x1e203800),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0ec01400),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 6, 0x0ea0d400),
};
DECLARE_INSTRUCTION(FSUB);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(HLT, (MatchUImm16|MatchOp1), RdImm16Shift, 0, 0xd4400000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(HVC, (MatchUImm16|MatchOp1), RdImm16Shift, 0, 0xd4000002);

constexpr EncodingVariant INS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchVB|MatchOp0|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket, MatchComma, MatchVB|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x6e000400),
	DECLARE_CANDIDATE((MatchVH|MatchOp0|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket, MatchComma, MatchVH|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x6e000400),
	DECLARE_CANDIDATE((MatchVS|MatchOp0|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket, MatchComma, MatchVS|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x6e000400),
	DECLARE_CANDIDATE((MatchVD|MatchOp0|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket, MatchComma, MatchVD|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x6e000400),
	
	DECLARE_CANDIDATE((MatchVB|MatchVH|MatchVS|MatchOp0|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket, MatchComma, MatchReg32|MatchOp1), FpRdRnIndex1Index2, 0, 0x4e001c00),
	DECLARE_CANDIDATE((MatchVD|MatchOp0|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp1), FpRdRnIndex1Index2, 0, 0x4e001c00),
};
DECLARE_INSTRUCTION(INS);

constexpr EncodingVariant LD1_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0c407000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x92, 0x0cc07000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0cc07000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0c40a000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x92, 0x0cc0a000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0cc0a000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0c406000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x92, 0x0cc06000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0cc06000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0c402000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x92, 0x0cc02000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0cc02000),

	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x40, 0x0d400000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x60, 0x0dc00000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x40, 0x0dc00000),
};
DECLARE_INSTRUCTION(LD1);

constexpr EncodingVariant LD1R_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0d40c000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x32, 0x0dc0c000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0dc0c000),
};
DECLARE_INSTRUCTION(LD1R);

constexpr EncodingVariant LD2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0c408000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x92, 0x0cc08000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0cc08000),
	
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x40, 0x0d600000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x60, 0x0de00000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x40, 0x0de00000),
};
DECLARE_INSTRUCTION(LD2);

constexpr EncodingVariant LD2R_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0d60c000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x32, 0x0de0c000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0de0c000),
};
DECLARE_INSTRUCTION(LD2R);

constexpr EncodingVariant LD3_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0c404000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x92, 0x0cc04000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0cc04000),
	
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x40, 0x0d402000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x60, 0x0dc02000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x40, 0x0dc02000),
};
DECLARE_INSTRUCTION(LD3);

constexpr EncodingVariant LD3R_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0d40e000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x32, 0x0dc0e000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0dc0e000),
};
DECLARE_INSTRUCTION(LD3R);

constexpr EncodingVariant LD4_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0c400000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x92, 0x0cc00000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0cc00000),
	
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x40, 0x0d602000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x60, 0x0de02000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x40, 0x0de02000),
};
DECLARE_INSTRUCTION(LD4);

constexpr EncodingVariant LD4R_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0d60e000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x32, 0x0de0e000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0de0e000),
};
DECLARE_INSTRUCTION(LD4R);

constexpr EncodingVariant LDADD_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8200000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8200000),
};
DECLARE_INSTRUCTION(LDADD);

constexpr EncodingVariant LDADDA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8a00000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8a00000),
};
DECLARE_INSTRUCTION(LDADDA);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDADDAB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38a00000);

constexpr EncodingVariant LDADDAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8e00000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8e00000),
};
DECLARE_INSTRUCTION(LDADDAL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDADDALB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38e00000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDADDB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38200000);

constexpr EncodingVariant LDADDL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8600000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8600000),
};
DECLARE_INSTRUCTION(LDADDL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDADDLB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38600000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDADDAH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78a00000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDADDALH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78e00000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDADDH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78200000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDADDLH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78600000);

constexpr EncodingVariant LDAPR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8bfc000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8bfc000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8bfc000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8bfc000),
};
DECLARE_INSTRUCTION(LDAPR);

constexpr EncodingVariant LDAPRB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78bfc000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x78bfc000),
};
DECLARE_INSTRUCTION(LDAPRB);

constexpr EncodingVariant LDAPRH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38bfc000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x38bfc000),
};
DECLARE_INSTRUCTION(LDAPRH);

constexpr EncodingVariant LDAPUR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x99400000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x99400000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xd9400000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xd9400000),
};
DECLARE_INSTRUCTION(LDAPUR);

constexpr EncodingVariant LDAPURB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x19400000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x19400000),
};
DECLARE_INSTRUCTION(LDAPURB);

constexpr EncodingVariant LDAPURH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x59400000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x59400000),
};
DECLARE_INSTRUCTION(LDAPURH);

constexpr EncodingVariant LDAPURSB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x19c00000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x19c00000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x19800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x19800000),
};
DECLARE_INSTRUCTION(LDAPURSB);

constexpr EncodingVariant LDAPURSH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x59c00000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x59c00000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x59800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x59800000),
};
DECLARE_INSTRUCTION(LDAPURSH);

constexpr EncodingVariant LDAPURSW_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x99800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x99800000),
};
DECLARE_INSTRUCTION(LDAPURSW);

constexpr EncodingVariant LDAR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x88dffc00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x88dffc00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xc8dffc00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0xc8dffc00),
};
DECLARE_INSTRUCTION(LDAR);

constexpr EncodingVariant LDARB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x08dffc00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x08dffc00),
};
DECLARE_INSTRUCTION(LDARB);

constexpr EncodingVariant LDARH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x48dffc00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x48dffc00),
};
DECLARE_INSTRUCTION(LDARH);

constexpr EncodingVariant LDAXP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x887f8000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x887f8000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xc87f8000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0xc87f8000),
};
DECLARE_INSTRUCTION(LDAXP);

constexpr EncodingVariant LDAXR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x885ffc00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x885ffc00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xc85ffc00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0xc85ffc00),
};
DECLARE_INSTRUCTION(LDAXR);

constexpr EncodingVariant LDAXRB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x085ffc00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x085ffc00),
};
DECLARE_INSTRUCTION(LDAXRB);

constexpr EncodingVariant LDAXRH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x485ffc00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x485ffc00),
};
DECLARE_INSTRUCTION(LDAXRH);

constexpr EncodingVariant LDCLR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8201000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8201000),
};
DECLARE_INSTRUCTION(LDCLR);

constexpr EncodingVariant LDCLRA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8a01000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8a01000),
};
DECLARE_INSTRUCTION(LDCLRA);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDCLRAB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38a01000);

constexpr EncodingVariant LDCLRAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8e01000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8e01000),
};
DECLARE_INSTRUCTION(LDCLRAL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDCLRALB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38e01000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDCLRB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38201000);

constexpr EncodingVariant LDCLRL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8601000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8601000),
};
DECLARE_INSTRUCTION(LDCLRL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDCLRLB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38601000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDCLRAH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78a01000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDCLRALH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78e01000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDCLRH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78201000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDCLRLH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78601000);

constexpr EncodingVariant LDEOR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8202000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8202000),
};
DECLARE_INSTRUCTION(LDEOR);

constexpr EncodingVariant LDEORA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8a02000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8a02000),
};
DECLARE_INSTRUCTION(LDEORA);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDEORAB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38a02000);

constexpr EncodingVariant LDEORAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8e02000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8e02000),
};
DECLARE_INSTRUCTION(LDEORAL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDEORALB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38e02000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDEORB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38202000);

constexpr EncodingVariant LDEORL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8602000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8602000),
};
DECLARE_INSTRUCTION(LDEORL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDEORLB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38602000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDEORAH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78a02000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDEORALH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78e02000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDEORH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78202000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDEORLH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78602000);

constexpr EncodingVariant LDLARB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x08df7c00),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x08df7c00),
};
DECLARE_INSTRUCTION(LDLARB);

constexpr EncodingVariant LDLARH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x48df7c00),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x48df7c00),
};
DECLARE_INSTRUCTION(LDLARH);

constexpr EncodingVariant LDLAR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x88df7c00),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x88df7c00),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xc8df7c00),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0xc8df7c00),
};
DECLARE_INSTRUCTION(LDLAR);

constexpr EncodingVariant LDSET_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8203000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8203000),
};
DECLARE_INSTRUCTION(LDSET);

constexpr EncodingVariant LDSETA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8a03000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8a03000),
};
DECLARE_INSTRUCTION(LDSETA);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSETAB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38a03000);

constexpr EncodingVariant LDSETAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8e03000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8e03000),
};
DECLARE_INSTRUCTION(LDSETAL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSETALB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38e03000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSETB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38203000);

constexpr EncodingVariant LDSETL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8603000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8603000),
};
DECLARE_INSTRUCTION(LDSETL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSETLB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38603000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSETAH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78a03000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSETALH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78e03000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSETH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78203000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSETLH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78603000);

constexpr EncodingVariant LDSMAX_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8204000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8204000),
};
DECLARE_INSTRUCTION(LDSMAX);

constexpr EncodingVariant LDSMAXA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8a04000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8a04000),
};
DECLARE_INSTRUCTION(LDSMAXA);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMAXAB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38a04000);

constexpr EncodingVariant LDSMAXAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8e04000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8e04000),
};
DECLARE_INSTRUCTION(LDSMAXAL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMAXALB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38e04000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMAXB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38204000);

constexpr EncodingVariant LDSMAXL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8604000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8604000),
};
DECLARE_INSTRUCTION(LDSMAXL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMAXLB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38604000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMAXAH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78a04000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMAXALH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78e04000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMAXH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78204000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMAXLH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78604000);

constexpr EncodingVariant LDSMIN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8205000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8205000),
};
DECLARE_INSTRUCTION(LDSMIN);

constexpr EncodingVariant LDSMINA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8a05000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8a05000),
};
DECLARE_INSTRUCTION(LDSMINA);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMINAB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38a05000);

constexpr EncodingVariant LDSMINAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8e05000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8e05000),
};
DECLARE_INSTRUCTION(LDSMINAL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMINALB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38e05000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMINB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38205000);

constexpr EncodingVariant LDSMINL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8605000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8605000),
};
DECLARE_INSTRUCTION(LDSMINL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMINLB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38605000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMINAH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78a05000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMINALH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78e05000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMINH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78205000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDSMINLH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78605000);

constexpr EncodingVariant LDUMAX_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8206000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8206000),
};
DECLARE_INSTRUCTION(LDUMAX);

constexpr EncodingVariant LDUMAXA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8a06000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8a06000),
};
DECLARE_INSTRUCTION(LDUMAXA);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMAXAB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38a06000);

constexpr EncodingVariant LDUMAXAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8e06000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8e06000),
};
DECLARE_INSTRUCTION(LDUMAXAL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMAXALB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38e06000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMAXB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38206000);

constexpr EncodingVariant LDUMAXL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8606000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8606000),
};
DECLARE_INSTRUCTION(LDUMAXL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMAXLB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38606000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMAXAH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78a06000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMAXALH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78e06000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMAXH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78206000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMAXLH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78606000);

constexpr EncodingVariant LDUMIN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8207000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8207000),
};
DECLARE_INSTRUCTION(LDUMIN);

constexpr EncodingVariant LDUMINA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8a07000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8a07000),
};
DECLARE_INSTRUCTION(LDUMINA);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMINAB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38a07000);

constexpr EncodingVariant LDUMINAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8e07000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8e07000),
};
DECLARE_INSTRUCTION(LDUMINAL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMINALB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38e07000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMINB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38207000);

constexpr EncodingVariant LDUMINL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8607000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8607000),
};
DECLARE_INSTRUCTION(LDUMINL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMINLB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38607000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMINAH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78a07000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMINALH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78e07000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMINH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78207000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LDUMINLH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78607000);

constexpr EncodingVariant LDNP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket), LoadStorePair, 4, 0x28400000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket), LoadStorePair, 8, 0xa8400000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 4, 0x28400000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 8, 0xa8400000),

	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchS|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 4, 0x2c400000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchD|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 8, 0x6c400000),
	DECLARE_CANDIDATE((MatchQ|MatchOp0, MatchComma, MatchQ|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 16, 0xac400000),
};
DECLARE_INSTRUCTION(LDNP);

constexpr EncodingVariant LDP_ENCODING_VARIANTS[] =
{
	// Post index
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp3), LoadStorePair, 4, 0x28c00000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp3), LoadStorePair, 8, 0xa8c00000),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchS|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp3), LoadStorePair, 4, 0x2cc00000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchD|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp3), LoadStorePair, 8, 0x6cc00000),
	DECLARE_CANDIDATE((MatchQ|MatchOp0, MatchComma, MatchQ|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp3), LoadStorePair, 16, 0xacc00000),

	// Pre index
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket, MatchExclamationMark), LoadStorePair, 4, 0x29c00000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket, MatchExclamationMark), LoadStorePair, 8, 0xa9c00000),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchS|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket, MatchExclamationMark), LoadStorePair, 4, 0x2dc00000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchD|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket, MatchExclamationMark), LoadStorePair, 8, 0x6dc00000),
	DECLARE_CANDIDATE((MatchQ|MatchOp0, MatchComma, MatchQ|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket, MatchExclamationMark), LoadStorePair, 16, 0xadc00000),

	// Signed offset
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket), LoadStorePair, 4, 0x29400000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket), LoadStorePair, 8, 0xa9400000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 4, 0x29400000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 8, 0xa9400000),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchS|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 4, 0x2d400000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchD|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 8, 0x6d400000),
	DECLARE_CANDIDATE((MatchQ|MatchOp0, MatchComma, MatchQ|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 16, 0xad400000),
};
DECLARE_INSTRUCTION(LDP);

constexpr EncodingVariant LDPSW_ENCODING_VARIANTS[] =
{
	// Post index
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp3), LoadStorePair, 8, 0x68c00000),
	
	// Pre index
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket, MatchExclamationMark), LoadStorePair, 8, 0x69c00000),
	
	// Signed offset
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket), LoadStorePair, 8, 0x69400000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 8, 0x69400000),
};
DECLARE_INSTRUCTION(LDPSW);

constexpr EncodingVariant LDR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchReg32|MatchReg32Z, 4, 0xb8400000),
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchReg64|MatchReg64Z, 8, 0xf8400000),
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchB, 1, 0x3c400000),
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchH, 2, 0x7c400000),
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchS, 4, 0xbc400000),
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchD, 8, 0xfc400000),
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchQ, 16, 0x3cc00000),

	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchRel|MatchOp1), RtRel19, 0, 0x18000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchRel|MatchOp1), RtRel19, 0, 0x58000000),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchRel|MatchOp1), RtRel19, 0, 0x1c000000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchRel|MatchOp1), RtRel19, 0, 0x5c000000),
	DECLARE_CANDIDATE((MatchQ|MatchOp0, MatchComma, MatchRel|MatchOp1), RtRel19, 0, 0x9c000000),

	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchReg32|MatchReg32Z, 2, 0xb8600800),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchReg64|MatchReg64Z, 3, 0xf8600800),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchB, 0, 0x3c600800),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchH, 1, 0x7c600800),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchS, 2, 0xbc600800),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchD, 3, 0xfc600800),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchQ, 4, 0x3ce00800),
};
DECLARE_INSTRUCTION(LDR);

constexpr EncodingVariant LDRB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchReg32|MatchReg32Z, 1, 0x38400000),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchReg32|MatchReg32Z, 0, 0x38600800),
};
DECLARE_INSTRUCTION(LDRB);

constexpr EncodingVariant LDRH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchReg32|MatchReg32Z, 2, 0x78400000),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchReg32|MatchReg32Z, 1, 0x78600800),
};
DECLARE_INSTRUCTION(LDRH);

constexpr EncodingVariant LDRSB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchReg32|MatchReg32Z, 1, 0x38c00000),
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchReg64|MatchReg64Z, 1, 0x38800000),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchReg32|MatchReg32Z, 0, 0x38e00800),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchReg64|MatchReg64Z, 0, 0x38a00800),
};
DECLARE_INSTRUCTION(LDRSB);

constexpr EncodingVariant LDRSH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchReg32|MatchReg32Z, 2, 0x78c00000),
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchReg64|MatchReg64Z, 2, 0x78800000),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchReg32|MatchReg32Z, 1, 0x78e00800),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchReg64|MatchReg64Z, 1, 0x78a00800),
};
DECLARE_INSTRUCTION(LDRSH);

constexpr EncodingVariant LDRSW_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchReg64|MatchReg64Z, 8, 0xb8800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchRel|MatchOp1), RtRel19, 0, 0x98000000),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchReg64|MatchReg64Z, 3, 0xb8a00800),
};
DECLARE_INSTRUCTION(LDRSW);

constexpr EncodingVariant LDTR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xb8400800),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xb8400800),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xf8400800),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xf8400800),
};
DECLARE_INSTRUCTION(LDTR);

constexpr EncodingVariant LDTRB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x38400800),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x38400800),
};
DECLARE_INSTRUCTION(LDTRB);

constexpr EncodingVariant LDTRH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x78400800),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x78400800),
};
DECLARE_INSTRUCTION(LDTRH);

constexpr EncodingVariant LDTRSB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x38c00800),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x38c00800),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x38800800),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x38800800),
};
DECLARE_INSTRUCTION(LDTRSB);

constexpr EncodingVariant LDTRSH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x78c00800),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x78c00800),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x78800800),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x78800800),
};
DECLARE_INSTRUCTION(LDTRSH);

constexpr EncodingVariant LDTRSW_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xb8800800),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xb8800800),
};
DECLARE_INSTRUCTION(LDTRSW);

constexpr EncodingVariant LDUR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xb8400000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xb8400000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xf8400000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xf8400000),
	DECLARE_CANDIDATE((MatchB|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x3c400000),
	DECLARE_CANDIDATE((MatchB|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x3c400000),
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x7c400000),
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x7c400000),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xbc400000),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xbc400000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xfc400000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xfc400000),
	DECLARE_CANDIDATE((MatchQ|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x3cc00000),
	DECLARE_CANDIDATE((MatchQ|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x3cc00000),
};
DECLARE_INSTRUCTION(LDUR);

constexpr EncodingVariant LDURB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x38400000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x38400000),
};
DECLARE_INSTRUCTION(LDURB);

constexpr EncodingVariant LDURH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x78400000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x78400000),
};
DECLARE_INSTRUCTION(LDURH);

constexpr EncodingVariant LDURSB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x38c00000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x38c00000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x38800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x38800000),
};
DECLARE_INSTRUCTION(LDURSB);

constexpr EncodingVariant LDURSH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x78c00000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x78c00000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x78800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x78800000),
};
DECLARE_INSTRUCTION(LDURSH);

constexpr EncodingVariant LDURSW_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xb8800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xb8800000),
};
DECLARE_INSTRUCTION(LDURSW);

constexpr EncodingVariant LDXP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x887f0000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x887f0000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xc87f0000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0xc87f0000),
};
DECLARE_INSTRUCTION(LDXP);

constexpr EncodingVariant LDXR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x885f7c00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x885f7c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xc85f7c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0xc85f7c00),
};
DECLARE_INSTRUCTION(LDXR);

constexpr EncodingVariant LDXRB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x085f7c00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x085f7c00),
};
DECLARE_INSTRUCTION(LDXRB);

constexpr EncodingVariant LDXRH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x485f7c00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x485f7c00),
};
DECLARE_INSTRUCTION(LDXRH);

constexpr EncodingVariant LSL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchImm|MatchOp2|MatchOp3), RdRnImmrImms, 0x25, 0x53000000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchImm|MatchOp2|MatchOp3), RdRnImmrImms, 0x36, 0xd3400000),
	
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2), RdRnRmImm6, 0, 0x1ac02000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchOp2), RdRnRmImm6, 0, 0x9ac02000),
};
DECLARE_INSTRUCTION(LSL);

constexpr EncodingVariant LSLV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2), RdRnRmImm6, 0, 0x1ac02000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchOp2), RdRnRmImm6, 0, 0x9ac02000),
};
DECLARE_INSTRUCTION(LSLV);

constexpr EncodingVariant LSR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchImm|MatchOp2), RdRnImmrImms, 0, 0x53007c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchImm|MatchOp2), RdRnImmrImms, 0, 0xd340fc00),
	
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2), RdRnRmImm6, 0, 0x1ac02400),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchOp2), RdRnRmImm6, 0, 0x9ac02400),
};
DECLARE_INSTRUCTION(LSR);

constexpr EncodingVariant LSRV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2), RdRnRmImm6, 0, 0x1ac02400),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchOp2), RdRnRmImm6, 0, 0x9ac02400),
};
DECLARE_INSTRUCTION(LSRV);

constexpr EncodingVariant MADD_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp3), RdRnRmRa, 0, 0x1b000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp3), RdRnRmRa, 0, 0x9b000000),
};
DECLARE_INSTRUCTION(MADD);

constexpr EncodingVariant MLA_ENCODING_VARIANTS[] =
{
	// Vector Element
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f000000),
	DECLARE_CANDIDATE((MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f000000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVD|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f000000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e209400),
};
DECLARE_INSTRUCTION(MLA);

constexpr EncodingVariant MLS_ENCODING_VARIANTS[] =
{
	// Vector Element
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f004000),
	DECLARE_CANDIDATE((MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f004000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVD|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f004000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e209400),
};
DECLARE_INSTRUCTION(MLS);

constexpr EncodingVariant MNEG_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmRa, 0, 0x1b00fc00),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), RdRnRmRa, 0, 0x9b00fc00),
};
DECLARE_INSTRUCTION(MNEG);

constexpr EncodingVariant MOV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x2a0003e0),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), RdRnRmImm6, 0, 0xaa0003e0),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1), RdRnRmImm6, 0, 0x11000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1), RdRnRmImm6, 0, 0x91000000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchExpressionImm|MatchOp1), RdExpressionImm, 32, 0),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchExpressionImm|MatchOp1), RdExpressionImm, 64, 0),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchHwImm16|MatchOp1), RdHwImm16, 0, 0x52800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchHwImm16|MatchOp1), RdHwImm16, 0, 0xd2800000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchNot32HwImm16|MatchOp1), RdNot32HwImm16, 0, 0x12800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchNot64HwImm16|MatchOp1), RdNot64HwImm16, 0, 0x92800000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchLogical32Imm|MatchOp2), LogicalImmediate, 32, 0x320003e0),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchLogical64Imm|MatchOp2), LogicalImmediate, 64, 0xb20003e0),
	
	// SIMD Scalar
	DECLARE_CANDIDATE((MatchB|MatchOp0|MatchOp7, MatchComma, MatchVB|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x5e000400),
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchVH|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x5e000400),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchVS|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x5e000400),
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchVD|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x5e000400),

	// SIMD Element
	DECLARE_CANDIDATE((MatchVB|MatchOp0|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket, MatchComma, MatchVB|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x6e000400),
	DECLARE_CANDIDATE((MatchVH|MatchOp0|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket, MatchComma, MatchVH|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x6e000400),
	DECLARE_CANDIDATE((MatchVS|MatchOp0|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket, MatchComma, MatchVS|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x6e000400),
	DECLARE_CANDIDATE((MatchVD|MatchOp0|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket, MatchComma, MatchVD|MatchOp1, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x6e000400),
	
	// SIMD from general
	DECLARE_CANDIDATE((MatchVB|MatchVH|MatchVS|MatchOp0|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket, MatchComma, MatchReg32|MatchOp1), FpRdRnIndex1Index2, 0, 0x4e001c00),
	DECLARE_CANDIDATE((MatchVD|MatchOp0|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp1), FpRdRnIndex1Index2, 0, 0x4e001c00),

	// SIMD vector
	DECLARE_CANDIDATE((MatchV8B16B|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1|MatchOp2), FpRdRnRmRa, 2, 0x0ea01c00),

	// SIMD to general
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchVS|MatchOp1|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x0e003c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchVD|MatchOp1|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x4e103c00),

	// MOV Rd, {target}[highbit:lowbit]
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchNumber|MatchImm|MatchRel|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchColon, MatchNumber|MatchOp4, MatchRightSquareBracket), RdRnImmSubfield, 0x1005, 0x52800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchNumber|MatchImm|MatchRel|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchColon, MatchNumber|MatchOp4, MatchRightSquareBracket), RdRnImmSubfield, 0x1005, 0xd2800000),

	// MOV Rd, {target}[highbit:lowbit], LSL #n
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchNumber|MatchImm|MatchRel|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchColon, MatchNumber|MatchOp4, MatchRightSquareBracket, MatchComma, MatchLSL, MatchImm|MatchOp5), RdRnImmSubfield, 0x1005, 0x52800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchNumber|MatchImm|MatchRel|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchColon, MatchNumber|MatchOp4, MatchRightSquareBracket, MatchComma, MatchLSL, MatchImm|MatchOp5), RdRnImmSubfield, 0x1005, 0xd2800000),
};
DECLARE_INSTRUCTION(MOV);

constexpr EncodingVariant MOVI_ENCODING_VARIANTS[] =
{
	// MOVI <Vd>.T, #imm
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchImm|MatchOp1), VectorImmediate, 2, 0x2f00e400),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchImm|MatchOp1), VectorImmediate, 2, 0x2f00e400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S|MatchOp0, MatchComma, MatchImm|MatchOp1), VectorImmediate, 0, 0x0f000400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S|MatchOp0, MatchComma, MatchImm|MatchOp1, MatchComma, MatchLSL, MatchImm|MatchOp3), VectorImmediate, 0, 0x0f000400),
	DECLARE_CANDIDATE((MatchV2S4S|MatchOp0, MatchComma, MatchImm|MatchOp1, MatchComma, MatchRel|MatchOp2 /*MSL*/, MatchImm|MatchOp3), VectorImmediate, 1, 0x0f00c400),
};
DECLARE_INSTRUCTION(MOVI);

constexpr EncodingVariant MOVK_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchUImm16|MatchOp1), RdImm16Shift, 0, 0x72800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchUImm16|MatchOp1), RdImm16Shift, 0, 0xf2800000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchUImm16|MatchOp1, MatchComma, MatchLSL, MatchImm|MatchOp2), RdImm16Shift, 0, 0x72800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchUImm16|MatchOp1, MatchComma, MatchLSL, MatchImm|MatchOp2), RdImm16Shift, 0, 0xf2800000),

	// MOVK Rd, {target}[highbit:lowbit]
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchNumber|MatchImm|MatchRel|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchColon, MatchNumber|MatchOp4, MatchRightSquareBracket), RdRnImmSubfield, 0x1005, 0x72800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchNumber|MatchImm|MatchRel|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchColon, MatchNumber|MatchOp4, MatchRightSquareBracket), RdRnImmSubfield, 0x1005, 0xf2800000),
	
	// MOVK Rd, {target}[highbit:lowbit], LSL #n
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchNumber|MatchImm|MatchRel|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchColon, MatchNumber|MatchOp4, MatchRightSquareBracket, MatchComma, MatchLSL, MatchImm|MatchOp5), RdRnImmSubfield, 0x1005, 0x72800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchNumber|MatchImm|MatchRel|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchColon, MatchNumber|MatchOp4, MatchRightSquareBracket, MatchComma, MatchLSL, MatchImm|MatchOp5), RdRnImmSubfield, 0x1005, 0xf2800000),
};
DECLARE_INSTRUCTION(MOVK);

constexpr EncodingVariant MOVN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchUImm16|MatchOp1), RdImm16Shift, 0, 0x12800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchUImm16|MatchOp1), RdImm16Shift, 0, 0x92800000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchUImm16|MatchOp1, MatchComma, MatchLSL, MatchImm|MatchOp2), RdImm16Shift, 0, 0x12800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchUImm16|MatchOp1, MatchComma, MatchLSL, MatchImm|MatchOp2), RdImm16Shift, 0, 0x92800000),
};
DECLARE_INSTRUCTION(MOVN);

constexpr EncodingVariant MOVZ_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchUImm16|MatchOp1), RdImm16Shift, 0, 0x52800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchUImm16|MatchOp1), RdImm16Shift, 0, 0xd2800000),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchUImm16|MatchOp1, MatchComma, MatchLSL, MatchImm|MatchOp2), RdImm16Shift, 0, 0x52800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchUImm16|MatchOp1, MatchComma, MatchLSL, MatchImm|MatchOp2), RdImm16Shift, 0, 0xd2800000),

	// MOVZ Rd, {target}[highbit:lowbit]
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchNumber|MatchImm|MatchRel|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchColon, MatchNumber|MatchOp4, MatchRightSquareBracket), RdRnImmSubfield, 0x1005, 0x52800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchNumber|MatchImm|MatchRel|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchColon, MatchNumber|MatchOp4, MatchRightSquareBracket), RdRnImmSubfield, 0x1005, 0xd2800000),
	
	// MOVZ Rd, {target}[highbit:lowbit], LSL #n
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchNumber|MatchImm|MatchRel|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchColon, MatchNumber|MatchOp4, MatchRightSquareBracket, MatchComma, MatchLSL, MatchImm|MatchOp5), RdRnImmSubfield, 0x1005, 0x52800000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchNumber|MatchImm|MatchRel|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchColon, MatchNumber|MatchOp4, MatchRightSquareBracket, MatchComma, MatchLSL, MatchImm|MatchOp5), RdRnImmSubfield, 0x1005, 0xd2800000),
};
DECLARE_INSTRUCTION(MOVZ);

constexpr EncodingVariant MSUB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp3), RdRnRmRa, 0, 0x1b008000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp3), RdRnRmRa, 0, 0x9b008000),
};
DECLARE_INSTRUCTION(MSUB);

constexpr EncodingVariant MUL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmRa, 0, 0x1b007c00),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), RdRnRmRa, 0, 0x9b007c00),

	// Vector Element
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f008000),
	DECLARE_CANDIDATE((MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f008000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVD|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f008000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e209c00),
};
DECLARE_INSTRUCTION(MUL);

constexpr EncodingVariant MVN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x2a2003e0),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xaa2003e0),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x2a2003e0),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xaa2003e0),

	// MVN <Vd>.<T>, <Vn>.<T>
	DECLARE_CANDIDATE((MatchV8B16B|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2e205800),
};
DECLARE_INSTRUCTION(MVN);

constexpr EncodingVariant MVNI_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4H8H|MatchV2S4S|MatchOp0, MatchComma, MatchImm|MatchOp1), VectorImmediate, 0, 0x2f000400),
	DECLARE_CANDIDATE((MatchV4H8H|MatchV2S4S|MatchOp0, MatchComma, MatchImm|MatchOp1, MatchComma, MatchLSL, MatchImm|MatchOp3), VectorImmediate, 0, 0x2f000400),
	DECLARE_CANDIDATE((MatchV2S4S|MatchOp0, MatchComma, MatchImm|MatchOp1, MatchComma, MatchRel|MatchOp2 /*MSL*/, MatchImm|MatchOp3), VectorImmediate, 1, 0x2f00c400),
};
DECLARE_INSTRUCTION(MVNI);

constexpr EncodingVariant NEG_ENCODING_VARIANTS[] =
{
	// SUB <Wd|WSP>, WZR, <Wm>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x4b0003e0),
	// SUB <Wd|WSP>, WZR, <Wm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x4b0003e0),
	
	// SUB <Xd|SP>, XZR, <Xm>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xcb0003e0),
	// SUB <Xd|SP>, XZR, <Xm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xcb0003e0),

	// NEG <V><d>, <V><n>
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 0, 0x7ee0b800),

	// NEG <Vd>.<T>, <Vn>.<T>
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 3, 0x2e20b800),
};
DECLARE_INSTRUCTION(NEG);

constexpr EncodingVariant NEGS_ENCODING_VARIANTS[] =
{
	// SUBS Wd, WZR, <Wm>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x6b0003e0),
	// SUBS Wd, WZR, <Wm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x6b0003e0),
	
	// SUBS Xd, XZR, <Xm>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xeb0003e0),
	// SUBS Xd, XZR, <Xm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xeb0003e0),
};
DECLARE_INSTRUCTION(NEGS);

constexpr EncodingVariant NGC_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x5a0003e0),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), RdRnRmImm6, 0, 0xda0003e0),
};
DECLARE_INSTRUCTION(NGC);

constexpr EncodingVariant NGCS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x7a0003e0),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), RdRnRmImm6, 0, 0xfa0003e0),
};
DECLARE_INSTRUCTION(NGCS);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(NOP, (), Fixed, 0, 0xd503201f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(NOT, (MatchV8B16B|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2e205800);

constexpr EncodingVariant ORN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x2a200000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xaa200000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x2a200000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xaa200000),
	DECLARE_CANDIDATE((MatchV8B16B|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0ee01c00),
};
DECLARE_INSTRUCTION(ORN);

constexpr EncodingVariant ORR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchImm|MatchOp2), LogicalImmediate, 32, 0x32000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchImm|MatchOp2), LogicalImmediate, 64, 0xb2000000),
	
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x2a000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xaa000000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x2a000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xaa000000),

	// ORR <Vd>.T, #imm
	DECLARE_CANDIDATE((MatchV4H8H|MatchV2S4S|MatchOp0, MatchComma, MatchImm|MatchOp1), VectorImmediate, 0, 0x0f001400),
	DECLARE_CANDIDATE((MatchV4H8H|MatchV2S4S|MatchOp0, MatchComma, MatchImm|MatchOp1, MatchComma, MatchLSL, MatchImm|MatchOp3), VectorImmediate, 0, 0x0f001400),
	
	// ORR <Vd>.T, <Vn>.T, <Vm>.T
	DECLARE_CANDIDATE((MatchV8B16B|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x0ea01c00),
};
DECLARE_INSTRUCTION(ORR);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACDA, (MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1), RdRnRmRa, 0, 0xdac10800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACDZA, (MatchReg64|MatchOp0), RdRnRmRa, 0, 0xdac12be0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACDB, (MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1), RdRnRmRa, 0, 0xdac10c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACDZB, (MatchReg64|MatchOp0), RdRnRmRa, 0, 0xdac12fe0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACGA, (MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchReg64SP|MatchOp2), RdRnRmRa, 0, 0x9ac03000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACIA, (MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1), RdRnRmRa, 0, 0xdac10000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACIA1716, (), Fixed, 0, 0xd503211f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACIASP, (), Fixed, 0, 0xd503233f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACIAZ, (), Fixed, 0, 0xd503231f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACIZA, (MatchReg64|MatchOp0), RdRnRmRa, 0, 0xdac123e0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACIB, (MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1), RdRnRmRa, 0, 0xdac10400);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACIB1716, (), Fixed, 0, 0xd503215f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACIBSP, (), Fixed, 0, 0xd503237f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACIBZ, (), Fixed, 0, 0xd503235f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PACIZB, (MatchReg64|MatchOp0), RdRnRmRa, 0, 0xdac127e0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PMUL, (MatchV8B16B|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 2, 0x2e209c00);

constexpr EncodingVariant PMULL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7, MatchComma, MatchRep), FpRdRnRmRa, 1, 0x0e20e000),
	DECLARE_CANDIDATE((MatchV1Q|MatchOp0, MatchComma, MatchV1D|MatchOp1|MatchOp7, MatchComma, MatchRep), FpRdRnRmRa, 1, 0x0e20e000),
};
DECLARE_INSTRUCTION(PMULL);

constexpr EncodingVariant PMULL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7, MatchComma, MatchRep), FpRdRnRmRa, 1, 0x4e20e000),
	DECLARE_CANDIDATE((MatchV1Q|MatchOp0, MatchComma, MatchV2D|MatchOp1|MatchOp7, MatchComma, MatchRep), FpRdRnRmRa, 1, 0x4e20e000),
};
DECLARE_INSTRUCTION(PMULL2);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(PSSBB, (), Fixed, 0, 0xd503349f);

constexpr EncodingVariant RADDHN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 1, 0x2e204000),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 1, 0x2e204000),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV2D|MatchOp2), FpRdRnRmRa, 1, 0x2e204000),
};
DECLARE_INSTRUCTION(RADDHN);

constexpr EncodingVariant RADDHN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 1, 0x6e204000),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 1, 0x6e204000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV2D|MatchOp2), FpRdRnRmRa, 1, 0x6e204000),
};
DECLARE_INSTRUCTION(RADDHN2);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(RAX1, (MatchV2D|MatchOp0, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV2D|MatchOp2), FpRdRnRmRa, 0, 0xce608c00);

constexpr EncodingVariant RBIT_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1), RdRnRmImm6, 0, 0x5ac00000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1), RdRnRmImm6, 0, 0xdac00000),
	DECLARE_CANDIDATE((MatchV8B16B|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2e605800),
};
DECLARE_INSTRUCTION(RBIT);

constexpr EncodingVariant RET_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((), Fixed, 0, 0xd65f03c0),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp1), RdRnRmImm6, 0, 0xd65f0000),
};
DECLARE_INSTRUCTION(RET);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(RETAA, (), Fixed, 0, 0xd65f0bff);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(RETAB, (), Fixed, 0, 0xd65f0fff);

constexpr EncodingVariant REV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1), RdRnRmRa, 0, 0x5ac00800),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1), RdRnRmRa, 0, 0xdac00c00),
};
DECLARE_INSTRUCTION(REV);

constexpr EncodingVariant REV16_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1), RdRnRmRa, 0, 0x5ac00400),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1), RdRnRmRa, 0, 0xdac00400),
	DECLARE_CANDIDATE((MatchV8B16B|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x0e201800),
};
DECLARE_INSTRUCTION(REV16);

constexpr EncodingVariant REV32_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1), RdRnRmRa, 0, 0xdac00800),
	DECLARE_CANDIDATE((MatchV8B16B|MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 3, 0x2e200800),
};
DECLARE_INSTRUCTION(REV32);

constexpr EncodingVariant REV64_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1), RdRnRmRa, 0, 0xdac00c00),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 3, 0x0e200800),
};
DECLARE_INSTRUCTION(REV64);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(RMIF, (MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchImm|MatchOp1, MatchComma, MatchImm|MatchOp2), RnImmNzcvCond, 0, 0xba000400);

constexpr EncodingVariant ROR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnRmImm6, 0, 0x13800000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnRmImm6, 0, 0x93c00000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x1ac02c00),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), RdRnRmImm6, 0, 0x9ac02c00),
};
DECLARE_INSTRUCTION(ROR);

constexpr EncodingVariant RORV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x1ac02c00),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), RdRnRmImm6, 0, 0x9ac02c00),
};
DECLARE_INSTRUCTION(RORV);

constexpr EncodingVariant RSHRN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x0f008c00),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x0f008c00),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x0f008c00),
};
DECLARE_INSTRUCTION(RSHRN);

constexpr EncodingVariant RSHRN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x4f008c00),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x4f008c00),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x4f008c00),
};
DECLARE_INSTRUCTION(RSHRN2);

constexpr EncodingVariant RSUBHN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 1, 0x2e206000),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 1, 0x2e206000),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV2D|MatchOp2), FpRdRnRmRa, 1, 0x2e206000),
};
DECLARE_INSTRUCTION(RSUBHN);

constexpr EncodingVariant RSUBHN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 1, 0x6e206000),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 1, 0x6e206000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV2D|MatchOp2), FpRdRnRmRa, 1, 0x6e206000),
};
DECLARE_INSTRUCTION(RSUBHN2);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SABA, (MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e207c00);

constexpr EncodingVariant SABAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 0, 0x0e205000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 0, 0x0e605000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 0, 0x0ea05000),
};
DECLARE_INSTRUCTION(SABAL);

constexpr EncodingVariant SABAL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 0, 0x4e205000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 0, 0x4e605000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 0, 0x4ea05000),
};
DECLARE_INSTRUCTION(SABAL2);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SABD, (MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e207400);

constexpr EncodingVariant SABDL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 0, 0x0e207000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 0, 0x0e607000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 0, 0x0ea07000),
};
DECLARE_INSTRUCTION(SABDL);

constexpr EncodingVariant SABDL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 0, 0x4e207000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 0, 0x4e607000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 0, 0x4ea07000),
};
DECLARE_INSTRUCTION(SABDL2);

constexpr EncodingVariant SADALP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e206800),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e206800),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e206800),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e206800),
	DECLARE_CANDIDATE((MatchV1D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e206800),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e206800),
};
DECLARE_INSTRUCTION(SADALP);

constexpr EncodingVariant SADDL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 3, 0x0e200000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 3, 0x0e200000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 3, 0x0e200000),
};
DECLARE_INSTRUCTION(SADDL);

constexpr EncodingVariant SADDL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 3, 0x0e200000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 3, 0x0e200000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 3, 0x0e200000),
};
DECLARE_INSTRUCTION(SADDL2);

constexpr EncodingVariant SADDLP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e202800),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e202800),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e202800),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e202800),
	DECLARE_CANDIDATE((MatchV1D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e202800),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e202800),
};
DECLARE_INSTRUCTION(SADDLP);

constexpr EncodingVariant SADDLV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e303800),
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e303800),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e303800),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e303800),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e303800),
};
DECLARE_INSTRUCTION(SADDLV);

constexpr EncodingVariant SADDW_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8B|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x0e201000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4H|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x0e201000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV2S|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x0e201000),
};
DECLARE_INSTRUCTION(SADDW);

constexpr EncodingVariant SADDW2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV16B|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x0e201000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV8H|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x0e201000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV4S|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x0e201000),
};
DECLARE_INSTRUCTION(SADDW2);

constexpr EncodingVariant SBC_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x5a000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), RdRnRmImm6, 0, 0xda000000),
};
DECLARE_INSTRUCTION(SBC);

constexpr EncodingVariant SBCS_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), RdRnRmImm6, 0, 0x7a000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), RdRnRmImm6, 0, 0xfa000000),
};
DECLARE_INSTRUCTION(SBCS);

constexpr EncodingVariant SBFIZ_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0x21, 0x13000000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0x31, 0x93400000),
};
DECLARE_INSTRUCTION(SBFIZ);

constexpr EncodingVariant SBFM_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0, 0x13000000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0, 0x93400000),
};
DECLARE_INSTRUCTION(SBFM);

constexpr EncodingVariant SBFX_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0x04, 0x13000000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0x04, 0x93400000),
};
DECLARE_INSTRUCTION(SBFX);

constexpr EncodingVariant SCVTF_ENCODING_VARIANTS[] =
{
	// Vector, Fixed Point
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x5f00e400),
	DECLARE_CANDIDATE((MatchV4H8H|MatchV2S4S|MatchV2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x202, 0x0f00e400),
	
	// Vector, Integer
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x5e79d800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x5e21d800),
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x0e79d800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x0e21d800),
	
	// Scalar, Fixed Point
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x1008, 0x1e020000),
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x1008, 0x9e020000),
	
	// Scalar, Integer
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchReg32|MatchOp1), FpRdRnRmRa, 8, 0x1e220000),
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchReg64|MatchOp1), FpRdRnRmRa, 8, 0x9e220000),
};
DECLARE_INSTRUCTION(SCVTF);

constexpr EncodingVariant SDIV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2), RdRnRmImm6, 0, 0x1ac00c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchOp2), RdRnRmImm6, 0, 0x9ac00c00),
};
DECLARE_INSTRUCTION(SDIV);

constexpr EncodingVariant SDOT_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV8B|MatchOp1, MatchComma, MatchV4B|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f00e000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV16B|MatchOp1, MatchComma, MatchV4B|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f00e000),
};
DECLARE_INSTRUCTION(SDOT);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SETF8, (MatchReg32|MatchOp1), RtRnImm9, 0, 0x3a00080d);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SETF16, (MatchReg32|MatchOp1), RtRnImm9, 0, 0x3a00480d);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SEV, (), Fixed, 0, 0xd503209f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SEVL, (), Fixed, 0, 0xd50320bf);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHA1C, (MatchQ|MatchOp0, MatchComma, MatchS|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 0, 0x5e000000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHA1H, (MatchS|MatchOp0, MatchComma, MatchS|MatchOp1), FpRdRnRmRa, 0, 0x5e280800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHA1M, (MatchQ|MatchOp0, MatchComma, MatchS|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 0, 0x5e004000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHA1P, (MatchQ|MatchOp0, MatchComma, MatchS|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 0, 0x5e001000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHA1SU0, (MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 0, 0x5e003000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHA1SU1, (MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1), FpRdRnRmRa, 0, 0x5e281800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHA256H, (MatchQ|MatchOp0, MatchComma, MatchQ|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 0, 0x5e004000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHA256H2, (MatchQ|MatchOp0, MatchComma, MatchQ|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 0, 0x5e005000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHA256SU0, (MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1), FpRdRnRmRa, 0, 0x5e282800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHA256SU1, (MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 0, 0x5e006000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHA512H, (MatchQ|MatchOp0, MatchComma, MatchQ|MatchOp1, MatchComma, MatchV2D|MatchOp2), FpRdRnRmRa, 0, 0xce608000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHA512H2, (MatchQ|MatchOp0, MatchComma, MatchQ|MatchOp1, MatchComma, MatchV2D|MatchOp2), FpRdRnRmRa, 0, 0xce608400);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHA512SU0, (MatchV2D|MatchOp0, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 0, 0xcec08000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHA512SU1, (MatchV2D|MatchOp0, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV2D|MatchOp2), FpRdRnRmRa, 0, 0xce608800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHADD, (MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e200400);

constexpr EncodingVariant SHL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x400, 0x5f005400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x0f005400),
};
DECLARE_INSTRUCTION(SHL);

constexpr EncodingVariant SHLL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x0f00a400),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x0f00a400),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x0f00a400),
};
DECLARE_INSTRUCTION(SHLL);

constexpr EncodingVariant SHLL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x0f00a400),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x0f00a400),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x0f00a400),
};
DECLARE_INSTRUCTION(SHLL2);

constexpr EncodingVariant SHRN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x0f008400),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x0f008400),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x0f008400),
};
DECLARE_INSTRUCTION(SHRN);

constexpr EncodingVariant SHRN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x4f008400),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x4f008400),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x4f008400),
};
DECLARE_INSTRUCTION(SHRN2);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SHSUB, (MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e202400);

constexpr EncodingVariant SLI_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x400, 0x7f005400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x2f005400),
};
DECLARE_INSTRUCTION(SLI);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SM3PARTW1, (MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 0, 0xce60c000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SM3PARTW2, (MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 0, 0xce60c400);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SM3SS1, (MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2, MatchComma, MatchV4S|MatchOp3), FpRdRnRmRa, 0, 0xce400000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SM3TT1A, (MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchRightSquareBracket), RdRnRmImm2, 0, 0xce408000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SM3TT1B, (MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchRightSquareBracket), RdRnRmImm2, 0, 0xce408400);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SM3TT2A, (MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchRightSquareBracket), RdRnRmImm2, 0, 0xce408800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SM3TT2B, (MatchVS|MatchOp0, MatchComma, MatchVS|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp3, MatchRightSquareBracket), RdRnRmImm2, 0, 0xce408c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SM4E, (MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1), FpRdRnRmRa, 0, 0xcec08400);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SM4EKEY, (MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 0, 0xce60c800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SMADDL, (MatchReg64|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp3), RdRnRmRa, 0, 0x9b200000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SMAX, (MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e206400);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SMAXP, (MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e20a400);

constexpr EncodingVariant SMAXV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0, MatchComma, MatchV8B16B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e30a800),
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchV4H8H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e30a800),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e30a800),
};
DECLARE_INSTRUCTION(SMAXV);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SMC, (MatchUImm16|MatchOp1), RdImm16Shift, 0, 0xd4000003);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SMIN, (MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e206c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SMINP, (MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e20ac00);

constexpr EncodingVariant SMINV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0, MatchComma, MatchV8B16B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e31a800),
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchV4H8H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e31a800),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x0e31a800),
};
DECLARE_INSTRUCTION(SMINV);

constexpr EncodingVariant SMLAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f002000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f002000),

	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 3, 0x0e208000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 3, 0x0e208000),
};
DECLARE_INSTRUCTION(SMLAL);

constexpr EncodingVariant SMLAL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f002000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f002000),
	
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 3, 0x0e208000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 3, 0x0e208000),
};
DECLARE_INSTRUCTION(SMLAL2);

constexpr EncodingVariant SMLSL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f006000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f006000),
	
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 3, 0x0e20a000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 3, 0x0e20a000),
};
DECLARE_INSTRUCTION(SMLSL);

constexpr EncodingVariant SMLSL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f006000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f006000),
	
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 3, 0x0e20a000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 3, 0x0e20a000),
};
DECLARE_INSTRUCTION(SMLSL2);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SMNEGL, (MatchReg64|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2), RdRnRmRa, 0, 0x9b20fc00);

constexpr EncodingVariant SMOV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchVB|MatchVH|MatchOp1|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x0e002c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchVB|MatchVH|MatchVS|MatchOp1|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x4e102c00),
};
DECLARE_INSTRUCTION(SMOV);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SMSUBL, (MatchReg64|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp3), RdRnRmRa, 0, 0x9b208000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SMULH, (MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchOp2), RdRnRmRa, 0, 0x9b407c00);

constexpr EncodingVariant SMULL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2), RdRnRmRa, 0, 0x9b207c00),

	// Vector Element
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f00a000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f00a000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 3, 0x0e20c000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 3, 0x0e20c000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 3, 0x0e20c000),
};
DECLARE_INSTRUCTION(SMULL);

constexpr EncodingVariant SMULL2_ENCODING_VARIANTS[] =
{
	// Vector Element
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f00a000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f00a000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 3, 0x0e20c000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 3, 0x0e20c000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 3, 0x0e20c000),
};
DECLARE_INSTRUCTION(SMULL2);

constexpr EncodingVariant SQABS_ENCODING_VARIANTS[] =
{
	// SQABS <V><d>, <V><n>
	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 1, 0x5e207800),
	
	// SQABS <Vd>.<T>, <Vn>.<T>
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 3, 0x0e207800),
};
DECLARE_INSTRUCTION(SQABS);

constexpr EncodingVariant SQADD_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x5e200c00),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e200c00),
};
DECLARE_INSTRUCTION(SQADD);

constexpr EncodingVariant SQDMLAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchH|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x5f003000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchS|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x5f003000),

	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f003000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f003000),

	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchH|MatchOp1|MatchOp7, MatchComma, MatchH|MatchOp2), FpRdRnRmRa, 3, 0x5e209000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchS|MatchOp1|MatchOp7, MatchComma, MatchS|MatchOp2), FpRdRnRmRa, 3, 0x5e209000),

	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 3, 0x0e209000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 3, 0x0e209000),
};
DECLARE_INSTRUCTION(SQDMLAL);

constexpr EncodingVariant SQDMLAL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f003000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f003000),
	
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 3, 0x0e209000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 3, 0x0e209000),
};
DECLARE_INSTRUCTION(SQDMLAL2);

constexpr EncodingVariant SQDMLSL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchH|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x5f007000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchS|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x5f007000),
	
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f007000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f007000),

	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchH|MatchOp1|MatchOp7, MatchComma, MatchH|MatchOp2), FpRdRnRmRa, 3, 0x5e20b000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchS|MatchOp1|MatchOp7, MatchComma, MatchS|MatchOp2), FpRdRnRmRa, 3, 0x5e20b000),

	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 3, 0x0e20b000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 3, 0x0e20b000),
};
DECLARE_INSTRUCTION(SQDMLSL);

constexpr EncodingVariant SQDMLSL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f007000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f007000),
	
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 3, 0x0e20b000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 3, 0x0e20b000),
};
DECLARE_INSTRUCTION(SQDMLSL2);

constexpr EncodingVariant SQDMULH_ENCODING_VARIANTS[] =
{
	// Scalar Element
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x801, 0x5f00c000),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x801, 0x5f00c000),
	
	// Vector Element
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f00c000),
	DECLARE_CANDIDATE((MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f00c000),
	
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x5e20b400),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e20b400),
};
DECLARE_INSTRUCTION(SQDMULH);

constexpr EncodingVariant SQDMULL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchH|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x5f00b000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchS|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x5f00b000),
	
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f00b000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f00b000),
	
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchH|MatchOp1|MatchOp7, MatchComma, MatchH|MatchOp2), FpRdRnRmRa, 3, 0x5e20d000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchS|MatchOp1|MatchOp7, MatchComma, MatchS|MatchOp2), FpRdRnRmRa, 3, 0x5e20d000),
	
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 3, 0x0e20d000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 3, 0x0e20d000),
};
DECLARE_INSTRUCTION(SQDMULL);

constexpr EncodingVariant SQDMULL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f00b000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f00b000),
	
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 3, 0x0e20d000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 3, 0x0e20d000),
};
DECLARE_INSTRUCTION(SQDMULL2);

constexpr EncodingVariant SQNEG_ENCODING_VARIANTS[] =
{
	// SQNEG <V><d>, <V><n>
	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 1, 0x7e207800),
	
	// SQNEG <Vd>.<T>, <Vn>.<T>
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 3, 0x2e207800),
};
DECLARE_INSTRUCTION(SQNEG);

constexpr EncodingVariant SQRDMLAH_ENCODING_VARIANTS[] =
{
	// Scalar Element
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x801, 0x7f00d000),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x801, 0x7f00d000),
	
	// Vector Element
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f00d000),
	DECLARE_CANDIDATE((MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f00d000),
	
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x7e008400),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e008400),
};
DECLARE_INSTRUCTION(SQRDMLAH);

constexpr EncodingVariant SQRDMLSH_ENCODING_VARIANTS[] =
{
	// Scalar Element
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x801, 0x7f00f000),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x801, 0x7f00f000),
	
	// Vector Element
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f00f000),
	DECLARE_CANDIDATE((MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f00f000),
	
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x7e008c00),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e008c00),
};
DECLARE_INSTRUCTION(SQRDMLSH);

constexpr EncodingVariant SQRDMULH_ENCODING_VARIANTS[] =
{
	// Scalar Element
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x801, 0x5f00d000),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x801, 0x5f00d000),
	
	// Vector Element
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f00d000),
	DECLARE_CANDIDATE((MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x0f00d000),
	
	// Scalar
	DECLARE_CANDIDATE((MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x7e20b400),
	
	// Vector
	DECLARE_CANDIDATE((MatchV4H8H|MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e20b400),
};
DECLARE_INSTRUCTION(SQRDMULH);

constexpr EncodingVariant SQRSHL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x5e205c00),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e205c00),
};
DECLARE_INSTRUCTION(SQRSHL);

constexpr EncodingVariant SQRSHRN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0|MatchOp7, MatchComma, MatchH|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x5f009c00),
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchS|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x5f009c00),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x5f009c00),

	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x0f009c00),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x0f009c00),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x0f009c00),
};
DECLARE_INSTRUCTION(SQRSHRN);

constexpr EncodingVariant SQRSHRN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x4f009c00),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x4f009c00),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x4f009c00),
};
DECLARE_INSTRUCTION(SQRSHRN2);

constexpr EncodingVariant SQSHL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x5e204c00),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e204c00),

	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x400, 0x5f007400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x0f007400),
};
DECLARE_INSTRUCTION(SQSHL);

constexpr EncodingVariant SQSHLU_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x400, 0x7f006400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x2f006400),
};
DECLARE_INSTRUCTION(SQSHLU);

constexpr EncodingVariant SQRSHRUN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0|MatchOp7, MatchComma, MatchH|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f008c00),
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchS|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f008c00),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f008c00),
	
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x2f008c00),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x2f008c00),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x2f008c00),
};
DECLARE_INSTRUCTION(SQRSHRUN);

constexpr EncodingVariant SQRSHRUN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x6f008c00),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x6f008c00),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x6f008c00),
};
DECLARE_INSTRUCTION(SQRSHRUN2);

constexpr EncodingVariant SQSHRN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0|MatchOp7, MatchComma, MatchH|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x5f009400),
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchS|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x5f009400),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x5f009400),
	
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x0f009400),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x0f009400),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x0f009400),
};
DECLARE_INSTRUCTION(SQSHRN);

constexpr EncodingVariant SQSHRN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x4f009400),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x4f009400),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x4f009400),
};
DECLARE_INSTRUCTION(SQSHRN2);

constexpr EncodingVariant SQSHRUN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0|MatchOp7, MatchComma, MatchH|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f008400),
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchS|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f008400),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f008400),
	
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x2f008400),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x2f008400),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x2f008400),
};
DECLARE_INSTRUCTION(SQSHRUN);

constexpr EncodingVariant SQSHRUN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x6f008400),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x6f008400),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x6f008400),
};
DECLARE_INSTRUCTION(SQSHRUN2);

constexpr EncodingVariant SQSUB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x5e202c00),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e202c00),
};
DECLARE_INSTRUCTION(SQSUB);

constexpr EncodingVariant SQXTN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0|MatchOp7, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 1, 0x5e214800),
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchS|MatchOp1), FpRdRnRmRa, 1, 0x5e214800),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1), FpRdRnRmRa, 1, 0x5e214800),
	
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1), FpRdRnRmRa, 3, 0x0e214800),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1), FpRdRnRmRa, 3, 0x0e214800),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 3, 0x0e214800),
};
DECLARE_INSTRUCTION(SQXTN);

constexpr EncodingVariant SQXTN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1), FpRdRnRmRa, 3, 0x0e214800),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1), FpRdRnRmRa, 3, 0x0e214800),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 3, 0x0e214800),
};
DECLARE_INSTRUCTION(SQXTN2);

constexpr EncodingVariant SQXTUN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0|MatchOp7, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 1, 0x7e212800),
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchS|MatchOp1), FpRdRnRmRa, 1, 0x7e212800),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1), FpRdRnRmRa, 1, 0x7e212800),
	
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1), FpRdRnRmRa, 3, 0x2e212800),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1), FpRdRnRmRa, 3, 0x2e212800),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 3, 0x2e212800),
};
DECLARE_INSTRUCTION(SQXTUN);

constexpr EncodingVariant SQXTUN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1), FpRdRnRmRa, 3, 0x2e212800),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1), FpRdRnRmRa, 3, 0x2e212800),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 3, 0x2e212800),
};
DECLARE_INSTRUCTION(SQXTUN2);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SRHADD, (MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e201400);

constexpr EncodingVariant SRI_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f004400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x202, 0x2f004400),
};
DECLARE_INSTRUCTION(SRI);

constexpr EncodingVariant SRSHL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x5e205400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e205400),
};
DECLARE_INSTRUCTION(SRSHL);

constexpr EncodingVariant SRSHR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x5f002400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x202, 0x0f002400),
};
DECLARE_INSTRUCTION(SRSHR);

constexpr EncodingVariant SRSRA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x5f003400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x202, 0x0f003400),
};
DECLARE_INSTRUCTION(SRSRA);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SSBB, (), Fixed, 0, 0xd503309f);

constexpr EncodingVariant SSHL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x5e204400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e204400),
};
DECLARE_INSTRUCTION(SSHL);

constexpr EncodingVariant SSHLL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x0f00a400),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x0f00a400),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x0f00a400),
};
DECLARE_INSTRUCTION(SSHLL);

constexpr EncodingVariant SSHLL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x0f00a400),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x0f00a400),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x0f00a400),
};
DECLARE_INSTRUCTION(SSHLL2);

constexpr EncodingVariant SSHR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x5f000400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x202, 0x0f000400),
};
DECLARE_INSTRUCTION(SSHR);

constexpr EncodingVariant SSRA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x5f001400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x202, 0x0f001400),
};
DECLARE_INSTRUCTION(SSRA);

constexpr EncodingVariant SSUBL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 3, 0x0e202000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 3, 0x0e202000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 3, 0x0e202000),
};
DECLARE_INSTRUCTION(SSUBL);

constexpr EncodingVariant SSUBL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 3, 0x0e202000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 3, 0x0e202000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 3, 0x0e202000),
};
DECLARE_INSTRUCTION(SSUBL2);

constexpr EncodingVariant SSUBW_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8B|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x0e203000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4H|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x0e203000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV2S|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x0e203000),
};
DECLARE_INSTRUCTION(SSUBW);

constexpr EncodingVariant SSUBW2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV16B|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x0e203000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV8H|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x0e203000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV4S|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x0e203000),
};
DECLARE_INSTRUCTION(SSUBW2);

constexpr EncodingVariant ST1_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0c007000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x92, 0x0c807000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0c807000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0c00a000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x92, 0x0c80a000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0c80a000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0c006000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x92, 0x0c806000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0c806000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0c002000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x92, 0x0c802000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVAll|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0c802000),
	
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x40, 0x0d000000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x60, 0x0d800000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x40, 0x0d800000),
};
DECLARE_INSTRUCTION(ST1);

constexpr EncodingVariant ST2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0c008000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x92, 0x0c808000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0c808000),
	
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x40, 0x0d200000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x60, 0x0da00000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x40, 0x0da00000),
};
DECLARE_INSTRUCTION(ST2);

constexpr EncodingVariant ST3_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0c004000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x92, 0x0c804000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0c804000),
	
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x40, 0x0d002000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x60, 0x0d802000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x40, 0x0d802000),
};
DECLARE_INSTRUCTION(ST3);

constexpr EncodingVariant ST4_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x12, 0x0c000000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x92, 0x0c800000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x12, 0x0c800000),
	
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), FpRdRnRmRa, 0x40, 0x0d202000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x60, 0x0da02000),
	DECLARE_CANDIDATE((MatchLeftBrace, MatchVB|MatchVH|MatchVS|MatchVD|MatchOp0|MatchOp7, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket, MatchComma, MatchReg64|MatchOp2), FpRdRnRmRa, 0x40, 0x0da02000),
};
DECLARE_INSTRUCTION(ST4);

constexpr EncodingVariant STADD_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb820001f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf820001f),
};
DECLARE_INSTRUCTION(STADD);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STADDB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3820001f);

constexpr EncodingVariant STADDL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb860001f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf860001f),
};
DECLARE_INSTRUCTION(STADDL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STADDLB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3860001f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STADDH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7820001f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STADDLH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7860001f);

constexpr EncodingVariant STCLR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb820101f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf820101f),
};
DECLARE_INSTRUCTION(STCLR);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STCLRB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3820101f);

constexpr EncodingVariant STCLRL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb860101f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf860101f),
};
DECLARE_INSTRUCTION(STCLRL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STCLRLB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3860101f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STCLRH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7820101f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STCLRLH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7860101f);

constexpr EncodingVariant STEOR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb820201f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf820201f),
};
DECLARE_INSTRUCTION(STEOR);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STEORB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3820201f);

constexpr EncodingVariant STEORL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb860201f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf860201f),
};
DECLARE_INSTRUCTION(STEORL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STEORLB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3860201f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STEORH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7820201f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STEORLH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7860201f);

constexpr EncodingVariant STLLR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x889f7c00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RtRnImm9, 1, 0x889f7c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xc89f7c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RtRnImm9, 1, 0xc89f7c00),
};
DECLARE_INSTRUCTION(STLLR);

constexpr EncodingVariant STLLRB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x089f7c00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RtRnImm9, 1, 0x089f7c00),
};
DECLARE_INSTRUCTION(STLLRB);

constexpr EncodingVariant STLLRH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x489f7c00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RtRnImm9, 1, 0x489f7c00),
};
DECLARE_INSTRUCTION(STLLRH);

constexpr EncodingVariant STLR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x889ffc00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RtRnImm9, 1, 0x889ffc00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xc89ffc00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RtRnImm9, 1, 0xc89ffc00),
};
DECLARE_INSTRUCTION(STLR);

constexpr EncodingVariant STLRB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x089ffc00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RtRnImm9, 1, 0x089ffc00),
};
DECLARE_INSTRUCTION(STLRB);

constexpr EncodingVariant STLRH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x489ffc00),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RtRnImm9, 1, 0x489ffc00),
};
DECLARE_INSTRUCTION(STLRH);

constexpr EncodingVariant STLUR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x99000000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x99000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xd9000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xd9000000),
};
DECLARE_INSTRUCTION(STLUR);

constexpr EncodingVariant STLURB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x19000000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x19000000),
};
DECLARE_INSTRUCTION(STLURB);

constexpr EncodingVariant STLURH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x59000000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x59000000),
};
DECLARE_INSTRUCTION(STLURH);

constexpr EncodingVariant STLXP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x88208000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x88208000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xc8208000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0xc8208000),
};
DECLARE_INSTRUCTION(STLXP);

constexpr EncodingVariant STLXR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x8800fc00),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x8800fc00),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xc800fc00),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0xc800fc00),
};
DECLARE_INSTRUCTION(STLXR);

constexpr EncodingVariant STLXRB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x0800fc00),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x0800fc00),
};
DECLARE_INSTRUCTION(STLXRB);

constexpr EncodingVariant STLXRH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x4800fc00),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x4800fc00),
};
DECLARE_INSTRUCTION(STLXRH);

constexpr EncodingVariant STNP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket), LoadStorePair, 4, 0x28000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket), LoadStorePair, 8, 0xa8000000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 4, 0x28000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 8, 0xa8000000),
	
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchS|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 4, 0x2c000000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchD|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 8, 0x6c000000),
	DECLARE_CANDIDATE((MatchQ|MatchOp0, MatchComma, MatchQ|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 16, 0xac000000),
};
DECLARE_INSTRUCTION(STNP);

constexpr EncodingVariant STP_ENCODING_VARIANTS[] =
{
	// Post index
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp3), LoadStorePair, 4, 0x28800000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp3), LoadStorePair, 8, 0xa8800000),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchS|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp3), LoadStorePair, 4, 0x2c800000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchD|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp3), LoadStorePair, 8, 0x6c800000),
	DECLARE_CANDIDATE((MatchQ|MatchOp0, MatchComma, MatchQ|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket, MatchComma, MatchImm|MatchOp3), LoadStorePair, 16, 0xac800000),
	
	// Pre index
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket, MatchExclamationMark), LoadStorePair, 4, 0x29800000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket, MatchExclamationMark), LoadStorePair, 8, 0xa9800000),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchS|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket, MatchExclamationMark), LoadStorePair, 4, 0x2d800000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchD|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket, MatchExclamationMark), LoadStorePair, 8, 0x6d800000),
	DECLARE_CANDIDATE((MatchQ|MatchOp0, MatchComma, MatchQ|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket, MatchExclamationMark), LoadStorePair, 16, 0xad800000),
	
	// Signed offset
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket), LoadStorePair, 4, 0x29000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchRightSquareBracket), LoadStorePair, 8, 0xa9000000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 4, 0x29000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 8, 0xa9000000),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchS|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 4, 0x2d000000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchD|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 8, 0x6d000000),
	DECLARE_CANDIDATE((MatchQ|MatchOp0, MatchComma, MatchQ|MatchOp1, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp2, MatchComma, MatchImm|MatchOp3, MatchRightSquareBracket), LoadStorePair, 16, 0xad000000),
};
DECLARE_INSTRUCTION(STP);

constexpr EncodingVariant STR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchReg32|MatchReg32Z, 4, 0xb8000000),
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchReg64|MatchReg64Z, 8, 0xf8000000),
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchB, 1, 0x3c000000),
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchH, 2, 0x7c000000),
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchS, 4, 0xbc000000),
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchD, 8, 0xfc000000),
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchQ, 16, 0x3c800000),
	
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchReg32|MatchReg32Z, 2, 0xb8200800),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchReg64|MatchReg64Z, 3, 0xf8200800),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchB, 0, 0x3c200800),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchH, 1, 0x7c200800),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchS, 2, 0xbc200800),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchD, 3, 0xfc200800),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchQ, 4, 0x3ca00800),
};
DECLARE_INSTRUCTION(STR);

constexpr EncodingVariant STRB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchReg32|MatchReg32Z, 1, 0x38000000),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchReg32|MatchReg32Z, 0, 0x38200800),
};
DECLARE_INSTRUCTION(STRB);

constexpr EncodingVariant STRH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATES_LOAD_STORE_IMMEDIATE_OFFSET(MatchReg32|MatchReg32Z, 2, 0x78000000),
	DECLARE_CANDIDATES_LOAD_STORE_REGISTER_OFFSET(MatchReg32|MatchReg32Z, 1, 0x78200800),
};
DECLARE_INSTRUCTION(STRH);

constexpr EncodingVariant STSET_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb820301f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf820301f),
};
DECLARE_INSTRUCTION(STSET);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STSETB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3820301f);

constexpr EncodingVariant STSETL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb860301f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf860301f),
};
DECLARE_INSTRUCTION(STSETL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STSETLB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3860301f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STSETH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7820301f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STSETLH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7860301f);

constexpr EncodingVariant STSMAX_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb820401f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf820401f),
};
DECLARE_INSTRUCTION(STSMAX);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STSMAXB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3820401f);

constexpr EncodingVariant STSMAXL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb860401f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf860401f),
};
DECLARE_INSTRUCTION(STSMAXL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STSMAXLB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3860401f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STSMAXH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7820401f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STSMAXLH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7860401f);

constexpr EncodingVariant STSMIN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb820501f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf820501f),
};
DECLARE_INSTRUCTION(STSMIN);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STSMINB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3820501f);

constexpr EncodingVariant STSMINL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb860501f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf860501f),
};
DECLARE_INSTRUCTION(STSMINL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STSMINLB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3860501f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STSMINH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7820501f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STSMINLH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7860501f);

constexpr EncodingVariant STTR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xb8000800),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xb8000800),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xf8000800),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xf8000800),
};
DECLARE_INSTRUCTION(STTR);

constexpr EncodingVariant STTRB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x38000800),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x38000800),
};
DECLARE_INSTRUCTION(STTRB);

constexpr EncodingVariant STTRH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x78000800),
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x78000800),
};
DECLARE_INSTRUCTION(STTRH);

constexpr EncodingVariant STUMAX_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb820601f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf820601f),
};
DECLARE_INSTRUCTION(STUMAX);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STUMAXB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3820601f);

constexpr EncodingVariant STUMAXL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb860601f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf860601f),
};
DECLARE_INSTRUCTION(STUMAXL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STUMAXLB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3860601f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STUMAXH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7820601f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STUMAXLH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7860601f);

constexpr EncodingVariant STUMIN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb820701f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf820701f),
};
DECLARE_INSTRUCTION(STUMIN);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STUMINB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3820701f);

constexpr EncodingVariant STUMINL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb860701f),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf860701f),
};
DECLARE_INSTRUCTION(STUMINL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STUMINLB, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x3860701f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STUMINH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7820701f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(STUMINLH, (MatchReg32|MatchOp2, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x7860701f);

constexpr EncodingVariant STUR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xb8000000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xb8000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xf8000000),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xf8000000),
	DECLARE_CANDIDATE((MatchB|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x3c000000),
	DECLARE_CANDIDATE((MatchB|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x3c000000),
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x7c000000),
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x7c000000),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xbc000000),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xbc000000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0xfc000000),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0xfc000000),
	DECLARE_CANDIDATE((MatchQ|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x3c800000),
	DECLARE_CANDIDATE((MatchQ|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x3c800000),
};
DECLARE_INSTRUCTION(STUR);

constexpr EncodingVariant STURB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x38000000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x38000000),
};
DECLARE_INSTRUCTION(STURB);

constexpr EncodingVariant STURH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RtRnImm9, 1, 0x78000000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchRightSquareBracket), RtRnImm9, 1, 0x78000000),
};
DECLARE_INSTRUCTION(STURH);

constexpr EncodingVariant STXP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x88200000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x88200000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xc8200000),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp3, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0xc8200000),
};
DECLARE_INSTRUCTION(STXP);

constexpr EncodingVariant STXR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x88007c00),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x88007c00),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xc8007c00),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0xc8007c00),
};
DECLARE_INSTRUCTION(STXR);

constexpr EncodingVariant STXRB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x08007c00),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x08007c00),
};
DECLARE_INSTRUCTION(STXRB);

constexpr EncodingVariant STXRH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x48007c00),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm0, MatchRightSquareBracket), RdRnRmRa, 0, 0x48007c00),
};
DECLARE_INSTRUCTION(STXRH);

constexpr EncodingVariant SUB_ENCODING_VARIANTS[] =
{
	// SUB <Wd|WSP>, <Wn|WSP>, #<imm>, <shift>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchImm|MatchOp2), ArithmeticImmediate, 0, 0x51000000),
	// SUB <Wd|WSP>, <Wn|WSP>, #<imm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchUImm12|MatchOp2, MatchComma, MatchLSL, MatchImm|MatchOp3), ArithmeticImmediate, 0, 0x51000000),
	// SUB <Wd>, <Wn>, <Wm>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x4b000000),
	// SUB <Wd>, <Wn>, <Wm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x4b000000),
	// SUB <Wd|WSP>, <Wn|WSP>, <Wm>, <extend>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchExtendReg32|MatchOp3), ArithmeticExtendedRegister, 0, 0x4b200000),
	// SUB <Wd|WSP>, <Wn|WSP>, <Wm>, <extend> #<amount>}
	DECLARE_CANDIDATE((MatchReg32|MatchReg32SP|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg32|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0x4b200000),
	
	// SUB <Xd|SP>, <Xn|SP>, #<imm>, <shift>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2), ArithmeticImmediate, 0, 0xd1000000),
	// SUB <Xd|SP>, <Xn|SP>, #<imm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchUImm12|MatchOp2, MatchComma, MatchLSL, MatchImm|MatchOp3), ArithmeticImmediate, 0, 0xd1000000),
	// SUB <Xd>, <Xn>, <Xm>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xcb000000),
	// SUB <Xd>, <Xn>, <Xm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xcb000000),
	// SUB <Xd|SP>, <Xn|WSP>, <Wm>, <extend>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchExtendReg32|MatchOp3), ArithmeticExtendedRegister, 0, 0xcb200000),
	// SUB <Xd|SP>, <Xn|WSP>, <Wm>, <extend> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg32|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0xcb200000),
	// SUB <Xd|SP>, <Xn|WSP>, <Xm>, <extend>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchExtendReg64|MatchOp3), ArithmeticExtendedRegister, 0, 0xcb200000),
	// SUB <Xd|SP>, <Xn|WSP>, <Xm>, <extend> #<amount>}
	DECLARE_CANDIDATE((MatchReg64|MatchReg64SP|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg64|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0xcb200000),

	// SUB Dd, Dn, Dm
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchD|MatchOp2), FpRdRnRmRa, 0, 0x7ee08400),
	
	// SUB <Vd>.T, <Vn>.T, <Vm>.T
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e208400),
};
DECLARE_INSTRUCTION(SUB);

constexpr EncodingVariant SUBHN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 1, 0x0e206000),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 1, 0x0e206000),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV2D|MatchOp2), FpRdRnRmRa, 1, 0x0e206000),
};
DECLARE_INSTRUCTION(SUBHN);

constexpr EncodingVariant SUBHN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 1, 0x4e206000),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 1, 0x4e206000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV2D|MatchOp2), FpRdRnRmRa, 1, 0x4e206000),
};
DECLARE_INSTRUCTION(SUBHN2);

constexpr EncodingVariant SUBS_ENCODING_VARIANTS[] =
{
	// SUBS Wd, <Wn|WSP>, #<imm>, <shift>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchImm|MatchOp2), ArithmeticImmediate, 0, 0x71000000),
	// SUBS Wd, <Wn|WSP>, #<imm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchUImm12|MatchOp2, MatchComma, MatchLSL, MatchImm|MatchOp3), ArithmeticImmediate, 0, 0x71000000),
	// SUBS Wd, <Wn>, <Wm>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x6b000000),
	// SUBS Wd, <Wn>, <Wm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x6b000000),
	// SUBS Wd, <Wn|WSP>, <Wm>, <extend>
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchExtendReg32|MatchOp3), ArithmeticExtendedRegister, 0, 0x6b200000),
	// SUBS Wd, <Wn|WSP>, <Wm>, <extend> #<amount>}
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchReg32|MatchReg32SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg32|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0x6b200000),
	
	// SUBS Xd, <Xn|SP>, #<imm>, <shift>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchImm|MatchOp2), ArithmeticImmediate, 0, 0xf1000000),
	// SUBS Xd, <Xn|SP>, #<imm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchUImm12|MatchOp2, MatchComma, MatchLSL, MatchImm|MatchOp3), ArithmeticImmediate, 0, 0xf1000000),
	// SUBS Xd, <Xn>, <Xm>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xeb000000),
	// SUBS Xd, <Xn>, <Xm>, <shift> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xeb000000),
	// SUBS Xd, <Xn|WSP>, <Wm>, <extend>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchExtendReg32|MatchOp3), ArithmeticExtendedRegister, 0, 0xeb200000),
	// SUBS Xd, <Xn|WSP>, <Wm>, <extend> #<amount>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg32|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0xeb200000),
	// SUBS Xd, <Xn|WSP>, <Xm>, <extend>
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchExtendReg64|MatchOp3), ArithmeticExtendedRegister, 0, 0xeb200000),
	// SUBS Xd, <Xn|WSP>, <Xm>, <extend> #<amount>}
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchReg64|MatchReg64SP|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchLSL|MatchExtendReg64|MatchOp3, MatchImm|MatchOp4), ArithmeticExtendedRegister, 0, 0xeb200000),
};
DECLARE_INSTRUCTION(SUBS);

constexpr EncodingVariant SUQADD_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 1, 0x5e203800),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 3, 0x0e203800),
};
DECLARE_INSTRUCTION(SUQADD);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SVC, (MatchUImm16|MatchOp1), RdImm16Shift, 0, 0xd4000001);

constexpr EncodingVariant SWP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8208000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8208000),
};
DECLARE_INSTRUCTION(SWP);

constexpr EncodingVariant SWPA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8a08000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8a08000),
};
DECLARE_INSTRUCTION(SWPA);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SWPAB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38a08000);

constexpr EncodingVariant SWPAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8e08000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8e08000),
};
DECLARE_INSTRUCTION(SWPAL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SWPALB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38e08000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SWPB, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38208000);

constexpr EncodingVariant SWPL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xb8608000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0xf8608000),
};
DECLARE_INSTRUCTION(SWPL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SWPLB, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x38608000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SWPAH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78a08000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SWPALH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78e08000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SWPH, (MatchReg32|MatchOp2, MatchComma, MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78208000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SWPLH, (MatchReg32|MatchOp2, MatchComma,MatchReg32|MatchReg32Z|MatchOp0, MatchComma, MatchLeftSquareBracket, MatchReg64|MatchReg64SP|MatchOp1, MatchRightSquareBracket), RdRnRmRa, 0, 0x78608000);

constexpr EncodingVariant SXTB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1), RdRnImmrImms, 0, 0x13001c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1), RdRnImmrImms, 0, 0x93401c00),
};
DECLARE_INSTRUCTION(SXTB);

constexpr EncodingVariant SXTH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1), RdRnImmrImms, 0, 0x13003c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1), RdRnImmrImms, 0, 0x93403c00),
};
DECLARE_INSTRUCTION(SXTH);

constexpr EncodingVariant SXTL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x0f08a400),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x0f10a400),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x0f20a400),
};
DECLARE_INSTRUCTION(SXTL);

constexpr EncodingVariant SXTL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x0f08a400),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x0f10a400),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x0f20a400),
};
DECLARE_INSTRUCTION(SXTL2);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SXTW, (MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1), RdRnImmrImms, 0, 0x93407c00);

constexpr EncodingVariant TBL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchRightBrace, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 2, 0x0e000000),
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchRightBrace, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 2, 0x0e000000),

	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 2, 0x0e002000),
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 2, 0x0e002000),

	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 2, 0x0e004000),
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 2, 0x0e004000),

	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 2, 0x0e006000),
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 2, 0x0e006000),
};
DECLARE_INSTRUCTION(TBL);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(TBNZ, (MatchReg32|MatchReg64|MatchOp0, MatchComma, MatchImm|MatchOp1, MatchComma, MatchRel|MatchOp2), RtImmRel14, 0, 0x37000000);

constexpr EncodingVariant TBX_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchRightBrace, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 2, 0x0e001000),
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchRightBrace, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 2, 0x0e001000),
	
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 2, 0x0e003000),
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 2, 0x0e003000),
	
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 2, 0x0e005000),
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 2, 0x0e005000),
	
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 2, 0x0e007000),
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchLeftBrace, MatchV16B|MatchOp1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchComma, MatchRegNP1, MatchRightBrace, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 2, 0x0e007000),
};
DECLARE_INSTRUCTION(TBX);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(TBZ, (MatchReg32|MatchReg64|MatchOp0, MatchComma, MatchImm|MatchOp1, MatchComma, MatchRel|MatchOp2), RtImmRel14, 0, 0x36000000);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(TRN1, (MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e002800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(TRN2, (MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e006800);

constexpr EncodingVariant TST_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchImm|MatchOp2), LogicalImmediate, 32, 0x7200001f),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchImm|MatchOp2), LogicalImmediate, 64, 0xf200001f),
	
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2), ArithmeticShiftedRegister, 0, 0x6a00001f),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2), ArithmeticShiftedRegister, 0, 0xea00001f),
	DECLARE_CANDIDATE((MatchReg32|MatchReg32Z|MatchOp1, MatchComma, MatchReg32|MatchReg32Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0x6a00001f),
	DECLARE_CANDIDATE((MatchReg64|MatchReg64Z|MatchOp1, MatchComma, MatchReg64|MatchReg64Z|MatchOp2, MatchComma, MatchShift|MatchROR|MatchOp3, MatchImm|MatchOp4), ArithmeticShiftedRegister, 0, 0xea00001f),
};
DECLARE_INSTRUCTION(TST);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UABA, (MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e207c00);

constexpr EncodingVariant UABAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 0, 0x2e205000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 0, 0x2e605000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 0, 0x2ea05000),
};
DECLARE_INSTRUCTION(UABAL);

constexpr EncodingVariant UABAL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 0, 0x6e205000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 0, 0x6e605000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 0, 0x6ea05000),
};
DECLARE_INSTRUCTION(UABAL2);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UABD, (MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e207400);

constexpr EncodingVariant UABDL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 0, 0x2e207000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 0, 0x2e607000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 0, 0x2ea07000),
};
DECLARE_INSTRUCTION(UABDL);

constexpr EncodingVariant UABDL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 0, 0x6e207000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 0, 0x6e607000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 0, 0x6ea07000),
};
DECLARE_INSTRUCTION(UABDL2);

constexpr EncodingVariant UADALP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e206800),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e206800),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e206800),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e206800),
	DECLARE_CANDIDATE((MatchV1D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e206800),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e206800),
};
DECLARE_INSTRUCTION(UADALP);

constexpr EncodingVariant UADDL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 3, 0x2e200000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 3, 0x2e200000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 3, 0x2e200000),
};
DECLARE_INSTRUCTION(UADDL);

constexpr EncodingVariant UADDL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 3, 0x2e200000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 3, 0x2e200000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 3, 0x2e200000),
};
DECLARE_INSTRUCTION(UADDL2);

constexpr EncodingVariant UADDLP_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e202800),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e202800),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e202800),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e202800),
	DECLARE_CANDIDATE((MatchV1D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e202800),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e202800),
};
DECLARE_INSTRUCTION(UADDLP);

constexpr EncodingVariant UADDLV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e303800),
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e303800),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e303800),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e303800),
	DECLARE_CANDIDATE((MatchD|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e303800),
};
DECLARE_INSTRUCTION(UADDLV);

constexpr EncodingVariant UADDW_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8B|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x2e201000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4H|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x2e201000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV2S|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x2e201000),
};
DECLARE_INSTRUCTION(UADDW);

constexpr EncodingVariant UADDW2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV16B|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x2e201000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV8H|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x2e201000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV4S|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x2e201000),
};
DECLARE_INSTRUCTION(UADDW2);

constexpr EncodingVariant UBFIZ_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0x21, 0x53000000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0x31, 0xd3400000),
};
DECLARE_INSTRUCTION(UBFIZ);

constexpr EncodingVariant UBFM_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0, 0x53000000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0, 0xd3400000),
};
DECLARE_INSTRUCTION(UBFM);

constexpr EncodingVariant UBFX_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0x04, 0x53000000),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchImm|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnImmrImms, 0x04, 0xd3400000),
};
DECLARE_INSTRUCTION(UBFX);

constexpr EncodingVariant UCVTF_ENCODING_VARIANTS[] =
{
	// Vector, Fixed Point
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f00e400),
	DECLARE_CANDIDATE((MatchV4H8H|MatchV2S4S|MatchV2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x202, 0x2f00e400),

	// Vector, Integer
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 0, 0x7e79d800),
	DECLARE_CANDIDATE((MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 4, 0x7e21d800),
	DECLARE_CANDIDATE((MatchV4H8H|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2e79d800),
	DECLARE_CANDIDATE((MatchV2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 6, 0x2e21d800),

	// Scalar, Fixed Point
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x1008, 0x1e030000),
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x1008, 0x9e030000),

	// Scalar, Integer
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchReg32|MatchOp1), FpRdRnRmRa, 8, 0x1e230000),
	DECLARE_CANDIDATE((MatchFpAll|MatchOp0|MatchOp7, MatchComma, MatchReg64|MatchOp1), FpRdRnRmRa, 8, 0x9e230000),
};
DECLARE_INSTRUCTION(UCVTF);

constexpr EncodingVariant UDIV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2), RdRnRmImm6, 0, 0x1ac00800),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchOp2), RdRnRmImm6, 0, 0x9ac00800),
};
DECLARE_INSTRUCTION(UDIV);

constexpr EncodingVariant UDOT_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV8B|MatchOp1, MatchComma, MatchV4B|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f00e000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV16B|MatchOp1, MatchComma, MatchV4B|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f00e000),
};
DECLARE_INSTRUCTION(UDOT);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UHADD, (MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e200400);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UHSUB, (MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e202400);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UMADDL, (MatchReg64|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp3), RdRnRmRa, 0, 0x9ba00000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UMAX, (MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e206400);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UMAXP, (MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e20a400);

constexpr EncodingVariant UMAXV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0, MatchComma, MatchV8B16B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e30a800),
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchV4H8H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e30a800),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e30a800),
};
DECLARE_INSTRUCTION(UMAXV);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UMIN, (MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e206c00);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UMINP, (MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e20ac00);

constexpr EncodingVariant UMINV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0, MatchComma, MatchV8B16B|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e31a800),
	DECLARE_CANDIDATE((MatchH|MatchOp0, MatchComma, MatchV4H8H|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e31a800),
	DECLARE_CANDIDATE((MatchS|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 3, 0x2e31a800),
};
DECLARE_INSTRUCTION(UMINV);

constexpr EncodingVariant UMLAL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f002000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f002000),
	
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 3, 0x2e208000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 3, 0x2e208000),
};
DECLARE_INSTRUCTION(UMLAL);

constexpr EncodingVariant UMLAL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f002000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f002000),
	
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 3, 0x2e208000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 3, 0x2e208000),
};
DECLARE_INSTRUCTION(UMLAL2);

constexpr EncodingVariant UMLSL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f006000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f006000),
	
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 3, 0x2e20a000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 3, 0x2e20a000),
};
DECLARE_INSTRUCTION(UMLSL);

constexpr EncodingVariant UMLSL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f006000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f006000),
	
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 3, 0x2e20a000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 3, 0x2e20a000),
};
DECLARE_INSTRUCTION(UMLSL2);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UMNEGL, (MatchReg64|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2), RdRnRmRa, 0, 0x9ba0fc00);

constexpr EncodingVariant UMOV_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchVB|MatchVH|MatchVS|MatchOp1|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x0e003c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchVD|MatchOp1|MatchOp7, MatchLeftSquareBracket, MatchNumber|MatchOp2, MatchRightSquareBracket), FpRdRnIndex1Index2, 0, 0x4e103c00),
};
DECLARE_INSTRUCTION(UMOV);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UMSUBL, (MatchReg64|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2, MatchComma, MatchReg64|MatchReg64Z|MatchOp3), RdRnRmRa, 0, 0x9ba08000);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UMULH, (MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1, MatchComma, MatchReg64|MatchOp2), RdRnRmRa, 0, 0x9bc07c00);

constexpr EncodingVariant UMULL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg32|MatchOp1, MatchComma, MatchReg32|MatchOp2), RdRnRmRa, 0, 0x9ba07c00),
	
	// Vector Element
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f00a000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f00a000),

	// Vector
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 3, 0x2e20c000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 3, 0x2e20c000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 3, 0x2e20c000),
};
DECLARE_INSTRUCTION(UMULL);

constexpr EncodingVariant UMULL2_ENCODING_VARIANTS[] =
{
	// Vector Element
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchVH|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f00a000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchVS|MatchOp2, MatchLeftSquareBracket, MatchNumber|MatchOp5, MatchRightSquareBracket), FpRdRnRmRa, 0x803, 0x2f00a000),
	
	// Vector
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 3, 0x2e20c000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 3, 0x2e20c000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 3, 0x2e20c000),
};
DECLARE_INSTRUCTION(UMULL2);

constexpr EncodingVariant UQADD_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x7e200c00),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e200c00),
};
DECLARE_INSTRUCTION(UQADD);

constexpr EncodingVariant UQRSHL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x7e205c00),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e205c00),
};
DECLARE_INSTRUCTION(UQRSHL);

constexpr EncodingVariant UQRSHRN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0|MatchOp7, MatchComma, MatchH|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f009c00),
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchS|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f009c00),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f009c00),
	
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x2f009c00),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x2f009c00),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x2f009c00),
};
DECLARE_INSTRUCTION(UQRSHRN);

constexpr EncodingVariant UQRSHRN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x6f009c00),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x6f009c00),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x6f009c00),
};
DECLARE_INSTRUCTION(UQRSHRN2);

constexpr EncodingVariant UQSHL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x7e204c00),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e204c00),
	
	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x400, 0x7f007400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x2f007400),
};
DECLARE_INSTRUCTION(UQSHL);

constexpr EncodingVariant UQSHRN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0|MatchOp7, MatchComma, MatchH|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f009400),
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchS|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f009400),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f009400),
	
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x2f009400),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x2f009400),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x2f009400),
};
DECLARE_INSTRUCTION(UQSHRN);

constexpr EncodingVariant UQSHRN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x6f009400),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x6f009400),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x6f009400),
};
DECLARE_INSTRUCTION(UQSHRN2);

constexpr EncodingVariant UQSUB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x7e202c00),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e202c00),
};
DECLARE_INSTRUCTION(UQSUB);

constexpr EncodingVariant UQXTN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchOp0|MatchOp7, MatchComma, MatchH|MatchOp1), FpRdRnRmRa, 1, 0x7e214800),
	DECLARE_CANDIDATE((MatchH|MatchOp0|MatchOp7, MatchComma, MatchS|MatchOp1), FpRdRnRmRa, 1, 0x7e214800),
	DECLARE_CANDIDATE((MatchS|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1), FpRdRnRmRa, 1, 0x7e214800),

	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1), FpRdRnRmRa, 3, 0x2e214800),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1), FpRdRnRmRa, 3, 0x2e214800),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 3, 0x2e214800),
};
DECLARE_INSTRUCTION(UQXTN);

constexpr EncodingVariant UQXTN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1), FpRdRnRmRa, 3, 0x2e214800),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1), FpRdRnRmRa, 3, 0x2e214800),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 3, 0x2e214800),
};
DECLARE_INSTRUCTION(UQXTN2);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(URECPE, (MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x0ea1c800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(URHADD, (MatchV8B16B4H8H2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e201400);

constexpr EncodingVariant URSHL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x7e205400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e205400),
};
DECLARE_INSTRUCTION(URSHL);

constexpr EncodingVariant URSHR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f002400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x202, 0x2f002400),
};
DECLARE_INSTRUCTION(URSHR);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(URSQRTE, (MatchV2S4S|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 2, 0x2ea1c800);

constexpr EncodingVariant URSRA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f003400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x202, 0x2f003400),
};
DECLARE_INSTRUCTION(URSRA);

constexpr EncodingVariant USHL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 1, 0x7e204400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x2e204400),
};
DECLARE_INSTRUCTION(USHL);

constexpr EncodingVariant USHLL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x2f00a400),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x2f00a400),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x2f00a400),
};
DECLARE_INSTRUCTION(USHLL);

constexpr EncodingVariant USHLL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x2f00a400),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x2f00a400),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x402, 0x2f00a400),
};
DECLARE_INSTRUCTION(USHLL2);

constexpr EncodingVariant USHR_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f000400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x202, 0x2f000400),
};
DECLARE_INSTRUCTION(USHR);

constexpr EncodingVariant USQADD_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchB|MatchH|MatchS|MatchD|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 1, 0x7e203800),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1), FpRdRnRmRa, 3, 0x2e203800),
};
DECLARE_INSTRUCTION(USQADD);

constexpr EncodingVariant USRA_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchD|MatchOp0|MatchOp7, MatchComma, MatchD|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x200, 0x7f001400),
	DECLARE_CANDIDATE((MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchImm|MatchOp4), FpRdRnRmRa, 0x202, 0x2f001400),
};
DECLARE_INSTRUCTION(USRA);

constexpr EncodingVariant USUBL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7, MatchComma, MatchV8B|MatchOp2), FpRdRnRmRa, 3, 0x2e202000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7, MatchComma, MatchV4H|MatchOp2), FpRdRnRmRa, 3, 0x2e202000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7, MatchComma, MatchV2S|MatchOp2), FpRdRnRmRa, 3, 0x2e202000),
};
DECLARE_INSTRUCTION(USUBL);

constexpr EncodingVariant USUBL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7, MatchComma, MatchV16B|MatchOp2), FpRdRnRmRa, 3, 0x2e202000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7, MatchComma, MatchV8H|MatchOp2), FpRdRnRmRa, 3, 0x2e202000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7, MatchComma, MatchV4S|MatchOp2), FpRdRnRmRa, 3, 0x2e202000),
};
DECLARE_INSTRUCTION(USUBL2);

constexpr EncodingVariant USUBW_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV8B|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x2e203000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV4H|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x2e203000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV2S|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x2e203000),
};
DECLARE_INSTRUCTION(USUBW);

constexpr EncodingVariant USUBW2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8H|MatchOp1, MatchComma, MatchV16B|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x2e203000),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4S|MatchOp1, MatchComma, MatchV8H|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x2e203000),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2D|MatchOp1, MatchComma, MatchV4S|MatchOp2|MatchOp7), FpRdRnRmRa, 3, 0x2e203000),
};
DECLARE_INSTRUCTION(USUBW2);

constexpr EncodingVariant UXTB_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1), RdRnImmrImms, 0, 0x53001c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1), RdRnImmrImms, 0, 0xd3401c00),
};
DECLARE_INSTRUCTION(UXTB);

constexpr EncodingVariant UXTL_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV8B|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x2f08a400),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV4H|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x2f10a400),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV2S|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x2f20a400),
};
DECLARE_INSTRUCTION(UXTL);

constexpr EncodingVariant UXTL2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8H|MatchOp0, MatchComma, MatchV16B|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x2f08a400),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0, MatchComma, MatchV8H|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x2f10a400),
	DECLARE_CANDIDATE((MatchV2D|MatchOp0, MatchComma, MatchV4S|MatchOp1|MatchOp7), FpRdRnRmRa, 2, 0x2f20a400),
};
DECLARE_INSTRUCTION(UXTL2);

constexpr EncodingVariant UXTH_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchReg32|MatchOp0, MatchComma, MatchReg32|MatchOp1), RdRnImmrImms, 0, 0x53003c00),
	DECLARE_CANDIDATE((MatchReg64|MatchOp0, MatchComma, MatchReg64|MatchOp1), RdRnImmrImms, 0, 0xd3403c00),
};
DECLARE_INSTRUCTION(UXTH);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UZP1, (MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e001800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(UZP2, (MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e005800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(WFE, (), Fixed, 0, 0xd503205f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(WFI, (), Fixed, 0, 0xd503207f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(XAR, (MatchV2D|MatchOp0, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2, MatchComma, MatchImm|MatchOp3), RdRnRmImm6, 0, 0xce800000);

constexpr EncodingVariant XTN_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV8B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1), FpRdRnRmRa, 3, 0x0e212800),
	DECLARE_CANDIDATE((MatchV4H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1), FpRdRnRmRa, 3, 0x0e212800),
	DECLARE_CANDIDATE((MatchV2S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 3, 0x0e212800),
};
DECLARE_INSTRUCTION(XTN);

constexpr EncodingVariant XTN2_ENCODING_VARIANTS[] =
{
	DECLARE_CANDIDATE((MatchV16B|MatchOp0|MatchOp7, MatchComma, MatchV8H|MatchOp1), FpRdRnRmRa, 3, 0x0e212800),
	DECLARE_CANDIDATE((MatchV8H|MatchOp0|MatchOp7, MatchComma, MatchV4S|MatchOp1), FpRdRnRmRa, 3, 0x0e212800),
	DECLARE_CANDIDATE((MatchV4S|MatchOp0|MatchOp7, MatchComma, MatchV2D|MatchOp1), FpRdRnRmRa, 3, 0x0e212800),
};
DECLARE_INSTRUCTION(XTN2);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(XPACD, (MatchReg64|MatchOp0), RdRnRmRa, 0, 0xdac147e0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(XPACI, (MatchReg64|MatchOp0), RdRnRmRa, 0, 0xdac143e0);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(XPACLRI, (), Fixed, 0, 0xd50320ff);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(YIELD, (), Fixed, 0, 0xd503203f);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(ZIP1, (MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e003800);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(ZIP2, (MatchV8B16B4H8H2S4S2D|MatchOp0|MatchOp7, MatchComma, MatchRep|MatchOp1, MatchComma, MatchRep|MatchOp2), FpRdRnRmRa, 3, 0x0e007800);

//============================================================================

InstructionMap::InstructionMap()
{
	(*this)["abs"] = &ABS;
	(*this)["adc"] = &ADC;
	(*this)["adcs"] = &ADCS;
	(*this)["add"] = &ADD;
	(*this)["addhn"] = &ADDHN;
	(*this)["addhn2"] = &ADDHN2;
	(*this)["addp"] = &ADDP;
	(*this)["adds"] = &ADDS;
	(*this)["addv"] = &ADDV;
	(*this)["adr"] = &ADR;
	(*this)["adrp"] = &ADRP;
	(*this)["aesd"] = &AESD;
	(*this)["aese"] = &AESE;
	(*this)["aesimc"] = &AESIMC;
	(*this)["aesmc"] = &AESMC;
	(*this)["and"] = &AND;
	(*this)["ands"] = &ANDS;
	(*this)["asr"] = &ASR;
	(*this)["asrv"] = &ASRV;
//	(*this)["at"] = &AT;
	(*this)["autda"] = &AUTDA;
	(*this)["autdza"] = &AUTDZA;
	(*this)["autdb"] = &AUTDB;
	(*this)["autdzb"] = &AUTDZB;
	(*this)["autia"] = &AUTIA;
	(*this)["autia1716"] = &AUTIA1716;
	(*this)["autiasp"] = &AUTIASP;
	(*this)["autiaz"] = &AUTIAZ;
	(*this)["autiza"] = &AUTIZA;
	(*this)["autib"] = &AUTIB;
	(*this)["autib1716"] = &AUTIB1716;
	(*this)["autibsp"] = &AUTIBSP;
	(*this)["autibz"] = &AUTIBZ;
	(*this)["autizb"] = &AUTIZB;
	(*this)["b"] = &B;
	(*this)["b.cc"] = &B_CC;
	(*this)["b.cs"] = &B_CS;
	(*this)["b.eq"] = &B_EQ;
	(*this)["b.ge"] = &B_GE;
	(*this)["b.gt"] = &B_GT;
	(*this)["b.hi"] = &B_HI;
	(*this)["b.hs"] = &B_CS;
	(*this)["b.le"] = &B_LE;
	(*this)["b.lo"] = &B_CC;
	(*this)["b.ls"] = &B_LS;
	(*this)["b.lt"] = &B_LT;
	(*this)["b.mi"] = &B_MI;
	(*this)["b.ne"] = &B_NE;
	(*this)["b.pl"] = &B_PL;
	(*this)["b.vc"] = &B_VC;
	(*this)["b.vs"] = &B_VS;
	(*this)["bcax"] = &BCAX;
	(*this)["bfc"] = &BFC;
	(*this)["bfi"] = &BFI;
	(*this)["bfm"] = &BFM;
	(*this)["bfxil"] = &BFXIL;
	(*this)["bic"] = &BIC;
	(*this)["bics"] = &BICS;
	(*this)["bif"] = &BIF;
	(*this)["bit"] = &BIT;
	(*this)["bl"] = &BL;
	(*this)["blr"] = &BLR;
	(*this)["blraa"] = &BLRAA;
	(*this)["blraaz"] = &BLRAAZ;
	(*this)["blrab"] = &BLRAB;
	(*this)["blrabz"] = &BLRABZ;
	(*this)["br"] = &BR;
	(*this)["braa"] = &BRAA;
	(*this)["braaz"] = &BRAAZ;
	(*this)["brab"] = &BRAB;
	(*this)["brabz"] = &BRABZ;
	(*this)["brk"] = &BRK;
	(*this)["bsl"] = &BSL;
	(*this)["casab"] = &CASAB;
	(*this)["casalb"] = &CASALB;
	(*this)["casb"] = &CASB;
	(*this)["caslb"] = &CASLB;
	(*this)["casah"] = &CASAH;
	(*this)["casalh"] = &CASALH;
	(*this)["cash"] = &CASH;
	(*this)["caslh"] = &CASLH;
	(*this)["casp"] = &CASP;
	(*this)["caspa"] = &CASPA;
	(*this)["caspal"] = &CASPAL;
	(*this)["caspl"] = &CASPL;
	(*this)["casa"] = &CASA;
	(*this)["casal"] = &CASAL;
	(*this)["cas"] = &CAS;
	(*this)["casl"] = &CASL;
	(*this)["cbnz"] = &CBNZ;
	(*this)["cbz"] = &CBZ;
	(*this)["ccmn"] = &CCMN;
	(*this)["ccmp"] = &CCMP;
	(*this)["cfinv"] = &CFINV;
	(*this)["cinc"] = &CINC;
	(*this)["cinv"] = &CINV;
	(*this)["clrex"] = &CLREX;
	(*this)["cls"] = &CLS;
	(*this)["clz"] = &CLZ;
	(*this)["cmeq"] = &CMEQ;
	(*this)["cmge"] = &CMGE;
	(*this)["cmgt"] = &CMGT;
	(*this)["cmhi"] = &CMHI;
	(*this)["cmhs"] = &CMHS;
	(*this)["cmle"] = &CMLE;
	(*this)["cmlt"] = &CMLT;
	(*this)["cmn"] = &CMN;
	(*this)["cmp"] = &CMP;
	(*this)["cmtst"] = &CMTST;
	(*this)["cneg"] = &CNEG;
	(*this)["cnt"] = &CNT;
	(*this)["crc32b"] = &CRC32B;
	(*this)["crc32h"] = &CRC32H;
	(*this)["crc32w"] = &CRC32W;
	(*this)["crc32x"] = &CRC32X;
	(*this)["crc32cb"] = &CRC32CB;
	(*this)["crc32ch"] = &CRC32CH;
	(*this)["crc32cw"] = &CRC32CW;
	(*this)["crc32cx"] = &CRC32CX;
	(*this)["csdb"] = &CSDB;
	(*this)["csel"] = &CSEL;
	(*this)["cset"] = &CSET;
	(*this)["csetm"] = &CSETM;
	(*this)["csinc"] = &CSINC;
	(*this)["csinv"] = &CSINV;
	(*this)["csneg"] = &CSNEG;
//	(*this)["dc"] = &DC;
	(*this)["dcps1"] = &DCPS1;
	(*this)["dcps2"] = &DCPS2;
	(*this)["dcps3"] = &DCPS3;
//	(*this)["dmb"] = &DMB;
	(*this)["drps"] = &DRPS;
//	(*this)["dsb"] = &DSB;
	(*this)["dup"] = &DUP;
	(*this)["eon"] = &EON;
	(*this)["eor"] = &EOR;
	(*this)["eor3"] = &EOR3;
	(*this)["eret"] = &ERET;
	(*this)["eretaa"] = &ERETAA;
	(*this)["eretab"] = &ERETAB;
	(*this)["esb"] = &ESB;
	(*this)["ext"] = &EXT;
	(*this)["extr"] = &EXTR;
	(*this)["fabd"] = &FABD;
	(*this)["fabs"] = &FABS;
	(*this)["facge"] = &FACGE;
	(*this)["facgt"] = &FACGT;
	(*this)["fadd"] = &FADD;
	(*this)["faddp"] = &FADDP;
//	(*this)["fcadd"] = &FCADD;
	(*this)["fccmp"] = &FCCMP;
	(*this)["fccmpe"] = &FCCMPE;
	(*this)["fcmeq"] = &FCMEQ;
	(*this)["fcmge"] = &FCMGE;
	(*this)["fcmgt"] = &FCMGT;
//	(*this)["fcmla"] = &FCMLA;
	(*this)["fcmle"] = &FCMLE;
	(*this)["fcmlt"] = &FCMLT;
	(*this)["fcmp"] = &FCMP;
	(*this)["fcmpe"] = &FCMPE;
	(*this)["fcsel"] = &FCSEL;
	(*this)["fcvt"] = &FCVT;
	(*this)["fcvtas"] = &FCVTAS;
	(*this)["fcvtau"] = &FCVTAU;
	(*this)["fcvtl"] = &FCVTL;
	(*this)["fcvtl2"] = &FCVTL2;
	(*this)["fcvtms"] = &FCVTMS;
	(*this)["fcvtmu"] = &FCVTMU;
	(*this)["fcvtn"] = &FCVTN;
	(*this)["fcvtn2"] = &FCVTN2;
	(*this)["fcvtns"] = &FCVTNS;
	(*this)["fcvtnu"] = &FCVTNU;
	(*this)["fcvtps"] = &FCVTPS;
	(*this)["fcvtpu"] = &FCVTPU;
	(*this)["fcvtxn"] = &FCVTXN;
	(*this)["fcvtxn2"] = &FCVTXN2;
	(*this)["fcvtzs"] = &FCVTZS;
	(*this)["fcvtzu"] = &FCVTZU;
	(*this)["fdiv"] = &FDIV;
	(*this)["fjcvtzs"] = &FJCVTZS;
	(*this)["fmadd"] = &FMADD;
	(*this)["fmax"] = &FMAX;
	(*this)["fmaxnm"] = &FMAXNM;
	(*this)["fmaxnmp"] = &FMAXNMP;
	(*this)["fmaxnmv"] = &FMAXNMV;
	(*this)["fmaxp"] = &FMAXP;
	(*this)["fmaxv"] = &FMAXV;
	(*this)["fmin"] = &FMIN;
	(*this)["fminnm"] = &FMINNM;
	(*this)["fminnmp"] = &FMINNMP;
	(*this)["fminnmv"] = &FMINNMV;
	(*this)["fminp"] = &FMINP;
	(*this)["fminv"] = &FMINV;
	(*this)["fmla"] = &FMLA;
	(*this)["fmlal"] = &FMLAL;
	(*this)["fmlal2"] = &FMLAL2;
	(*this)["fmls"] = &FMLS;
	(*this)["fmlsl"] = &FMLSL;
	(*this)["fmlsl2"] = &FMLSL2;
	(*this)["fmov"] = &FMOV;
	(*this)["fmsub"] = &FMSUB;
	(*this)["fmul"] = &FMUL;
	(*this)["fmulx"] = &FMULX;
	(*this)["fneg"] = &FNEG;
	(*this)["fnmadd"] = &FNMADD;
	(*this)["fnmsub"] = &FNMSUB;
	(*this)["fnmul"] = &FNMUL;
	(*this)["frecpe"] = &FRECPE;
	(*this)["frecps"] = &FRECPS;
	(*this)["frecpx"] = &FRECPX;
	(*this)["frinta"] = &FRINTA;
	(*this)["frinti"] = &FRINTI;
	(*this)["frintm"] = &FRINTM;
	(*this)["frintn"] = &FRINTN;
	(*this)["frintp"] = &FRINTP;
	(*this)["frintx"] = &FRINTX;
	(*this)["frintz"] = &FRINTZ;
	(*this)["frsqrte"] = &FRSQRTE;
	(*this)["frsqrts"] = &FRSQRTS;
	(*this)["fsqrt"] = &FSQRT;
	(*this)["fsub"] = &FSUB;
	(*this)["hlt"] = &HLT;
	(*this)["hvc"] = &HVC;
	(*this)["ins"] = &INS;
	(*this)["ld1"] = &LD1;
	(*this)["ld1r"] = &LD1R;
	(*this)["ld2"] = &LD2;
	(*this)["ld2r"] = &LD2R;
	(*this)["ld3"] = &LD3;
	(*this)["ld3r"] = &LD3R;
	(*this)["ld4"] = &LD4;
	(*this)["ld4r"] = &LD4R;
	(*this)["ldadd"] = &LDADD;
	(*this)["ldadda"] = &LDADDA;
	(*this)["ldaddab"] = &LDADDAB;
	(*this)["ldaddal"] = &LDADDAL;
	(*this)["ldaddalb"] = &LDADDALB;
	(*this)["ldaddb"] = &LDADDB;
	(*this)["ldaddl"] = &LDADDL;
	(*this)["ldaddlb"] = &LDADDLB;
	(*this)["ldaddah"] = &LDADDAH;
	(*this)["ldaddalh"] = &LDADDALH;
	(*this)["ldaddh"] = &LDADDH;
	(*this)["ldaddlh"] = &LDADDLH;
	(*this)["ldapr"] = &LDAPR;
	(*this)["ldaprb"] = &LDAPRB;
	(*this)["ldaprh"] = &LDAPRH;
	(*this)["ldapur"] = &LDAPUR;
	(*this)["ldapurb"] = &LDAPURB;
	(*this)["ldapurh"] = &LDAPURH;
	(*this)["ldapursb"] = &LDAPURSB;
	(*this)["ldapursh"] = &LDAPURSH;
	(*this)["ldapursw"] = &LDAPURSW;
	(*this)["ldar"] = &LDAR;
	(*this)["ldarb"] = &LDARB;
	(*this)["ldarh"] = &LDARH;
	(*this)["ldaxp"] = &LDAXP;
	(*this)["ldaxr"] = &LDAXR;
	(*this)["ldaxrb"] = &LDAXRB;
	(*this)["ldaxrh"] = &LDAXRH;
	(*this)["ldclr"] = &LDCLR;
	(*this)["ldclra"] = &LDCLRA;
	(*this)["ldclrab"] = &LDCLRAB;
	(*this)["ldclral"] = &LDCLRAL;
	(*this)["ldclralb"] = &LDCLRALB;
	(*this)["ldclrb"] = &LDCLRB;
	(*this)["ldclrl"] = &LDCLRL;
	(*this)["ldclrlb"] = &LDCLRLB;
	(*this)["ldclrah"] = &LDCLRAH;
	(*this)["ldclralh"] = &LDCLRALH;
	(*this)["ldclrh"] = &LDCLRH;
	(*this)["ldclrlh"] = &LDCLRLH;
	(*this)["ldeor"] = &LDEOR;
	(*this)["ldeora"] = &LDEORA;
	(*this)["ldeorab"] = &LDEORAB;
	(*this)["ldeoral"] = &LDEORAL;
	(*this)["ldeoralb"] = &LDEORALB;
	(*this)["ldeorb"] = &LDEORB;
	(*this)["ldeorl"] = &LDEORL;
	(*this)["ldeorlb"] = &LDEORLB;
	(*this)["ldeorah"] = &LDEORAH;
	(*this)["ldeoralh"] = &LDEORALH;
	(*this)["ldeorh"] = &LDEORH;
	(*this)["ldeorlh"] = &LDEORLH;
	(*this)["ldlarb"] = &LDLARB;
	(*this)["ldlarh"] = &LDLARH;
	(*this)["ldlar"] = &LDLAR;
	(*this)["ldnp"] = &LDNP;
	(*this)["ldp"] = &LDP;
	(*this)["ldpsw"] = &LDPSW;
	(*this)["ldr"] = &LDR;
	(*this)["ldrb"] = &LDRB;
	(*this)["ldrh"] = &LDRH;
	(*this)["ldrsb"] = &LDRSB;
	(*this)["ldrsh"] = &LDRSH;
	(*this)["ldrsw"] = &LDRSW;
	(*this)["ldset"] = &LDSET;
	(*this)["ldseta"] = &LDSETA;
	(*this)["ldsetab"] = &LDSETAB;
	(*this)["ldsetal"] = &LDSETAL;
	(*this)["ldsetalb"] = &LDSETALB;
	(*this)["ldsetb"] = &LDSETB;
	(*this)["ldsetl"] = &LDSETL;
	(*this)["ldsetlb"] = &LDSETLB;
	(*this)["ldsetah"] = &LDSETAH;
	(*this)["ldsetalh"] = &LDSETALH;
	(*this)["ldseth"] = &LDSETH;
	(*this)["ldsetlh"] = &LDSETLH;
	(*this)["ldsmax"] = &LDSMAX;
	(*this)["ldsmaxa"] = &LDSMAXA;
	(*this)["ldsmaxab"] = &LDSMAXAB;
	(*this)["ldsmaxal"] = &LDSMAXAL;
	(*this)["ldsmaxalb"] = &LDSMAXALB;
	(*this)["ldsmaxb"] = &LDSMAXB;
	(*this)["ldsmaxl"] = &LDSMAXL;
	(*this)["ldsmaxlb"] = &LDSMAXLB;
	(*this)["ldsmaxah"] = &LDSMAXAH;
	(*this)["ldsmaxalh"] = &LDSMAXALH;
	(*this)["ldsmaxh"] = &LDSMAXH;
	(*this)["ldsmaxlh"] = &LDSMAXLH;
	(*this)["ldsmin"] = &LDSMIN;
	(*this)["ldsmina"] = &LDSMINA;
	(*this)["ldsminab"] = &LDSMINAB;
	(*this)["ldsminal"] = &LDSMINAL;
	(*this)["ldsminalb"] = &LDSMINALB;
	(*this)["ldsminb"] = &LDSMINB;
	(*this)["ldsminl"] = &LDSMINL;
	(*this)["ldsminlb"] = &LDSMINLB;
	(*this)["ldsminah"] = &LDSMINAH;
	(*this)["ldsminalh"] = &LDSMINALH;
	(*this)["ldsminh"] = &LDSMINH;
	(*this)["ldsminlh"] = &LDSMINLH;
	(*this)["ldtr"] = &LDTR;
	(*this)["ldtrb"] = &LDTRB;
	(*this)["ldtrh"] = &LDTRH;
	(*this)["ldtrsb"] = &LDTRSB;
	(*this)["ldtrsh"] = &LDTRSH;
	(*this)["ldtrsw"] = &LDTRSW;
	(*this)["ldumax"] = &LDUMAX;
	(*this)["ldumaxa"] = &LDUMAXA;
	(*this)["ldumaxab"] = &LDUMAXAB;
	(*this)["ldumaxal"] = &LDUMAXAL;
	(*this)["ldumaxalb"] = &LDUMAXALB;
	(*this)["ldumaxb"] = &LDUMAXB;
	(*this)["ldumaxl"] = &LDUMAXL;
	(*this)["ldumaxlb"] = &LDUMAXLB;
	(*this)["ldumaxah"] = &LDUMAXAH;
	(*this)["ldumaxalh"] = &LDUMAXALH;
	(*this)["ldumaxh"] = &LDUMAXH;
	(*this)["ldumaxlh"] = &LDUMAXLH;
	(*this)["ldumin"] = &LDUMIN;
	(*this)["ldumina"] = &LDUMINA;
	(*this)["lduminab"] = &LDUMINAB;
	(*this)["lduminal"] = &LDUMINAL;
	(*this)["lduminalb"] = &LDUMINALB;
	(*this)["lduminb"] = &LDUMINB;
	(*this)["lduminl"] = &LDUMINL;
	(*this)["lduminlb"] = &LDUMINLB;
	(*this)["lduminah"] = &LDUMINAH;
	(*this)["lduminalh"] = &LDUMINALH;
	(*this)["lduminh"] = &LDUMINH;
	(*this)["lduminlh"] = &LDUMINLH;
	(*this)["ldur"] = &LDUR;
	(*this)["ldurb"] = &LDURB;
	(*this)["ldurh"] = &LDURH;
	(*this)["ldursb"] = &LDURSB;
	(*this)["ldursh"] = &LDURSH;
	(*this)["ldursw"] = &LDURSW;
	(*this)["ldxp"] = &LDXP;
	(*this)["ldxr"] = &LDXR;
	(*this)["ldxrb"] = &LDXRB;
	(*this)["ldxrh"] = &LDXRH;
	(*this)["lsl"] = &LSL;
	(*this)["lslv"] = &LSLV;
	(*this)["lsr"] = &LSR;
	(*this)["lsrv"] = &LSRV;
	(*this)["madd"] = &MADD;
	(*this)["mla"] = &MLA;
	(*this)["mls"] = &MLS;
	(*this)["mneg"] = &MNEG;
	(*this)["mov"] = &MOV;
	(*this)["movi"] = &MOVI;
	(*this)["movk"] = &MOVK;
	(*this)["movn"] = &MOVN;
	(*this)["movz"] = &MOVZ;
//	(*this)["mrs"] = &MRS;
//	(*this)["msr"] = &MSR;
	(*this)["msub"] = &MSUB;
	(*this)["mvn"] = &MVN;
	(*this)["mvni"] = &MVNI;
	(*this)["mul"] = &MUL;
	(*this)["neg"] = &NEG;
	(*this)["negs"] = &NEGS;
	(*this)["ngc"] = &NGC;
	(*this)["ngcs"] = &NGCS;
	(*this)["nop"] = &NOP;
	(*this)["not"] = &NOT;
	(*this)["orn"] = &ORN;
	(*this)["orr"] = &ORR;
	(*this)["pacda"] = &PACDA;
	(*this)["pacdza"] = &PACDZA;
	(*this)["pacdb"] = &PACDB;
	(*this)["pacdzb"] = &PACDZB;
	(*this)["pacga"] = &PACGA;
	(*this)["pacia"] = &PACIA;
	(*this)["pacia1716"] = &PACIA1716;
	(*this)["paciasp"] = &PACIASP;
	(*this)["paciaz"] = &PACIAZ;
	(*this)["paciza"] = &PACIZA;
	(*this)["pacib"] = &PACIB;
	(*this)["pacib1716"] = &PACIB1716;
	(*this)["pacibsp"] = &PACIBSP;
	(*this)["pacibz"] = &PACIBZ;
	(*this)["pacizb"] = &PACIZB;
	(*this)["pmul"] = &PMUL;
	(*this)["pmull"] = &PMULL;
	(*this)["pmull2"] = &PMULL2;
	// prfm*
	// psb csync
	(*this)["pssbb"] = &PSSBB;
	(*this)["raddhn"] = &RADDHN;
	(*this)["raddhn2"] = &RADDHN2;
	(*this)["rax1"] = &RAX1;
	(*this)["rbit"] = &RBIT;
	(*this)["ret"] = &RET;
	(*this)["retaa"] = &RETAA;
	(*this)["retab"] = &RETAB;
	(*this)["rev"] = &REV;
	(*this)["rev16"] = &REV16;
	(*this)["rev32"] = &REV32;
	(*this)["rev64"] = &REV64;
	(*this)["rmif"] = &RMIF;
	(*this)["ror"] = &ROR;
	(*this)["rorv"] = &RORV;
	(*this)["rshrn"] = &RSHRN;
	(*this)["rshrn2"] = &RSHRN2;
	(*this)["rsubhn"] = &RSUBHN;
	(*this)["rsubhn2"] = &RSUBHN2;
	(*this)["saba"] = &SABA;
	(*this)["sabal"] = &SABAL;
	(*this)["sabal2"] = &SABAL2;
	(*this)["sabd"] = &SABD;
	(*this)["sabdl"] = &SABDL;
	(*this)["sabdl2"] = &SABDL2;
	(*this)["sadalp"] = &SADALP;
	(*this)["saddl"] = &SADDL;
	(*this)["saddl2"] = &SADDL2;
	(*this)["saddlp"] = &SADDLP;
	(*this)["saddlv"] = &SADDLV;
	(*this)["saddw"] = &SADDW;
	(*this)["saddw2"] = &SADDW2;
	(*this)["sbc"] = &SBC;
	(*this)["sbcs"] = &SBCS;
	(*this)["sbfiz"] = &SBFIZ;
	(*this)["sbfm"] = &SBFM;
	(*this)["sbfx"] = &SBFX;
	(*this)["scvtf"] = &SCVTF;
	(*this)["sdiv"] = &SDIV;
	(*this)["sdot"] = &SDOT;
	(*this)["setf8"] = &SETF8;
	(*this)["setf16"] = &SETF16;
	(*this)["sev"] = &SEV;
	(*this)["sevl"] = &SEVL;
	(*this)["sha1c"] = &SHA1C;
	(*this)["sha1h"] = &SHA1H;
	(*this)["sha1m"] = &SHA1M;
	(*this)["sha1p"] = &SHA1P;
	(*this)["sha1su0"] = &SHA1SU0;
	(*this)["sha1su1"] = &SHA1SU1;
	(*this)["sha256h"] = &SHA256H;
	(*this)["sha256h2"] = &SHA256H2;
	(*this)["sha256su0"] = &SHA256SU0;
	(*this)["sha256su1"] = &SHA256SU1;
	(*this)["sha512h"] = &SHA512H;
	(*this)["sha512h2"] = &SHA512H2;
	(*this)["sha512su0"] = &SHA512SU0;
	(*this)["sha512su1"] = &SHA512SU1;
	(*this)["shadd"] = &SHADD;
	(*this)["shl"] = &SHL;
	(*this)["shll"] = &SHLL;
	(*this)["shll2"] = &SHLL2;
	(*this)["shrn"] = &SHRN;
	(*this)["shrn2"] = &SHRN2;
	(*this)["shsub"] = &SHSUB;
	(*this)["sli"] = &SLI;
	(*this)["sm3partw1"] = &SM3PARTW1;
	(*this)["sm3partw2"] = &SM3PARTW2;
	(*this)["sm3ss1"] = &SM3SS1;
	(*this)["sm3tt1a"] = &SM3TT1A;
	(*this)["sm3tt1b"] = &SM3TT1B;
	(*this)["sm3tt2a"] = &SM3TT2A;
	(*this)["sm3tt2b"] = &SM3TT2B;
	(*this)["sm4e"] = &SM4E;
	(*this)["sm4ekey"] = &SM4EKEY;
	(*this)["smaddl"] = &SMADDL;
	(*this)["smax"] = &SMAX;
	(*this)["smaxp"] = &SMAXP;
	(*this)["smaxv"] = &SMAXV;
	(*this)["smin"] = &SMIN;
	(*this)["sminp"] = &SMINP;
	(*this)["sminv"] = &SMINV;
	(*this)["smlal"] = &SMLAL;
	(*this)["smlal2"] = &SMLAL2;
	(*this)["smlsl"] = &SMLSL;
	(*this)["smlsl2"] = &SMLSL2;
	(*this)["smc"] = &SMC;
	(*this)["smnegl"] = &SMNEGL;
	(*this)["smov"] = &SMOV;
	(*this)["smsubl"] = &SMSUBL;
	(*this)["smulh"] = &SMULH;
	(*this)["smull"] = &SMULL;
	(*this)["smull2"] = &SMULL2;
	(*this)["sqabs"] = &SQABS;
	(*this)["sqadd"] = &SQADD;
	(*this)["sqdmlal"] = &SQDMLAL;
	(*this)["sqdmlal2"] = &SQDMLAL2;
	(*this)["sqdmlsl"] = &SQDMLSL;
	(*this)["sqdmlsl2"] = &SQDMLSL2;
	(*this)["sqdmulh"] = &SQDMULH;
	(*this)["sqdmull"] = &SQDMULL;
	(*this)["sqdmull2"] = &SQDMULL2;
	(*this)["sqneg"] = &SQNEG;
	(*this)["sqrdmlah"] = &SQRDMLAH;
	(*this)["sqrdmlsh"] = &SQRDMLSH;
	(*this)["sqrdmulh"] = &SQRDMULH;
	(*this)["sqrshl"] = &SQRSHL;
	(*this)["sqrshrn"] = &SQRSHRN;
	(*this)["sqrshrn2"] = &SQRSHRN2;
	(*this)["sqrshrun"] = &SQRSHRUN;
	(*this)["sqrshrun2"] = &SQRSHRUN2;
	(*this)["sqshl"] = &SQSHL;
	(*this)["sqshlu"] = &SQSHLU;
	(*this)["sqshrn"] = &SQSHRN;
	(*this)["sqshrn2"] = &SQSHRN2;
	(*this)["sqshrun"] = &SQSHRUN;
	(*this)["sqshrun2"] = &SQSHRUN2;
	(*this)["sqsub"] = &SQSUB;
	(*this)["sqxtn"] = &SQXTN;
	(*this)["sqxtn2"] = &SQXTN2;
	(*this)["sqxtun"] = &SQXTUN;
	(*this)["sqxtun2"] = &SQXTUN2;
	(*this)["srhadd"] = &SRHADD;
	(*this)["sri"] = &SRI;
	(*this)["srshl"] = &SRSHL;
	(*this)["srshr"] = &SRSHR;
	(*this)["srsra"] = &SRSRA;
	(*this)["ssbb"] = &SSBB;
	(*this)["sshl"] = &SSHL;
	(*this)["sshll"] = &SSHLL;
	(*this)["sshll2"] = &SSHLL2;
	(*this)["sshr"] = &SSHR;
	(*this)["ssra"] = &SSRA;
	(*this)["ssubl"] = &SSUBL;
	(*this)["ssubl2"] = &SSUBL2;
	(*this)["ssubw"] = &SSUBW;
	(*this)["ssubw2"] = &SSUBW2;
	(*this)["st1"] = &ST1;
	(*this)["st2"] = &ST2;
	(*this)["st3"] = &ST3;
	(*this)["st4"] = &ST4;
	(*this)["stadd"] = &STADD;
	(*this)["staddb"] = &STADDB;
	(*this)["staddl"] = &STADDL;
	(*this)["staddlb"] = &STADDLB;
	(*this)["staddh"] = &STADDH;
	(*this)["staddlh"] = &STADDLH;
	(*this)["stclr"] = &STCLR;
	(*this)["stclrb"] = &STCLRB;
	(*this)["stclrl"] = &STCLRL;
	(*this)["stclrlb"] = &STCLRLB;
	(*this)["stclrh"] = &STCLRH;
	(*this)["stclrlh"] = &STCLRLH;
	(*this)["steor"] = &STEOR;
	(*this)["steorb"] = &STEORB;
	(*this)["steorl"] = &STEORL;
	(*this)["steorlb"] = &STEORLB;
	(*this)["steorh"] = &STEORH;
	(*this)["steorlh"] = &STEORLH;
	(*this)["stllr"] = &STLLR;
	(*this)["stllrb"] = &STLLRB;
	(*this)["stllrh"] = &STLLRH;
	(*this)["stlr"] = &STLR;
	(*this)["stlrb"] = &STLRB;
	(*this)["stlrh"] = &STLRH;
	(*this)["stlur"] = &STLUR;
	(*this)["stlurb"] = &STLURB;
	(*this)["stlurh"] = &STLURH;
	(*this)["stlxp"] = &STLXP;
	(*this)["stlxr"] = &STLXR;
	(*this)["stlxrb"] = &STLXRB;
	(*this)["stlxrh"] = &STLXRH;
	(*this)["stnp"] = &STNP;
	(*this)["stp"] = &STP;
	(*this)["str"] = &STR;
	(*this)["strb"] = &STRB;
	(*this)["strh"] = &STRH;
	(*this)["stset"] = &STSET;
	(*this)["stsetb"] = &STSETB;
	(*this)["stsetl"] = &STSETL;
	(*this)["stsetlb"] = &STSETLB;
	(*this)["stseth"] = &STSETH;
	(*this)["stsetlh"] = &STSETLH;
	(*this)["stsmax"] = &STSMAX;
	(*this)["stsmaxb"] = &STSMAXB;
	(*this)["stsmaxl"] = &STSMAXL;
	(*this)["stsmaxlb"] = &STSMAXLB;
	(*this)["stsmaxh"] = &STSMAXH;
	(*this)["stsmaxlh"] = &STSMAXLH;
	(*this)["stsmin"] = &STSMIN;
	(*this)["stsminb"] = &STSMINB;
	(*this)["stsminl"] = &STSMINL;
	(*this)["stsminlb"] = &STSMINLB;
	(*this)["stsminh"] = &STSMINH;
	(*this)["stsminlh"] = &STSMINLH;
	(*this)["sttr"] = &STTR;
	(*this)["sttrb"] = &STTRB;
	(*this)["sttrh"] = &STTRH;
	(*this)["stumax"] = &STUMAX;
	(*this)["stumaxb"] = &STUMAXB;
	(*this)["stumaxl"] = &STUMAXL;
	(*this)["stumaxlb"] = &STUMAXLB;
	(*this)["stumaxh"] = &STUMAXH;
	(*this)["stumaxlh"] = &STUMAXLH;
	(*this)["stumin"] = &STUMIN;
	(*this)["stuminb"] = &STUMINB;
	(*this)["stuminl"] = &STUMINL;
	(*this)["stuminlb"] = &STUMINLB;
	(*this)["stuminh"] = &STUMINH;
	(*this)["stuminlh"] = &STUMINLH;
	(*this)["stur"] = &STUR;
	(*this)["sturb"] = &STURB;
	(*this)["sturh"] = &STURH;
	(*this)["stxp"] = &STXP;
	(*this)["stxr"] = &STXR;
	(*this)["stxrb"] = &STXRB;
	(*this)["stxrh"] = &STXRH;
	(*this)["sub"] = &SUB;
	(*this)["subhn"] = &SUBHN;
	(*this)["subhn2"] = &SUBHN2;
	(*this)["subs"] = &SUBS;
	(*this)["suqadd"] = &SUQADD;
	(*this)["svc"] = &SVC;
	(*this)["swp"] = &SWP;
	(*this)["swpa"] = &SWPA;
	(*this)["swpab"] = &SWPAB;
	(*this)["swpal"] = &SWPAL;
	(*this)["swpalb"] = &SWPALB;
	(*this)["swpb"] = &SWPB;
	(*this)["swpl"] = &SWPL;
	(*this)["swplb"] = &SWPLB;
	(*this)["swpah"] = &SWPAH;
	(*this)["swpalh"] = &SWPALH;
	(*this)["swph"] = &SWPH;
	(*this)["swplh"] = &SWPLH;
	(*this)["sxtb"] = &SXTB;
	(*this)["sxth"] = &SXTH;
	(*this)["sxtl"] = &SXTL;
	(*this)["sxtl2"] = &SXTL2;
	(*this)["sxtw"] = &SXTW;
	// sys
	// sysl
	(*this)["tbl"] = &TBL;
	(*this)["tbnz"] = &TBNZ;
	(*this)["tbx"] = &TBX;
	(*this)["tbz"] = &TBZ;
	// tlbi
	(*this)["trn1"] = &TRN1;
	(*this)["trn2"] = &TRN2;
	// tsb csync
	(*this)["tst"] = &TST;
	(*this)["uaba"] = &UABA;
	(*this)["uabal"] = &UABAL;
	(*this)["uabal2"] = &UABAL2;
	(*this)["uabd"] = &UABD;
	(*this)["uabdl"] = &UABDL;
	(*this)["uabdl2"] = &UABDL2;
	(*this)["uadalp"] = &UADALP;
	(*this)["uaddl"] = &UADDL;
	(*this)["uaddl2"] = &UADDL2;
	(*this)["uaddlp"] = &UADDLP;
	(*this)["uaddlv"] = &UADDLV;
	(*this)["uaddw"] = &UADDW;
	(*this)["uaddw2"] = &UADDW2;
	(*this)["ubfiz"] = &UBFIZ;
	(*this)["ubfm"] = &UBFM;
	(*this)["ubfx"] = &UBFX;
	(*this)["ucvtf"] = &UCVTF;
	(*this)["udiv"] = &UDIV;
	(*this)["udot"] = &UDOT;
	(*this)["uhadd"] = &UHADD;
	(*this)["uhsub"] = &UHSUB;
	(*this)["umaddl"] = &UMADDL;
	(*this)["umax"] = &UMAX;
	(*this)["umaxp"] = &UMAXP;
	(*this)["umaxv"] = &UMAXV;
	(*this)["umin"] = &UMIN;
	(*this)["uminp"] = &UMINP;
	(*this)["uminv"] = &UMINV;
	(*this)["umlal"] = &UMLAL;
	(*this)["umlal2"] = &UMLAL2;
	(*this)["umlsl"] = &UMLSL;
	(*this)["umlsl2"] = &UMLSL2;
	(*this)["umnegl"] = &UMNEGL;
	(*this)["umov"] = &UMOV;
	(*this)["umsubl"] = &UMSUBL;
	(*this)["umulh"] = &UMULH;
	(*this)["umull"] = &UMULL;
	(*this)["umull2"] = &UMULL2;
	(*this)["uqadd"] = &UQADD;
	(*this)["uqrshl"] = &UQRSHL;
	(*this)["uqrshrn"] = &UQRSHRN;
	(*this)["uqrshrn2"] = &UQRSHRN2;
	(*this)["uqshl"] = &UQSHL;
	(*this)["uqshrn"] = &UQSHRN;
	(*this)["uqshrn2"] = &UQSHRN2;
	(*this)["uqsub"] = &UQSUB;
	(*this)["uqxtn"] = &UQXTN;
	(*this)["uqxtn2"] = &UQXTN2;
	(*this)["urecpe"] = &URECPE;
	(*this)["urhadd"] = &URHADD;
	(*this)["urshl"] = &URSHL;
	(*this)["urshr"] = &URSHR;
	(*this)["ursqrte"] = &URSQRTE;
	(*this)["ursra"] = &URSRA;
	(*this)["ushl"] = &USHL;
	(*this)["ushll"] = &USHLL;
	(*this)["ushll2"] = &USHLL2;
	(*this)["ushr"] = &USHR;
	(*this)["usqadd"] = &USQADD;
	(*this)["usra"] = &USRA;
	(*this)["usubl"] = &USUBL;
	(*this)["usubl2"] = &USUBL2;
	(*this)["usubw"] = &USUBW;
	(*this)["usubw2"] = &USUBW2;
	(*this)["uxtb"] = &UXTB;
	(*this)["uxtl"] = &UXTL;
	(*this)["uxtl2"] = &UXTL2;
	(*this)["uxth"] = &UXTH;
	(*this)["uzp1"] = &UZP1;
	(*this)["uzp2"] = &UZP2;
	(*this)["wfe"] = &WFE;
	(*this)["wfi"] = &WFI;
	(*this)["xar"] = &XAR;
	(*this)["xpacd"] = &XPACD;
	(*this)["xpaci"] = &XPACI;
	(*this)["xpaclri"] = &XPACLRI;
	(*this)["xtn"] = &XTN;
	(*this)["xtn2"] = &XTN2;
	(*this)["yield"] = &YIELD;
	(*this)["zip1"] = &ZIP1;
	(*this)["zip2"] = &ZIP2;
};

//============================================================================
