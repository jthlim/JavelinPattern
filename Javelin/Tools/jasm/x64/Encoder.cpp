//============================================================================

#include "Javelin/Tools/jasm/x64/Encoder.h"

#include "Javelin/Tools/jasm/x64/Action.h"
#include "Javelin/Tools/jasm/x64/Assembler.h"
#include "Javelin/Tools/jasm/x64/Register.h"
#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/Log.h"

#include <assert.h>
#include <memory.h>

//============================================================================

using namespace Javelin::Assembler;
using namespace Javelin::Assembler::x64;

//============================================================================
// Encoding is always:
// Legacy Prefixes | REX | Opcode | ModR/M | SIB | Displacement | Immediate
//============================================================================

union ModRM
{
	enum class Mod : uint8_t
	{
		IndirectDisplacement0 = 0,
		IndirectDisplacement8 = 1,
		IndirectDisplacement32 = 2,
		Register = 3,
	};
	
	struct
	{
		uint8_t rm  : 3;
		uint8_t reg : 3;
		Mod     mod : 2;
	};
	uint8_t value;
};
static_assert(sizeof(ModRM) == 1, "Size of ModRM expected to by 1 byte");

union SIB
{
	struct
	{
		uint8_t base  : 3;
		uint8_t index : 3;
		uint8_t scale : 2;
	};
	uint8_t value;
};

static_assert(sizeof(SIB) == 1, "Size of SIB expected to by 1 byte");

union REX
{
	struct
	{
		uint8_t b : 1;			// 4th bit of ModRM:r/m, SIB:b or opcode reg field.
		uint8_t x : 1;			// 4th bit of SIB:index
		uint8_t r : 1;			// 4th bit of ModRM:reg
		uint8_t w : 1;			// 64 bit register
		uint8_t constant4 : 4;
	};
	uint8_t value;
};
static_assert(sizeof(REX) == 1, "Size of REX expected to by 1 byte");

union VEX2B2
{
	struct
	{
		uint8_t pp : 2;
		uint8_t l : 1;
		uint8_t vvvv : 4;
		uint8_t r : 1;
	};
	uint8_t value;
};
static_assert(sizeof(VEX2B2) == 1, "Size of VEX2B2 expected to by 1 byte");

union VEX3B2
{
	struct
	{
		uint8_t mmmmm : 5;
		uint8_t b : 1;
		uint8_t x : 1;
		uint8_t r : 1;
	};
	uint8_t value;
};
static_assert(sizeof(VEX3B2) == 1, "Size of VEX3B2 expected to by 1 byte");

union VEX3B3
{
	struct
	{
		uint8_t pp : 2;
		uint8_t l : 1;
		uint8_t vvvv : 4;
		uint8_t w : 1;
	};
	uint8_t value;
};
static_assert(sizeof(VEX3B3) == 1, "Size of VEX3B3 expected to by 1 byte");

const uint8_t kSIBNoIndex = 0b100;
const uint8_t kSIBNoBase = 0b101;
const uint8_t kModRMRmSIB = 0b100;
const uint8_t kModRMRipRelative = 0b101;
const uint8_t kOperandSizePrefix = 0x66;
const uint8_t kOpcodeVEX2 = 0xc5;
const uint8_t kOpcodeVEX3 = 0xc4;

//============================================================================

static bool RequiresSIB(const MemoryOperand *memoryOperand)
{
	if(memoryOperand->index != nullptr) return true;

	if(memoryOperand->base == nullptr)
	{
		throw AssemblerException("Missing base pointer in memory access");
	}
	if((memoryOperand->base->index&7) == kModRMRmSIB) return true;

	return false;
}

static bool RequiresVEX3(VEX3B2 b2, VEX3B3 b3)
{
	return b2.x || b2.b || b2.mmmmm != 1 || b3.w;
}

static void ProcessOperandWidth(ListAction& listAction,
								const Instruction& instruction,
								const EncodingVariant &encodingVariant,
								REX &rex)
{
	if(encodingVariant.operationWidth == instruction.defaultWidth) return;
	
	switch(encodingVariant.operationWidth)
	{
	case 0:
	case 8:
		break;
	case 16:
		listAction.Append(new LiteralAction({kOperandSizePrefix}));
		break;
	case 32:
	case 64:
		rex.constant4 = 4;
		rex.w = 1;
		break;
	default:
		assert(!"Unexpected data");
	}
}

static int AppendImmediateOperand(ListAction& listAction,
								  uint64_t matchMask,
								  const ImmediateOperand *immediateOperand)
{
	if(!immediateOperand) return 0;

	MatchBitIndex matchBitIndex = (MatchBitIndex) __builtin_ctzll(matchMask);
	switch(matchBitIndex)
	{
	case MatchBitIndex::Imm8:
		if(immediateOperand->IsExpression())
		{
			listAction.Append(new ExpressionAction(1, immediateOperand->expressionIndex));
		}
		else
		{
			uint8_t v;
			memcpy(&v, &immediateOperand->value, sizeof(v));
			listAction.Append(new LiteralAction({v}));
		}
		return 1;
	case MatchBitIndex::Imm16:
		if(immediateOperand->IsExpression())
		{
			listAction.Append(new ExpressionAction(2, immediateOperand->expressionIndex));
		}
		else
		{
			uint8_t v[2];
			memcpy(&v, &immediateOperand->value, sizeof(v));
			listAction.Append(new LiteralAction({v, v+sizeof(v)}));
		}
		return 2;
	case MatchBitIndex::Imm32:
	case MatchBitIndex::UImm32:
		if(immediateOperand->IsExpression())
		{
			listAction.Append(new ExpressionAction(4, immediateOperand->expressionIndex));
		}
		else
		{
			uint8_t v[4];
			memcpy(&v, &immediateOperand->value, sizeof(v));
			listAction.Append(new LiteralAction({v, v+sizeof(v)}));
		}
		return 4;
	case MatchBitIndex::Imm64:
		if(immediateOperand->IsExpression())
		{
			listAction.Append(new ExpressionAction(8, immediateOperand->expressionIndex));
		}
		else
		{
			uint8_t v[8];
			memcpy(&v, &immediateOperand->value, sizeof(v));
			listAction.Append(new LiteralAction({v, v+sizeof(v)}));
		}
		return 8;
	default:
		// ERROR
		return 0;
	}
}

static void EncodeRRI(Assembler &assembler,
					  ListAction& listAction,
					  const Instruction& instruction,
					  const EncodingVariant &encodingVariant,
					  const uint64_t *encodingMatchMasks,
					  const RegisterOperand *reg,
					  const RegisterOperand *rm,
					  const ImmediateOperand *imm)
{
	assert(reg && reg->type == Operand::Type::Register);
	assert(rm && rm->type == Operand::Type::Register);

	// Encoding is:
	//   [prefix] | [REX] | opcodes | ModR/M | [Immediate]
	// prefix can be literal encoded.
	// REX, opcodes, ModR/M can be literal if reg and rm are constant.
	
	REX rex = { .value = 0 };
	ModRM modRM = { .value = 0 };
	
	ProcessOperandWidth(listAction, instruction, encodingVariant, rex);
	
	modRM.mod = ModRM::Mod::Register;
	modRM.reg = reg->index;
	modRM.rm = rm->index;
	if(reg->RequiresREX() || rm->RequiresREX()) rex.constant4 = 4;
	if(reg->index & 8) rex.r = 1;
	if(rm->index & 8) rex.b = 1;

	if(rex.value)
	{
		if(reg->IsHighByte()) throw AssemblerException("High byte registers cannot be used with REX");
		if(rm->IsHighByte()) throw AssemblerException("High byte registers cannot be used with REX");
	}

	if(reg->IsExpression() || rm->IsExpression())
	{
		listAction.Append(new DynamicOpcodeRR(rex.value,
											  modRM.value,
											  reg->expressionIndex,
											  rm->expressionIndex,
											  encodingVariant.GetOpcodeVector()));
	}
	else
	{
		std::vector<uint8_t> bytes;
		if(rex.value) bytes.push_back(rex.value);
		bytes.insert(bytes.end(),
					 encodingVariant.opcodes,
					 encodingVariant.opcodes+encodingVariant.opcodeLength);
		bytes.push_back(modRM.value);
		listAction.Append(new LiteralAction(bytes));
	}

	if(imm) AppendImmediateOperand(listAction, encodingMatchMasks[2], imm);
}

