//============================================================================

#if defined(__arm64__)

//============================================================================

#include "Javelin/Assembler/arm64/Assembler.h"

#include "Javelin/Assembler/JitMemoryManager.h"
#include "Javelin/Assembler/x64/ActionType.h"
#include <algorithm>
#include <stdint.h>

//============================================================================

#if DEBUG
  #define USE_GOTO_LABELS	0
#else
  #define USE_GOTO_LABELS	1
#endif

#define USE_OPTIMIZED_APPEND_INSTRUCTION_DATA	1

//============================================================================

using namespace Javelin;
using namespace Javelin::arm64Assembler;

//============================================================================

static int64_t cls(int64_t v)
{
	int64_t result;
	asm("cls %0, %1" : "=r"(result) : "r"(v));
	return result;
}

//============================================================================

SegmentAssembler::SegmentAssembler(JitMemoryManager &aMemoryManager)
: memoryManager(aMemoryManager)
{
	buildData.Reserve(8192);
}

//============================================================================

#if USE_OPTIMIZED_APPEND_INSTRUCTION_DATA
__attribute__((naked))
void* SegmentAssembler::AppendInstructionData(uint32_t blockByteCodeSize,
											  const uint8_t *s,
											  uint32_t referenceAndDataLength,
											  uint32_t labelData)
{
	// Define JIT_OFFSETOF to avoid compiler warnings on offsetof()
	#define JIT_OFFSETOF(t,f)    (size_t(&((t*)64)->f) - 64)

	asm volatile(".global __ZN7Javelin16SegmentAssembler21AppendInstructionDataEjPKhj");

	// Update numberOfLabels, numberOfForwardLabelReferences
	asm volatile("ldp w6, w7, [x0, %0]" : : "i"(JIT_OFFSETOF(Assembler, aggregateData.numberOfLabels)));
	asm volatile("add w6, w6, w4, uxtb");
	asm volatile("add w7, w7, w4, lsr #8");
	asm volatile("stp w6, w7, [x0, %0]" : : "i"(JIT_OFFSETOF(Assembler, aggregateData.numberOfLabels)));
	asm volatile("b __ZN7Javelin16SegmentAssembler21AppendInstructionDataEjPKhj");

	// Definition of AppendInstructionData(blockByteCodeSize, s);
	asm volatile(".global __ZN7Javelin16SegmentAssembler21AppendInstructionDataEjPKh");
	asm volatile("__ZN7Javelin16SegmentAssembler21AppendInstructionDataEjPKh:");
	asm volatile("mov w3, %0" : : "i"(sizeof(AppendAssemblyReference)));
	
	// Definition for AppendInstructionData(blockByteCodeSize, s, referenceAndDataLength);
	asm volatile("__ZN7Javelin16SegmentAssembler21AppendInstructionDataEjPKhj:");
	
	// Update byteCodeSize
	asm volatile("ldr w5, [x0, %0]" : : "i"(JIT_OFFSETOF(Assembler, aggregateData.byteCodeSize)));
	asm volatile("add w5, w5, w1");
	asm volatile("str w5, [x0, %0]" : : "i"(JIT_OFFSETOF(Assembler, aggregateData.byteCodeSize)));
	
	// buildData.Append, x5 = offset, x6 = capacity.
	asm volatile("ldp w5, w6, [x0, %0]" : : "i"(JIT_OFFSETOF(Assembler, buildData)));
	asm volatile("add w7, w5, w3");
	asm volatile("cmp w7, w6");
	asm volatile("b.hi 1f");
	asm volatile("str w7, [x0, %0]" : : "i"(JIT_OFFSETOF(Assembler, buildData)));
	asm volatile("ldr x0, [x0, %0]" : : "i"(JIT_OFFSETOF(Assembler, buildData.data)));
	asm volatile("add x0, x0, x5");
	
	// Write referenceSize and assemblerData
	asm volatile("stp x2, x3, [x0]");
	asm volatile("ret");

	asm volatile("1:");
	// Update offset and capacity
	asm volatile("add w1, w7, w7");
	asm volatile("stp w7, w1, [x0, %0]" : : "i"(JIT_OFFSETOF(Assembler, buildData)));
	
	//  Update data (inline of JitVectorBase::ExpandAndAppend
	asm volatile("stp x2, lr, [sp, #-48]!");
	asm volatile("stp x0, x3, [sp, #16]");
	asm volatile("str x5, [sp, #32]");
	asm volatile("ldr x0, [x0, %0]" : : "i"(JIT_OFFSETOF(Assembler, buildData.data)));
	
	asm volatile("bl _realloc");
	asm volatile("ldr x5, [sp, #32]");
	asm volatile("ldp x7, x3, [sp, #16]");
	asm volatile("ldp x2, lr, [sp], #48");
	asm volatile("str x0, [x7, %0]" : : "i"(JIT_OFFSETOF(Assembler, buildData.data)));
	asm volatile("add x0, x0, x5");
	
	// Write referenceSize and assemblerData
	asm volatile("stp x2, x3, [x0]");
	asm volatile("ret");
}

