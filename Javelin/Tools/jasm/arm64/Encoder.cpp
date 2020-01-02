//============================================================================

#include "Javelin/Tools/jasm/arm64/Encoder.h"

#include "Javelin/Assembler/arm64/ActionType.h"
#include "Javelin/Tools/jasm/arm64/Action.h"
#include "Javelin/Tools/jasm/arm64/Assembler.h"
#include "Javelin/Tools/jasm/arm64/Register.h"
#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/Log.h"

#include <assert.h>
#include <inttypes.h>
#include <string.h>

//============================================================================

using namespace Javelin::Assembler;
using namespace Javelin::Assembler::arm64;
using namespace Javelin::arm64Assembler;

//============================================================================

static bool ImmsModifierDependendsOnImmr(int modifier)
{
	switch(modifier)
	{
	case 4:
	case 5:
	case 6:
		return true;
	default:
		return false;
	}
}

static int ImmModifier(int modifier, int x, int immr, int imms)
{
	switch(modifier)
	{
	case 0: return x;
	case 1: return x-1;
	case 2: return -x & 0x1f;
	case 3: return -x & 0x3f;
	case 4: return x + immr - 1;
	case 5: return 31 - immr;
	case 6: return 63 - immr;
	case 7: return 64 - x;
	default: assert(!"Internal error"); return x;
	}
}

static std::string ImmModifierExpression(int modifier,
										 const ImmediateOperand *x,
										 const ImmediateOperand *immr,
										 const ImmediateOperand *imms,
										 const Assembler &assembler)
{
	switch(modifier)
	{
	case 0:
		assert(x->IsExpression());
		return assembler.GetExpression(x->expressionIndex);
	case 1:
		assert(x->IsExpression());
		return "(" + assembler.GetExpression(x->expressionIndex) + ")-1";
	case 2:
		assert(x->IsExpression());
		return "-(" + assembler.GetExpression(x->expressionIndex) + ") & 0x1f";
	case 3:
		assert(x->IsExpression());
		return "-(" + assembler.GetExpression(x->expressionIndex) + ") & 0x3f";
	case 4:
		{
			std::string result;
			if(x->IsExpression()) result = "(" + assembler.GetExpression(x->expressionIndex) + ")";
			else result = "(" + std::to_string(x->value) + ")";

			if(immr->IsExpression()) result += "+(" + assembler.GetExpression(immr->expressionIndex) + ")";
			else result += "+(" + std::to_string(immr->value) + ")";

			result += "-1";
			return result;
		}
	case 5:
		assert(immr->IsExpression());
		return "31-(" + assembler.GetExpression(x->expressionIndex) + ")";
	case 6:
		assert(immr->IsExpression());
		return "63-(" + assembler.GetExpression(x->expressionIndex) + ")";
	case 7:
		assert(x->IsExpression());
		return "64-(" + assembler.GetExpression(x->expressionIndex) + ")";
	default:
		assert(!"Internal error");
		return "";
	}
}