static void EncodeRMI(Assembler &assembler,
					  ListAction& listAction,
					  const Instruction& instruction,
					  const EncodingVariant &encodingVariant,
					  const uint64_t *encodingMatchMasks,
					  const RegisterOperand *reg,
					  const MemoryOperand *rm,
					  const ImmediateOperand *imm)
{
	assert(reg && reg->type == Operand::Type::Register);
	assert(rm && rm->type == Operand::Type::Memory);

	// Encoding is:
	//   [Legacy Prefixes] | [REX] | Opcodes | ModR/M | [SIB] | [Displacement] | [Immediate]

	REX rex = { .value = 0 };
	ModRM modRM = { .value = 0 };
	SIB sib = { .value = 0 };

	ProcessOperandWidth(listAction, instruction, encodingVariant, rex);

	modRM.reg = reg->index;
	if(reg->RequiresREX()) rex.constant4 = 4;
	if(reg->index & 8) rex.r = 1;
	
	if(rm->base)
	{
		modRM.rm = rm->base->index;
		if(rm->base->index & 8)
		{
			rex.constant4 = 4;
			rex.b = 1;
		}
	}
	
	int32_t displacement = rm->displacement ? (int32_t) rm->displacement->value : 0;
	if(rm->scale)
	{
		switch(rm->scale->value)
		{
		case 1:	sib.scale = 0; break;
		case 2: sib.scale = 1; break;
		case 4: sib.scale = 2; break;
		case 8: sib.scale = 3; break;
		}
	}

	if(rm->index)
	{
		sib.index = rm->index->index;
		if(rm->index->index & 8)
		{
			rex.constant4 = 4;
			rex.x = 1;
		}
	}
	else
	{
		sib.index = kSIBNoIndex;
	}
	sib.base = rm->base ? rm->base->index : kSIBNoBase;

	if(rex.value)
	{
		if(reg->IsHighByte()) throw AssemblerException("High byte registers cannot be used with REX");
	}

	bool requiresSIB = RequiresSIB(rm);
	if(requiresSIB)
	{
		modRM.rm = kModRMRmSIB;
		if(rm->base == &Register::RIP || rm->index == &Register::RIP)
		{
			Log::Error("RIP cannot be used in this address expression");
		}
	}
	
	// It is necessary to set modRM.mod even for displacement expressions since
	// the hint for RIP vs RBP for the assembler is in the modRM.mod field
	int writeDisplacementBytes = 0;
	if(rm->base == nullptr || rm->base == &Register::RIP)
	{
		modRM.mod = ModRM::Mod::IndirectDisplacement0;
		writeDisplacementBytes = 4;
	}
	else if(displacement == 0
			&& (rm->base->index & 7) != Register::RIP.index)
	{
		modRM.mod = ModRM::Mod::IndirectDisplacement0;
		writeDisplacementBytes = 0;
	}
	else if(displacement == (int8_t)displacement)
	{
		modRM.mod = ModRM::Mod::IndirectDisplacement8;
		writeDisplacementBytes = 1;
	}
	else
	{
		modRM.mod = ModRM::Mod::IndirectDisplacement32;
		writeDisplacementBytes = 4;
	}

	if(rm->displacement && rm->displacement->IsExpression())
	{
		assembler.UpdateExpressionBitWidth(rm->displacement->expressionIndex, 32);
	}
	
	if(!reg->IsExpression()
	    && (!rm->base || !rm->base->IsExpression())
		&& (!rm->index || !rm->index->IsExpression())
		&& (!rm->scale || !rm->scale->IsExpression())
		&& (!rm->displacement || !rm->displacement->IsExpression()))
	{
		// Everything is constant.
		std::vector<uint8_t> bytes;
		if(rex.value) bytes.push_back(rex.value);
		bytes.insert(bytes.end(), encodingVariant.opcodes, encodingVariant.opcodes+encodingVariant.opcodeLength);

		bytes.push_back(modRM.value);
		if(requiresSIB) bytes.push_back(sib.value);
		switch(writeDisplacementBytes)
		{
		case 1:
			bytes.push_back(displacement);
			break;
		case 4:
			uint8_t disp32[4];
			memcpy(disp32, &displacement, 4);
			bytes.insert(bytes.end(), disp32, disp32+4);
			break;
		}
		listAction.Append(new LiteralAction(bytes));
	}
	else if(!reg->IsExpression()
			&& (!rm->base || !rm->base->IsExpression())
			&& (!rm->index || !rm->index->IsExpression())
			&& (!rm->scale || !rm->scale->IsExpression())
			&& (rm->displacement
				&&rm->displacement->IsExpression()
				&& rm->displacement->value == 0
				&& (rm->displacement->matchBitfield == MatchImm8
					|| rm->displacement->matchBitfield == (MatchImm32|MatchUImm32)
					|| rm->base == &Register::RIP)
			  )
			&& modRM.mod == ModRM::Mod::IndirectDisplacement0)
	{
		// Everything is a constant except for the displacement,
		// and the displacement has a qualified imm size,
		// or is a RIP relative displacement (which must be 4 bytes).
		Action *expressionAction = nullptr;

		if (rm->base == &Register::RIP)
		{
			expressionAction = new ExpressionAction(4, rm->displacement->expressionIndex);
		}
		else switch(rm->displacement->matchBitfield)
		{
		case MatchImm8:
			expressionAction = new ExpressionAction(1, rm->displacement->expressionIndex);
			modRM.mod = ModRM::Mod::IndirectDisplacement8;
			break;
		case MatchImm32|MatchUImm32:
			expressionAction = new ExpressionAction(4, rm->displacement->expressionIndex);
			modRM.mod = ModRM::Mod::IndirectDisplacement32;
			break;
		default:
			assert(!"Internal error: Unexpected case");
		}

		std::vector<uint8_t> bytes;
		if(rex.value) bytes.push_back(rex.value);
		bytes.insert(bytes.end(), encodingVariant.opcodes, encodingVariant.opcodes+encodingVariant.opcodeLength);
		
		bytes.push_back(modRM.value);
		if(requiresSIB) bytes.push_back(sib.value);
		listAction.Append(new LiteralAction(bytes));
		listAction.Append(expressionAction);
	}
	else
	{
		listAction.Append(new DynamicOpcodeRM(rex.value,
											  modRM.value,
											  sib.value,
											  displacement,
											  reg->expressionIndex,
											  rm->scale ? rm->scale->expressionIndex : 0,
											  rm->index ? rm->index->expressionIndex : 0,
											  rm->base ? rm->base->expressionIndex : 0,
											  rm->displacement ? rm->displacement->expressionIndex : 0,
											  encodingVariant.GetOpcodeVector()));
	}
	
	if(imm) AppendImmediateOperand(listAction, encodingMatchMasks[2], imm);
}

