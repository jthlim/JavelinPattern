//============================================================================

#include "Javelin/Tools/jasm/riscv/Instruction.h"

//============================================================================

using namespace Javelin::Assembler::riscv;

//============================================================================

const InstructionMap InstructionMap::instance;

//============================================================================

#define UNWRAP(...)          __VA_ARGS__
#define NUM_ELEMENTS(x)      (sizeof(x) / sizeof(x[0]))
#define NUM_OPERANDS(...)    (sizeof((MatchBitfield[]){__VA_ARGS__})/sizeof(MatchBitfield))

#define DECLARE_CANDIDATE(operand_list, enc, extensions, op)                 \
    {                                                                        \
        .encoder = Encoder::enc,                                             \
        .operandMatchMasksLength = NUM_OPERANDS(UNWRAP operand_list),        \
        .extensionBitmask = extensions,                                      \
        .opcode = op,                                                        \
        .operandMatchMasks = (const MatchBitfield[]){ UNWRAP operand_list }, \
    }

#define DECLARE_ENCODING_VARIANT(name, operand_list, enc, extensions, op) \
    constexpr EncodingVariant name##_ENCODING_VARIANTS[] =                \
    {                                                                     \
        DECLARE_CANDIDATE(operand_list, enc, extensions, op),             \
    }

#define DECLARE_INSTRUCTION(name) \
    constexpr Instruction name =                                            \
    {                                                                       \
        .encodingVariantLength = NUM_ELEMENTS(name##_ENCODING_VARIANTS),    \
        .encodingVariants = name##_ENCODING_VARIANTS                        \
    }

#define DECLARE_LOAD_INSTRUCTION(name, opcode, extensions) \
    constexpr EncodingVariant name##_ENCODING_VARIANTS[] = \
    {                                                      \
        /* load xn, (xn) */                                \
        DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchParenthesizedRegX|MatchOp1), I, extensions, opcode), \
        /* load xn, offset(xn) */                          \
        DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchImm12|MatchOp2, MatchParenthesizedRegX|MatchOp1), I, extensions, opcode), \
    };                                                     \
    DECLARE_INSTRUCTION(name)

#define DECLARE_STORE_INSTRUCTION(name, opcode, extensions) \
    constexpr EncodingVariant name##_ENCODING_VARIANTS[] =  \
    {                                                       \
        /* store xn, (xn) */                                \
        DECLARE_CANDIDATE((MatchRegX|MatchOp1, MatchComma, MatchParenthesizedRegX|MatchOp0), S, extensions, opcode), \
        /* store xn, offset(xn) */                          \
        DECLARE_CANDIDATE((MatchRegX|MatchOp1, MatchComma, MatchImm12|MatchOp2, MatchParenthesizedRegX|MatchOp0), S, extensions, opcode), \
    };                                                      \
    DECLARE_INSTRUCTION(name)


#define DECLARE_SINGLE_CANDIDATE_INSTRUCTION(name, operand_list, enc, extensions, op) \
    DECLARE_ENCODING_VARIANT(name, operand_list, enc, extensions, op);                \
    DECLARE_INSTRUCTION(name)