void* (SegmentAssembler::*volatile reference0)(uint32_t, const uint8_t*, uint32_t, uint32_t);
//void* (SegmentAssembler::*volatile reference1)(uint32_t, const uint8_t*, uint32_t);
//void (SegmentAssembler::*volatile reference2)(uint32_t, const uint8_t*);

__attribute__((constructor)) static void EnsureLinkage()
{
	reference0 = &SegmentAssembler::AppendInstructionData;
//	reference1 = &SegmentAssembler::AppendInstructionData;
//	reference2 = &SegmentAssembler::AppendInstructionData;
}

#else

void SegmentAssembler::AppendInstructionData(uint32_t blockByteCodeSize, const uint8_t *s)
{
	AppendInstructionData(blockByteCodeSize, s, sizeof(AppendAssemblyReference), 0);
}

void* SegmentAssembler::AppendInstructionData(uint32_t blockByteCodeSize,
											  const uint8_t *s,
											  uint32_t referenceAndDataLength)
{
	return AppendInstructionData(blockByteCodeSize, s, referenceAndDataLength, 0);
}

void* SegmentAssembler::AppendInstructionData(uint32_t blockByteCodeSize,
											  const uint8_t *s,
											  uint32_t referenceAndDataLength,
											  uint32_t labelData)
{
	aggregateData.byteCodeSize += blockByteCodeSize;
	ProcessLabelData(labelData);

	AppendAssemblyReference *reference = (AppendAssemblyReference*) buildData.Append(referenceAndDataLength);
	reference->referenceSize = referenceAndDataLength;
	reference->assemblerData = s;

	return reference;
}

void SegmentAssembler::ProcessLabelData(uint32_t labelData)
{
	int numberOfLabels = labelData & 0xff;
	int numberOfForwardLabelReferences = labelData >> 8;
	
	aggregateData.numberOfLabels += numberOfLabels;
	aggregateData.numberOfForwardLabelReferences += numberOfForwardLabelReferences;
}

#endif

void* SegmentAssembler::AppendData(uint32_t byteSize)
{
	static constexpr ActionType appendDataActions[2] =
	{
		ActionType::DataBlock,
		ActionType::Return,
	};
	static_assert(sizeof(AppendByteReference) == 16, "Expected AppendByteReference to be 16 bytes");
	
	uint32_t allocationSize = (sizeof(AppendByteReference) + byteSize + 7) & -8;
	AppendByteReference *reference = (AppendByteReference*) AppendInstructionData(byteSize,
																				  (const uint8_t*) &appendDataActions,
																				  allocationSize);

	reference->dataSize = byteSize;
	return reference + 1;
}

void SegmentAssembler::AppendDataPointer(const void *data, uint32_t byteSize)
{
	static constexpr ActionType appendDataActions[2] =
	{
		ActionType::DataPointer,
		ActionType::Return,
	};
	
	AppendDataPointerReference *reference =
		(AppendDataPointerReference*) AppendInstructionData(byteSize,
															(const uint8_t*) &appendDataActions,
															sizeof(AppendDataPointerReference));
	reference->dataSize = byteSize;
	reference->pData = (const uint8_t*) data;
}

//============================================================================

__attribute__((always_inline)) int32_t SegmentAssembler::ReadSigned16(const uint8_t* &s)
{
	int16_t result;
	memcpy(&result, s, sizeof(result));
	s += sizeof(result);
	return result;
}

__attribute__((always_inline)) uint32_t SegmentAssembler::ReadUnsigned16(const uint8_t* &s)
{
	uint16_t result;
	memcpy(&result, s, sizeof(result));
	s += sizeof(result);
	return result;
}

__attribute__((always_inline)) uint32_t SegmentAssembler::ReadUnsigned32(const uint8_t* &s)
{
	uint32_t result;
	memcpy(&result, s, sizeof(result));
	s += sizeof(result);
	return result;
}