static void EncodeRLI(Assembler &assembler,
					  ListAction& listAction,
					  const Instruction& instruction,
					  const EncodingVariant &encodingVariant,
					  const uint64_t *encodingMatchMasks,
					  const RegisterOperand *reg,
					  const LabelOperand *labelOperand,
					  const ImmediateOperand *imm)
{
	assert(reg && reg->type == Operand::Type::Register);
	assert(labelOperand && labelOperand->type == Operand::Type::Label);
	
	// Encoding is:
	//   [Legacy Prefixes] | [REX] | Opcodes | ModR/M | [Displacement] | [Immediate]

	REX rex = { .value = 0 };
	ModRM modRM = { .value = 0 };
	
	ProcessOperandWidth(listAction, instruction, encodingVariant, rex);

	modRM.mod = ModRM::Mod::IndirectDisplacement0;	// actually displacement 4 for special case of RIP
	modRM.reg = reg->index;
	modRM.rm = kModRMRipRelative;
	if(reg->RequiresREX()) rex.constant4 = 4;
	if(reg->index & 8) rex.r = 1;

	if(rex.value)
	{
		if(reg->IsHighByte()) throw AssemblerException("High byte registers cannot be used with REX");
	}
	
	if(reg->IsExpression())
	{
		listAction.Append(new DynamicOpcodeR(rex.value,
											 modRM.value,
											 reg->expressionIndex,
											 encodingVariant.GetOpcodeVector()));
	}
	else
	{
		std::vector<uint8_t> bytes;
		if(rex.value) bytes.push_back(rex.value);
		bytes.insert(bytes.end(),
					 encodingVariant.opcodes,
					 encodingVariant.opcodes+encodingVariant.opcodeLength);
		bytes.push_back(modRM.value);
		listAction.Append(new LiteralAction(bytes));
	}
	
	std::vector<uint8_t> bytes;
	uint8_t offsetBytes[4] = {};
	if(labelOperand->displacement)
	{
		memcpy(offsetBytes, &labelOperand->displacement->value, 4);
	}
	bytes.insert(bytes.end(), offsetBytes, offsetBytes+4);
	listAction.Append(new LiteralAction(bytes));

	if(labelOperand->displacement && labelOperand->displacement->IsExpression())
	{
		assembler.UpdateExpressionBitWidth(labelOperand->displacement->expressionIndex, 32);
		listAction.Append(new AddExpressionAction(4, labelOperand->displacement->expressionIndex));
	}

	int immediateWidth = 0;
	if(imm) immediateWidth = AppendImmediateOperand(listAction, encodingMatchMasks[2], imm);
	
	int patchBytes = 4;
	Action *patchAction = labelOperand->CreatePatchAction(patchBytes, patchBytes+immediateWidth);
	listAction.Append(patchAction);
}

static void EncodeR_RM_I(Assembler &assembler,
						 ListAction& listAction,
						 const Instruction& instruction,
						 const EncodingVariant &encodingVariant,
						 const uint64_t *encodingMatchMasks,
						 const RegisterOperand *reg,
						 const Operand *rm,
						 const ImmediateOperand *imm)
{
	assert(reg);
	assert(reg->type == Operand::Type::Register);
	assert(rm);
	assert(rm->type == Operand::Type::Register
		   || rm->type == Operand::Type::Memory
		   || rm->type == Operand::Type::Label);

	if(encodingVariant.opcodePrefix)
	{
		listAction.Append(new LiteralAction({encodingVariant.opcodePrefix}));
	}

	switch(rm->type)
	{
	case Operand::Type::Register:
		EncodeRRI(assembler,
				  listAction,
				  instruction,
				  encodingVariant,
				  encodingMatchMasks,
				  reg,
				  (const RegisterOperand*) rm,
				  imm);
		break;
	case Operand::Type::Memory:
		EncodeRMI(assembler,
				  listAction,
				  instruction,
				  encodingVariant,
				  encodingMatchMasks,
				  reg,
				  (const MemoryOperand*) rm,
				  imm);
		break;
	case Operand::Type::Label:
		EncodeRLI(assembler,
				  listAction,
				  instruction,
				  encodingVariant,
				  encodingMatchMasks,
				  reg,
				  (const LabelOperand*) rm,
				  imm);
		break;
	default:
		assert(!"Unexpected behavior");
		break;
	}
}

/**
 * For VEX encoded instructions, the prefix is taken from
 * the opcodePrefix byte, the width field is taken from opcodeData & 0x10.
 */
static void EncodeRVRI(Assembler &assembler,
					   ListAction& listAction,
					   const Instruction& instruction,
					   const EncodingVariant &encodingVariant,
					   const uint64_t *encodingMatchMasks,
					   const RegisterOperand *reg0,
					   const RegisterOperand *reg1,
					   const RegisterOperand *reg2,
					   const ImmediateOperand *imm)
{
	assert(reg0 && reg0->type == Operand::Type::Register);
	assert(reg1 && reg1->type == Operand::Type::Register);
	assert(reg2 && reg2->type == Operand::Type::Register);

	// Encoding is:
	//   [VEX2|VEX3] | opcodes | ModR/M | [Immediate]
	// prefix can be literal encoded.

	VEX3B2 vex3B2 = { .value = 0 };
	VEX3B3 vex3B3 = { .value = 0 };
	ModRM modRM = { .value = 0 };
	modRM.mod = ModRM::Mod::Register;
	modRM.reg = reg0->index;

	if(reg0->index & 8) vex3B2.r = 1;
	vex3B3.vvvv = reg1->index;
	modRM.rm = reg2->index;
	if(reg2->index & 8) vex3B2.b = 1;
	if(encodingVariant.operationWidth == 256) vex3B3.l = 1;

	vex3B3.w = (encodingVariant.opcodeData & 0x10) ? 1 : 0;
	vex3B3.pp = encodingVariant.GetAvxPrefixValue();
	
	const uint8_t *p;
	uint32_t opcodeLength;
	vex3B2.mmmmm = encodingVariant.GetAvxMmValue(p, opcodeLength);

	if(reg0->IsExpression()
	   || reg1->IsExpression()
	   || reg2->IsExpression())
	{
		listAction.Append(new DynamicOpcodeRVR(vex3B2.value,
											   vex3B3.value,
											   modRM.value,
											   reg0->expressionIndex,
											   reg1->expressionIndex,
											   reg2->expressionIndex,
											   {p, p + opcodeLength}));
	}
	else
	{
		std::vector<uint8_t> bytes;
		if(RequiresVEX3(vex3B2, vex3B3))
		{
			// 3 byte VEX required.
			bytes.push_back(kOpcodeVEX3);
			bytes.push_back(vex3B2.value ^ 0xe0);
			bytes.push_back(vex3B3.value ^ 0x78);
		}
		else
		{
			bytes.push_back(kOpcodeVEX2);
			bytes.push_back(vex3B2.value ^ vex3B3.value ^ 0xf9);
		}
		bytes.insert(bytes.end(),
					 p,
					 p + opcodeLength);
		bytes.push_back(modRM.value);
		listAction.Append(new LiteralAction(bytes));
	}

	if(imm) AppendImmediateOperand(listAction, encodingMatchMasks[3], imm);
}