#define DECLARE_SINGLE_CANDIDATE_SDQ_INSTRUCTION(name, operand_list, enc, op) \
    DECLARE_SINGLE_CANDIDATE_INSTRUCTION(name##_S, operand_list, enc, ExtensionBitmask::F, op);             \
    DECLARE_SINGLE_CANDIDATE_INSTRUCTION(name##_D, operand_list, enc, ExtensionBitmask::D, (op|0x2000000)); \
    DECLARE_SINGLE_CANDIDATE_INSTRUCTION(name##_Q, operand_list, enc, ExtensionBitmask::Q, (op|0x6000000));

#define DECLARE_FLOAT_INSTRUCTION2(name, extensions, opcode) \
    constexpr EncodingVariant name##_ENCODING_VARIANTS[] = \
    {                                                      \
        /* fsqrt f1, f2 */                              \
        DECLARE_CANDIDATE((MatchRegF|MatchOp0, MatchComma, MatchRegF|MatchOp1), R, extensions, (opcode | 0x00007000)), \
        /* fsqrt f1, f2, rm */                          \
        DECLARE_CANDIDATE((MatchRegF|MatchOp0, MatchComma, MatchRegF|MatchOp1, MatchComma, MatchRoundingMode|MatchOp4), R, extensions, opcode), \
    };                                                     \
    DECLARE_INSTRUCTION(name)

#define DECLARE_SDQ_INSTRUCTION2(name, opcode) \
    DECLARE_FLOAT_INSTRUCTION2(name##_S, ExtensionBitmask::F, opcode);             \
    DECLARE_FLOAT_INSTRUCTION2(name##_D, ExtensionBitmask::D, (opcode|0x2000000)); \
    DECLARE_FLOAT_INSTRUCTION2(name##_Q, ExtensionBitmask::Q, (opcode|0x6000000));

#define DECLARE_FLOAT_INSTRUCTION3(name, extensions, opcode) \
    constexpr EncodingVariant name##_ENCODING_VARIANTS[] = \
    {                                                      \
        /* fadd f1, f1, f2 */                              \
        DECLARE_CANDIDATE((MatchRegF|MatchOp0, MatchComma, MatchRegF|MatchOp1, MatchComma, MatchRegF|MatchOp2), R, extensions, (opcode | 0x00007000)), \
        /* fadd f1, f2, f3, rm */                          \
        DECLARE_CANDIDATE((MatchRegF|MatchOp0, MatchComma, MatchRegF|MatchOp1, MatchComma, MatchRegF|MatchOp2, MatchComma, MatchRoundingMode|MatchOp4), R, extensions, opcode), \
    };                                                     \
    DECLARE_INSTRUCTION(name)

#define DECLARE_SDQ_INSTRUCTION3(name, opcode) \
    DECLARE_FLOAT_INSTRUCTION3(name##_S, ExtensionBitmask::F, opcode);             \
    DECLARE_FLOAT_INSTRUCTION3(name##_D, ExtensionBitmask::D, (opcode|0x2000000)); \
    DECLARE_FLOAT_INSTRUCTION3(name##_Q, ExtensionBitmask::Q, (opcode|0x6000000));

#define DECLARE_FLOAT_INSTRUCTION4(name, extensions, opcode) \
    constexpr EncodingVariant name##_ENCODING_VARIANTS[] = \
    {                                                      \
        /* fmadd f1, f2, f3, f4 */                              \
        DECLARE_CANDIDATE((MatchRegF|MatchOp0, MatchComma, MatchRegF|MatchOp1, MatchComma, MatchRegF|MatchOp2, MatchComma, MatchRegF|MatchOp3), R, extensions, (opcode | 0x00007000)), \
        /* fmadd f1, f2, f3, f4, rm */                          \
        DECLARE_CANDIDATE((MatchRegF|MatchOp0, MatchComma, MatchRegF|MatchOp1, MatchComma, MatchRegF|MatchOp2, MatchComma, MatchRegF|MatchOp3, MatchComma, MatchRoundingMode|MatchOp4), R, extensions, opcode), \
    };                                                     \
    DECLARE_INSTRUCTION(name)

#define DECLARE_SDQ_INSTRUCTION4(name, opcode) \
    DECLARE_FLOAT_INSTRUCTION4(name##_S, ExtensionBitmask::F, opcode);             \
    DECLARE_FLOAT_INSTRUCTION4(name##_D, ExtensionBitmask::D, (opcode|0x2000000)); \
    DECLARE_FLOAT_INSTRUCTION4(name##_Q, ExtensionBitmask::Q, (opcode|0x6000000));

//============================================================================

// RV32I.
constexpr EncodingVariant ADD_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1), CR, ExtensionBitmask::C, 0x9002),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x00000033),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRepeatOp0, MatchComma, MatchRegX|MatchOp1), CR, ExtensionBitmask::C, 0x9002),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRepeatOp0), CR, ExtensionBitmask::C, 0x9002),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x00000033),
};
DECLARE_INSTRUCTION(ADD);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AND, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x00007033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(OR, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x00006033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SLL, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x00001033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SLT, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x00002033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SLTU, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x00003033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SRA, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x40005033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SRL, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x00005033);

constexpr EncodingVariant SUB_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((MatchRegCX|MatchOp0, MatchComma, MatchRegCX|MatchOp1), CA, ExtensionBitmask::C, 0x8c01),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x40000033),
    DECLARE_CANDIDATE((MatchRegCX|MatchOp0, MatchComma, MatchRepeatOp0, MatchComma, MatchRegCX|MatchOp1), CA, ExtensionBitmask::C, 0x8c01),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x40000033),
};
DECLARE_INSTRUCTION(SUB);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(XOR, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x00004033);