uint32_t SegmentAssembler::LogicalOpcodeValue(uint64_t v)
{
	BitMaskEncodeResult result = EncodeBitMask(v);
	assert(result.size != 0 && "Unable to encode logical immediate");
	
	uint32_t opcodeValue = result.rotate << 16;
	if(result.size == 64) opcodeValue |= 1 << 22;
	
	uint32_t imms = ((0x1e << __builtin_ctz(result.size)) + result.length - 1) & 0x3f;
	opcodeValue |= imms << 10;
	return opcodeValue;
}

void SegmentAssembler::Patch(uint8_t *p, uint32_t encoding, int64_t delta)
{
	switch(encoding)
	{
	case 0:	// Rel26
		{
			uint32_t opcode;
			memcpy(&opcode, p, 4);
			assert((delta & 3) == 0);
			delta = opcode + (delta >> 2);
			opcode = (opcode & ~0x3ffffff) | (delta & 0x3ffffff);
			memcpy(p, &opcode, 4);
		}
		break;
	case 1:	// Rel19Offset5
		{
			uint32_t opcode;
			memcpy(&opcode, p, 4);
			assert((delta & 3) == 0);
			delta = opcode + (uint32_t(delta) << 3);
			opcode = (opcode & ~0xffffe0) | (delta & 0xffffe0);
			memcpy(p, &opcode, 4);
		}
		break;
	case 2: // Adrp
		{
			uint64_t current = uint64_t(p) >> 12;
			uint64_t target = uint64_t(p + delta) >> 12;
			delta = target - current;
		}
		[[fallthrough]];
	case 3: // Rel21HiLoSplit
		{
//			struct Opcode
//			{
//				uint32_t _dummy0 : 5;
//				uint32_t offsetHi : 19;
//				uint32_t _dummy24 : 5;
//				uint32_t offsetLo : 2;
//				uint32_t _dummy31 : 1;
//			};
//
//			Opcode opcode;
//			memcpy(&opcode, p, 4);
//
//			uint32_t rel = (opcode.offsetHi << 2) | opcode.offsetLo;
//			rel += delta;
//			opcode.offsetLo = rel;
//			opcode.offsetHi = rel >> 2;
//			memcpy(p, &opcode, 4);

			// The compiler does a poor job with the above code. It generates a
			// constant that is hoisted all the way to the start of
			// GenerateByteCode, which becomes overhead for every single call.
			// Attempts to use uint32_t with appropriate shift and masking still do
			// not result in the desired generated code.
			// -> Manually code it. It is both shorter and has less register pressure.
			uint32_t opcode;
			memcpy(&opcode, p, 4);
			uint32_t rel;
			asm volatile("sbfx %w1, %w0, #3, #21		\n\t"
						 "bfxil %w1, %w0, #29, #2		\n\t"
						 "add %w1, %w1, %w2				\n\t"
						 "bfi %w0, %w1, #29, #2			\n\t"
						 "lsr %w1, %w1, #2				\n\t"
						 "bfi %w0, %w1, #5, #19			\n\t"
						 : "+r"(opcode), "=&r"(rel)
						 : "r"(delta));
			memcpy(p, &opcode, 4);
		}
		break;
	case 4:	// Rel14Offset5
		{
			uint32_t opcode;
			memcpy(&opcode, p, 4);
			delta = opcode + (uint32_t(delta) << 3);
			opcode = (opcode & ~0x7ffe0) | (delta & 0x7ffe0);
			memcpy(p, &opcode, 4);
		}
		break;
	case 5: // Imm12
		{
			uint32_t opcode;
			memcpy(&opcode, p, 4);
			uint32_t rel = (opcode >> 10) & 0xfff;

			if(int64_t(p) + delta == 0) rel = int32_t(delta);
			else rel += (int64_t(p) + delta);
			opcode = (opcode & ~0x3ffc00) | ((rel << 10) & 0x3ffc00);
			memcpy(p, &opcode, 4);
		}
		break;
	case 6: // Rel64
		{
			int64_t rel;
			memcpy(&rel, p, 8);
			rel += delta;
			memcpy(p, &rel, 8);
		}
		break;
	default:
		__builtin_unreachable();
	}
}

//============================================================================

void SegmentAssembler::ProcessByteCode()
{
	programStart = (uint8_t*) memoryManager.Allocate(aggregateData.byteCodeSize+4);	// +4 is because some actions assume extra buffer.
	uint8_t *programEnd = GenerateByteCode(programStart);
	
	// Shrink allocation
	memoryManager.EndWrite(programStart);
	uint32_t codeSize = uint32_t(programEnd - programStart);
	assert(codeSize <= aggregateData.byteCodeSize);
	memoryManager.Shrink(programStart, codeSize);
}

/**
 * There is a lot of function call overhead in GenerateBytecode() because a call to memcpy
 * requires the compiler to preserve all registers.
 * InlineMemcpy means that no external calls are made, and the compiler can make more use
 * of scracth registers.
 */
