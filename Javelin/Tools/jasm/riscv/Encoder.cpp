//============================================================================

#include "Javelin/Tools/jasm/riscv/Encoder.h"

#include "Javelin/Assembler/BitUtility.h"
#include "Javelin/Assembler/riscv/ActionType.h"
#include "Javelin/Tools/jasm/riscv/Action.h"
#include "Javelin/Tools/jasm/riscv/Assembler.h"
#include "Javelin/Tools/jasm/riscv/Register.h"
#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/Log.h"

#include <assert.h>
#include <inttypes.h>
#include <string.h>

//============================================================================

using namespace Javelin::Assembler;
using namespace Javelin::Assembler::riscv;
using namespace Javelin::riscvAssembler;

//============================================================================

namespace Javelin::Assembler::riscv::Encoders
{
    // Ops:
    //  0: rd
    //  1: rs1
    //  2: rs2
    //  3: rs3
    //  4: funct3 (Bits 12-14)
    //  5: funct7 (Bits 15-31)
	static void R(Assembler &assembler,
                  ListAction& listAction,
                  const Instruction& instruction,
                  const EncodingVariant &encodingVariant,
                  const Operand *const *operands)
	{
        assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
        assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
        assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Register);
        assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Register);
        assert(operands[4] == nullptr || operands[4]->type == Operand::Type::Number);
        assert(operands[5] == nullptr || operands[5]->type == Operand::Type::Number);

        const RegisterOperand *rd = (const RegisterOperand*) operands[0];
        const RegisterOperand *rs1 = (const RegisterOperand*) operands[1];
        const RegisterOperand *rs2 = (const RegisterOperand*) operands[2];
        const RegisterOperand *rs3 = (const RegisterOperand*) operands[3];
        const ImmediateOperand *funct3 = (const ImmediateOperand*) operands[4];
        const ImmediateOperand *funct7 = (const ImmediateOperand*) operands[5];

        uint32_t opcode = encodingVariant.opcode;
        
        if(rd && !rd->IsExpression()) opcode |= rd->index << 7;
        if(rs1 && !rs1->IsExpression()) opcode |= rs1->index << 15;
        if(rs2 && !rs2->IsExpression()) opcode |= rs2->index << 20;
        if(rs3 && !rs3->IsExpression()) opcode |= rs3->index << 27;
        if(funct3 && !funct3->IsExpression()) opcode |= funct3->value << 12;
        if(funct7 && !funct7->IsExpression()) opcode |= funct7->value << 25;

        listAction.AppendOpcode(opcode);

        if(rd && rd->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 7, 0, rd->expressionIndex, assembler));
        }
        if(rs1 && rs1->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 15, 0, rs1->expressionIndex, assembler));
        }
        if(rs2 && rs2->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 20, 0, rs2->expressionIndex, assembler));
        }
        if(rs3 && rs3->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 27, 0, rs3->expressionIndex, assembler));
        }
        if(funct3 && funct3->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 3, 12, 0, funct3->expressionIndex, assembler));
        }
        if(funct7 && funct7->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 7, 25, 0, funct7->expressionIndex, assembler));
        }
	}

    static void I(Assembler &assembler,
                  ListAction& listAction,
                  const Instruction& instruction,
                  const EncodingVariant &encodingVariant,
                  const Operand *const *operands)
    {
        assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
        assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
        assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Number);
        
        const RegisterOperand *rd = (const RegisterOperand*) operands[0];
        const RegisterOperand *rs1 = (const RegisterOperand*) operands[1];
        const ImmediateOperand *i = (const ImmediateOperand*) operands[2];
        
        uint32_t opcode = encodingVariant.opcode;
        
        if(rd && !rd->IsExpression()) opcode |= rd->index << 7;
        if(rs1 && !rs1->IsExpression()) opcode |= rs1->index << 15;
        if(i && !i->IsExpression()) opcode |= uint32_t(i->value) << 20;
        
        listAction.AppendOpcode(opcode);

        if(rd && rd->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 7, 0, rd->expressionIndex, assembler));
        }
        if(rs1 && rs1->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 15, 0, rs1->expressionIndex, assembler));
        }
        if(i && i->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Signed, 4, 12, 20, 0, i->expressionIndex, assembler));
        }
    }

    static void S(Assembler &assembler,
                  ListAction& listAction,
                  const Instruction& instruction,
                  const EncodingVariant &encodingVariant,
                  const Operand *const *operands)
    {
        assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
        assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
        assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Number);
        
        const RegisterOperand *rs1 = (const RegisterOperand*) operands[0];
        const RegisterOperand *rs2 = (const RegisterOperand*) operands[1];
        const ImmediateOperand *i = (const ImmediateOperand*) operands[2];
        
        uint32_t opcode = encodingVariant.opcode;
        
        if(rs1 && !rs1->IsExpression()) opcode |= rs1->index << 15;
        if(rs2 && !rs2->IsExpression()) opcode |= rs2->index << 20;
        if(i && !i->IsExpression())
        {
            opcode |= (i->value & 0x1f) << 7;
            opcode |= ((i->value & 0xfe0) >> 5) << 25;
        }
        
        listAction.AppendOpcode(opcode);

        if(rs1 && rs1->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 15, 0, rs1->expressionIndex, assembler));
        }
        if(rs2 && rs2->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 20, 0, rs2->expressionIndex, assembler));
        }
        if(i && i->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Masked, 4, 5, 7, 0, i->expressionIndex, assembler));
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Masked, 4, 7, 25, 5, i->expressionIndex, assembler));
        }
    }

    static void J(Assembler &assembler,
                  ListAction& listAction,
                  const Instruction& instruction,
                  const EncodingVariant &encodingVariant,
                  const Operand *const *operands)
    {
        assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
        assert(operands[1] != nullptr);
        assert(operands[1]->type == Operand::Type::Label
               || operands[1]->type == Operand::Type::Number);
        
        const RegisterOperand *rd = (const RegisterOperand*) operands[0];
        
        uint32_t opcode = encodingVariant.opcode;
        
        if(rd && !rd->IsExpression()) opcode |= rd->index << 7;
        
        switch(operands[1]->type) {
        case Operand::Type::Label:
            {
                const LabelOperand *label = (const LabelOperand*) operands[1];
                
                int32_t displacement = int32_t(label->displacement ? label->displacement->value : 0);
                
                assert((displacement & 1) == 0);
                assert(BitUtility::IsValidSignedImmediate(displacement, 20));

                union Opcode
                {
                    uint32_t v;
                    struct
                    {
                        uint32_t opcode : 7;
                        uint32_t rd : 5;
                        uint32_t imm19_12 : 8;
                        uint32_t imm11 : 1;
                        uint32_t imm10_1 : 10;
                        int32_t imm20 : 1;
                    };
                };

                Opcode rel = {};
                rel.imm10_1 = displacement >> 1;
                rel.imm11 = displacement >> 11;
                rel.imm19_12 = displacement >> 12;
                rel.imm20 = displacement >> 20;
                
                opcode |= rel.v;
                
                listAction.AppendOpcode(opcode);
                listAction.Append(label->CreatePatchAction(RelEncoding::JDelta, 4));
                
                if(label->displacement && label->displacement->IsExpression())
                {
                    assert(!"Not implemented yet");
                }
                break;
            }
        case Operand::Type::Number:
            {
                const ImmediateOperand* imm = (const ImmediateOperand*) operands[1];

                listAction.AppendOpcode(opcode);
                listAction.Append(new PatchExpressionAction(RelEncoding::JDelta, 4, imm->expressionIndex, assembler));
            }
            break;
        default:
            assert(!"Internal error");
            break;
        }
        
        if(rd && rd->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 7, 0, rd->expressionIndex, assembler));
        }
    }

    static void B(Assembler &assembler,
                  ListAction& listAction,
                  const Instruction& instruction,
                  const EncodingVariant &encodingVariant,
                  const Operand *const *operands)
    {
        assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
        assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
        assert(operands[2] != nullptr);
        assert(operands[2]->type == Operand::Type::Label
               || operands[2]->type == Operand::Type::Number);
        
        const RegisterOperand *rs1 = (const RegisterOperand*) operands[0];
        const RegisterOperand *rs2 = (const RegisterOperand*) operands[1];
        
        uint32_t opcode = encodingVariant.opcode;
        
        if(rs1 && !rs1->IsExpression()) opcode |= rs1->index << 15;
        if(rs2 && !rs2->IsExpression()) opcode |= rs2->index << 20;
        
        switch(operands[2]->type) {
        case Operand::Type::Label:
            {
                const LabelOperand *label = (const LabelOperand*) operands[2];
                
                int32_t displacement = int32_t(label->displacement ? label->displacement->value : 0);
                
                assert((displacement & 1) == 0);
                assert(BitUtility::IsValidSignedImmediate(displacement, 12));

                union Opcode
                {
                    uint32_t v;
                    struct
                    {
                        uint32_t opcode : 7;
                        uint32_t imm11 : 1;
                        uint32_t imm4_1 : 4;
                        uint32_t funct3 : 3;
                        uint32_t rs1 : 5;
                        uint32_t rs2 : 5;
                        uint32_t imm10_5 : 6;
                        int32_t imm12 : 1;
                    };
                };

                Opcode rel = {};
                rel.imm4_1 = displacement >> 1;
                rel.imm10_5 = displacement >> 5;
                rel.imm11 = displacement >> 11;
                rel.imm12 = displacement >> 12;
                
                opcode |= rel.v;
                
                listAction.AppendOpcode(opcode);
                listAction.Append(label->CreatePatchAction(RelEncoding::BDelta, 4));
                
                if(label->displacement && label->displacement->IsExpression())
                {
                    assert(!"Not implemented yet");
                }
                break;
            }                
        case Operand::Type::Number:
            assert(!"TODO");
            break;
		default:
			assert(!"Internal error");
			break;
        }
        
        if(rs1 && rs1->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 15, 0, rs1->expressionIndex, assembler));
        }
        if(rs2 && rs2->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 20, 0, rs2->expressionIndex, assembler));
        }
    }

    static void U(Assembler &assembler,
                  ListAction& listAction,
                  const Instruction& instruction,
                  const EncodingVariant &encodingVariant,
                  const Operand *const *operands)
    {
        assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
        assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Number);
        
        const RegisterOperand *rd = (const RegisterOperand*) operands[0];
        const ImmediateOperand *i = (const ImmediateOperand*) operands[1];
        
        uint32_t opcode = encodingVariant.opcode;
        
        if(rd && !rd->IsExpression()) opcode |= rd->index << 7;
        if(i && !i->IsExpression()) {
            opcode |= (i->value & 0xfffff) << 12;
        }
        
        listAction.AppendOpcode(opcode);

        if(rd && rd->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 7, 0, rd->expressionIndex, assembler));
        }
        if(i && i->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Signed, 4, 20, 12, 0, i->expressionIndex, assembler));
        }
    }

	static void Fixed(Assembler &assembler,
					  ListAction& listAction,
					  const Instruction& instruction,
					  const EncodingVariant &encodingVariant,
					  const Operand *const *operands)
	{
		listAction.AppendOpcode(encodingVariant.opcode);
	}

    // Ops:
    //  0: Destination
    //  1: csr
    //  2: rs1
    //  3: uimm
	static void Zicsr(Assembler &assembler,
                      ListAction& listAction,
                      const Instruction& instruction,
                      const EncodingVariant &encodingVariant,
                      const Operand *const *operands)
	{
        assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
        assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Number);
        assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Register);
        assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Number);

        const RegisterOperand *rd = (const RegisterOperand*) operands[0];
        const ImmediateOperand *csr = (const ImmediateOperand*) operands[1];
        const RegisterOperand *rs1 = (const RegisterOperand*) operands[2];
        const ImmediateOperand *i = (const ImmediateOperand*) operands[3];
        assert(rs1 == nullptr || i == nullptr);

        uint32_t opcode = encodingVariant.opcode;
        
        if(rd && !rd->IsExpression()) opcode |= rd->index << 7;
        if(csr && !csr->IsExpression()) opcode |= uint32_t(csr->value) << 20;
        if(rs1 && !rs1->IsExpression()) opcode |= rs1->index << 15;
        if(i && !i->IsExpression()) opcode |= uint32_t(i->value & 0x1f) << 15;

        listAction.AppendOpcode(opcode);

        if(rd && rd->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 7, 0, rd->expressionIndex, assembler));
        }
        if(csr && csr->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 12, 20, 0, csr->expressionIndex, assembler));
        }
        if(rs1 && rs1->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 15, 0, rs1->expressionIndex, assembler));
        }
        if(i && i->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 5, 15, 0, i->expressionIndex, assembler));
        }
	}


    static void CR(Assembler &assembler,
                   ListAction& listAction,
                   const Instruction& instruction,
                   const EncodingVariant &encodingVariant,
                   const Operand *const *operands)
    {
        assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
        assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
        
        const RegisterOperand *rdrs1 = (const RegisterOperand*) operands[0];
        const RegisterOperand *rs2 = (const RegisterOperand*) operands[1];
        
        uint32_t opcode = encodingVariant.opcode;
        
        if(rdrs1 && !rdrs1->IsExpression()) opcode |= rdrs1->index << 7;
        if(rs2 && !rs2->IsExpression()) opcode |= rs2->index << 2;
        
        listAction.AppendCOpcode(opcode);

        if(rdrs1 && rdrs1->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 2, 5, 7, 0, rdrs1->expressionIndex, assembler));
        }
        if(rs2 && rs2->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 2, 5, 2, 0, rs2->expressionIndex, assembler));
        }
    }

    // Compressed-I Encoding
    //  0: Register
    //  1: Immediate
    static void CI(Assembler &assembler,
                   ListAction& listAction,
                   const Instruction& instruction,
                   const EncodingVariant &encodingVariant,
                   const Operand *const *operands)
    {
        assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
        assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Number);

        const RegisterOperand *rdrs1 = (const RegisterOperand*) operands[0];
        const ImmediateOperand *i = (const ImmediateOperand*) operands[1];

        uint32_t opcode = encodingVariant.opcode;
        
        if(rdrs1 && !rdrs1->IsExpression()) opcode |= rdrs1->index << 7;
        if(i && !i->IsExpression()) opcode |= (uint32_t(i->value) << 2) & 0x107c;

        listAction.AppendCOpcode(opcode);

        if(rdrs1 && rdrs1->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 2, 5, 7, 0, rdrs1->expressionIndex, assembler));
        }
        if(i && i->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::CIValue, 2, 6, 0, 0, i->expressionIndex, assembler));
        }
    }

    static void CJ(Assembler &assembler,
                  ListAction& listAction,
                  const Instruction& instruction,
                  const EncodingVariant &encodingVariant,
                  const Operand *const *operands)
    {
        assert(operands[0] != nullptr);
        assert(operands[0]->type == Operand::Type::Label
               || operands[0]->type == Operand::Type::Number);
        
        uint32_t opcode = encodingVariant.opcode;
        
        switch(operands[0]->type) {
        case Operand::Type::Label:
            {
                const LabelOperand *label = (const LabelOperand*) operands[0];
                
                int32_t displacement = int32_t(label->displacement ? label->displacement->value : 0);
                
                assert((displacement & 1) == 0);
                assert(BitUtility::IsValidSignedImmediate(displacement, 11));

                union Opcode
                {
                    uint16_t v;
                    struct
                    {
                        uint16_t opcode : 2;
                        uint16_t imm5 : 1;
                        uint16_t imm3_1 : 3;
                        uint16_t imm7 : 1;
                        uint16_t imm6 : 1;
                        uint16_t imm10 : 1;
                        uint16_t imm9_8 : 2;
                        uint16_t imm4 : 1;
                        int16_t imm11 : 1;
                        uint16_t funct3 : 3;
                    };
                };
                static_assert(sizeof(Opcode) == 2, "Opcode should be 2 bytes");

                Opcode rel = {};
                rel.imm3_1 = displacement >> 1;
                rel.imm4 = displacement >> 4;
                rel.imm5 = displacement >> 5;
                rel.imm6 = displacement >> 6;
                rel.imm7 = displacement >> 7;
                rel.imm9_8 = displacement >> 8;
                rel.imm10 = displacement >> 10;
                rel.imm11 = displacement >> 11;
                
                opcode |= rel.v;
                
                listAction.AppendCOpcode(opcode);
                listAction.Append(label->CreatePatchAction(RelEncoding::CJDelta, 2));
                
                if(label->displacement && label->displacement->IsExpression())
                {
                    assert(!"Not implemented yet");
                }
                break;
            }
        case Operand::Type::Number:
            {
                const ImmediateOperand* imm = (const ImmediateOperand*) operands[0];

                listAction.AppendOpcode(opcode);
                listAction.Append(new PatchExpressionAction(RelEncoding::CJDelta, 2, imm->expressionIndex, assembler));
            }
            break;
        default:
            assert(!"Internal error");
            break;
        }
    }

    // CB encoding ops:
    //  0: Compressed register
    //  1: Label/Addres expression
    //  2: Immediate
    static void CB(Assembler &assembler,
                  ListAction& listAction,
                  const Instruction& instruction,
                  const EncodingVariant &encodingVariant,
                  const Operand *const *operands)
    {
        assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
        assert(operands[1] == nullptr || (operands[1]->type == Operand::Type::Label
               || operands[1]->type == Operand::Type::Number));
        assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Number);

        uint32_t opcode = encodingVariant.opcode;
        
        const RegisterOperand *rs1 = (const RegisterOperand*) operands[0];
        const ImmediateOperand *i = (const ImmediateOperand*) operands[2];
        
        if(rs1 && !rs1->IsExpression()) opcode |= (rs1->index & 7) << 7;
        if(i && !i->IsExpression()) opcode |= (uint32_t(i->value) << 2) & 0x107c;

        if (operands[1])
        {
            switch(operands[1]->type) {
            case Operand::Type::Label:
                {
                    const LabelOperand *label = (const LabelOperand*) operands[1];
                    
                    int32_t displacement = int32_t(label->displacement ? label->displacement->value : 0);
                    
                    assert((displacement & 1) == 0);
                    assert(BitUtility::IsValidSignedImmediate(displacement, 8));

                    union Opcode
                    {
                        uint16_t v;
                        struct
                        {
                            uint16_t opcode : 2;
                            uint16_t imm5 : 1;
                            uint16_t imm2_1 : 2;
                            uint16_t imm7_6 : 2;
                            uint16_t rs1 : 3;
                            uint16_t imm4_3 : 2;
                            int16_t imm8 : 1;
                            uint16_t funct3 : 3;
                        };
                    };
                    static_assert(sizeof(Opcode) == 2, "Opcode should be 2 bytes");

                    Opcode rel = {};
                    rel.imm2_1 = displacement >> 1;
                    rel.imm4_3 = displacement >> 3;
                    rel.imm5 = displacement >> 5;
                    rel.imm7_6 = displacement >> 6;
                    rel.imm8 = displacement >> 8;
                    
                    opcode |= rel.v;
                    
                    listAction.AppendCOpcode(opcode);
                    listAction.Append(label->CreatePatchAction(RelEncoding::CBDelta, 2));
                    
                    if(label->displacement && label->displacement->IsExpression())
                    {
                        assert(!"Not implemented yet");
                    }
                    break;
                }
            case Operand::Type::Number:
                {
                    const ImmediateOperand* imm = (const ImmediateOperand*) operands[1];

                    listAction.AppendOpcode(opcode);
                    listAction.Append(new PatchExpressionAction(RelEncoding::CBDelta, 2, imm->expressionIndex, assembler));
                }
                break;
            default:
                assert(!"Internal error");
                break;
            }
        }
        
        if(rs1 && rs1->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Masked, 2, 3, 7, 0, rs1->expressionIndex, assembler));
        }
        if(i && i->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::CIValue, 2, 6, 0, 0, i->expressionIndex, assembler));
        }
    }

    static void CA(Assembler &assembler,
                   ListAction& listAction,
                   const Instruction& instruction,
                   const EncodingVariant &encodingVariant,
                   const Operand *const *operands)
    {
        assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
        assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
        
        const RegisterOperand *rdrs1 = (const RegisterOperand*) operands[0];
        const RegisterOperand *rs2 = (const RegisterOperand*) operands[1];
        
        uint32_t opcode = encodingVariant.opcode;
        
        if(rdrs1 && !rdrs1->IsExpression()) opcode |= (rdrs1->index & 7) << 7;
        if(rs2 && !rs2->IsExpression()) opcode |= (rs2->index & 7) << 2;
        
        listAction.AppendCOpcode(opcode);

        if(rdrs1 && rdrs1->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Masked, 2, 3, 7, 0, rdrs1->expressionIndex, assembler));
        }
        if(rs2 && rs2->IsExpression())
        {
            listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Masked, 2, 3, 2, 0, rs2->expressionIndex, assembler));
        }
    }

	static void CFixed(Assembler &assembler,
					  ListAction& listAction,
					  const Instruction& instruction,
					  const EncodingVariant &encodingVariant,
					  const Operand *const *operands)
	{
		listAction.AppendCOpcode(encodingVariant.opcode);
	}

//============================================================================
} // namespace Encoders
//============================================================================

InstructionEncoder* Encoder::GetFunction() const
{
	static InstructionEncoder *const ENCODERS[] =
	{
		#define TAG(x) &Encoders::x,
		#include "EncoderTags.h"
		#undef TAG
	};
	return ENCODERS[value];
}

//============================================================================