constexpr EncodingVariant ADDI_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchImm6|MatchOp1), CI, ExtensionBitmask::C, 0x0001),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRepeatOp0, MatchComma, MatchImm6|MatchOp1), CI, ExtensionBitmask::C, 0x0001),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0|MatchOp1, MatchComma, MatchImm12|MatchOp2), I, ExtensionBitmask::RV32I, 0x00000013),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchImm12|MatchOp2), I, ExtensionBitmask::RV32I, 0x00000013),
};
DECLARE_INSTRUCTION(ADDI);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(XORI, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchImm12|MatchOp2), I, ExtensionBitmask::RV32I, 0x00004013);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(ORI, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchImm12|MatchOp2), I, ExtensionBitmask::RV32I, 0x00006013);

constexpr EncodingVariant ANDI_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((MatchRegCX|MatchOp0, MatchComma, MatchImm6|MatchOp2), CB, ExtensionBitmask::C, 0x8402),
    DECLARE_CANDIDATE((MatchRegCX|MatchOp0, MatchComma, MatchRepeatOp0, MatchComma, MatchImm6|MatchOp2), CB, ExtensionBitmask::C, 0x8402),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0|MatchOp1, MatchComma, MatchImm12|MatchOp2), I, ExtensionBitmask::RV32I, 0x00007013),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchImm12|MatchOp2), I, ExtensionBitmask::RV32I, 0x00007013),
};
DECLARE_INSTRUCTION(ANDI);

constexpr EncodingVariant SLLI_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchCShiftImm|MatchOp1), CI, ExtensionBitmask::C, 0x0002),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRepeatOp0, MatchComma, MatchCShiftImm|MatchOp1), CI, ExtensionBitmask::C, 0x0002),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0|MatchOp1, MatchComma, MatchShiftImm|MatchOp2), I, ExtensionBitmask::RV32I, 0x00001013),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchShiftImm|MatchOp2), I, ExtensionBitmask::RV32I, 0x00001013),
};
DECLARE_INSTRUCTION(SLLI);

constexpr EncodingVariant SRLI_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchCShiftImm|MatchOp2), CB, ExtensionBitmask::C, 0x8001),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRepeatOp0, MatchComma, MatchCShiftImm|MatchOp2), CB, ExtensionBitmask::C, 0x8001),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0|MatchOp1, MatchComma, MatchShiftImm|MatchOp2), I, ExtensionBitmask::RV32I, 0x00005013),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchShiftImm|MatchOp2), I, ExtensionBitmask::RV32I, 0x00005013),
};
DECLARE_INSTRUCTION(SRLI);

constexpr EncodingVariant SRAI_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchCShiftImm|MatchOp2), CB, ExtensionBitmask::C, 0x8401),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRepeatOp0, MatchComma, MatchCShiftImm|MatchOp2), CB, ExtensionBitmask::C, 0x8401),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0|MatchOp1, MatchComma, MatchShiftImm|MatchOp2), I, ExtensionBitmask::RV32I, 0x40005013),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchShiftImm|MatchOp2), I, ExtensionBitmask::RV32I, 0x40005013),
};
DECLARE_INSTRUCTION(SRAI);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SLTI, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchImm12|MatchOp2), I, ExtensionBitmask::RV32I, 0x00002013);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SLTIU, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchImm12|MatchOp2), I, ExtensionBitmask::RV32I, 0x00003013);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SEQZ, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1), I, ExtensionBitmask::RV32I, 0x00103013);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SNEZ, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x00003033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SLTZ, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1), R, ExtensionBitmask::RV32I, 0x00002033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(SGTZ, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x00002033);

constexpr EncodingVariant EBREAK_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((), CFixed, ExtensionBitmask::C, 0x9002),
    DECLARE_CANDIDATE((), Fixed, ExtensionBitmask::RV32I, 0x00100073),
};
DECLARE_INSTRUCTION(EBREAK);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(ECALL, (), Fixed, ExtensionBitmask::RV32I, 0x00000073);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(URET, (), Fixed, ExtensionBitmask::RV32I, 0x00200073);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(WFI, (), Fixed, ExtensionBitmask::RV32I, 0x10500073);

DECLARE_LOAD_INSTRUCTION(LB, 0x00000003, ExtensionBitmask::RV32I);
DECLARE_LOAD_INSTRUCTION(LH, 0x00001003, ExtensionBitmask::RV32I);
DECLARE_LOAD_INSTRUCTION(LW, 0x00002003, ExtensionBitmask::RV32I);
DECLARE_LOAD_INSTRUCTION(LD, 0x00003003, ExtensionBitmask::RV64I);
DECLARE_LOAD_INSTRUCTION(LBU, 0x00004003, ExtensionBitmask::RV32I);
DECLARE_LOAD_INSTRUCTION(LHU, 0x00005003, ExtensionBitmask::RV32I);
DECLARE_LOAD_INSTRUCTION(LWU, 0x00006003, ExtensionBitmask::RV32I);