static void EncodeRVMI(Assembler &assembler,
					   ListAction& listAction,
					   const Instruction& instruction,
					   const EncodingVariant &encodingVariant,
					   const uint64_t *encodingMatchMasks,
					   const RegisterOperand *reg0,
					   const RegisterOperand *reg1,
					   const MemoryOperand *rm,
					   const ImmediateOperand *imm)
{
	assert(reg0 && reg0->type == Operand::Type::Register);
	assert(reg1 && reg1->type == Operand::Type::Register);
	assert(rm && rm->type == Operand::Type::Memory);

	// Encoding is:
	//   [VEX2|VEX3] | Opcodes | ModR/M | [SIB] | [Displacement] | [Immediate]

	VEX3B2 vex3B2 = { .value = 0 };
	VEX3B3 vex3B3 = { .value = 0 };
	ModRM modRM = { .value = 0 };
	SIB sib = { .value = 0 };

	modRM.reg = reg0->index;
	if(reg0->index & 8) vex3B2.r = 1;
	vex3B3.vvvv = reg1->index;
	if(encodingVariant.operationWidth == 256) vex3B3.l = 1;
	
	vex3B3.w = (encodingVariant.opcodeData & 0x10) ? 1 : 0;
	vex3B3.pp = encodingVariant.GetAvxPrefixValue();

	const uint8_t *p;
	uint32_t opcodeLength;
	vex3B2.mmmmm = encodingVariant.GetAvxMmValue(p, opcodeLength);
	
	if(rm->base)
	{
		modRM.rm = rm->base->index;
		if(rm->base->index & 8) vex3B2.b = 1;
	}
	
	int32_t displacement = rm->displacement ? (int32_t) rm->displacement->value : 0;
	if(rm->scale)
	{
		switch(rm->scale->value)
		{
		case 1:	sib.scale = 0; break;
		case 2: sib.scale = 1; break;
		case 4: sib.scale = 2; break;
		case 8: sib.scale = 3; break;
		}
	}

	if(rm->index)
	{
		sib.index = rm->index->index;
		if(rm->index->index & 8) vex3B2.x = 1;
	}
	else
	{
		sib.index = kSIBNoIndex;
	}
	sib.base = rm->base ? rm->base->index : kSIBNoBase;

	bool requiresSIB = RequiresSIB(rm);
	if(requiresSIB)
	{
		modRM.rm = kModRMRmSIB;
		if(rm->base == &Register::RIP || rm->index == &Register::RIP)
		{
			Log::Error("RIP cannot be used in this address expression");
		}
	}
	
	// It is necessary to set modRM.mod even for displacement expressions since
	// the hint for RIP vs RBP for the assembler is in the modRM.mod field
	int writeDisplacementBytes = 0;
	if(rm->base == nullptr || rm->base == &Register::RIP)
	{
		modRM.mod = ModRM::Mod::IndirectDisplacement0;
		writeDisplacementBytes = 4;
	}
	else if(displacement == 0
			&& (rm->base->index & 7) != Register::RIP.index)
	{
		modRM.mod = ModRM::Mod::IndirectDisplacement0;
		writeDisplacementBytes = 0;
	}
	else if(displacement == (int8_t)displacement)
	{
		modRM.mod = ModRM::Mod::IndirectDisplacement8;
		writeDisplacementBytes = 1;
	}
	else
	{
		modRM.mod = ModRM::Mod::IndirectDisplacement32;
		writeDisplacementBytes = 4;
	}

	if(rm->displacement && rm->displacement->IsExpression())
	{
		assembler.UpdateExpressionBitWidth(rm->displacement->expressionIndex, 32);
	}
	
	if(!reg0->IsExpression()
	   && !reg1->IsExpression()
	   && (!rm->base || !rm->base->IsExpression())
	   && (!rm->index || !rm->index->IsExpression())
	   && (!rm->scale || !rm->scale->IsExpression())
	   && (!rm->displacement || !rm->displacement->IsExpression()))
	{
		// Everything is constant.
		std::vector<uint8_t> bytes;
		if(RequiresVEX3(vex3B2, vex3B3))
		{
			// 3 byte VEX required.
			bytes.push_back(kOpcodeVEX3);
			bytes.push_back(vex3B2.value ^ 0xe0);
			bytes.push_back(vex3B3.value ^ 0x78);
		}
		else
		{
			bytes.push_back(kOpcodeVEX2);
			bytes.push_back(vex3B2.value ^ vex3B3.value ^ 0xf9);
		}

		bytes.insert(bytes.end(), p, p+opcodeLength);

		bytes.push_back(modRM.value);
		if(requiresSIB) bytes.push_back(sib.value);
		switch(writeDisplacementBytes)
		{
		case 1:
			bytes.push_back(displacement);
			break;
		case 4:
			uint8_t disp32[4];
			memcpy(disp32, &displacement, 4);
			bytes.insert(bytes.end(), disp32, disp32+4);
			break;
		}
		listAction.Append(new LiteralAction(bytes));
	}
	else if(!reg0->IsExpression()
			&& !reg1->IsExpression()
			&& (!rm->base || !rm->base->IsExpression())
			&& (!rm->index || !rm->index->IsExpression())
			&& (!rm->scale || !rm->scale->IsExpression())
			&& (rm->displacement &&
				rm->displacement->IsExpression()
				&& (rm->displacement->matchBitfield == MatchImm8
					|| rm->displacement->matchBitfield == (MatchImm32|MatchUImm32)))
			&& modRM.mod == ModRM::Mod::IndirectDisplacement0
			&& writeDisplacementBytes == 0)
	{
		// Everything is a constant except for the displacement,
		// and the displacement has a qualified imm size.
		// Everything is constant.
		
		Action *expressionAction = nullptr;
		switch(rm->displacement->matchBitfield)
		{
		case MatchImm8:
			expressionAction = new ExpressionAction(1, rm->displacement->expressionIndex);
			modRM.mod = ModRM::Mod::IndirectDisplacement8;
			break;
		case MatchImm32|MatchUImm32:
			expressionAction = new ExpressionAction(4, rm->displacement->expressionIndex);
			modRM.mod = ModRM::Mod::IndirectDisplacement32;
			break;
		default:
			assert(!"Internal error: Unexpected case");
		}

		std::vector<uint8_t> bytes;
		if(RequiresVEX3(vex3B2, vex3B3))
		{
			// 3 byte VEX required.
			bytes.push_back(kOpcodeVEX3);
			bytes.push_back(vex3B2.value ^ 0xe0);
			bytes.push_back(vex3B3.value ^ 0x78);
		}
		else
		{
			bytes.push_back(kOpcodeVEX2);
			bytes.push_back(vex3B2.value ^ vex3B3.value ^ 0xf9);
		}
		
		bytes.insert(bytes.end(), p, p+opcodeLength);
		
		bytes.push_back(modRM.value);
		if(requiresSIB) bytes.push_back(sib.value);
		listAction.Append(new LiteralAction(bytes));
		listAction.Append(expressionAction);
	}
	else
	{
		listAction.Append(new DynamicOpcodeRVM(vex3B2.value,
											   vex3B3.value,
											   modRM.value,
											   sib.value,
											   displacement,
											   reg0->expressionIndex,
											   reg1->expressionIndex,
											   rm->scale ? rm->scale->expressionIndex : 0,
											   rm->index ? rm->index->expressionIndex : 0,
											   rm->base ? rm->base->expressionIndex : 0,
											   rm->displacement ? rm->displacement->expressionIndex : 0,
											   {p, p+opcodeLength}));
	}
	
	if(imm) AppendImmediateOperand(listAction, encodingMatchMasks[3], imm);
}