__attribute__((always_inline))
static void InlineMemcpyAndAdvancePointers(uint8_t* &dest, const uint8_t* &source, uint64_t dataSize)
{
	int64_t scratch;
	asm volatile("  tst %1, #3				\n\t"
				 "  b.eq 2f					\n\t"
				 "1:						\n\t"
				 "  ldrb %w0, [%3], #1		\n\t"
				 "  strb %w0, [%2], #1		\n\t"
				 "  sub %1, %1, #1			\n\t"
				 "  tst %1, #3				\n\t"
				 "  b.ne 1b					\n\t"
				 "2:						\n\t"
				 "  tbz %1, #2, 1f			\n\t"
				 "  ldr %w0, [%3], #4		\n\t"
				 "  str %w0, [%2], #4		\n\t"
				 "  sub %1, %1, #4			\n\t"
				 "1:						\n\t"
				 "  tbz %1, #3, 1f			\n\t"
				 "  ldr %0, [%3], #8		\n\t"
				 "  str %0, [%2], #8		\n\t"
				 "  sub %1, %1, #8			\n\t"
				 "1:						\n\t"
				 "  tbz %1, #4, 1f			\n\t"
				 "  ldr q0, [%3], #16		\n\t"
				 "  str q0, [%2], #16		\n\t"
				 "  sub %1, %1, #16			\n\t"
				 "1:						\n\t"
				 "  cbz %1, 2f				\n\t"
				 "1:						\n\t"
				 "  ldp q0, q1, [%3], #32	\n\t"
				 "  stp q0, q1, [%2], #32	\n\t"
				 "  subs %1, %1, #32		\n\t"
				 "  b.ne 1b					\n\t"
				 "2:						\n\t"
				 : "=&r"(scratch), "+r"(dataSize), "+r"(dest), "+r"(source)
				 :
				 : "v0", "v1", "memory");
}

#if USE_GOTO_LABELS
	#define CASE(x) 		x
	#define CONTINUE1(x)	do { const uint8_t* pAction = s+x; void *jumpTarget = jumpOffsets[*pAction];
	#define CONTINUE2		assert(s == pAction); ++s; goto *jumpTarget; } while(0)
	#define CONTINUE 		goto *jumpOffsets[*s++]
#else
	#define CASE(x) 		case ActionType::x
	#define CONTINUE1(x)	{ const uint8_t* pAction = s+x;
	#define CONTINUE2		assert(s == pAction); continue; }
	#define CONTINUE 		continue
#endif