DECLARE_STORE_INSTRUCTION(SB, 0x00000023, ExtensionBitmask::RV32I);
DECLARE_STORE_INSTRUCTION(SH, 0x00001023, ExtensionBitmask::RV32I);
DECLARE_STORE_INSTRUCTION(SW, 0x00002023, ExtensionBitmask::RV32I);
DECLARE_STORE_INSTRUCTION(SD, 0x00003023, ExtensionBitmask::RV64I);

constexpr EncodingVariant BEQ_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((MatchRegCX|MatchOp0, MatchComma, MatchRegZero, MatchComma, MatchCBRel|MatchOp1), CB, ExtensionBitmask::C, 0xc001),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00000063),
};
DECLARE_INSTRUCTION(BEQ);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BNE, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00001063);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BLT, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00004063);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BGE, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00005063);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BLTU, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00006063);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BGEU, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00007063);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(JAL, (MatchRegX|MatchOp0, MatchComma, MatchJRel|MatchOp1), J, ExtensionBitmask::RV32I, 0x0000006f);

constexpr EncodingVariant JALR_ENCODING_VARIANTS[] =
{
    // c.jalr xs
    DECLARE_CANDIDATE((MatchRegX|MatchOp0), CR, ExtensionBitmask::C, 0x9002),
    // jalr xs
    DECLARE_CANDIDATE((MatchRegX|MatchOp1), I, ExtensionBitmask::RV32I, 0x000000e7),
    // jalr xs, offset
    DECLARE_CANDIDATE((MatchRegX|MatchOp1, MatchComma, MatchImm12|MatchOp2), I, ExtensionBitmask::RV32I, 0x000000e7),
    // jalr xd, xs, offset
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchImm12|MatchOp2), I, ExtensionBitmask::RV32I, 0x00000067),
    // jalr xd, offset(xs)
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchImm12|MatchOp2, MatchParenthesizedRegX|MatchOp1), I, ExtensionBitmask::RV32I, 0x00000067),
};
DECLARE_INSTRUCTION(JALR);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(LUI, (MatchRegX|MatchOp0, MatchComma, MatchImm20|MatchUimm20|MatchOp1), U, ExtensionBitmask::RV32I, 0x00000037);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(AUIPC, (MatchRegX|MatchOp0, MatchComma, MatchImm20|MatchUimm20|MatchOp1), U, ExtensionBitmask::RV32I, 0x00000017);

// RV32I Pseudo instructions.
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(NOP, (), Fixed, ExtensionBitmask::RV32I, 0x00000013);

constexpr EncodingVariant LI_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchImm6|MatchOp1), CI, ExtensionBitmask::C, 0x4001),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchImm12|MatchOp2), I, ExtensionBitmask::RV32I, 0x00000013),
};
DECLARE_INSTRUCTION(LI);

constexpr EncodingVariant MV_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1), CR, ExtensionBitmask::C, 0x8002),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1), I, ExtensionBitmask::RV32I, 0x00000013),
};
DECLARE_INSTRUCTION(MV);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(NEG, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::RV32I, 0x40000033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(NOT, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1), I, ExtensionBitmask::RV32I, 0xfff04013);

constexpr EncodingVariant BEQZ_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((MatchRegCX|MatchOp0, MatchComma, MatchCBRel|MatchOp1), CB, ExtensionBitmask::C, 0xc001),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00000063),
};
DECLARE_INSTRUCTION(BEQZ);

constexpr EncodingVariant BNEZ_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((MatchRegCX|MatchOp0, MatchComma, MatchCBRel|MatchOp1), CB, ExtensionBitmask::C, 0xe001),
    DECLARE_CANDIDATE((MatchRegX|MatchOp0, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00001063),
};
DECLARE_INSTRUCTION(BNEZ);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BLEZ, (MatchRegX|MatchOp1, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00005063);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BGEZ, (MatchRegX|MatchOp0, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00005063);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BLTZ, (MatchRegX|MatchOp0, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00004063);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BGTZ, (MatchRegX|MatchOp1, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00004063);

DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BGT, (MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp0, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00004063);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BLE, (MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp0, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00005063);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BGTU, (MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp0, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00006063);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(BLEU, (MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp0, MatchComma, MatchBRel|MatchOp2), B, ExtensionBitmask::RV32I, 0x00007063);