static void EncodeRVLI(Assembler &assembler,
					   ListAction& listAction,
					   const Instruction& instruction,
					   const EncodingVariant &encodingVariant,
					   const uint64_t *encodingMatchMasks,
					   const RegisterOperand *reg0,
					   const RegisterOperand *reg1,
					   const LabelOperand *labelOperand,
					   const ImmediateOperand *imm)
{
	assert(reg0 && reg0->type == Operand::Type::Register);
	assert(reg1 && reg1->type == Operand::Type::Register);
	assert(labelOperand && labelOperand->type == Operand::Type::Label);
	
	// Encoding is:
	//   | [VEX2|VEX3] | Opcodes | ModR/M | [Displacement] | [Immediate]

	VEX3B2 vex3B2 = { .value = 0 };
	VEX3B3 vex3B3 = { .value = 0 };
	ModRM modRM = { .value = 0 };
	
	modRM.mod = ModRM::Mod::IndirectDisplacement0;	// actually displacement 4 for special case of RIP
	modRM.reg = reg0->index;
	modRM.rm = kModRMRipRelative;

	if(reg0->index & 8) vex3B2.r = 1;
	vex3B3.vvvv = reg1->index;
	if(encodingVariant.operationWidth == 256) vex3B3.l = 1;
	
	vex3B3.w = (encodingVariant.opcodeData & 0x10) ? 1 : 0;
	vex3B3.pp = encodingVariant.GetAvxPrefixValue();
	
	const uint8_t *p;
	uint32_t opcodeLength;
	vex3B2.mmmmm = encodingVariant.GetAvxMmValue(p, opcodeLength);

	if(reg0->IsExpression() || reg1->IsExpression())
	{
		listAction.Append(new DynamicOpcodeRV(vex3B2.value,
											  vex3B3.value,
											  modRM.value,
											  reg0->expressionIndex,
											  reg1->expressionIndex,
											  {p, p + opcodeLength}));
	}
	else
	{
		std::vector<uint8_t> bytes;
		if(RequiresVEX3(vex3B2, vex3B3))
		{
			// 3 byte VEX required.
			bytes.push_back(kOpcodeVEX3);
			bytes.push_back(vex3B2.value ^ 0xe0);
			bytes.push_back(vex3B3.value ^ 0x78);
		}
		else
		{
			bytes.push_back(kOpcodeVEX2);
			bytes.push_back(vex3B2.value ^ vex3B3.value ^ 0xf9);
		}
		bytes.insert(bytes.end(), p, p + opcodeLength);
		bytes.push_back(modRM.value);
		listAction.Append(new LiteralAction(bytes));
	}

	std::vector<uint8_t> bytes;
	uint8_t offsetBytes[4] = {};
	if(labelOperand->displacement)
	{
		memcpy(offsetBytes, &labelOperand->displacement->value, 4);
	}
	bytes.insert(bytes.end(), offsetBytes, offsetBytes+4);
	listAction.Append(new LiteralAction(bytes));

	if(labelOperand->displacement && labelOperand->displacement->IsExpression())
	{
		assembler.UpdateExpressionBitWidth(labelOperand->displacement->expressionIndex, 32);
		listAction.Append(new AddExpressionAction(4, labelOperand->displacement->expressionIndex));
	}

	int immediateWidth = 0;
	if(imm) immediateWidth = AppendImmediateOperand(listAction, encodingMatchMasks[3], imm);
	
	int patchBytes = 4;
	Action *patchAction = labelOperand->CreatePatchAction(patchBytes, patchBytes+immediateWidth);
	
	listAction.Append(patchAction);
}

static void Encode_R_V_RML_I(Assembler &assembler,
							 ListAction& listAction,
							 const Instruction& instruction,
							 const EncodingVariant &encodingVariant,
							 const uint64_t *encodingMatchMasks,
							 const RegisterOperand *reg0,
							 const RegisterOperand *reg1,
							 const Operand *rm,
							 const ImmediateOperand *imm)
{
	assert(reg0 && reg0->type == Operand::Type::Register);
	assert(reg1 && reg1->type == Operand::Type::Register);
	assert(rm);
	assert(rm->type == Operand::Type::Register
		   || rm->type == Operand::Type::Memory
		   || rm->type == Operand::Type::Label);
	
	switch(rm->type)
	{
	case Operand::Type::Register:
		EncodeRVRI(assembler,
				   listAction,
				   instruction,
				   encodingVariant,
				   encodingMatchMasks,
				   reg0,
				   reg1,
				   (const RegisterOperand*) rm,
				   imm);
		break;
	case Operand::Type::Memory:
		EncodeRVMI(assembler,
				   listAction,
				   instruction,
				   encodingVariant,
				   encodingMatchMasks,
				   reg0,
				   reg1,
				   (const MemoryOperand*) rm,
				   imm);
		break;
	case Operand::Type::Label:
		EncodeRVLI(assembler,
				   listAction,
				   instruction,
				   encodingVariant,
				   encodingMatchMasks,
				   reg0,
				   reg1,
				   (const LabelOperand*) rm,
				   imm);
		break;
	default:
		assert(!"Unexpected behavior");
		break;
	}
}

static void EncodeDirectMemory(Assembler &assembler,
							   ListAction& listAction,
							   const Instruction& instruction,
							   const EncodingVariant &encodingVariant,
							   const RegisterOperand *reg,
							   const ImmediateOperand *imm)
{
	assert(reg && reg->type == Operand::Type::Register);
	
	if(encodingVariant.opcodePrefix)
	{
		listAction.Append(new LiteralAction({encodingVariant.opcodePrefix}));
	}
	
	REX rex = { .value = 0 };
	ProcessOperandWidth(listAction, instruction, encodingVariant, rex);
	
	if(reg->IsExpression())
	{
		throw AssemblerException("FD Encoding (Direct memory) cannot use a register expression target");
	}
	
	if(reg->RequiresREX()) rex.constant4 = 4;
	
	std::vector<uint8_t> bytes;
	if(rex.value)
	{
		bytes.push_back(rex.value);
	}
	bytes.insert(bytes.end(),
				 encodingVariant.opcodes,
				 encodingVariant.opcodes+encodingVariant.opcodeLength);
	listAction.Append(new LiteralAction(bytes));
	
	if(!imm)
	{
		uint8_t v[8] = { };
		memcpy(&v, &imm->value, sizeof(v));
		listAction.Append(new LiteralAction({v, v+sizeof(v)}));
	}
	else if(imm->IsExpression())
	{
		assembler.UpdateExpressionBitWidth(imm->expressionIndex, 64);
		listAction.Append(new ExpressionAction(8, imm->expressionIndex));
	}
	else
	{
		uint8_t v[8];
		memcpy(&v, &imm->value, sizeof(v));
		listAction.Append(new LiteralAction({v, v+sizeof(v)}));
	}
}

static void EncodeOI(Assembler &assembler,
					 ListAction& listAction,
					 const Instruction& instruction,
					 const EncodingVariant &encodingVariant,
					 const uint64_t *encodingMatchMasks,
					 const RegisterOperand *reg,
					 const ImmediateOperand *imm)
{
	assert(reg && reg->type == Operand::Type::Register);
	
	if(encodingVariant.opcodePrefix)
	{
		listAction.Append(new LiteralAction({encodingVariant.opcodePrefix}));
	}

	REX rex = { .value = 0 };
	ProcessOperandWidth(listAction, instruction, encodingVariant, rex);

	if(reg->IsExpression())
	{
		listAction.Append(new DynamicOpcodeO(rex.value,
											 reg->expressionIndex,
											 encodingVariant.GetOpcodeVector()));
	}
	else
	{
		if(reg->RequiresREX()) rex.constant4 = 4;
		if(reg->index & 8) rex.b = 1;
		
		std::vector<uint8_t> bytes;
		if(rex.value)
		{
			if(reg->IsHighByte()) throw AssemblerException("High byte registers cannot be used with REX");
			bytes.push_back(rex.value);
		}
		bytes.insert(bytes.end(),
					 encodingVariant.opcodes,
					 encodingVariant.opcodes+encodingVariant.opcodeLength);
		bytes.back() += reg->index & 7;
		listAction.Append(new LiteralAction(bytes));
	}
	
	if(imm) AppendImmediateOperand(listAction, encodingMatchMasks[1], imm);
}