uint8_t *SegmentAssembler::GenerateByteCode(__restrict uint8_t* p)
{
#if USE_GOTO_LABELS
	static constexpr void *jumpOffsets[] =
	{
		#define TAG(x) &&x,
		#include "ActionTypeTags.h"
		#undef TAG
	};
#endif
	const AppendAssemblyReference *blockData = (AppendAssemblyReference*) buildData.begin();
	const AppendAssemblyReference *const blockDataEnd = (AppendAssemblyReference*) buildData.end();
	const __restrict uint8_t* s = blockData->assemblerData;
#if !USE_GOTO_LABELS
	uint32_t opcodeValue;
	for(;;)
	{
#endif
#if USE_GOTO_LABELS
		CONTINUE;
#else
		switch((ActionType) *s++)
#endif
		{
		CASE(Return):
			blockData = blockData->GetNext();
			if(blockData == blockDataEnd) return p;
			s = blockData->assemblerData;
			CONTINUE;
		CASE(Literal4):  CONTINUE1(4); memcpy(p, s, 4); p += 4; s += 4; CONTINUE2;
		CASE(Literal8):  CONTINUE1(8); memcpy(p, s, 8); p += 8; s += 8; CONTINUE2;
		CASE(Literal12): CONTINUE1(12); memcpy(p, s, 16); p += 12; s += 12; CONTINUE2;
		CASE(Literal16): CONTINUE1(16); memcpy(p, s, 16); p += 16; s += 16; CONTINUE2;
		CASE(Literal20): CONTINUE1(20); memcpy(p, s, 20); p += 20; s += 20; CONTINUE2;
		CASE(Literal24): CONTINUE1(24); memcpy(p, s, 24); p += 24; s += 24; CONTINUE2;
		CASE(Literal28): CONTINUE1(28); memcpy(p, s, 32); p += 28; s += 28; CONTINUE2;
		CASE(Literal32): CONTINUE1(32); memcpy(p, s, 32); p += 32; s += 32; CONTINUE2;
		CASE(LiteralBlock):
			{
				uint32_t length = ReadUnsigned16(s);
				InlineMemcpyAndAdvancePointers(p, s, length);
				CONTINUE;
			}
		CASE(Align):
			{
				CONTINUE1(1);
				uint8_t alignmentM1 = *s++;
				while(size_t(p) & 3) *p++ = 0;
				while(size_t(p) & alignmentM1)
				{
					const uint32_t opcode = 0xd503201f;
					memcpy(p, &opcode, 4);
					p += 4;
				}
				CONTINUE2;
			}
		CASE(Unalign):
			{
				CONTINUE1(1);
				uint8_t alignmentM1 = *s++;
				assert(alignmentM1 > 3);
				if(((size_t) p & alignmentM1) == 0)
				{
					const uint32_t opcode = 0xd503201f;
					memcpy(p, &opcode, 4);
					p += 4;
				}
				CONTINUE2;
			}
		CASE(Jump):
			s += *s + 1;
			CONTINUE;
#if USE_GOTO_LABELS
		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
			uint32_t opcodeValue;
#pragma clang diagnostic pop
#endif
#ifndef NDEBUG
		// Masked, unsigned and signed all have the same implementation in release mode,
		// If asserts are enabled, build 3 separate versions, where unsigned and signed
		// check the bounds of the provided expression.
		CASE(MaskedPatchB1Opcode):
			{
				int bitMask = *s++;
				int bitOffset = *s++;
				int valueShift = *s++;
				uint8_t value = ReadB1ExpressionValue(s, blockData);
				opcodeValue = ((value >> valueShift) & bitMask) << bitOffset;
				goto ProcessPatchOpcode;
			}
		CASE(UnsignedPatchB1Opcode):
			{
				int bitMask = *s++;
				int bitOffset = *s++;
				int valueShift = *s++;
				uint8_t value = ReadB1ExpressionValue(s, blockData);
				assert((value & ((1<<valueShift)-1)) == 0);
				assert((value & ~bitMask) == 0);
				opcodeValue = value >> valueShift << bitOffset;
				goto ProcessPatchOpcode;
			}
#else
		CASE(MaskedPatchB1Opcode):
		CASE(UnsignedPatchB1Opcode):
#endif
		CASE(SignedPatchB1Opcode):
			{
				int bitMask = *s++;
				int bitOffset = *s++;
				int valueShift = *s++;
				int32_t value = ReadB1ExpressionValue(s, blockData);
				assert((value & ((1<<valueShift)-1)) == 0);
				value >>= valueShift;
				assert((value & ~bitMask) == 0 || (value | bitMask) == -1);
				
				opcodeValue = (value & bitMask) << bitOffset;
				goto ProcessPatchOpcode;
			}
#ifndef NDEBUG
		CASE(MaskedPatchB2Opcode):
			{
				int numberOfBits = *s++;
				int bitOffset = *s++;
				int valueShift = *s++;
				int32_t value = ReadB2ExpressionValue(s, blockData);
				value >>= valueShift;
				uint32_t mask = (1 << numberOfBits) - 1;
				opcodeValue = (value & mask) << bitOffset;
				goto ProcessPatchOpcode;
			}
#else
		CASE(MaskedPatchB2Opcode):
#endif
		CASE(SignedPatchB2Opcode):
			{
				int numberOfBits = *s++;
				int bitOffset = *s++;
				int valueShift = *s++;
				int32_t value = ReadB2ExpressionValue(s, blockData);
				assert((value & ((1<<valueShift)-1)) == 0);
				value >>= valueShift;
				assert(value >> numberOfBits == 0 || value >> numberOfBits == -1);
				
				uint32_t mask = (1 << numberOfBits) - 1;
				opcodeValue = (value & mask) << bitOffset;
				goto ProcessPatchOpcode;
			}
#ifndef NDEBUG
		CASE(MaskedPatchB4Opcode):
			{
				int numberOfBits = *s++;
				int bitOffset = *s++;
				int valueShift = *s++;
				int32_t value = ReadB4ExpressionValue(s, blockData);
				value >>= valueShift;
				
				uint32_t mask = (1 << numberOfBits) - 1;
				opcodeValue = (value & mask) << bitOffset;
				goto ProcessPatchOpcode;
			}
#else
		CASE(MaskedPatchB4Opcode):
#endif
		CASE(SignedPatchB4Opcode):
			{
				int numberOfBits = *s++;
				int bitOffset = *s++;
				int valueShift = *s++;
				int32_t value = ReadB4ExpressionValue(s, blockData);
				assert((value & ((1<<valueShift)-1)) == 0);
				value >>= valueShift;
				assert(value >> numberOfBits == 0 || value >> numberOfBits == -1);
				
				uint32_t mask = (1 << numberOfBits) - 1;
				opcodeValue = (value & mask) << bitOffset;
				goto ProcessPatchOpcode;
			}
		CASE(UnsignedPatchB2Opcode):
			{
				int numberOfBits = *s++;
				int bitOffset = *s++;
				int valueShift = *s++;
				uint16_t value = ReadB2ExpressionValue(s, blockData);
				assert((value & ((1<<valueShift)-1)) == 0);
				assert(value >> (numberOfBits + valueShift) == 0);
				(void) numberOfBits;
				opcodeValue = value >> valueShift << bitOffset;
				goto ProcessPatchOpcode;
			}
		CASE(UnsignedPatchB4Opcode):
			{
				int numberOfBits = *s++;
				int bitOffset = *s++;
				int valueShift = *s++;
				uint32_t value = ReadB4ExpressionValue(s, blockData);
				assert((value & ((1<<valueShift)-1)) == 0);
				value >>= valueShift;
				assert(value >> numberOfBits == 0);
				(void) numberOfBits;
				opcodeValue = value << bitOffset;
				goto ProcessPatchOpcode;
			}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
			{
				uint64_t v;
#pragma clang diagnostic pop
		CASE(LogicalImmediatePatchB4Opcode):
				{
					uint32_t value = ReadB4ExpressionValue(s, blockData);
					v = (uint64_t(value)<<32) | value;
					goto ProcessPatchLogical;
				}
		CASE(LogicalImmediatePatchB8Opcode):
				v = ReadB8ExpressionValue(s, blockData);
			ProcessPatchLogical:
				opcodeValue = LogicalOpcodeValue(v);
				goto ProcessPatchOpcode;
			}
		CASE(RepeatPatchOpcode):
		ProcessPatchOpcode:
			{
				int offset = ReadSigned16(s);
				uint32_t opcode;
				memcpy(&opcode, p+offset, 4);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconditional-uninitialized"
				opcode |= opcodeValue;
#pragma clang diagnostic pop
				memcpy(p+offset, &opcode, 4);
				CONTINUE;
			}
#if USE_GOTO_LABELS
		}
#endif
		CASE(B1Expression):
			*p++ = ReadB1ExpressionValue(s, blockData);
			CONTINUE;
		CASE(B2Expression):
			{
				int16_t v = ReadB2ExpressionValue(s, blockData);
				memcpy(p, &v, sizeof(v));
				p += sizeof(v);
				CONTINUE;
			}
		CASE(B4Expression):
			{
				int32_t v = ReadB4ExpressionValue(s, blockData);
				memcpy(p, &v, sizeof(v));
				p += sizeof(v);
				CONTINUE;
			}
		CASE(B8Expression):
			{
				int64_t v = ReadB8ExpressionValue(s, blockData);
				memcpy(p, &v, sizeof(v));
				p += sizeof(v);
				CONTINUE;
			}
		CASE(DataBlock):
			{
				uint32_t dataSize = ((const AppendByteReference*) blockData)->dataSize;
				const uint8_t *expressionData = (const uint8_t*) blockData + sizeof(AppendByteReference);
				InlineMemcpyAndAdvancePointers(p, expressionData, dataSize);
				CONTINUE;
			}
		CASE(DataPointer):
			{
				uint32_t dataSize = ((const AppendDataPointerReference*) blockData)->dataSize;
				const uint8_t *pData = ((const AppendDataPointerReference*) blockData)->pData;
				InlineMemcpyAndAdvancePointers(p, pData, dataSize);
				CONTINUE;
			}
		{	// Scope block for v
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
			uint64_t v;
#pragma clang diagnostic pop
		CASE(Imm0B1Condition):
			v = ReadB1ExpressionValue(s, blockData);
			goto ProcessImm0Condition;
		CASE(Imm0B2Condition):
			v = ReadB2ExpressionValue(s, blockData);
			goto ProcessImm0Condition;
		CASE(Imm0B4Condition):
			v = ReadB4ExpressionValue(s, blockData);
			goto ProcessImm0Condition;
		CASE(Imm0B8Condition):
			v = ReadB8ExpressionValue(s, blockData);
		ProcessImm0Condition:
			asm volatile("; This comment prevents the compiler from expanding code inappropriately.");
			uint32_t offset = *s++;
			if(v != 0) s += offset;
			CONTINUE;
		}
		CASE(Delta21Condition):
			{
				// ADR has 21 bits
				int64_t v = ReadB8ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				int64_t currentP = int64_t(p);
				int64_t delta = v - currentP;
				if(cls(delta) < 64-21) s += offset;
				CONTINUE;
			}
		CASE(Delta26x4Condition):
			{
				// Direct branches have 26 bits, representing delta*4
				int64_t v = ReadB8ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				int64_t currentP = int64_t(p);
				int64_t delta = v - currentP;
				if((delta & 3) != 0
				   || cls(delta) < 64-26-2) s += offset;
				CONTINUE;
			}
		CASE(AdrpCondition):
			{
				// ADRP has 21 bits
				uint64_t v = ReadB8ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				uint64_t currentP = uint64_t(p);
				int64_t delta = (v >> 12) - (currentP >> 12);
				if(cls(delta) < 64-21) s += offset;
				CONTINUE;
			}
		{	// Scope block for labelId
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
			uint32_t labelId;
#pragma clang diagnostic pop
		CASE(Label):
			labelId = ReadUnsigned32(s);
			goto ProcessLabel;
		CASE(ExpressionLabel):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessLabel:
			asm volatile("; This comment prevents the compiler from expanding code inappropriately.");
			// Insert into map.
			labels.Set(labelId, p);
			
			JitForwardReferenceMapLookupResult result = unresolvedLabels.Find(labelId);
			if(result.reference)
			{
				JitForwardReferenceData *data = result.reference;
				JitForwardReferenceData *last;
				do
				{
					Patch(data->p, data->data, (int64_t) p);
					last = data;
					data = data->next;
				} while(data);
				unresolvedLabels.Remove(result, last);
			}
			CONTINUE;
		}
		{	// Scope block for encoding, labelId
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
			uint8_t encoding;
			uint32_t labelId;
			const uint8_t *target;
			uint8_t *patchAddress;
			int64_t delta;
#pragma clang diagnostic pop
		CASE(PatchExpression):
			target = (const uint8_t*) ReadB8ExpressionValue(s, blockData);
			goto ProcessBackwardTarget;
		CASE(PatchExpressionLabel):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
			goto ProcessPatchLabel;
		CASE(PatchLabel):
			labelId = ReadUnsigned32(s);
		ProcessPatchLabel:
			target = (const uint8_t*) labels.GetIfExists(labelId);
			if(target == nullptr) goto ProcessForwardLabel;
			else goto ProcessBackwardTarget;
		CASE(PatchLabelForward):
			labelId = ReadUnsigned32(s);
			goto ProcessForwardLabel;
		CASE(PatchExpressionLabelForward):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessForwardLabel:
			{
				JitForwardReferenceData *refData = unresolvedLabels.Add(labelId);
				encoding = *s++;
				patchAddress = p + ReadSigned16(s);
				refData->data = encoding;
				refData->p = patchAddress;
			}
			delta = -(int64_t) patchAddress;
			goto ProcessPatch;
		CASE(PatchLabelBackward):
			labelId = ReadUnsigned32(s);
			goto ProcessBackwardLabel;
		CASE(PatchExpressionLabelBackward):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessBackwardLabel:
			target = (uint8_t*) labels.Get(labelId);
		ProcessBackwardTarget:
			encoding = *s++;
			patchAddress = p + ReadSigned16(s);
			delta = target - patchAddress;
		ProcessPatch:
			Patch(patchAddress, encoding, delta);
		}
			CONTINUE;	// Continue outside variable scope produces better register allocations
		CASE(PatchAbsoluteAddress):
			{
				int offset = ReadSigned16(s);
				uint64_t v;
				memcpy(&v, p + offset, 8);
				v += (uint64_t) p + offset;
				memcpy(p + offset, &v, 8);
				CONTINUE;
			}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
		{
			uint32_t opcode;
			uint64_t value;
			uint64_t notValue;
			uint64_t logicalValue;
#pragma clang diagnostic pop
		CASE(MovReg32Expression):
			opcode = *s++;
			value = (uint32_t) ReadB4ExpressionValue(s, blockData);
			notValue = ~uint32_t(value);
			logicalValue = value | (value << 32);
			goto ProcessMovExpression;
		CASE(MovReg64Expression):
			opcode = *s++ | 0x80000000;
			value = ReadB8ExpressionValue(s, blockData);
			notValue = ~value;
			logicalValue = value;
		ProcessMovExpression:
			if((value & 0xffffffffffff0000) == 0) opcode |= 0x52800000 | (value << 5);
			else if((value & 0xffffffff0000ffff) == 0) opcode |= 0x52a00000 | (value >> 11);
			else if((notValue & 0xffffffffffff0000) == 0) opcode |= 0x12800000 | (notValue << 5);
			else if((notValue & 0xffffffff0000ffff) == 0) opcode |= 0x12a00000 | (notValue >> 11);
			else if((value & 0xffff0000ffffffff) == 0) opcode |= 0x52c00000 | (value >> 27);
			else if((value & 0x0000ffffffffffff) == 0) opcode |= 0x52e00000 | (value >> 43);
			else if((notValue & 0xffff0000ffffffff) == 0) opcode |= 0x12c00000 | (notValue >> 27);
			else if((notValue & 0x0000ffffffffffff) == 0) opcode |= 0x12e00000 | (notValue >> 43);
			else
			{
				BitMaskEncodeResult result = EncodeBitMask(logicalValue);
				assert(result.size != 0 && "Unable to encode logical immediate");

				if(result.size == 64) opcode |= 1 << 22;
				uint32_t imms = ((0x1e << __builtin_ctz(result.size)) + result.length - 1) & 0x3f;
				opcode |= 0x320003e0 | (result.rotate << 16) | (imms << 10);
			}
			
			memcpy(p, &opcode, 4);
			p += 4;
		}
			CONTINUE;
#if !USE_GOTO_LABELS
		default:
			assert(!"Unhandled opcode");
#endif
		}
#if !USE_GOTO_LABELS
	}