constexpr EncodingVariant J_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((MatchCJRel|MatchOp0), CJ, ExtensionBitmask::C, 0xa001),
    DECLARE_CANDIDATE((MatchJRel|MatchOp1), J, ExtensionBitmask::RV32I, 0x0000006f),
};
DECLARE_INSTRUCTION(J);

constexpr EncodingVariant RET_ENCODING_VARIANTS[] =
{
    DECLARE_CANDIDATE((), CFixed, ExtensionBitmask::C, 0x8082),
    DECLARE_CANDIDATE((), Fixed, ExtensionBitmask::RV32I, 0x00008067),
};
DECLARE_INSTRUCTION(RET);

// Zifencei Standard Extension
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(FENCE_I, (), Fixed, ExtensionBitmask::Zifenci, 0x100f);

// Zicsr Standard Extension
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CSRRW, (MatchRegX|MatchOp0, MatchComma, MatchUimm12|MatchOp1, MatchComma, MatchRegX|MatchOp2), Zicsr, ExtensionBitmask::Zicsr, 0xc0001073);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CSRRS, (MatchRegX|MatchOp0, MatchComma, MatchUimm12|MatchOp1, MatchComma, MatchRegX|MatchOp2), Zicsr, ExtensionBitmask::Zicsr, 0xc0002073);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CSRRC, (MatchRegX|MatchOp0, MatchComma, MatchUimm12|MatchOp1, MatchComma, MatchRegX|MatchOp2), Zicsr, ExtensionBitmask::Zicsr, 0xc0003073);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CSRRWI, (MatchRegX|MatchOp0, MatchComma, MatchUimm12|MatchOp1, MatchComma, MatchUimm5|MatchOp3), Zicsr, ExtensionBitmask::Zicsr, 0xc0005073);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CSRRSI, (MatchRegX|MatchOp0, MatchComma, MatchUimm12|MatchOp1, MatchComma, MatchUimm5|MatchOp3), Zicsr, ExtensionBitmask::Zicsr, 0xc0006073);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(CSRRCI, (MatchRegX|MatchOp0, MatchComma, MatchUimm12|MatchOp1, MatchComma, MatchUimm5|MatchOp3), Zicsr, ExtensionBitmask::Zicsr, 0xc0007073);

// Base counters and timers
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(RDCYCLE, (MatchRegX|MatchOp0), Zicsr, ExtensionBitmask::Zicsr, 0xc0002073);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(RDCYCLEH, (MatchRegX|MatchOp0), Zicsr, ExtensionBitmask::Zicsr, 0xc8002073);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(RDINSTRET, (MatchRegX|MatchOp0), Zicsr, ExtensionBitmask::Zicsr, 0xc0202073);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(RDINSTRETH, (MatchRegX|MatchOp0), Zicsr, ExtensionBitmask::Zicsr, 0xc8202073);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(RDTIME, (MatchRegX|MatchOp0), Zicsr, ExtensionBitmask::Zicsr, 0xc0102073);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(RDTIMEH, (MatchRegX|MatchOp0), Zicsr, ExtensionBitmask::Zicsr, 0xc8102073);