static void EncodeD_Label(Assembler &assembler,
						  ListAction& listAction,
						  const Instruction& instruction,
						  const EncodingVariant &encodingVariant,
						  const LabelOperand *labelOperand)
{
	assert(labelOperand->type == Operand::Type::Label);
	
	std::vector<uint8_t> bytes = encodingVariant.GetOpcodeVector();
	
	if(encodingVariant.opcodePrefix)
	{
		bytes.push_back(encodingVariant.opcodePrefix);
	}

	uint64_t lastOperandMatchMask = encodingVariant.GetLastOperandMatchMask();
	MatchBitIndex matchBitIndex = (MatchBitIndex) __builtin_ctzll(lastOperandMatchMask);
	
	int8_t patchBytes = 0;
	switch(matchBitIndex)
	{
	case MatchBitIndex::Rel8:
		bytes.push_back(0);	// Dummy
		patchBytes = 1;
		break;
	case MatchBitIndex::Rel32:
		bytes.push_back(0);	// Dummy.
		bytes.push_back(0);
		bytes.push_back(0);
		bytes.push_back(0);
		patchBytes = 4;
		break;
	default:
		throw AssemblerException("Internal error: unexpected relative size");
	}
	Action *patchAction = labelOperand->CreatePatchAction(patchBytes, patchBytes);
	
	listAction.Append(new LiteralAction(bytes));
	listAction.Append(patchAction);
}

static void EncodeD_ImmExpression(Assembler &assembler,
								  ListAction& listAction,
								  const Instruction& instruction,
								  const EncodingVariant &encodingVariant,
								  const ImmediateOperand *immOperand)
{
	assert(immOperand->type == Operand::Type::Immediate
		   && immOperand->IsExpression());
	
	std::vector<uint8_t> bytes = encodingVariant.GetOpcodeVector();

	if(encodingVariant.opcodePrefix)
	{
		bytes.push_back(encodingVariant.opcodePrefix);
	}

	uint64_t lastOperandMatchMask = encodingVariant.GetLastOperandMatchMask();
	MatchBitIndex matchBitIndex = (MatchBitIndex) __builtin_ctzll(lastOperandMatchMask);
	
	int8_t patchBytes = 0;
	switch(matchBitIndex)
	{
	case MatchBitIndex::Rel8:
		bytes.push_back(0);	// Dummy
		patchBytes = 1;
		break;
	case MatchBitIndex::Rel32:
		bytes.push_back(0);	// Dummy.
		bytes.push_back(0);
		bytes.push_back(0);
		bytes.push_back(0);
		patchBytes = 4;
		break;
	default:
		throw AssemblerException("Internal error: unexpected relative size");
	}
	listAction.Append(new LiteralAction(bytes));

	assembler.UpdateExpressionBitWidth(immOperand->expressionIndex, 32);
	Action *patchAction = new DeltaExpressionAction(patchBytes, immOperand->expressionIndex);
	listAction.Append(patchAction);
}

//===========================================================================

namespace Javelin::Assembler::x64::Encoders
{
	static void RMI(Assembler &assembler,
					ListAction& listAction,
					const Instruction& instruction,
					const EncodingVariant &encodingVariant,
					int numberOfOperands,
					const Operand *const *operands)
	{
		assert(numberOfOperands == 3);
		assert(operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Register
			   || operands[1]->type == Operand::Type::Memory
			   || operands[1]->type == Operand::Type::Label);
		assert(operands[2]->type == Operand::Type::Immediate);
		
		EncodeR_RM_I(assembler,
					 listAction,
					 instruction,
					 encodingVariant,
					 encodingVariant.operandMatchMasks,
					 (const RegisterOperand*) operands[0],
					 operands[1],
					 (const ImmediateOperand*) operands[2]);
	}

	static void RMX(Assembler &assembler,
					ListAction& listAction,
					const Instruction& instruction,
					const EncodingVariant &encodingVariant,
					int numberOfOperands,
					const Operand *const *operands)
	{
		assert(numberOfOperands == 2);
		assert(operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Register
			   || operands[1]->type == Operand::Type::Memory
			   || operands[1]->type == Operand::Type::Label);

		ImmediateOperand imm((int64_t)encodingVariant.opcodeData);
		imm.matchBitfield = MatchImm8;
		
		uint64_t matchMasks[3] =
		{
			encodingVariant.operandMatchMasks[0],
			encodingVariant.operandMatchMasks[1],
			MatchImm8,
		};

		EncodeR_RM_I(assembler,
					 listAction,
					 instruction,
					 encodingVariant,
					 matchMasks,
					 (const RegisterOperand*) operands[0],
					 operands[1],
					 &imm);
	}
	
	static void MRI(Assembler &assembler,
					ListAction& listAction,
					const Instruction& instruction,
					const EncodingVariant &encodingVariant,
					int numberOfOperands,
					const Operand *const *operands)
	{
		assert(numberOfOperands == 3);
		assert(operands[0]->type == Operand::Type::Register
			   || operands[0]->type == Operand::Type::Memory
			   || operands[0]->type == Operand::Type::Label);
		assert(operands[1]->type == Operand::Type::Register);
		assert(operands[2]->type == Operand::Type::Immediate);
		
		EncodeR_RM_I(assembler,
					 listAction,
					 instruction,
					 encodingVariant,
					 encodingVariant.operandMatchMasks,
					 (const RegisterOperand*) operands[1],
					 operands[0],
					 (const ImmediateOperand*) operands[2]);
	}

	static void MR(Assembler &assembler,
				   ListAction& listAction,
				   const Instruction& instruction,
				   const EncodingVariant &encodingVariant,
				   int numberOfOperands,
				   const Operand *const *operands)
	{
		assert(numberOfOperands >= 2);		// shld mr, reg, cl
		assert(operands[0]->type == Operand::Type::Register
			   || operands[0]->type == Operand::Type::Memory
			   || operands[0]->type == Operand::Type::Label);
		assert(operands[1]->type == Operand::Type::Register);

		uint64_t matchMasks[2] = { encodingVariant.operandMatchMasks[1], encodingVariant.operandMatchMasks[0] };
		
		EncodeR_RM_I(assembler,
					 listAction,
					 instruction,
					 encodingVariant,
					 matchMasks,
					 (const RegisterOperand*) operands[1],
					 operands[0],
					 nullptr);
	}

	static void RM(Assembler &assembler,
				   ListAction& listAction,
				   const Instruction& instruction,
				   const EncodingVariant &encodingVariant,
				   int numberOfOperands,
				   const Operand *const *operands)
	{
		assert(numberOfOperands == 2);
		assert(operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Register
			   || operands[1]->type == Operand::Type::Memory
			   || operands[1]->type == Operand::Type::Label);

		EncodeR_RM_I(assembler,
					 listAction,
					 instruction,
					 encodingVariant,
					 encodingVariant.operandMatchMasks,
					 (const RegisterOperand*) operands[0],
					 operands[1],
					 nullptr);
	}