namespace Javelin::Assembler::arm64::Encoders
{
	// Bits 0:3: modifier for imms
	// Bits 4:7: modifier for immr
	//
	// Modifiers:
	//   f(0,x,immr,imms) = x      		# unchanged
	//   f(1,x,immr,imms) = x-1
	//   f(2,x,immr,imms) = -x mod 32
	//   f(3,x,immr,imms) = -x mod 64
	//   f(4,x,immr,imms) = x+immr-1
	//   f(5,x,immr,imms) = 31-immr
	//   f(6,x,immr,imms) = 63-immr
	//   f(7,x,immr,imms) = 64-x
	static void RdRnImmrImms(Assembler &assembler,
							 ListAction& listAction,
							 const Instruction& instruction,
							 const EncodingVariant &encodingVariant,
							 const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Immediate);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Immediate);
		
		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const ImmediateOperand *immr = (const ImmediateOperand*) operands[2];
		const ImmediateOperand *imms = (const ImmediateOperand*) operands[3];
		
		uint32_t opcode = encodingVariant.opcode;
		
		if(rd && !rd->IsExpression()) opcode |= rd->index;
		if(rn && !rn->IsExpression()) opcode |= rn->index << 5;
		
		int immr_value = immr ? int(immr->value) : 0;
		int imms_value = imms ? int(imms->value) : 0;

		if(immr && !immr->IsExpression())
		{
			if(!(0 <= immr->value && immr->value < 64)) throw AssemblerException("immr value %" PRId64 " not in [0,63]", immr->value);
			int v = ImmModifier((encodingVariant.data >> 4) & 0xf, int(immr->value), immr_value, imms_value);
			opcode |= v << 16;
		}
		if(imms && !imms->IsExpression())
		{
			if(!ImmsModifierDependendsOnImmr(encodingVariant.data & 0xf)
			   || (!immr || !immr->IsExpression()))
			{
				if(!(0 <= imms->value && imms->value < 64)) throw AssemblerException("imms value %" PRId64 " not in [0,63]", imms->value);;
				int v = ImmModifier(encodingVariant.data & 0xf, int(imms->value), immr_value, imms_value);
				opcode |= v << 10;
			}
		}
		
		listAction.AppendOpcode(opcode);
		
		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(immr && immr->IsExpression())
		{
			std::string expr = ImmModifierExpression((encodingVariant.data >> 4) & 0xf,
													 immr,
													 immr,
													 imms,
													 assembler);
			int expressionIndex = assembler.AddExpression(expr,
														  8,
														  assembler.GetExpressionSourceLine(immr->expressionIndex),
														  assembler.GetExpressionFileIndex(immr->expressionIndex));
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 6, 16, 0, expressionIndex, assembler));
		}
		if((imms && imms->IsExpression())
		   || (ImmsModifierDependendsOnImmr(encodingVariant.data & 0xf) && immr && immr->IsExpression()))
		{
			std::string expr = ImmModifierExpression(encodingVariant.data & 0xf,
													 imms,
													 immr,
													 imms,
													 assembler);
			int expressionIndex = assembler.AddExpression(expr,
														  8,
														  assembler.GetExpressionSourceLine(imms->IsExpression() ? imms->expressionIndex : immr->expressionIndex),
														  assembler.GetExpressionFileIndex(imms->IsExpression() ? imms->expressionIndex : immr->expressionIndex));
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 6, 10, 0, expressionIndex, assembler));
		}
	}

	static void LogicalImmediate(Assembler &assembler,
								 ListAction& listAction,
								 const Instruction& instruction,
								 const EncodingVariant &encodingVariant,
								 const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Immediate);
		
		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[2];
		
		uint32_t opcode = encodingVariant.opcode;
		if(rd && !rd->IsExpression()) opcode |= rd->index;
		if(rn && !rn->IsExpression()) opcode |= rn->index << 5;
		if(imm && !imm->IsExpression())
		{
			uint64_t value = imm->value;
			if(encodingVariant.data == 32) value |= value << 32;

			BitMaskEncodeResult result = EncodeBitMask(value);
			if(result.size == 0) throw AssemblerException("Unable to encode logical immediate %" PRId64, imm->value);
			
			opcode |= result.rotate << 16;
			if(result.size == 64) opcode |= 1 << 22;
			
			uint32_t imms = ((0x1e << __builtin_ctz(result.size)) + result.length - 1) & 0x3f;
			opcode |= imms << 10;
		}
		
		uint8_t opcodeBytes[4];
		memcpy(opcodeBytes, &opcode, 4);
		listAction.Append(new LiteralAction({opcodeBytes, opcodeBytes+4}));
		
		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			listAction.Append(new PatchLogicalImmediateOpcodeAction(encodingVariant.data, imm->expressionIndex, assembler));
		}
	}
	
	static void RtRnImm9(Assembler &assembler,
						 ListAction& listAction,
						 const Instruction& instruction,
						 const EncodingVariant &encodingVariant,
						 const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Immediate);

		const RegisterOperand *rt = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[2];

		uint32_t opcode = encodingVariant.opcode;
		if(rt && !rt->IsExpression()) opcode |= rt->index;
		if(rn && !rn->IsExpression()) opcode |= rn->index << 5;
		if(imm && !imm->IsExpression())
		{
			int offset = (int) imm->value;
			int scale = encodingVariant.data;
			if(offset % scale != 0) throw AssemblerException("Offset must be multiple of %d", scale);
			int32_t imm9Value = offset / scale;
			if((imm9Value >> 9) != 0 && (imm9Value >> 9) != -1) throw AssemblerException("Constant %d out of imm9 range", offset);
			imm9Value &= 0x1ff;
			opcode |= imm9Value << 12;
		}

		listAction.AppendOpcode(opcode);

		if(rt && rt->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rt->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			int valueShift = __builtin_ctz(encodingVariant.data);
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Signed, 9, 12, valueShift, imm->expressionIndex, assembler));
		}
	}
	
	static void Rel26(Assembler &assembler,
					  ListAction& listAction,
					  const Instruction& instruction,
					  const EncodingVariant &encodingVariant,
					  const Operand *const *operands)
	{
		assert(operands[0]->type == Operand::Type::Label
			   || (operands[0]->type == Operand::Type::Number && operands[0]->IsExpression()));

		uint32_t opcode = encodingVariant.opcode;

		switch(operands[0]->type)
		{
		case Operand::Type::Label:
			{
				const LabelOperand *label = (const LabelOperand*) operands[0];
				
				int displacement = int32_t(label->displacement ? label->displacement->value : 0);
				int32_t rel = displacement / 4;
				opcode |= rel & 0x3ffffff;
				
				listAction.AppendOpcode(opcode);
				listAction.Append(label->CreatePatchAction(RelEncoding::Rel26, 4));
				
				if(label->displacement && label->displacement->IsExpression())
				{
					assert(!"Not implemented yet");
				}
			}
			break;
		case Operand::Type::Number:
			{
				const ImmediateOperand* imm = (const ImmediateOperand*) operands[0];

				listAction.AppendOpcode(opcode);
				listAction.Append(new PatchExpressionAction(RelEncoding::Rel26, 4, imm->expressionIndex, assembler));
			}
			break;
		default:
			assert(!"Internal error");
			break;
		}
	}
	
	static void RtRel19(Assembler &assembler,
						ListAction& listAction,
						const Instruction& instruction,
						const EncodingVariant &encodingVariant,
						const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Label
			   || (operands[1]->type == Operand::Type::Number && operands[1]->IsExpression()));
		
		const RegisterOperand *rt = (const RegisterOperand*) operands[0];
		uint32_t opcode = encodingVariant.opcode;
		if(rt) opcode |= rt->index;

		switch(operands[1]->type)
		{
		case Operand::Type::Label:
			{
				const LabelOperand *label = (const LabelOperand*) operands[1];
		 
				int displacement = int32_t(label->displacement ? label->displacement->value : 0);
				int32_t rel = displacement / 4;
				opcode |= (rel << 5) & 0x7ffff;
				
				listAction.AppendOpcode(opcode);
				listAction.Append(label->CreatePatchAction(RelEncoding::Rel19Offset5, 4));
				
				if(label->displacement && label->displacement->IsExpression())
				{
					throw AssemblerException("Displacement expressions not implemented yet");
				}
			}
			break;
		case Operand::Type::Number:
			{
				const ImmediateOperand* imm = (const ImmediateOperand*) operands[1];
				
				listAction.AppendOpcode(opcode);
				listAction.Append(new PatchExpressionAction(RelEncoding::Rel19Offset5, 4, imm->expressionIndex, assembler));
			}
			break;
		default:
			assert(!"Internal error");
			break;
		}
		
		if(rt && rt->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rt->expressionIndex, assembler));
		}
	}
	
	static void RtImmRel14(Assembler &assembler,
						   ListAction& listAction,
						   const Instruction& instruction,
						   const EncodingVariant &encodingVariant,
						   const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Immediate);
		assert(operands[2]->type == Operand::Type::Label
			   || (operands[2]->type == Operand::Type::Number && operands[2]->IsExpression()));
		
		const RegisterOperand *rt = (const RegisterOperand*) operands[0];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[1];
		
		uint32_t opcode = encodingVariant.opcode;
		if(rt) opcode |= rt->index;
		if(imm && !imm->IsExpression())
		{
			if(imm->value & 32) opcode |= 0x80000000;
			opcode |= (imm->value & 0x1f) << 19;
		}

		switch(operands[2]->type)
		{
		case Operand::Type::Label:
			{
				const LabelOperand *label = (const LabelOperand*) operands[2];
				int displacement = int32_t(label->displacement ? label->displacement->value : 0);
				int32_t rel = displacement / 4;
				opcode |= (rel << 5) & 0x3fff;
				
				listAction.AppendOpcode(opcode);
				
				listAction.Append(label->CreatePatchAction(RelEncoding::Rel14Offset5, 4));
				if(label->displacement && label->displacement->IsExpression())
				{
					throw AssemblerException("Displacement expressions not implemented yet");
				}
			}
			break;
		case Operand::Type::Number:
			{
				const ImmediateOperand* imm = (const ImmediateOperand*) operands[2];
				
				listAction.AppendOpcode(opcode);
				listAction.Append(new PatchExpressionAction(RelEncoding::Rel14Offset5, 4, imm->expressionIndex, assembler));
			}
			break;
		default:
			assert(!"Internal error");
			break;
		}

		if(rt && rt->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rt->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Masked, 5, 19, 0, imm->expressionIndex, assembler));
			if(rt->matchBitfield & MatchReg64)
			{
				listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Masked, 1, 31, 5, imm->expressionIndex, assembler));
			}
		}
	}
	
	/**
	 * encodingVariant.Data
	 *   0: ADR
	 *   1: ADRP
	 */
	static void RdRel21HiLo(Assembler &assembler,
							ListAction& listAction,
							const Instruction& instruction,
							const EncodingVariant &encodingVariant,
							const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Label
			   || (operands[1]->type == Operand::Type::Number && operands[1]->IsExpression()));

		const RegisterOperand *rd = (const RegisterOperand*) operands[0];

		uint32_t opcode = encodingVariant.opcode;
		if(rd) opcode |= rd->index;

		RelEncoding relEncoding = (encodingVariant.data == 1) ? RelEncoding::Adrp : RelEncoding::Rel21HiLo;

		switch(operands[1]->type)
		{
		case Operand::Type::Label:
			{
				const LabelOperand *label = (const LabelOperand*) operands[1];
				int displacement = int32_t(label->displacement ? label->displacement->value : 0);
				int32_t rel = displacement;
				
				if(encodingVariant.data == 1)
				{
					if(rel != 0)
					{
						throw AssemblerException("ADRP does not support displacement offsets");
					}
				}
				else
				{
					opcode |= (rel & 3) << 29;
					opcode |= (rel & 0x1ffffc) << 3;
				}
				
				listAction.AppendOpcode(opcode);
				listAction.Append(label->CreatePatchAction(relEncoding, 4));

				if(label->displacement && label->displacement->IsExpression())
				{
					throw AssemblerException("Displacement expressions not implemented yet");
				}
			}
			break;
		case Operand::Type::Number:
			{
				const ImmediateOperand* imm = (const ImmediateOperand*) operands[1];
				
				listAction.AppendOpcode(opcode);
				listAction.Append(new PatchExpressionAction(relEncoding, 4, imm->expressionIndex, assembler));
			}
			break;
		default:
			assert(!"Internal error");
			break;
		}
		
		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
	}
	
	static void RtRnUImm12(Assembler &assembler,
						   ListAction& listAction,
						   const Instruction& instruction,
						   const EncodingVariant &encodingVariant,
						   const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Immediate);
		
		const RegisterOperand *rt = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[2];
		
		uint32_t opcode = encodingVariant.opcode;
		if(rt && !rt->IsExpression()) opcode |= rt->index;
		if(rn && !rn->IsExpression()) opcode |= rn->index << 5;
		if(imm && !imm->IsExpression())
		{
			int offset = (int) imm->value;
			int scale = encodingVariant.data;
			if(offset % scale != 0) throw AssemblerException("Offset must be multiple of %d", scale);
			uint32_t uimm12Value = offset / scale;
			if((uimm12Value >> 12) != 0) throw AssemblerException("Constant %d out of uimm12 range", offset);
			uimm12Value &= 0xfff;
			opcode |= uimm12Value << 10;
		}

		listAction.AppendOpcode(opcode);

		if(rt && rt->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rt->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			int valueShift = __builtin_ctz(encodingVariant.data);
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 12, 10, valueShift, imm->expressionIndex, assembler));
		}
	}
	
	static void RdRnRmImm6(Assembler &assembler,
						   ListAction& listAction,
						   const Instruction& instruction,
						   const EncodingVariant &encodingVariant,
						   const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Register);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Immediate);

		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const RegisterOperand *rm = (const RegisterOperand*) operands[2];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[3];

		uint32_t opcode = encodingVariant.opcode;

		if(rd && !rd->IsExpression()) opcode |= rd->index;
		if(rn && !rn->IsExpression()) opcode |= rn->index << 5;
		if(rm && !rm->IsExpression()) opcode |= rm->index << 16;
		if(imm && !imm->IsExpression())
		{
			assert(0 <= imm->value && imm->value < 64);
			opcode |= imm->value << 10;
		}

		listAction.AppendOpcode(opcode);

		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(rm && rm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 16, 0, rm->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 6, 10, 0, imm->expressionIndex, assembler));
		}
	}
	
	static void RnImmNzcvCond(Assembler &assembler,
							  ListAction& listAction,
							  const Instruction& instruction,
							  const EncodingVariant &encodingVariant,
							  const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Immediate);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Immediate);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Condition);
		
		const RegisterOperand *rn = (const RegisterOperand*) operands[0];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[1];
		const ImmediateOperand *nzcv = (const ImmediateOperand*) operands[2];
		const Operand *cond = operands[3];
		
		uint32_t opcode = encodingVariant.opcode;
		
		if(rn && !rn->IsExpression()) opcode |= rn->index << 5;
		if(imm && !imm->IsExpression())
		{
			assert(0 <= imm->value && imm->value < 32);
			opcode |= imm->value << 16;
		}
		if(nzcv && !nzcv->IsExpression())
		{
			assert(0 <= nzcv->value && nzcv->value < 16);
			opcode |= nzcv->value;
		}
		if(cond)
		{
			assert(!cond->IsExpression());
			int c = int(cond->condition);
			if(encodingVariant.data & 0x100) c ^= 1;
			opcode |= c << 12;
		}

		listAction.AppendOpcode(opcode);
		
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 16, 0, imm->expressionIndex, assembler));
		}
		if(nzcv && nzcv->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 0, 0, nzcv->expressionIndex, assembler));
		}
	}
	
	/**
	 * RegisterData is taken from Op7
	 * encodingVariant.data bits:
	 *  8: Opcode[22:23] = (registerData >> 4) & 3
	 */
	static void RnRmNzcvCond(Assembler &assembler,
							 ListAction& listAction,
							 const Instruction& instruction,
							 const EncodingVariant &encodingVariant,
							 const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Immediate);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Condition);
		
		const RegisterOperand *rn = (const RegisterOperand*) operands[0];
		const RegisterOperand *rm = (const RegisterOperand*) operands[1];
		const ImmediateOperand *nzcv = (const ImmediateOperand*) operands[2];
		const Operand *cond = operands[3];
		
		uint32_t opcode = encodingVariant.opcode;
		
		if(rn && !rn->IsExpression()) opcode |= rn->index << 5;
		if(rm && !rm->IsExpression()) opcode |= rm->index << 16;
		if(nzcv && !nzcv->IsExpression())
		{
			assert(0 <= nzcv->value && nzcv->value < 16);
			opcode |= nzcv->value;
		}
		if(cond)
		{
			assert(!cond->IsExpression());
			int c = int(cond->condition);
			if(encodingVariant.data & 0x100) c ^= 1;
			opcode |= c << 12;
		}
		
		const RegisterOperand *enc = (const RegisterOperand*) operands[7];
		if(encodingVariant.data & 8)
		{
			assert(enc);
			opcode |= ((enc->registerData >> 4) & 3) << 22;
		}
		
		listAction.AppendOpcode(opcode);
		
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(rm && rm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 16, 0, rm->expressionIndex, assembler));
		}
		if(nzcv && nzcv->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 0, 0, nzcv->expressionIndex, assembler));
		}
	}
	
	static void RdRnRmCond(Assembler &assembler,
						   ListAction& listAction,
						   const Instruction& instruction,
						   const EncodingVariant &encodingVariant,
						   const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Register);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Condition);
		
		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const RegisterOperand *rm = (const RegisterOperand*) operands[2];
		const Operand *cond = operands[3];
		
		uint32_t opcode = encodingVariant.opcode;
		
		if(rd && !rd->IsExpression()) opcode |= rd->index;
		if(rn && !rn->IsExpression()) opcode |= rn->index << 5;
		if(rm && !rm->IsExpression()) opcode |= rm->index << 16;
		if(cond)
		{
			assert(!cond->IsExpression());
			int c = int(cond->condition);
			if(encodingVariant.data & 0x100) c ^= 1;
			opcode |= c << 12;
		}
		
		listAction.AppendOpcode(opcode);
		
		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(rm && rm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 16, 0, rm->expressionIndex, assembler));
		}
	}
	
	static void RdRnRmRa(Assembler &assembler,
						 ListAction& listAction,
						 const Instruction& instruction,
						 const EncodingVariant &encodingVariant,
						 const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Register);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Register);
		
		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const RegisterOperand *rm = (const RegisterOperand*) operands[2];
		const RegisterOperand *ra = (const RegisterOperand*) operands[3];
		
		uint32_t opcode = encodingVariant.opcode;
		
		if(rd && !rd->IsExpression()) opcode |= rd->index;
		if(rn && !rn->IsExpression()) opcode |= rn->index << 5;
		if(rm && !rm->IsExpression()) opcode |= rm->index << 16;
		if(ra && !ra->IsExpression()) opcode |= ra->index << 10;
		
		listAction.AppendOpcode(opcode);

		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(rm && rm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 16, 0, rm->expressionIndex, assembler));
		}
		if(ra && ra->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 10, 0, ra->expressionIndex, assembler));
		}
	}
	
	static void RdHwImm16(Assembler &assembler,
						  ListAction& listAction,
						  const Instruction& instruction,
						  const EncodingVariant &encodingVariant,
						  const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] && operands[1]->type == Operand::Type::Immediate);

		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[1];
		assert((imm->value & 0xffffffffffff0000) == 0
			   || (imm->value & 0xffffffff0000ffff) == 0
			   || (imm->value & 0xffff0000ffffffff) == 0
			   || (imm->value & 0x0000ffffffffffff) == 0);

		uint32_t opcode = encodingVariant.opcode;
		if(rd) opcode |= rd->index;

		uint64_t v = imm->value;
		uint8_t hw = 0;
		while((v >> 16) != 0)
		{
			++hw;
			v >>= 16;
		}
		
		opcode |= v << 5;
		opcode |= hw << 21;
		
		listAction.AppendOpcode(opcode);

		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(imm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 16, 5, 0, imm->expressionIndex, assembler));
		}
	}

	static void RdExpressionImm(Assembler &assembler,
								ListAction& listAction,
								const Instruction& instruction,
								const EncodingVariant &encodingVariant,
								const Operand *const *operands)
	{
		assert(operands[0]->type == Operand::Type::Register);
		assert(operands[1]
			   && operands[1]->type == Operand::Type::Immediate
			   && operands[1]->IsExpression());

		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[1];

		assert(encodingVariant.data == 32 || encodingVariant.data == 64);

		listAction.Append(new MovExpressionImmediateAction(encodingVariant.data, rd->index, imm->expressionIndex, assembler));
		
		if(rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
	}

	// operands[0] = destination
	// operands[1] = imm
	// operands[2] = shift
	static void RdImm16Shift(Assembler &assembler,
							 ListAction& listAction,
							 const Instruction& instruction,
							 const EncodingVariant &encodingVariant,
							 const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Immediate);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Immediate);
		
		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[1];
		const ImmediateOperand *shiftImm = (const ImmediateOperand*) operands[2];
		
		uint32_t opcode = encodingVariant.opcode;
		
		if(rd) opcode |= rd->index;
		if(imm) opcode |= imm->value << 5;
		
		if(shiftImm)
		{
			switch(shiftImm->value)
			{
			case 0: 					break;
			case 16: opcode |= 1 << 21;	break;
			case 32: opcode |= 2 << 21;	break;
			case 48: opcode |= 3 << 21;	break;
			default: throw AssemblerException("Invalid shift value : %" PRId64, shiftImm->value);
			}
		}
		
		listAction.AppendOpcode(opcode);
		
		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 16, 5, 0, imm->expressionIndex, assembler));
		}
		if(shiftImm && shiftImm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 2, 21, 4, shiftImm->expressionIndex, assembler));
		}
	}
	
	static void RdNot32HwImm16(Assembler &assembler,
							   ListAction& listAction,
							   const Instruction& instruction,
							   const EncodingVariant &encodingVariant,
							   const Operand *const *operands)
	{
		assert(operands[0] && operands[0]->type == Operand::Type::Register);
		assert(operands[1] && operands[1]->type == Operand::Type::Immediate);

		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[1];
		assert(int32_t(imm->value | 0xffff) == -1
			   || int32_t(imm->value | 0xffff0000) == -1);
		
		uint32_t opcode = encodingVariant.opcode;
		if(rd) opcode |= rd->index;

		if(imm->IsExpression())
		{
			throw AssemblerException("Unsupported expression");
		}
		
		uint32_t v = ~uint32_t(imm->value);
		uint8_t hw = 0;
		while((v >> 16) != 0)
		{
			++hw;
			v >>= 16;
		}
		
		opcode |= v << 5;
		opcode |= hw << 21;
		
		listAction.AppendOpcode(opcode);
		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
	}

	static void RdNot64HwImm16(Assembler &assembler,
							   ListAction& listAction,
							   const Instruction& instruction,
							   const EncodingVariant &encodingVariant,
							   const Operand *const *operands)
	{
		assert(operands[0] && operands[0]->type == Operand::Type::Register);
		assert(operands[1] && operands[1]->type == Operand::Type::Immediate);
		
		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[1];
		assert((imm->value | 0xffff) == -1
			   || (imm->value | 0xffff0000) == -1
			   || (imm->value | 0xffff00000000) == -1
			   || (imm->value | 0xffff000000000000) == -1);
		
		uint32_t opcode = encodingVariant.opcode;
		if(rd) opcode |= rd->index;

		if(imm->IsExpression())
		{
			throw AssemblerException("Unsupported expression");
		}

		uint64_t v = ~imm->value;
		uint8_t hw = 0;
		while((v >> 16) != 0)
		{
			++hw;
			v >>= 16;
		}
		
		opcode |= v << 5;
		opcode |= hw << 21;
		
		listAction.AppendOpcode(opcode);
		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
	}

	// operands[0] = load/store register
	// operands[1] = base register
	// operands[2] = offset register
	// operands[3] = extend
	// operands[4] = shift immediate
	static void LoadStoreOffsetRegister(Assembler &assembler,
										ListAction& listAction,
										const Instruction& instruction,
										const EncodingVariant &encodingVariant,
										const Operand *const *operands)
	{
		assert(operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Register);
		assert(operands[2]->type == Operand::Type::Register);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Extend ||
			   (operands[3]->type == Operand::Type::Shift && operands[3]->shift == Operand::Shift::LSL));
		assert(operands[4] == nullptr || operands[4]->type == Operand::Type::Immediate);

		const RegisterOperand *rt = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const RegisterOperand *rm = (const RegisterOperand*) operands[2];
		const Operand *extend = operands[3];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[4];
		
		uint32_t opcode = encodingVariant.opcode;
		
		opcode |= rt->index;
		opcode |= rn->index << 5;
		opcode |= rm->index << 16;
		
		if(extend)
		{
			int option = 0;
			switch(extend->type)
			{
			case Operand::Type::Extend:
				option = (int) extend->extend;
				break;
			case Operand::Type::Shift:
				assert(extend->shift == Operand::Shift::LSL);
				option = 3;
				break;
			default:
				assert(!"Unknown extend command");
				break;
			}
			opcode |= option << 13;

			if(imm)
			{
				if(imm->IsExpression())
				{
					throw AssemblerException("Shift expression not permitted for LoadStore");
				}
				
				int shift = 0;
				if(imm->value == encodingVariant.data) shift = 1;
				else if(imm->value == 0) shift = 0;
				else throw AssemblerException("Invalid shift %" PRId64 ". Must be 0 or %d", imm->value, encodingVariant.data);
				opcode |= shift << 12;
			}
		}
		
		listAction.AppendOpcode(opcode);

		if(rt->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rt->expressionIndex, assembler));
		}
		if(rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(rm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 16, 0, rm->expressionIndex, assembler));
		}
	}
	
	// operands[0] = load/store reg1
	// operands[1] = load/store reg2
	// operands[2] = base register
	// operands[3] = offset immediate
	static void LoadStorePair(Assembler &assembler,
							  ListAction& listAction,
							  const Instruction& instruction,
							  const EncodingVariant &encodingVariant,
							  const Operand *const *operands)
	{
		assert(operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Register);
		assert(operands[2]->type == Operand::Type::Register);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Immediate);
		
		const RegisterOperand *rt1 = (const RegisterOperand*) operands[0];
		const RegisterOperand *rt2 = (const RegisterOperand*) operands[1];
		const RegisterOperand *rn = (const RegisterOperand*) operands[2];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[3];
		
		uint32_t opcode = encodingVariant.opcode;
		
		opcode |= rt1->index;
		opcode |= rt2->index << 10;
		opcode |= rn->index << 5;
		
		if(imm)
		{
			assert(imm->value % encodingVariant.data == 0);
			int immValue = int(imm->value / encodingVariant.data);
			assert((immValue >> 7) == 0 || (immValue >> 7) == -1);

			immValue &= 0x7f;
			
			opcode |= immValue << 15;
		}

		listAction.AppendOpcode(opcode);

		if(rt1->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rt1->expressionIndex, assembler));
		}
		if(rt2->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 10, 0, rt2->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			int valueShift = __builtin_ctz(encodingVariant.data);
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Signed, 7, 15, valueShift, imm->expressionIndex, assembler));
		}
	}
	
	// operands[0] = destination
	// operands[1] = opA
	// operands[2] = opB
	// operands[3] = extend
	// operands[4] = shift immediate
	static void ArithmeticExtendedRegister(Assembler &assembler,
										   ListAction& listAction,
										   const Instruction& instruction,
										   const EncodingVariant &encodingVariant,
										   const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Register);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Extend
			   || (operands[3]->type == Operand::Type::Shift && operands[3]->shift == Operand::Shift::LSL));
		assert(operands[4] == nullptr || operands[4]->type == Operand::Type::Immediate);
		
		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const RegisterOperand *rm = (const RegisterOperand*) operands[2];
		const Operand *extend = operands[3];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[4];
		
		uint32_t opcode = encodingVariant.opcode;
		
		if(rd) opcode |= rd->index;
		if(rn) opcode |= rn->index << 5;
		if(rm) opcode |= rm->index << 16;
		
		if(extend)
		{
			int option = 0;
			switch(extend->type)
			{
			case Operand::Type::Extend:
				option = (int) extend->extend;
				break;
			case Operand::Type::Shift:
				assert(extend->shift == Operand::Shift::LSL);
				option = (encodingVariant.operandMatchMasks[0] & MatchReg32) ? 2 : 3;
				break;
			default:
				assert(!"Unknown extend command");
				break;
			}
			opcode |= option << 13;
			
			if(imm)
			{
				assert(0 <= imm->value && imm->value <= 4);
				opcode |= imm->value << 10;
			}
		}
		
		listAction.AppendOpcode(opcode);

		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(rm && rm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 16, 0, rm->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 6, 10, 0, imm->expressionIndex, assembler));
		}
	}
	
	// operands[0] = destination
	// operands[1] = opA
	// operands[2] = immediate
	// operands[3] = shift immediate
	static void ArithmeticImmediate(Assembler &assembler,
									ListAction& listAction,
									const Instruction& instruction,
									const EncodingVariant &encodingVariant,
									const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2]->type == Operand::Type::Immediate);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Immediate);
		
		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[2];
		const ImmediateOperand *shiftImm = (const ImmediateOperand*) operands[3];
		
		uint32_t opcode = encodingVariant.opcode;
		
		if(rd) opcode |= rd->index;
		if(rn) opcode |= rn->index << 5;
		
		if((imm->value & ~0xfff000ull) != 0
		   && (imm->value & ~0xfffull) != 0)
		{
			throw AssemblerException("Value %" PRId64 " not permissible for instruction", imm->value);
		}

		if((imm->value >> 12) != 0)
		{
			opcode |= imm->value >> 2;
			opcode |= (1 << 22);
		}
		else
		{
			opcode |= imm->value << 10;
		}
		
		
		if(shiftImm)
		{
			if(shiftImm->IsExpression())
			{
				throw AssemblerException("Shift expressions not supported");
			}
			if(shiftImm->value != 0 && shiftImm->value != 12)
			{
				throw AssemblerException("Shift (%" PRId64 ") must be #0 or #12", shiftImm->value);
			}
			
			if(shiftImm->value == 12) opcode |= 1 << 22;
		}

		listAction.AppendOpcode(opcode);

		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 12, 10, 0, imm->expressionIndex, assembler));
		}
	}
	
	// operands[0] = destination
	// operands[1] = opA
	// operands[2] = opB
	// operands[3] = shift
	// operands[4] = immediate
	static void ArithmeticShiftedRegister(Assembler &assembler,
										  ListAction& listAction,
										  const Instruction& instruction,
										  const EncodingVariant &encodingVariant,
										  const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Register);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Shift);
		assert(operands[4] == nullptr || operands[4]->type == Operand::Type::Immediate);
		
		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const RegisterOperand *rm = (const RegisterOperand*) operands[2];
		const Operand *shift = operands[3];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[4];
		
		uint32_t opcode = encodingVariant.opcode;
		
		if(rd) opcode |= rd->index;
		if(rn) opcode |= rn->index << 5;
		if(rm) opcode |= rm->index << 16;
		
		if(shift)
		{
			int option = (int) shift->shift;
			opcode |= option << 22;
			
			if(imm)
			{
				if(0 <= imm->value && imm->value <= 63)
				{
					opcode |= imm->value << 10;
				}
				else
				{
					throw AssemblerException("Invalid shift value: %" PRId64, imm->value);
				}
			}
		}

		listAction.AppendOpcode(opcode);

		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(rm && rm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 16, 0, rm->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 6, 10, 0, imm->expressionIndex, assembler));
		}
	}

	/**
	 * RegisterData is taken from Op7
	 * encodingVariant.data bits:
	 *      1: Opcode[23:22] = registerData & 3
	 *      2: Opcode[30] = (registerData & 4) != 0
	 *      4: Opcode[22] = (registerData >> 4) & 1
	 *      8: Opcode[23:22] = (registerData >> 4) & 3
	 *   0x10: Opcode[11:10] = registerData & 3
	 *   0x20: Immediate must be count << (registerData & 3). For LD#R instructions
	 *   0x40: Opcode[30, 15:14, 12:10] encoded as required for LD/STn (single structure)
	 *   0x80: Immediate must be n * registerSize. For LD#R instructions
	 *  0x100: Immediate is encoded in Opcode[14:11], used for EXT instruction
	 *  0x200: Immediate is used to encode Opcode[22:16], used for right shift instructions (SRSHR, SRSRA, SSHR, etc)
	 *  0x400: Immediate is used to encode Opcode[22:16], used for left shift instructions (SHL, SHLL, SSHL, etc)
	 *  0x800: Rm is verified to be 0..15, and index is encoded in Opcode[11,21:20]
	 * 0x1000: Immediate is encoded in Opcode[15:10] as 64-imm. Used for SCVTF, UCVTF
	 */
	static void FpRdRnRmRa(Assembler &assembler,
						   ListAction& listAction,
						   const Instruction& instruction,
						   const EncodingVariant &encodingVariant,
						   const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Register);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Register);
		assert(operands[4] == nullptr || operands[4]->type == Operand::Type::Immediate || operands[4]->type == Operand::Type::Number);
		assert(operands[5] == nullptr || operands[5]->type == Operand::Type::Number);

		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const RegisterOperand *rm = (const RegisterOperand*) operands[2];
		const RegisterOperand *ra = (const RegisterOperand*) operands[3];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[4];
		const ImmediateOperand *index = (const ImmediateOperand*) operands[5];

		uint32_t opcode = encodingVariant.opcode;
		const RegisterOperand *enc = (const RegisterOperand*) operands[7];
		if(encodingVariant.data & 1)
		{
			assert(enc);
			opcode |= ((enc->registerData & 3) << 22);
		}
		if(encodingVariant.data & 2)
		{
			assert(enc);
			opcode |= (enc->registerData & 4) << 28;
		}
		if(encodingVariant.data & 4)
		{
			assert(enc);
			opcode |= ((enc->registerData >> 4) & 1) << 22;
		}
		if(encodingVariant.data & 8)
		{
			assert(enc);
			opcode |= ((enc->registerData >> 4) & 3) << 22;
		}
		if(encodingVariant.data & 0x10)
		{
			assert(enc);
			opcode |= ((enc->registerData & 3) << 10);
		}
		if(encodingVariant.data & 0x20)
		{
			assert(enc);

			if(!imm) throw AssemblerException("Internal error: Immediate expected for encodingVariant.data & 0x20");
			if(imm->IsExpression()) throw AssemblerException("Expressions not available for fixed-immediate post indexing");
			
			int count = encodingVariant.GetNP1Count() + 1;

			int validImmediate = count << (enc->registerData & 3);
			if(imm->value != validImmediate) throw AssemblerException("Specified post index %" PRId64 " cannot be encoded. Must be %d.", imm->value, validImmediate);
			
			opcode |= 0x1f0000;
		}
		if(encodingVariant.data & 0x40)
		{
			assert(enc);
			if(!index) throw AssemblerException("Internal error: Index expected for encodingVariant.data & 0x40");

			int registerSize = (enc->registerData & 3); // 0 = B, 1 = H, 2 = S, 3 = D
			
			switch(registerSize)
			{
			case 0: break;
			case 1: opcode |= 0x4000; break;
			case 2: opcode |= 0x8000; break;
			case 3: opcode |= 0x8400; break;
			}
			
			int i = int(index->value) << registerSize;
			if(i >= 16) throw AssemblerException("Index %" PRId64 " out of range", index->value);
			
			if(i & 8) opcode |= 1 << 30;
			opcode |= (i & 7) << 10;
		}
		if(encodingVariant.data & 0x80)
		{
			assert(enc);
			
			if(!imm) throw AssemblerException("Internal error: Immediate expected for encodingVariant.data & 0x80");
			if(imm->IsExpression()) throw AssemblerException("Expressions not available for fixed-immediate post indexing");
			
			int count = encodingVariant.GetNP1Count() + 1;
			int registerSize = (enc->registerData & 4) ? 16 : 8;
			
			int validImmediate = count * registerSize;
			if(imm->value != validImmediate) throw AssemblerException("Specified post index %" PRId64 " cannot be encoded. Must be %d.", imm->value, validImmediate);
			
			opcode |= 0x1f0000;
		}
		if(encodingVariant.data & 0x100)
		{
			assert(enc);
			int Q = (enc->registerData >> 2) & 1;
			
			if(!imm) throw AssemblerException("Internal error: Immediate expected for encodingVariant.data & 0x100");
			if(imm->value < 1 || imm->value > 8*Q + 7) throw AssemblerException("Shift %" PRId64 " out of range 1..%d", imm->value, 8*Q + 7);
			
			opcode |= imm->value << 11;
		}
		if(encodingVariant.data & 0x200)
		{
			assert(enc);

			int size = enc->registerData & 3;
			if(!imm) throw AssemblerException("Internal error: Immediate expected for encodingVariant.data & 0x200");
			if(imm->value < 0 || imm->value >= (8 << size)) throw AssemblerException("Index %" PRId64 " out of range 0..%d", imm->value, (8 << size) - 1);

			opcode |= ((16 << size) - imm->value) << 16;
		}
		if(encodingVariant.data & 0x400)
		{
			assert(enc);
			
			int size = enc->registerData & 3;
			if(!imm) throw AssemblerException("Internal error: Immediate expected for encodingVariant.data & 0x400");
			if(imm->value < 0 || imm->value >= (8 << size)) throw AssemblerException("Index %" PRId64 " out of range 0..%d", imm->value, (8 << size) - 1);
			
			opcode |= ((8 << size) + imm->value) << 16;
		}
		if(encodingVariant.data & 0x800)
		{
			assert(enc);
			if(rm && rm->index > 15) throw AssemblerException("Element vector must be in range 0..15");
			
			int registerSize = (enc->registerData & 3); // 0 = B, 1 = H, 2 = S, 3 = D
			assert(registerSize != 0);
			
			int i = int(index->value) << (registerSize-1);
			if(i >= 16) throw AssemblerException("Index %" PRId64 " out of range", index->value);
			
			opcode |= (i & 4) << 9;
			opcode |= (i & 3) << 20;
		}
		if(encodingVariant.data & 0x1000)
		{
			if(!imm) throw AssemblerException("Internal error: Immediate expected for encodingVariant.data & 0x1000");
			if(imm->value < 1 || imm->value > 64) throw AssemblerException("Fixed point %" PRId64 " out of range 1..64", imm->value);
			
			opcode |= (64 - imm->value) << 10;
		}

		if(rd && !rd->IsExpression()) opcode |= rd->index;
		if(rn && !rn->IsExpression()) opcode |= rn->index << 5;
		if(rm && !rm->IsExpression()) opcode |= rm->index << 16;
		if(ra && !ra->IsExpression()) opcode |= rm->index << 10;

		listAction.AppendOpcode(opcode);
		
		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(rm && rm->IsExpression())
		{
			if(encodingVariant.data & 0x800)
			{
				listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 16, 0, rm->expressionIndex, assembler));
			}
			else
			{
				listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 16, 0, rm->expressionIndex, assembler));
			}
		}
		if(ra && ra->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 10, 0, rm->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			if(encodingVariant.data & 0x100)
			{
				listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4, 1, 0, imm->expressionIndex, assembler));
			}
			if(encodingVariant.data & 0x200)
			{
				throw AssemblerException("Dynamic encoding of vector shift immediate not supported");
			}
			if(encodingVariant.data & 0x400)
			{
				throw AssemblerException("Dynamic encoding of vector shift immediate not supported");
			}
			if(encodingVariant.data & 0x1000)
			{
				throw AssemblerException("Dynamic encoding of fixed point bits not supported");
			}
		}
		if(index && index->IsExpression())
		{
			if(encodingVariant.data & 0x40)
			{
				int registerSize = (enc->registerData & 3); // 0 = B, 1 = H, 2 = S, 3 = D
				int extraShift = registerSize;
				if(extraShift < 3)
				{
					listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Masked, 3-extraShift, 10+extraShift, 0, index->expressionIndex, assembler));
				}
				listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Masked, 1, 30, 3-extraShift, index->expressionIndex, assembler));
			}
			if(encodingVariant.data & 0x800)
			{
				int registerSize = (enc->registerData & 3); // 0 = B, 1 = H, 2 = S, 3 = D
				int extraShift = registerSize-1;
				if(extraShift < 2)
				{
					listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Masked, 2-extraShift, 20+extraShift, 0, index->expressionIndex, assembler));
				}
				listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Masked, 1, 11, 2-extraShift, index->expressionIndex, assembler));
			}
		}
	}

	/**
	 * encodingVariant.data bits:
	 *     1: Shift is encoded in immh:immb
	 *     2: Shift is encoded in scale
	 *     4: Opcode[30] = (rn->registerData & 4) != 0
	 *     8: Opcode[23:22] = = (rn->registerData >> 4)
	 *  0x10: Opcode[31] = rd->matchBitfield & MatchReg64
	 */
	static void FpRdRnFixedPointShift(Assembler &assembler,
									  ListAction& listAction,
									  const Instruction& instruction,
									  const EncodingVariant &encodingVariant,
									  const Operand *const *operands)
	{
		assert(operands[0] && operands[0]->type == Operand::Type::Register);
		assert(operands[1] && operands[1]->type == Operand::Type::Register);
		assert(operands[2] && operands[2]->type == Operand::Type::Immediate);
		
		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[2];

		uint32_t opcode = encodingVariant.opcode;
		if(encodingVariant.data & 1)
		{
			opcode |= (rn->registerData & 4) << 28;
		}
		
		opcode |= rd->index;
		opcode |= rn->index << 5;

		if(encodingVariant.data & 1)
		{
			// fpSize: 0 = 16 bit, 1 = 32 bit, 2 = 64 bit
			int fpSize = (rn->registerData & 3) - 1;
			
			// Sets operand width bit.
			opcode |= (1 << 20) << fpSize;
			
			if(!imm->IsExpression())
			{
				int maxShift = 16 << ((rn->registerData >> 4) & 3);
				if(imm->value <= 0 || imm->value > maxShift) throw AssemblerException("Fixed point shift must be between #1 and #%d", maxShift);
				
				int mask = (16 << fpSize) - 1;
				int immValue = -imm->value & mask;
				opcode |= immValue << 16;
			}
		}

		if(encodingVariant.data & 2)
		{
			if(!imm->IsExpression())
			{
				int immValue = int(64 - imm->value);
				opcode |= immValue << 10;
			}
		}
		
		if(encodingVariant.data & 4)
		{
			// Opcode[30] = (rn->registerData & 4) != 0
			opcode |= (rn->registerData & 4) << 28;
		}

		if(encodingVariant.data & 8)
		{
			int fpType = (rn->registerData >> 4) & 3;
			opcode |= fpType << 22;
		}
		
		if(encodingVariant.data & 0x10)
		{
			if(rd->matchBitfield & MatchReg64)
			{
				opcode |= 0x80000000;
			}
		}

		listAction.AppendOpcode(opcode);
		
		if(rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(imm->IsExpression())
		{
			if(encodingVariant.data & 1)
			{
				// fpSize: 0 = 16 bit, 1 = 32 bit, 2 = 64 bit
				int fpSize = (rn->registerData & 3) - 1;

				char buffer[16];
				sprintf(buffer, "%d-(", 16 << fpSize);
				
				std::string expr = buffer + assembler.GetExpression(imm->expressionIndex) + ")";
				int expressionIndex = assembler.AddExpression(expr,
															  8,
															  assembler.GetExpressionSourceLine(imm->expressionIndex),
															  assembler.GetExpressionFileIndex(imm->expressionIndex));
				listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4+fpSize, 16, 0, expressionIndex, assembler));
			}
			
			if(encodingVariant.data & 2)
			{
				std::string expr = "64-(" + assembler.GetExpression(imm->expressionIndex) + ")";
				int expressionIndex = assembler.AddExpression(expr,
															  8,
															  assembler.GetExpressionSourceLine(imm->expressionIndex),
															  assembler.GetExpressionFileIndex(imm->expressionIndex));
				listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 6, 10, 0, expressionIndex, assembler));
			}
		}
	}
	
	static void FpRdRnIndex1Index2(Assembler &assembler,
								   ListAction& listAction,
								   const Instruction& instruction,
								   const EncodingVariant &encodingVariant,
								   const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Number);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Number);
		
		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const ImmediateOperand *index1 = (const ImmediateOperand*) operands[2];
		const ImmediateOperand *index2 = (const ImmediateOperand*) operands[3];

		uint32_t opcode = encodingVariant.opcode;

		const RegisterOperand *enc = (const RegisterOperand*) operands[7];
		assert(enc);

		if(encodingVariant.data & 2) opcode |= (enc->registerData & 4) << 28;

		int size = enc->registerData & 3;
		if(!index1) throw AssemblerException("Internal error: Immediate expected for encodingVariant.data & 0x200");
		if(index1->value < 0 || index1->value > (15 >> size)) throw AssemblerException("Index %" PRId64 " out of range 0..%d", index1->value, 15 >> size);
		
		opcode |= ((2*index1->value + 1) << size) << 16;

		if(index2)
		{
			int i2 = int(index2->value << size);
			if(i2 < 0 || i2 > 15) throw AssemblerException("Index %" PRId64 " out of range 0..%d", index2->value, 15 >> size);
			opcode |= i2 << 11;
		}
		
		if(rd && !rd->IsExpression()) opcode |= rd->index;
		if(rn && !rn->IsExpression()) opcode |= rn->index << 5;
		
		listAction.AppendOpcode(opcode);
		
		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(index1 && index1->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5-size, 17+size, 0, index1->expressionIndex, assembler));
		}
		if(index2 && index2->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 4-size, 11+size, 0, index2->expressionIndex, assembler));
		}
	}

	static uint8_t EncodeFloat(const ImmediateOperand *imm)
	{
		long double ld = (imm->immediateType == ImmediateOperand::ImmediateType::Integer) ?
							(long double) imm->value :
							imm->realValue;
		
		double d = ld;
		static_assert(sizeof(d) == 8, "Expect double to be 4 bytes");
		uint64_t u;
		memcpy(&u, &d, 8);
		
		uint8_t result = 0;
		if(u & 0x8000000000000000) result |= 0x80;
		
		int64_t exp = (u >> 52) & 0x7ff;
		int64_t unbiasedExp = exp - 1023;
		if(unbiasedExp <= -4 || unbiasedExp > 4) throw AssemblerException("Cannot encode %f accurately (exponent %d out of range)", d, (int) unbiasedExp);
		result |= (exp & 7) << 4;
		
		uint64_t frac = u & 0xfffffffffffff;
		if(frac & 0xffffffffffff) throw AssemblerException("Cannot encode %f accurately (fraction bits truncated)", d);
		result |= frac >> 48;
		
		return result;
	}
	
	/**
	 * RegisterData is taken from Op7
	 * RegisterData2 is taken from Op6
	 * encodingVariant.data bits:
	 *      1: Opcode[23:22] = registerData & 3
	 *      2: Opcode[30] = (registerData & 4) != 0
	 *      4: Opcode[29] = (registerData >> 4) & 1
	 *	    8: Opcode[31] = (registerData2.matchBitField & MatchReg64) != 0
	 *   0x10: Opcode[20:13] = EncodeFloat(imm->value)
	 *   0x20: Opcode[18:16, 9:5] = EncodeFloat(imm->value)
	 *   0x40: Opcode[19] = index
	 */
	static void Fmov(Assembler &assembler,
					 ListAction& listAction,
					 const Instruction& instruction,
					 const EncodingVariant &encodingVariant,
					 const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Immediate);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Register);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Number);
		
		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[1];
		const RegisterOperand *rn = (const RegisterOperand*) operands[2];
		const ImmediateOperand *index = (const ImmediateOperand*) operands[3];

		uint32_t opcode = encodingVariant.opcode;
		
		const RegisterOperand *enc = (const RegisterOperand*) operands[7];
		const RegisterOperand *enc2 = (const RegisterOperand*) operands[6];

		if(encodingVariant.data & 1)
		{
			assert(enc);
			opcode |= ((enc->registerData >> 4) & 3) << 22;
		}
		if(encodingVariant.data & 2)
		{
			assert(enc);
			opcode |= (enc->registerData & 4) << 28;
		}
		if(encodingVariant.data & 4)
		{
			assert(enc);
			opcode |= ((enc->registerData >> 4) & 1) << 29;
		}
		if(encodingVariant.data & 8)
		{
			assert(enc2);
			if(enc2->matchBitfield & MatchReg64) opcode |= 0x80000000;
		}
		if(encodingVariant.data & 0x10)
		{
			opcode |= EncodeFloat(imm) << 13;
		}
		if(encodingVariant.data & 0x20)
		{
			uint8_t f = EncodeFloat(imm);
			opcode |= (f & 0x1f) << 5;
			opcode |= (f & 0xe0) << 11;
		}
		if(encodingVariant.data & 0x40)
		{
			assert(index);
			if(index->value != 1) throw AssemblerException("Index %" PRId64 " must be 1", index->value);
			opcode |= 1 << 19;
		}

		if(rd && !rd->IsExpression()) opcode |= rd->index;
		if(rn && !rn->IsExpression()) opcode |= rn->index << 5;

		listAction.AppendOpcode(opcode);
		
		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			throw AssemblerException("Floating point constants not yet supported");
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
	}

	// Label, Number and immediate expressions supported
	// encodingValue.data & 0xff = lowest bit of opcode
	// encodingValue.data & 0xff00 = number of bits of opcode.
	static void RdRnImmSubfield(Assembler &assembler,
								ListAction& listAction,
								const Instruction& instruction,
								const EncodingVariant &encodingVariant,
								const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2]->type == Operand::Type::Label
			   || (operands[2]->type == Operand::Type::Number && operands[2]->IsExpression())
			   || (operands[2]->type == Operand::Type::Immediate && operands[2]->IsExpression()));
		assert(operands[3] != nullptr && operands[3]->type == Operand::Type::Number);
		assert(operands[4] != nullptr && operands[4]->type == Operand::Type::Number);
		assert(operands[5] == nullptr || operands[5]->type == Operand::Type::Immediate);

		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const Operand *target = operands[2];
		const ImmediateOperand *highBitIndex = (const ImmediateOperand*) operands[3];
		const ImmediateOperand *lowBitIndex = (const ImmediateOperand*) operands[4];
		const ImmediateOperand *hwShift = (const ImmediateOperand*) operands[5];

		uint32_t opcode = encodingVariant.opcode;
		if(rd) opcode |= rd->index;
		if(rn) opcode |= rn->index << 5;

		if(hwShift)
		{
			if(hwShift->IsExpression()) throw AssemblerException("Shift expression not supported");
			switch(hwShift->value)
			{
			case 0:
			case 16:
			case 32:
			case 48:
				opcode |= (hwShift->value / 16) << 21;
				break;
			default:
				throw AssemblerException("Invalid shift value (%" PRId64 ")", hwShift->value);
			}
		}
		
		switch(target->type)
		{
		case Operand::Type::Label:
			{
				const LabelOperand *label = (const LabelOperand*) target;
				int displacement = int32_t(label->displacement ? label->displacement->value : 0);
				if(displacement != 0)
				{
					throw AssemblerException("Label displacements are not supported when subfields are specified");
				}

				if(lowBitIndex->value != 0 || highBitIndex->value != 11)
				{
					throw AssemblerException("Only bitrange [11:0] is supported for label targets");
				}

				listAction.AppendOpcode(opcode);
				listAction.Append(label->CreatePatchAction(RelEncoding::Imm12, 4));
				
				if(label->displacement && label->displacement->IsExpression())
				{
					throw AssemblerException("Label displacements are not supported when subfields are specified");
				}
			}
			break;
		case Operand::Type::Number:
			{
				if(lowBitIndex->value != 0 || highBitIndex->value != 11)
				{
					throw AssemblerException("Only bitrange [11:0] is supported for absolute targets");
				}
				
				listAction.AppendOpcode(opcode);
				listAction.Append(new PatchExpressionAction(RelEncoding::Imm12, 4, target->expressionIndex, assembler));
			}
			break;
		case Operand::Type::Immediate:
			{
				listAction.AppendOpcode(opcode);
				
				if(lowBitIndex->value >= highBitIndex->value)
				{
					throw AssemblerException("HighBitIndex (%" PRId64 ") must be greater than LowBitIndex (%" PRId64 ")", highBitIndex->value, lowBitIndex->value);
				}
				int numberOfBits = int(highBitIndex->value - lowBitIndex->value + 1);
				if(numberOfBits != (encodingVariant.data >> 8))
				{
					throw AssemblerException("Bit range [%" PRId64 ":%" PRId64 "] expected to be width %d", highBitIndex->value, lowBitIndex->value, encodingVariant.data >> 8);
				}
				
				listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Masked,
														numberOfBits,
														encodingVariant.data & 0xff,
														lowBitIndex->value,
														target->expressionIndex, assembler));
			}
			break;
		default:
			assert(!"Internal error");
			break;
		}
		
		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
	}

	/**
	 * data: 0 = Encoding for BIC, ORR, MOVI (non-64 bit)
	 *       1 = MOVI MSL
	 *       2 = MOVI 64-bit.
	 */
	static void VectorImmediate(Assembler &assembler,
								ListAction& listAction,
								const Instruction& instruction,
								const EncodingVariant &encodingVariant,
								const Operand *const *operands)
	{
		assert(operands[0] != nullptr && operands[0]->type == Operand::Type::Register);
		assert(operands[1] != nullptr && operands[1]->type == Operand::Type::Immediate);
		assert(operands[2] == nullptr
			   || operands[2]->type == Operand::Type::Label
			   || (operands[2]->type == Operand::Type::Shift && operands[2]->shift == Operand::Shift::LSL));
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Immediate);
		
		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[1];
		const ImmediateOperand *amountImm = (const ImmediateOperand*) operands[3];
		
		int64_t shiftAmount = amountImm ? amountImm->value : 0;
		
		uint32_t opcode = encodingVariant.opcode;
		if(rd->registerData & 4) opcode |= 0x40000000;

		int64_t value = imm->value;
		
		if(rd && !rd->IsExpression()) opcode |= rd->index;
		
		switch(encodingVariant.data)
		{
		case 0: // LSL
			switch(rd->registerData & 3)
			{
			case 0:
				if(shiftAmount != 0) throw AssemblerException("ShiftAmount must be #0");
				opcode |= 0xe000;
				break;
			case 1:
				switch(shiftAmount)
				{
				case 0:
					opcode |= 0x8000;
					break;
				case 8:
					opcode |= 0xa000;
					break;
				default:
					throw AssemblerException("ShiftAmount must be #0 or #8");
				}
				break;
			case 2:
				switch(shiftAmount)
				{
				case 0:
					break;
				case 8:
					opcode |= 0x2000;
					break;
				case 16:
					opcode |= 0x4000;
					break;
				case 24:
					opcode |= 0x6000;
					break;
				default:
					throw AssemblerException("ShiftAmount must be #0, #8, #16 or #24");
				}
				break;
			case 3:
				opcode |= 0x2000e000;
				break;
			}
			break;
		case 1:	// MSL
			if((rd->registerData & 3) != 2)
			{
				// not 32-bit
				throw AssemblerException("MSL shifts can only be used with 32-bit destinations.");
			}
			if(operands[2]->type != Operand::Type::Label
			   || ((LabelOperand*) operands[2])->labelName != "msl")
			{
				throw AssemblerException("Only lsl or msl are allowed");
			}
			switch(shiftAmount)
			{
			case 8:
				break;
			case 16:
				opcode |= 0x1000;
				break;
			default:
				throw AssemblerException("ShiftAmount must be #8 or #16");
			}
			break;
		case 2:	// 64-bit
			{
				int result = 0;
				for(int i = 0; i < 8; ++i)
				{
					int byte = (value >> (i*8)) & 0xff;
					switch(byte)
					{
					case 0:
						break;
					case 0xff:
						result |= 1 << i;
						break;
					default:
						throw AssemblerException("Invalid 64-bit immediate (%" PRId64 ")", value);
					}
				}
				value = result;
			}
			break;
		}

		if((value >> 8) != 0)
		{
			throw AssemblerException("VectorImmediate must be 8 bits wide");
		}
		
		int lower5Bits = int(value & 0x1f);
		int upper3Bits = int(value >> 5);
		opcode |= lower5Bits << 5;
		opcode |= upper3Bits << 16;

		listAction.AppendOpcode(opcode);
		
		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			switch(encodingVariant.data)
			{
			case 0:
			case 1:
				listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Masked, 5, 5, 0, imm->expressionIndex, assembler));
				listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Masked, 3, 16, 5, imm->expressionIndex, assembler));
				break;
			default:
				throw AssemblerException("Expressions for 64-bit immediates Not yet supported");
			}
		}
	}

	static void RdRnRmImm2(Assembler &assembler,
						   ListAction& listAction,
						   const Instruction& instruction,
						   const EncodingVariant &encodingVariant,
						   const Operand *const *operands)
	{
		assert(operands[0] == nullptr || operands[0]->type == Operand::Type::Register);
		assert(operands[1] == nullptr || operands[1]->type == Operand::Type::Register);
		assert(operands[2] == nullptr || operands[2]->type == Operand::Type::Register);
		assert(operands[3] == nullptr || operands[3]->type == Operand::Type::Number);
		
		const RegisterOperand *rd = (const RegisterOperand*) operands[0];
		const RegisterOperand *rn = (const RegisterOperand*) operands[1];
		const RegisterOperand *rm = (const RegisterOperand*) operands[2];
		const ImmediateOperand *imm = (const ImmediateOperand*) operands[3];
		
		uint32_t opcode = encodingVariant.opcode;
		
		if(rd && !rd->IsExpression()) opcode |= rd->index;
		if(rn && !rn->IsExpression()) opcode |= rn->index << 5;
		if(rm && !rm->IsExpression()) opcode |= rm->index << 16;
		if(imm && !imm->IsExpression())
		{
			assert(0 <= imm->value && imm->value < 4);
			opcode |= imm->value << 12;
		}
		
		listAction.AppendOpcode(opcode);
		
		if(rd && rd->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 0, 0, rd->expressionIndex, assembler));
		}
		if(rn && rn->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 5, 0, rn->expressionIndex, assembler));
		}
		if(rm && rm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 5, 16, 0, rm->expressionIndex, assembler));
		}
		if(imm && imm->IsExpression())
		{
			listAction.Append(new PatchOpcodeAction(PatchOpcodeAction::Unsigned, 2, 12, 0, imm->expressionIndex, assembler));
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