#endif
}

//============================================================================

int8_t SegmentAssembler::ReadB1ExpressionValue(const uint8_t *&s, const AppendAssemblyReference *reference)
{
	const uint8_t *expressionData = (const uint8_t*) reference;
	uint32_t offset = ReadUnsigned16(s);
	return expressionData[offset];
}

int32_t SegmentAssembler::ReadB2ExpressionValue(const uint8_t *&s, const AppendAssemblyReference *reference)
{
	const uint8_t *expressionData = (const uint8_t*) reference;
	uint32_t offset = ReadUnsigned16(s);
	int16_t result;
	memcpy(&result, expressionData + offset, sizeof(result));
	return result;
}

int32_t SegmentAssembler::ReadB4ExpressionValue(const uint8_t *&s, const AppendAssemblyReference *reference)
{
	const uint8_t *expressionData = (const uint8_t*) reference;
	uint32_t offset = ReadUnsigned16(s);
	int32_t result;
	memcpy(&result, expressionData + offset, sizeof(result));
	return result;
}

int64_t SegmentAssembler::ReadB8ExpressionValue(const uint8_t *&s, const AppendAssemblyReference *reference)
{
	const uint8_t *expressionData = (const uint8_t*) reference;
	uint32_t offset = ReadUnsigned16(s);
	int64_t result;
	memcpy(&result, expressionData + offset, sizeof(result));
	return result;
}