	static void M(Assembler &assembler,
				  ListAction& listAction,
				  const Instruction& instruction,
				  const EncodingVariant &encodingVariant,
				  int numberOfOperands,
				  const Operand *const *operands)
	{
		assert(numberOfOperands >= 1);	// shl rm, cl
		assert(operands[0]->type == Operand::Type::Register
			   || operands[0]->type == Operand::Type::Memory
			   || operands[0]->type == Operand::Type::Label);
		
		const RegisterOperand reg(encodingVariant.opcodeData, 0);

		uint64_t matchMasks[2] =
		{
			encodingVariant.operandMatchMasks[0],
			encodingVariant.operandMatchMasks[0]
		};
		
		EncodeR_RM_I(assembler,
					 listAction,
					 instruction,
					 encodingVariant,
					 matchMasks,
					 &reg,
					 operands[0],
					 nullptr);
	}

	static void MI(Assembler &assembler,
				   ListAction& listAction,
				   const Instruction &instruction,
				   const EncodingVariant &encodingVariant,
				   int numberOfOperands,
				   const Operand *const *operands)
	{
		assert(numberOfOperands == 2);
		assert(operands[0]->type == Operand::Type::Register
			   || operands[0]->type == Operand::Type::Memory
			   || operands[0]->type == Operand::Type::Label);
		assert(operands[1]->type == Operand::Type::Immediate);

		const RegisterOperand reg(encodingVariant.opcodeData, 0);

		uint64_t matchMasks[3] =
		{
			encodingVariant.operandMatchMasks[0],
			encodingVariant.operandMatchMasks[0],
			encodingVariant.operandMatchMasks[1]
		};

		EncodeR_RM_I(assembler,
					 listAction,
					 instruction,
					 encodingVariant,
					 matchMasks,
					 &reg,
					 operands[0],
					 (ImmediateOperand*)operands[1]);
	}

	static void FD(Assembler &assembler,
				   ListAction& listAction,
				   const Instruction& instruction,
				   const EncodingVariant &encodingVariant,
				   int numberOfOperands,
				   const Operand *const *operands)
	{
		assert(numberOfOperands == 2);
		assert(operands[1]->type == Operand::Type::Memory);
		const MemoryOperand *memoryOperand = (MemoryOperand *) operands[1];
		assert(memoryOperand->IsDirectMemoryAdddress());
		
		EncodeDirectMemory(assembler,
						   listAction,
						   instruction,
						   encodingVariant,
						   (RegisterOperand*) operands[0],
						   memoryOperand->displacement);
	}
	
	static void TD(Assembler &assembler,
				   ListAction& listAction,
				   const Instruction& instruction,
				   const EncodingVariant &encodingVariant,
				   int numberOfOperands,
				   const Operand *const *operands)
	{
		assert(numberOfOperands == 2);
		assert(operands[0]->type == Operand::Type::Memory);
		const MemoryOperand *memoryOperand = (MemoryOperand *) operands[0];
		assert(memoryOperand->IsDirectMemoryAdddress());
		
		EncodeDirectMemory(assembler,
						   listAction,
						   instruction,
						   encodingVariant,
						   (RegisterOperand*) operands[1],
						   memoryOperand->displacement);
	}
	
	static void I(Assembler &assembler,
				  ListAction& listAction,
				  const Instruction& instruction,
				  const EncodingVariant &encodingVariant,
				  int numberOfOperands,
				  const Operand *const *operands)
	{
		assert(numberOfOperands >= 1);
		const ImmediateOperand *immediateOperand = (ImmediateOperand *) operands[numberOfOperands-1];
		assert(immediateOperand->type == Operand::Type::Immediate);

		listAction.Append(new LiteralAction(encodingVariant.GetOpcodeVector()));

		uint64_t lastOperandMatchMask = encodingVariant.GetLastOperandMatchMask();
		AppendImmediateOperand(listAction, lastOperandMatchMask, immediateOperand);
	}

	static void II(Assembler &assembler,
				   ListAction& listAction,
				   const Instruction& instruction,
				   const EncodingVariant &encodingVariant,
				   int numberOfOperands,
				   const Operand *const *operands)
	{
		assert(numberOfOperands >= 2);
		assert(operands[0]->type == Operand::Type::Immediate);
		assert(operands[1]->type == Operand::Type::Immediate);
		
		if(encodingVariant.opcodePrefix)
		{
			listAction.Append(new LiteralAction({encodingVariant.opcodePrefix}));
		}

		listAction.Append(new LiteralAction(encodingVariant.GetOpcodeVector()));
		AppendImmediateOperand(listAction, encodingVariant.operandMatchMasks[0], (ImmediateOperand *) operands[0]);
		AppendImmediateOperand(listAction, encodingVariant.operandMatchMasks[1], (ImmediateOperand *) operands[1]);
	}

	static void OI(Assembler &assembler,
				   ListAction& listAction,
				   const Instruction& instruction,
				   const EncodingVariant &encodingVariant,
				   int numberOfOperands,
				   const Operand *const *operands)
	{
		assert(numberOfOperands == 2);
		assert(operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Immediate);
		
		EncodeOI(assembler,
				 listAction,
				 instruction,
				 encodingVariant,
				 encodingVariant.operandMatchMasks,
				 (const RegisterOperand*) operands[0],
				 (const ImmediateOperand*) operands[1]);
	}

	static void O(Assembler &assembler,
				  ListAction& listAction,
				  const Instruction& instruction,
				  const EncodingVariant &encodingVariant,
				  int numberOfOperands,
				  const Operand *const *operands)
	{
		assert(numberOfOperands >= 1);
		assert(operands[0]->type == Operand::Type::Register);
		
		EncodeOI(assembler,
				 listAction,
				 instruction,
				 encodingVariant,
				 encodingVariant.operandMatchMasks,
				 (const RegisterOperand*) operands[0],
				 nullptr);
	}

	static void O2(Assembler &assembler,
				   ListAction& listAction,
				   const Instruction& instruction,
				   const EncodingVariant &encodingVariant,
				   int numberOfOperands,
				   const Operand *const *operands)
	{
		assert(numberOfOperands == 2);
		
		EncodeOI(assembler,
				 listAction,
				 instruction,
				 encodingVariant,
				 &encodingVariant.operandMatchMasks[1],
				 (const RegisterOperand*) operands[1],
				 nullptr);
	}

	static void ZO(Assembler &assembler,
				  ListAction& listAction,
				  const Instruction& instruction,
				  const EncodingVariant &encodingVariant,
				  int numberOfOperands,
				  const Operand *const *operands)
	{
		assert(encodingVariant.opcodePrefix == 0);
		listAction.Append(new LiteralAction(encodingVariant.GetOpcodeVector()));
	}

	static void D(Assembler &assembler,
				  ListAction& listAction,
				  const Instruction& instruction,
				  const EncodingVariant &encodingVariant,
				  int numberOfOperands,
				  const Operand *const *operands)
	{
		assert(numberOfOperands > 0);
		const Operand* lastOperand = operands[numberOfOperands-1];
		if(lastOperand->type == Operand::Type::Label)
		{
			EncodeD_Label(assembler, listAction, instruction, encodingVariant, (LabelOperand*) lastOperand);
		}
		else if(lastOperand->type == Operand::Type::Immediate &&
				lastOperand->IsExpression())
		{
			assembler.UpdateExpressionBitWidth(lastOperand->expressionIndex, 64);
			EncodeD_ImmExpression(assembler, listAction, instruction, encodingVariant, (ImmediateOperand*) lastOperand);
		}
		else
		{
			assert(!"Invalid operand for D encoding");
		}
	}