// M extension
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MUL, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::M, 0x02000033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MULH, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::M, 0x02001033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MULHSU, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::M, 0x02002033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MULHU, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::M, 0x02003033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(DIV, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::M, 0x02004033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(DIVU, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::M, 0x02005033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(REM, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::M, 0x02006033);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(REMU, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::M, 0x02007033);

// RV64M extension
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(MULW, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::M, 0x0200003b);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(DIVW, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::M, 0x0200403b);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(DIVUW, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::M, 0x0200503b);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(REMW, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::M, 0x0200603b);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(REMUW, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1, MatchComma, MatchRegX|MatchOp2), R, ExtensionBitmask::M, 0x0200703b);

// C extension
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_ADD, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1), CR, ExtensionBitmask::C, 0x9002);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_ADDI, (MatchRegX|MatchOp0, MatchComma, MatchImm6|MatchOp1), CI, ExtensionBitmask::C, 0x0001);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_ADDIW, (MatchRegX|MatchOp0, MatchComma, MatchImm6|MatchOp1), CI, ExtensionBitmask::C, 0x2001);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_ADDW, (MatchRegCX|MatchOp0, MatchComma, MatchRegCX|MatchOp1), CA, ExtensionBitmask::C, 0x9c21);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_AND, (MatchRegCX|MatchOp0, MatchComma, MatchRegCX|MatchOp1), CA, ExtensionBitmask::C, 0x8c61);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_ANDI, (MatchRegCX|MatchOp0, MatchComma, MatchImm6|MatchOp2), CB, ExtensionBitmask::C, 0x8402);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_BEQZ, (MatchRegCX|MatchOp0, MatchComma, MatchCBRel|MatchOp1), CB, ExtensionBitmask::C, 0xc001);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_BNEZ, (MatchRegCX|MatchOp0, MatchComma, MatchCBRel|MatchOp1), CB, ExtensionBitmask::C, 0xe001);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_JALR, (MatchRegX|MatchOp0), CR, ExtensionBitmask::C, 0x9002);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_JR, (MatchRegX|MatchOp0), CR, ExtensionBitmask::C, 0x8002);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_EBREAK, (), CFixed, ExtensionBitmask::C, 0x9002);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_LI, (MatchRegX|MatchOp0, MatchComma, MatchImm6|MatchOp1), CI, ExtensionBitmask::C, 0x4001);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_MV, (MatchRegX|MatchOp0, MatchComma, MatchRegX|MatchOp1), CR, ExtensionBitmask::C, 0x8002);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_NOP, (), CFixed, ExtensionBitmask::C, 0x0001);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_OR, (MatchRegCX|MatchOp0, MatchComma, MatchRegCX|MatchOp1), CA, ExtensionBitmask::C, 0x8c41);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_RET, (), CFixed, ExtensionBitmask::C, 0x8082);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_SLLI, (MatchRegX|MatchOp0, MatchComma, MatchCShiftImm|MatchOp1), CI, ExtensionBitmask::C, 0x0002);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_SRAI, (MatchRegX|MatchOp0, MatchComma, MatchCShiftImm|MatchOp2), CB, ExtensionBitmask::C, 0x8401);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_SRLI, (MatchRegX|MatchOp0, MatchComma, MatchCShiftImm|MatchOp2), CB, ExtensionBitmask::C, 0x8001);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_SUB, (MatchRegCX|MatchOp0, MatchComma, MatchRegCX|MatchOp1), CA, ExtensionBitmask::C, 0x8c01);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_SUBW, (MatchRegCX|MatchOp0, MatchComma, MatchRegCX|MatchOp1), CA, ExtensionBitmask::C, 0x9c01);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_J, (MatchCJRel|MatchOp0), CJ, ExtensionBitmask::C, 0xa001);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_JAL, (MatchCJRel|MatchOp0), CJ, ExtensionBitmask::C, 0x2001);
DECLARE_SINGLE_CANDIDATE_INSTRUCTION(C_XOR, (MatchRegCX|MatchOp0, MatchComma, MatchRegCX|MatchOp1), CA, ExtensionBitmask::C, 0x8c21);

// F, D, Q extensions
DECLARE_SDQ_INSTRUCTION4(FMADD, 0x00000043);
DECLARE_SDQ_INSTRUCTION4(FMSUB, 0x00000047);
DECLARE_SDQ_INSTRUCTION4(FNMSUB, 0x0000004b);
DECLARE_SDQ_INSTRUCTION4(FNMADD, 0x0000004f);
DECLARE_SDQ_INSTRUCTION3(FADD, 0x00000053);
DECLARE_SDQ_INSTRUCTION3(FSUB, 0x08000053);
DECLARE_SDQ_INSTRUCTION3(FMUL, 0x10000053);
DECLARE_SDQ_INSTRUCTION3(FDIV, 0x18000053);
DECLARE_SDQ_INSTRUCTION2(FSQRT, 0x98000053);
DECLARE_SINGLE_CANDIDATE_SDQ_INSTRUCTION(FSGNJ, (MatchRegF|MatchOp0, MatchComma, MatchRegF|MatchOp1, MatchComma, MatchRegF|MatchOp2), R, 0x20000053);
DECLARE_SINGLE_CANDIDATE_SDQ_INSTRUCTION(FSGNJN, (MatchRegF|MatchOp0, MatchComma, MatchRegF|MatchOp1, MatchComma, MatchRegF|MatchOp2), R, 0x20001053);
DECLARE_SINGLE_CANDIDATE_SDQ_INSTRUCTION(FSGNJX, (MatchRegF|MatchOp0, MatchComma, MatchRegF|MatchOp1, MatchComma, MatchRegF|MatchOp2), R, 0x20002053);
DECLARE_SINGLE_CANDIDATE_SDQ_INSTRUCTION(FMIN, (MatchRegF|MatchOp0, MatchComma, MatchRegF|MatchOp1, MatchComma, MatchRegF|MatchOp2), R, 0x28000053);
DECLARE_SINGLE_CANDIDATE_SDQ_INSTRUCTION(FMAX, (MatchRegF|MatchOp0, MatchComma, MatchRegF|MatchOp1, MatchComma, MatchRegF|MatchOp2), R, 0x28001053);
DECLARE_SINGLE_CANDIDATE_SDQ_INSTRUCTION(FEQ, (MatchRegX|MatchOp0, MatchComma, MatchRegF|MatchOp1, MatchComma, MatchRegF|MatchOp2), R, 0xa0002053);
DECLARE_SINGLE_CANDIDATE_SDQ_INSTRUCTION(FLT, (MatchRegX|MatchOp0, MatchComma, MatchRegF|MatchOp1, MatchComma, MatchRegF|MatchOp2), R, 0xa0001053);
DECLARE_SINGLE_CANDIDATE_SDQ_INSTRUCTION(FLE, (MatchRegX|MatchOp0, MatchComma, MatchRegF|MatchOp1, MatchComma, MatchRegF|MatchOp2), R, 0xa0000053);
DECLARE_SINGLE_CANDIDATE_SDQ_INSTRUCTION(FCLASS, (MatchRegX|MatchOp0, MatchComma, MatchRegF|MatchOp1), R, 0xe2001053);