//============================================================================

bool SegmentAssembler::IsValidBitmask64(uint64_t value)
{
	return EncodeBitMask(value).size != 0;
}

//============================================================================

#pragma mark - Assembler

//============================================================================

Assembler::Assembler(JitMemoryManager *codeSegmentMemoryManager,
					 JitMemoryManager *dataSegmentMemoryManager)
: SegmentAssembler(*codeSegmentMemoryManager)
{
	if(dataSegmentMemoryManager)
	{
		hasDataSegment = true;
		new(&dataSegment) SegmentAssembler(*dataSegmentMemoryManager);
	}
}

Assembler::~Assembler()
{
	if(hasDataSegment) dataSegment.~SegmentAssembler();
}

__attribute__((flatten))
void* Assembler::Build()
{
	if(hasDataSegment)
	{
		uint32_t numberOfLabels = aggregateData.numberOfLabels + dataSegment.aggregateData.numberOfLabels;
		uint32_t numberOfForwardLabelReferences = aggregateData.numberOfForwardLabelReferences + dataSegment.aggregateData.numberOfForwardLabelReferences;
		
		labels.Reserve(numberOfLabels);
		unresolvedLabels.Reserve(numberOfForwardLabelReferences);
		
		dataSegment.labels.StartUseBacking(labels);
		dataSegment.unresolvedLabels.StartUseBacking(unresolvedLabels);

		dataSegment.ProcessByteCode();

		dataSegment.labels.StopUseBacking(labels);
		dataSegment.unresolvedLabels.StopUseBacking(unresolvedLabels);
	}
	else
	{
		labels.Reserve(aggregateData.numberOfLabels);
		unresolvedLabels.Reserve(aggregateData.numberOfForwardLabelReferences);
	}
	ProcessByteCode();
	assert(!unresolvedLabels.HasData() && "Not all references have been resolved");
	
	return programStart;
}

//============================================================================

#endif // defined(__arm64__)

//============================================================================