	static void RVM(Assembler &assembler,
					ListAction& listAction,
					const Instruction& instruction,
					const EncodingVariant &encodingVariant,
					int numberOfOperands,
					const Operand *const *operands)
	{
		assert(numberOfOperands >= 3);
		assert(operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Register);
		assert(operands[2]->type == Operand::Type::Register
			   || operands[2]->type == Operand::Type::Memory
			   || operands[2]->type == Operand::Type::Label);

		Encode_R_V_RML_I(assembler,
						 listAction,
						 instruction,
						 encodingVariant,
						 encodingVariant.operandMatchMasks,
						 (const RegisterOperand *)operands[0],
						 (const RegisterOperand *)operands[1],
						 operands[2],
						 nullptr);
	}
	
	static void RVMI(Assembler &assembler,
					 ListAction& listAction,
					 const Instruction& instruction,
					 const EncodingVariant &encodingVariant,
					 int numberOfOperands,
					 const Operand *const *operands)
	{
		assert(numberOfOperands >= 4);
		assert(operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Register);
		assert(operands[2]->type == Operand::Type::Register
			   || operands[2]->type == Operand::Type::Memory
			   || operands[2]->type == Operand::Type::Label);
		assert(operands[3]->type == Operand::Type::Immediate);

		Encode_R_V_RML_I(assembler,
						 listAction,
						 instruction,
						 encodingVariant,
						 encodingVariant.operandMatchMasks,
						 (const RegisterOperand *)operands[0],
						 (const RegisterOperand *)operands[1],
						 operands[2],
						 (const ImmediateOperand *)operands[3]);
	}
	
	static void RMV(Assembler &assembler,
					ListAction& listAction,
					const Instruction& instruction,
					const EncodingVariant &encodingVariant,
					int numberOfOperands,
					const Operand *const *operands)
	{
		assert(numberOfOperands >= 3);
		assert(operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Register
			   || operands[1]->type == Operand::Type::Memory
			   || operands[1]->type == Operand::Type::Label);
		assert(operands[2]->type == Operand::Type::Register);

		uint64_t matchMasks[3] =
		{
			encodingVariant.operandMatchMasks[0],
			encodingVariant.operandMatchMasks[2],
			encodingVariant.operandMatchMasks[1],
		};

		Encode_R_V_RML_I(assembler,
						 listAction,
						 instruction,
						 encodingVariant,
						 matchMasks,
						 (const RegisterOperand *)operands[0],
						 (const RegisterOperand *)operands[2],
						 operands[1],
						 nullptr);
	}
	
	static void VM(Assembler &assembler,
				   ListAction& listAction,
				   const Instruction& instruction,
				   const EncodingVariant &encodingVariant,
				   int numberOfOperands,
				   const Operand *const *operands)
	{
		assert(numberOfOperands >= 2);
		assert(operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Register
			   || operands[1]->type == Operand::Type::Memory
			   || operands[1]->type == Operand::Type::Label);


		const RegisterOperand reg(encodingVariant.opcodeData & 0xf, 0);
		
		uint64_t matchMasks[3] =
		{
			encodingVariant.operandMatchMasks[0],
			encodingVariant.operandMatchMasks[0],
			encodingVariant.operandMatchMasks[1]
		};

		Encode_R_V_RML_I(assembler,
						 listAction,
						 instruction,
						 encodingVariant,
						 matchMasks,
						 &reg,
						 (const RegisterOperand *)operands[0],
						 operands[1],
						 nullptr);
	}
	
	static void vRMI(Assembler &assembler,
					ListAction& listAction,
					const Instruction& instruction,
					const EncodingVariant &encodingVariant,
					int numberOfOperands,
					const Operand *const *operands)
	{
		assert(numberOfOperands >= 3);
		assert(operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Register
			   || operands[1]->type == Operand::Type::Memory
			   || operands[1]->type == Operand::Type::Label);
		assert(operands[2]->type == Operand::Type::Immediate);

		const RegisterOperand reg(encodingVariant.opcodeData & 0xf, 0);

		uint64_t matchMasks[4] =
		{
			encodingVariant.operandMatchMasks[0],
			0,
			encodingVariant.operandMatchMasks[1],
			encodingVariant.operandMatchMasks[2]
		};

		Encode_R_V_RML_I(assembler,
						 listAction,
						 instruction,
						 encodingVariant,
						 matchMasks,
						 (const RegisterOperand *)operands[0],
						 &reg,
						 operands[1],
						 (const ImmediateOperand *)operands[2]);
	}
	
	static void vRM(Assembler &assembler,
					ListAction& listAction,
					const Instruction& instruction,
					const EncodingVariant &encodingVariant,
					int numberOfOperands,
					const Operand *const *operands)
	{
		assert(numberOfOperands >= 2);
		assert(operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Register
			   || operands[1]->type == Operand::Type::Memory
			   || operands[1]->type == Operand::Type::Label);

		const RegisterOperand reg(encodingVariant.opcodeData & 0xf, 0);

		uint64_t matchMasks[4] =
		{
			encodingVariant.operandMatchMasks[0],
			0,
			encodingVariant.operandMatchMasks[1],
			0,
		};

		Encode_R_V_RML_I(assembler,
						 listAction,
						 instruction,
						 encodingVariant,
						 matchMasks,
						 (const RegisterOperand *)operands[0],
						 &reg,
						 operands[1],
						 nullptr);
	}
	
	static void vMR(Assembler &assembler,
					ListAction& listAction,
					const Instruction& instruction,
					const EncodingVariant &encodingVariant,
					int numberOfOperands,
					const Operand *const *operands)
	{
		assert(numberOfOperands >= 2);
		assert(operands[0]->type == Operand::Type::Register
			   || operands[0]->type == Operand::Type::Memory
			   || operands[0]->type == Operand::Type::Label);
		assert(operands[1]->type == Operand::Type::Register);

		const RegisterOperand reg(encodingVariant.opcodeData & 0xf, 0);

		uint64_t matchMasks[4] =
		{
			encodingVariant.operandMatchMasks[1],
			0,
			encodingVariant.operandMatchMasks[0],
			0,
		};

		Encode_R_V_RML_I(assembler,
						 listAction,
						 instruction,
						 encodingVariant,
						 matchMasks,
						 (const RegisterOperand *)operands[1],
						 &reg,
						 operands[0],
						 nullptr);
	}

	static void VMI(Assembler &assembler,
					ListAction& listAction,
					const Instruction& instruction,
					const EncodingVariant &encodingVariant,
					int numberOfOperands,
					const Operand *const *operands)
	{
		assert(numberOfOperands >= 3);
		assert(operands[0]->type == Operand::Type::Register);
		assert(operands[1]->type == Operand::Type::Register
			   || operands[1]->type == Operand::Type::Memory
			   || operands[1]->type == Operand::Type::Label);
		assert(operands[2]->type == Operand::Type::Immediate);

		const RegisterOperand reg(encodingVariant.opcodeData & 0xf, 0);

		uint64_t matchMasks[4] =
		{
			encodingVariant.operandMatchMasks[0],
			encodingVariant.operandMatchMasks[0],
			encodingVariant.operandMatchMasks[1],
			encodingVariant.operandMatchMasks[2]
		};

		Encode_R_V_RML_I(assembler,
						 listAction,
						 instruction,
						 encodingVariant,
						 matchMasks,
						 &reg,
						 (const RegisterOperand *)operands[0],
						 operands[1],
						 (const ImmediateOperand *)operands[2]);
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