//============================================================================

#define REGISTER_SDQ_INSTRUCTION(assembler_name, variable_name) \
    (*this)[assembler_name "s"] = &variable_name##_S; \
    (*this)[assembler_name "d"] = &variable_name##_D; \
    (*this)[assembler_name "q"] = &variable_name##_Q;

//============================================================================

InstructionMap::InstructionMap()
{
    // RVI32
    (*this)["add"] = &ADD;
    (*this)["and"] = &AND;
    (*this)["or"] = &OR;
    (*this)["sll"] = &SLL;
    (*this)["slt"] = &SLT;
    (*this)["sltu"] = &SLTU;
    (*this)["sra"] = &SRA;
    (*this)["srl"] = &SRL;
    (*this)["sub"] = &SUB;
    (*this)["xor"] = &XOR;

    (*this)["addi"] = &ADDI;
    (*this)["xori"] = &XORI;
    (*this)["ori"] = &ORI;
    (*this)["andi"] = &ANDI;
    (*this)["slli"] = &SLLI;
    (*this)["srli"] = &SRLI;
    (*this)["srai"] = &SRAI;
    (*this)["slti"] = &SLTI;
    (*this)["sltiu"] = &SLTIU;

    (*this)["lb"] = &LB;
    (*this)["lh"] = &LH;
    (*this)["lw"] = &LW;
    (*this)["ld"] = &LD;
    (*this)["lbu"] = &LBU;
    (*this)["lhu"] = &LHU;
    (*this)["lwu"] = &LWU;

    (*this)["sb"] = &SB;
    (*this)["sh"] = &SH;
    (*this)["sw"] = &SW;
    (*this)["sd"] = &SD;

    (*this)["beq"] = &BEQ;
    (*this)["bne"] = &BNE;
    (*this)["blt"] = &BLT;
    (*this)["bge"] = &BGE;
    (*this)["bltu"] = &BLTU;
    (*this)["bgeu"] = &BGEU;

    (*this)["jal"] = &JAL;
    (*this)["jalr"] = &JALR;

    (*this)["lui"] = &LUI;
    (*this)["auipc"] = &AUIPC;

    (*this)["ebreak"] = &EBREAK;
    (*this)["ecall"] = &ECALL;
    (*this)["wfi"] = &WFI;
    (*this)["uret"] = &URET;

    (*this)["nop"] = &NOP;
    (*this)["li"] = &LI;
    (*this)["mv"] = &MV;
    (*this)["neg"] = &NEG;
    (*this)["not"] = &NOT;

    (*this)["beqz"] = &BEQZ;
    (*this)["bnez"] = &BNEZ;
    (*this)["blez"] = &BLEZ;
    (*this)["bltz"] = &BLTZ;
    (*this)["bgez"] = &BGEZ;
    (*this)["bgtz"] = &BGTZ;

    (*this)["bgt"] = &BGT;
    (*this)["ble"] = &BLE;
    (*this)["bgtu"] = &BGTU;
    (*this)["bleu"] = &BLEU;

    (*this)["seqz"] = &SEQZ;
    (*this)["snez"] = &SNEZ;
    (*this)["sltz"] = &SLTZ;
    (*this)["sgtz"] = &SGTZ;

    (*this)["j"] = &J;
    (*this)["ret"] = &RET;
    
    // Zifencei Standard Extension
    (*this)["fence.i"] = &FENCE_I;

    // Zicsr standard extension
    (*this)["csrrw"] = &CSRRW;
    (*this)["csrrs"] = &CSRRS;
    (*this)["csrrc"] = &CSRRC;
    (*this)["csrrwi"] = &CSRRWI;
    (*this)["csrrsi"] = &CSRRSI;
    (*this)["csrrci"] = &CSRRCI;
    
    // Counters and timers
    (*this)["rdcycle"] = &RDCYCLE;
    (*this)["rdcycleh"] = &RDCYCLEH;
    (*this)["rdtime"] = &RDTIME;
    (*this)["rdtimeh"] = &RDTIMEH;
    (*this)["rdinstret"] = &RDINSTRET;
    (*this)["rdinstreth"] = &RDINSTRETH;

    // M extension
    (*this)["mul"] = &MUL;
    (*this)["mulh"] = &MULH;
    (*this)["mulhsu"] = &MULHSU;
    (*this)["mulhu"] = &MULHU;
    (*this)["div"] = &DIV;
    (*this)["divu"] = &DIVU;
    (*this)["rem"] = &REM;
    (*this)["remu"] = &REMU;

    // RV64M extension
    (*this)["mulw"] = &MULW;
    (*this)["divw"] = &DIVW;
    (*this)["divuw"] = &DIVUW;
    (*this)["remw"] = &REMW;
    (*this)["remuw"] = &REMUW;

    // C extension
    (*this)["c.add"] = &C_ADD;
    (*this)["c.addi"] = &C_ADDI;
    (*this)["c.addiw"] = &C_ADDIW;
    (*this)["c.addw"] = &C_ADDW;
    (*this)["c.and"] = &C_AND;
    (*this)["c.andi"] = &C_ANDI;
    (*this)["c.beqz"] = &C_BEQZ;
    (*this)["c.bnez"] = &C_BNEZ;
    (*this)["c.ebreak"] = &C_EBREAK;
    (*this)["c.j"] = &C_J;
    (*this)["c.jal"] = &C_JAL;
    (*this)["c.jalr"] = &C_JALR;
    (*this)["c.jr"] = &C_JR;
    (*this)["c.li"] = &C_LI;
    (*this)["c.mv"] = &C_MV;
    (*this)["c.nop"] = &C_NOP;
    (*this)["c.or"] = &C_OR;
    (*this)["c.ret"] = &C_RET;
    (*this)["c.slli"] = &C_SLLI;
    (*this)["c.srli"] = &C_SRLI;
    (*this)["c.srai"] = &C_SRAI;
    (*this)["c.sub"] = &C_SUB;
    (*this)["c.subw"] = &C_SUBW;
    (*this)["c.xor"] = &C_XOR;

    // F, D, Q extension
    REGISTER_SDQ_INSTRUCTION("fmadd.", FMADD);
    REGISTER_SDQ_INSTRUCTION("fmsub.", FMSUB);
    REGISTER_SDQ_INSTRUCTION("fnmsub.", FNMSUB);
    REGISTER_SDQ_INSTRUCTION("fnmadd.", FNMADD);
    REGISTER_SDQ_INSTRUCTION("fadd.", FADD);
    REGISTER_SDQ_INSTRUCTION("fsub.", FSUB);
    REGISTER_SDQ_INSTRUCTION("fmul.", FMUL);
    REGISTER_SDQ_INSTRUCTION("fdiv.", FDIV);
    REGISTER_SDQ_INSTRUCTION("fsqrt.", FSQRT);
    REGISTER_SDQ_INSTRUCTION("fsgnj.", FSGNJ);
    REGISTER_SDQ_INSTRUCTION("fsgnjn.", FSGNJN);
    REGISTER_SDQ_INSTRUCTION("fsgnjx.", FSGNJX);
    REGISTER_SDQ_INSTRUCTION("fmin.", FMIN);
    REGISTER_SDQ_INSTRUCTION("fmax.", FMAX);
    REGISTER_SDQ_INSTRUCTION("feq.", FEQ);
    REGISTER_SDQ_INSTRUCTION("flt.", FLT);
    REGISTER_SDQ_INSTRUCTION("fle.", FLE);
    REGISTER_SDQ_INSTRUCTION("fclass.", FCLASS);
};

//============================================================================
