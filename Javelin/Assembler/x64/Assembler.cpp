//============================================================================

#if defined(__amd64__)

//============================================================================

#include "Javelin/Assembler/x64/Assembler.h"

#include "Javelin/Assembler/JitMemoryManager.h"
#include "Javelin/Assembler/x64/ActionType.h"
#include <assert.h>
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
using namespace Javelin::x64Assembler;

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

const uint8_t kSIBNoIndex = 0b100;
const uint8_t kSIBNoBase = 0b101;
const uint8_t kModRMRmSIB = 0b100;

//============================================================================

SegmentAssembler::SegmentAssembler(JitMemoryManager &aMemoryManager)
: memoryManager(aMemoryManager)
{
	buildData.Reserve(8192);
}

//============================================================================

#if USE_OPTIMIZED_APPEND_INSTRUCTION_DATA

// Define JIT_OFFSETOF to avoid compiler warnings on offsetof()
#define JIT_OFFSETOF(t,f)    (size_t(&((t*)64)->f) - 64)

// Parameter order: RDI, RSI, RDX, RCX, R8, R9
// Scratch RAX, R10, R11

#if defined(__WIN32__) || defined(__APPLE__)
  #define FUNCTION_PREFIX "_"
#else
  #define FUNCTION_PREFIX ""
#endif

__attribute__((naked))
void* SegmentAssembler::AppendInstructionData(uint32_t blockByteCodeSize,
											  const uint8_t *s,
											  uint32_t referenceAndDataLength,
											  uint32_t labelData)
{
	asm volatile(".global " FUNCTION_PREFIX "_ZN7Javelin16SegmentAssembler21AppendInstructionDataEjPKhj");

	asm volatile("movzbl %r8b, %eax");
	asm volatile("shr $8, %r8d");
	asm volatile("addl %%eax, %c0(%%rdi)" : : "i"(JIT_OFFSETOF(Assembler, allLabelData.numberOfLabels)));
	asm volatile("movl %c0(%%rdi), %%r9d" : : "i"(JIT_OFFSETOF(Assembler, allLabelData.numberOfForwardLabelReferences)));
	asm volatile("add %r9d, %r8d");
	asm volatile("shlq $16, %r9");
	asm volatile("orq %r9, %rsi");
	asm volatile("movl %%r8d, %c0(%%rdi)" : : "i"(JIT_OFFSETOF(Assembler, allLabelData.numberOfForwardLabelReferences)));
	asm volatile("jmp " FUNCTION_PREFIX "_ZN7Javelin16SegmentAssembler21AppendInstructionDataEjPKhj");

	asm volatile(".global " FUNCTION_PREFIX "_ZN7Javelin16SegmentAssembler21AppendInstructionDataEjPKh");
	asm volatile(FUNCTION_PREFIX "_ZN7Javelin16SegmentAssembler21AppendInstructionDataEjPKh:");
	asm volatile("mov %0, %%ecx" : : "i"(sizeof(AppendAssemblyReference)));
	
	// Definition for AppendInstructionData(blockByteCodeSize, s, referenceAndDataLength);
	asm volatile(FUNCTION_PREFIX "_ZN7Javelin16SegmentAssembler21AppendInstructionDataEjPKhj:");

	// Merge rsi and rcx into a single write. This includes forwardLabelReferenceOffset from above.
	asm volatile("shlq $16, %rsi");
	asm volatile("orq %rcx, %rsi");

	asm volatile("movl %c0(%%rdi), %%eax" : : "i"(JIT_OFFSETOF(Assembler, buildData.offset)));
	asm volatile("addl %eax, %ecx");
	asm volatile("movl %%ecx, %c0(%%rdi)" : : "i"(JIT_OFFSETOF(Assembler, buildData.offset)));
	asm volatile("cmpl %c0(%%rdi), %%ecx" : : "i"(JIT_OFFSETOF(Assembler, buildData.capacity)));
	asm volatile("ja 1f");
	asm volatile("addq %c0(%%rdi), %%rax" : : "i"(JIT_OFFSETOF(Assembler, buildData.data)));
	asm volatile("movq %%rdx, %c0(%%rax)" : : "i"(JIT_OFFSETOF(AppendAssemblyReference, assemblerData)));
	asm volatile("movq %%rsi, %c0(%%rax)" : : "i"(JIT_OFFSETOF(AppendAssemblyReference, referenceSize)));
	asm volatile("ret");

	// Capacity doesn't fit
	asm volatile("1:");
	
	// Save all registers required to complete the update.
	asm volatile("push %rdi");
	asm volatile("push %rsi");
	asm volatile("push %rdx");
	asm volatile("push %rcx");
	asm volatile("push %rax");

	// Update capacity
	asm volatile("leal (%rcx, %rcx), %esi");
	asm volatile("movl %%esi, %c0(%%rdi)" : : "i"(JIT_OFFSETOF(Assembler, buildData.capacity)));
	asm volatile("movq %c0(%%rdi), %%rdi" : : "i"(JIT_OFFSETOF(Assembler, buildData.data)));
	asm volatile("call " FUNCTION_PREFIX "realloc");
	
	asm volatile("pop %r10");
	asm volatile("pop %rcx");
	asm volatile("pop %rdx");
	asm volatile("pop %rsi");
	asm volatile("pop %rdi");

	asm volatile("movq %%rax, %c0(%%rdi)" : : "i"(JIT_OFFSETOF(Assembler, buildData.data)));

	asm volatile("addq %r10, %rax");
	asm volatile("movq %%rdx, %c0(%%rax)" : : "i"(JIT_OFFSETOF(AppendAssemblyReference, assemblerData)));
	asm volatile("movq %%rsi, %c0(%%rax)" : : "i"(JIT_OFFSETOF(AppendAssemblyReference, referenceSize)));
	asm volatile("ret");
}

void* (SegmentAssembler::*volatile reference0)(uint32_t, const uint8_t*, uint32_t, uint32_t);
void* (SegmentAssembler::*volatile reference1)(uint32_t, const uint8_t*, uint32_t);
void (SegmentAssembler::*volatile reference2)(uint32_t, const uint8_t*);

__attribute__((constructor)) static void EnsureLinkage()
{
	reference0 = &SegmentAssembler::AppendInstructionData;
	reference1 = &SegmentAssembler::AppendInstructionData;
	reference2 = &SegmentAssembler::AppendInstructionData;
}

#else

void SegmentAssembler::AppendInstructionData(uint32_t blockByteCodeSize, const uint8_t *s)
{
	AppendAssemblyReference *reference = (AppendAssemblyReference*) buildData.Append(sizeof(AppendAssemblyReference));
	reference->referenceSize = sizeof(AppendAssemblyReference);
	reference->assemblerData = s;
	reference->blockByteCodeSize = blockByteCodeSize;
}

void* SegmentAssembler::AppendInstructionData(uint32_t blockByteCodeSize,
											  const uint8_t *s,
											  uint32_t referenceAndDataLength)
{
	AppendAssemblyReference *reference = (AppendAssemblyReference*) buildData.Append(referenceAndDataLength);
	reference->referenceSize = referenceAndDataLength;
	reference->assemblerData = s;
	reference->blockByteCodeSize = blockByteCodeSize;
	return reference;
}

void* SegmentAssembler::AppendInstructionData(uint32_t blockByteCodeSize,
											  const uint8_t *s,
											  uint32_t referenceAndDataLength,
											  uint32_t labelData)
{
	AppendAssemblyReference *reference = (AppendAssemblyReference*) buildData.Append(referenceAndDataLength);
	reference->referenceSize = referenceAndDataLength;
	reference->assemblerData = s;
	reference->blockByteCodeSize = blockByteCodeSize;
	reference->forwardLabelReferenceOffset = allLabelData.numberOfForwardLabelReferences;

	ProcessLabelData(labelData);
	
	return reference;
}

void SegmentAssembler::ProcessLabelData(uint32_t labelData)
{
	int numberOfLabels = labelData & 0xff;
	int numberOfForwardLabelReferences = labelData >> 8;
	
	allLabelData.numberOfLabels += numberOfLabels;
	allLabelData.numberOfForwardLabelReferences += numberOfForwardLabelReferences;
}

#endif

//============================================================================

void* SegmentAssembler::AppendData(uint32_t byteSize)
{
	static constexpr ActionType appendDataActions[2] =
	{
		ActionType::DataBlock,
		ActionType::Return,
	};
	static_assert(sizeof(AppendDataReference) == 24, "Expected AppendDataReference to be 24 bytes");
	
	uint32_t allocationSize = (sizeof(AppendDataReference) + byteSize + 7) & -8;
	AppendDataReference *reference = (AppendDataReference*) AppendInstructionData(byteSize,
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

__attribute__((always_inline)) int32_t SegmentAssembler::ReadSigned32(const uint8_t* __restrict &s)
{
	int32_t result;
	memcpy(&result, s, sizeof(result));
	s += sizeof(result);
	return result;
}

__attribute__((always_inline)) uint32_t SegmentAssembler::ReadUnsigned32(const uint8_t* __restrict &s)
{
	uint32_t result;
	memcpy(&result, s, sizeof(result));
	s += sizeof(result);
	return result;
}

void SegmentAssembler::Patch(uint8_t* __restrict p, uint32_t bytes, int32_t delta)
{
	assert(bytes == 1 || bytes == 4 || bytes == 8);
	switch(bytes)
	{
	case 1:
		*p += delta;
		break;
	case 4:
		{
			int32_t v;
			memcpy(&v, p, sizeof(v));
			v += delta;
			memcpy(p, &v, sizeof(v));
		}
		break;
	case 8:
		{
			int64_t v;
			memcpy(&v, p, sizeof(v));
			v += delta;
			memcpy(p, &v, sizeof(v));
		}
		break;
	}
}

//============================================================================

void SegmentAssembler::ProcessByteCode()
{
	// 1. Do a first pass to help determine rel8 vs rel32.
	//    - This records pessimistic offsets for forward label references
	//      and expressions.
	// 2. Build byte code directly into executable memory
	// 3. Shrink allocation
	
	// For relative patches that are encountered:
	// a) Backreferences are patched immediately.
	// b) Unresolved forward references are put into a queue.
	// c) When a new label is encountered, it checks whether any old
	//    addresses need to be patched, and clears the list

	uint32_t maximumCodeSize = PrepareGenerateByteCode();
	
	// Build byte code into result memory.
	// Allocate memory.
	programStart = (uint8_t*) memoryManager.Allocate(maximumCodeSize+3);	// +3 is because some actions assume extra buffer.
	uint8_t *programEnd = GenerateByteCode(programStart);

	// Shrink allocation
	memoryManager.EndWrite(programStart);
	uint32_t codeSize = uint32_t(programEnd - programStart);
	assert(codeSize <= maximumCodeSize);
	memoryManager.Shrink(programStart, codeSize);
}


#if USE_GOTO_LABELS
	#define CASE(x) 		x
	#define CONTINUE1(x)	do { const uint8_t action = s[x]; void *jumpTarget = jumpOffsets[action];
	#define CONTINUE2		++s; goto *jumpTarget; } while(0)
	#define CONTINUE	 	goto *jumpOffsets[*s++]
#else
	#define CASE(x) 		case ActionType::x
	#define CONTINUE1(x)	{ const uint8_t* pAction = s+x;
	#define CONTINUE2		assert(s == pAction); continue; }
	#define CONTINUE 		continue
#endif

uint32_t SegmentAssembler::PrepareGenerateByteCode()
{
	struct AlternateData
	{
		bool	 takeJump;
		uint32_t startOffset;
		uint32_t endOffset;
	};
	AlternateData alternateStack[8];
	bool takeJump = false;
	int alternateStackSize = 0;
	
#if USE_GOTO_LABELS
	static constexpr void *jumpOffsets[] =
	{
		#define TAG(x) &&x,
		#include "ActionTypeTags.h"
		#undef TAG
	};
#endif
	
	const AppendAssemblyReference *blockData = (AppendAssemblyReference*) buildData.begin();
	const AppendAssemblyReference* pEnd = (AppendAssemblyReference*) buildData.end();
	uint32_t offset = 0;

	for(;;)
	{
		if(blockData == pEnd) return offset;
		if(blockData->blockByteCodeSize == 0) break;
		offset += blockData->blockByteCodeSize;
	NextBlock:
		blockData = blockData->GetNext();
	}
	
	const  uint8_t* __restrict s = blockData->assemblerData;
	for(;;)
	{
#if USE_GOTO_LABELS
		CONTINUE;
#else
		switch((ActionType) *s++)
#endif
		{
		CASE(Return):
			goto NextBlock;
		CASE(Literal1):  CONTINUE1(1);  offset += 1;  s += 1;  CONTINUE2;
		CASE(Literal2):  CONTINUE1(2);  offset += 2;  s += 2;  CONTINUE2;
		CASE(Literal3):  CONTINUE1(3);  offset += 3;  s += 3;  CONTINUE2;
		CASE(Literal4):  CONTINUE1(4);  offset += 4;  s += 4;  CONTINUE2;
		CASE(Literal5):  CONTINUE1(5);  offset += 5;  s += 5;  CONTINUE2;
		CASE(Literal6):  CONTINUE1(6);  offset += 6;  s += 6;  CONTINUE2;
		CASE(Literal7):  CONTINUE1(7);  offset += 7;  s += 7;  CONTINUE2;
		CASE(Literal8):  CONTINUE1(8);  offset += 8;  s += 8;  CONTINUE2;
		CASE(Literal9):  CONTINUE1(9);  offset += 9;  s += 9;  CONTINUE2;
		CASE(Literal10): CONTINUE1(10); offset += 10; s += 10; CONTINUE2;
		CASE(Literal11): CONTINUE1(11); offset += 11; s += 11; CONTINUE2;
		CASE(Literal12): CONTINUE1(12); offset += 12; s += 12; CONTINUE2;
		CASE(Literal13): CONTINUE1(13); offset += 13; s += 13; CONTINUE2;
		CASE(Literal14): CONTINUE1(14); offset += 14; s += 14; CONTINUE2;
		CASE(Literal15): CONTINUE1(15); offset += 15; s += 15; CONTINUE2;
		CASE(Literal16): CONTINUE1(16); offset += 16; s += 16; CONTINUE2;
		CASE(LiteralBlock):
			{
				uint32_t blockSize = ReadUnsigned16(s);
				offset += blockSize;
				s += blockSize;
				CONTINUE;
			}
		CASE(Align):
			// Just add the maximum possible bytes for this pass.
			offset += *s++;
			CONTINUE;
		CASE(Unalign):
			// Just add the maximum possible bytes for this pass.
			++s;
			++offset;
			CONTINUE;
		CASE(Alternate):
			{
				uint32_t maximumSize = *s++;
				assert(alternateStackSize < sizeof(alternateStack)/sizeof(alternateStack[0]));
				alternateStack[alternateStackSize].takeJump = takeJump;
				alternateStack[alternateStackSize].startOffset = offset;
				alternateStack[alternateStackSize].endOffset = offset + maximumSize;
				++alternateStackSize;
				takeJump = false;
				CONTINUE;
			}
		CASE(EndAlternate):
			assert(alternateStackSize > 0);
			--alternateStackSize;
			offset = alternateStack[alternateStackSize].endOffset;
			takeJump = alternateStack[alternateStackSize].takeJump;
			CONTINUE;
		CASE(Jump):
			assert(alternateStackSize > 0);
			if(takeJump)
			{
				uint32_t jumpOffset = *s++;
				s += jumpOffset;
				--alternateStackSize;
				takeJump = alternateStack[alternateStackSize].takeJump;
			}
			else
			{
				++s;
				offset = alternateStack[alternateStackSize-1].startOffset;
			}
			CONTINUE;
		CASE(B1Expression):
			CONTINUE1(2);
			SkipExpressionValue(s);
			++offset;
			CONTINUE2;
		CASE(B2Expression):
			CONTINUE1(2);
			SkipExpressionValue(s);
			offset += 2;
			CONTINUE2;
		CASE(B4Expression):
			CONTINUE1(2);
			SkipExpressionValue(s);
			offset += 4;
			CONTINUE2;
		CASE(B8Expression):
			CONTINUE1(2);
			SkipExpressionValue(s);
			offset += 8;
			CONTINUE2;
		CASE(B1DeltaExpression):
		CASE(B4DeltaExpression):
		CASE(B4AddExpression):
			CONTINUE1(2);
			SkipExpressionValue(s);
			CONTINUE2;
		CASE(DataBlock):
		CASE(DataPointer):
			{
				uint32_t dataSize = ((const AppendDataReference*) blockData)->dataSize;
				offset += dataSize;
				CONTINUE;
			}
		CASE(Imm0B1Condition):
			{
				int8_t v = ReadB1ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != 0) s += offset;
				else takeJump = true;
				CONTINUE;
			}
		CASE(Imm0B2Condition):
			{
				int32_t v = ReadB2ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != 0) s += offset;
				else takeJump = true;
				CONTINUE;
			}
		CASE(Imm0B4Condition):
			{
				int32_t v = ReadB4ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != 0) s += offset;
				else takeJump = true;
				CONTINUE;
			}
		CASE(Imm0B8Condition):
			{
				int64_t v = ReadB8ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != 0) s += offset;
				else takeJump = true;
				CONTINUE;
			}
		CASE(Imm8B2Condition):
			{
				int32_t v = ReadB2ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != (int8_t) v) s += offset;
				else takeJump = true;
				CONTINUE;
			}
		CASE(Imm8B4Condition):
			{
				int32_t v = ReadB4ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != (int8_t) v) s += offset;
				else takeJump = true;
				CONTINUE;
			}
		CASE(Imm8B8Condition):
			{
				int64_t v = ReadB8ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != (int8_t) v) s += offset;
				else takeJump = true;
				CONTINUE;
			}
		CASE(Imm16B4Condition):
			{
				int32_t v = ReadB4ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != (int16_t) v) s += offset;
				else takeJump = true;
				CONTINUE;
			}
		CASE(Imm16B8Condition):
			{
				int64_t v = ReadB8ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != (int16_t) v) s += offset;
				else takeJump = true;
				CONTINUE;
			}
		CASE(Imm32B8Condition):
			{
				int64_t v = ReadB8ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != (int32_t) v) s += offset;
				else takeJump = true;
				CONTINUE;
			}
		CASE(UImm32B8Condition):
			{
				int64_t v = ReadB8ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if((v >> 32) != 0) s += offset;
				else takeJump = true;
				CONTINUE;
			}
		CASE(Delta32Condition):
			{
				// This depends on the actual allocated address,
				// so cannot be determined at this stage.
				SkipExpressionValue(s);
				++s;
				CONTINUE;
			}
		CASE(DynamicOpcodeO1):
			{
				uint8_t rex = *s++;
				int8_t reg = ReadB1ExpressionValue(s, blockData);
				if(reg & 8) rex |= 0x41;
				if(rex) ++offset;
				++offset;
				++s;
				CONTINUE;
			}
		CASE(DynamicOpcodeO2):
			{
				uint8_t rex = *s++;
				int8_t reg = ReadB1ExpressionValue(s, blockData);
				if(reg & 8) rex |= 0x41;
				if(rex) ++offset;
				offset += 2;
				s += 2;
				CONTINUE;
			}
		CASE(DynamicOpcodeR):
			{
				uint8_t rex = *s++;
				++s;
				
				DynamicOpcodeRControlByte controlByte = { .value = *s++ };
				assert(controlByte.hasRegExpression);
				int8_t reg = ReadB1ExpressionValue(s, blockData);
				if(reg & 8) rex |= 0x44;	// Set rex and .r bit.

				if(rex) ++offset;
				int opcodeLength = controlByte.opcodeLengthM1+1;
				offset += 1 + opcodeLength;
				s += opcodeLength;
				CONTINUE;
			}
		CASE(DynamicOpcodeRR):
			{
				uint8_t rex = *s++;
				++s;
				
				DynamicOpcodeRRControlByte controlByte = { .value = *s++ };
				if(controlByte.hasRegExpression)
				{
					int8_t reg = ReadB1ExpressionValue(s, blockData);
					if(reg & 8) rex |= 0x44;	// Set rex and .r bit.
				}
				if(controlByte.hasRmExpression)
				{
					int8_t rm = ReadB1ExpressionValue(s, blockData);
					if(rm & 8) rex |= 0x41;		// Set rex and .b bit.
				}
				if(rex) ++offset;
				int opcodeLength = controlByte.opcodeLengthM1+1;
				offset += 1 + opcodeLength;
				s += opcodeLength;
				CONTINUE;
			}
		CASE(DynamicOpcodeRM):
			{
				int rex = *s++;
				ModRM modRM = { .value = *s++ };
				SIB sib = { .value = *s++ };

				DynamicOpcodeRMControlByte controlByte = { .value = *s++ };
				
				int displacement = controlByte.hasDisplacementExpression ?
									ReadB4ExpressionValue(s, blockData) :
									ReadSigned32(s);
				int displacementBytes;
				if(modRM.mod == ModRM::Mod::IndirectDisplacement0 && sib.base == kSIBNoBase)
				{
					displacementBytes = 4;
				}
				else if(modRM.mod == ModRM::Mod::IndirectDisplacement0 && displacement == 0)
				{
					displacementBytes = 0;
				}
				else if(displacement == (int8_t)displacement)
				{
					modRM.mod = ModRM::Mod::IndirectDisplacement8;
					displacementBytes = 1;
				}
				else
				{
					modRM.mod = ModRM::Mod::IndirectDisplacement32;
					displacementBytes = 4;
				}
				
				if(controlByte.hasRegExpression)
				{
					int8_t reg = ReadB1ExpressionValue(s, blockData);
					if(reg & 8) rex |= 0x44;	// Set rex and .r bit.
				}
				if(controlByte.hasScaleExpression)
				{
					assert(modRM.rm == kModRMRmSIB);
					SkipExpressionValue(s);
				}
				if(controlByte.hasIndexExpression)
				{
					assert(modRM.rm == kModRMRmSIB);
					int8_t index = ReadB1ExpressionValue(s, blockData);
					assert(index != kSIBNoIndex);
					if(index & 8) rex |= 0x42;	// Set rex and .x bit.
				}
				if(controlByte.hasBaseExpression)
				{
					int8_t base = ReadB1ExpressionValue(s, blockData);
					if(base & 8) rex |= 0x41;	// Set rex and .b bit.
					
					if((base & 7) == kSIBNoBase && modRM.mod == ModRM::Mod::IndirectDisplacement0)
					{
						displacementBytes = 1;
					}
					if((base & 7) == kModRMRmSIB)
					{
						modRM.rm = kModRMRmSIB;
					}
				}
				
				if(rex) ++offset;
				int opcodeLength = controlByte.opcodeLengthM1+1;
				offset += opcodeLength;
				s += opcodeLength;
				++offset;
				if(modRM.rm == kModRMRmSIB) ++offset;
				offset += displacementBytes;
				CONTINUE;
			}
		CASE(DynamicOpcodeRV):
			{
				uint8_t vex3B2 = *s++;
				uint8_t vex3B3 = *s++;
				s++;	// Skip ModRM
				
				DynamicOpcodeRVControlByte controlByte;
				controlByte.value = *s++;
				if(controlByte.hasReg0Expression) SkipExpressionValue(s);
				if(controlByte.hasReg1Expression) SkipExpressionValue(s);

				if((vex3B2 & 0x7f) != 1 || (vex3B3 & 0x80)) offset += 3;
				else offset += 2;
				int opcodeLength = controlByte.opcodeLengthM1+1;
				offset += opcodeLength + 1;
				s += opcodeLength;
				CONTINUE;
			}
		CASE(DynamicOpcodeRVR):
			{
				uint8_t vex3B2 = *s++;
				uint8_t vex3B3 = *s++;
				s++;	// Skip ModRM
				
				DynamicOpcodeRVRControlByte controlByte;
				controlByte.value = *s++;
				if(controlByte.hasReg0Expression) SkipExpressionValue(s);
				if(controlByte.hasReg1Expression) SkipExpressionValue(s);
				if(controlByte.hasReg2Expression)
				{
					int8_t rm = ReadB1ExpressionValue(s, blockData);
					if(rm & 8) vex3B2 |= 0x20;		// Set .b bit.
				}

				if((vex3B2 & 0x7f) != 1 || (vex3B3 & 0x80)) offset += 3;
				else offset += 2;
				int opcodeLength = controlByte.opcodeLengthM1+1;
				offset += opcodeLength + 1;
				s += opcodeLength;
				CONTINUE;
			}
		CASE(DynamicOpcodeRVM):
			{
				uint8_t vex3B2 = *s++;
				uint8_t vex3B3 = *s++;
				ModRM modRM = { .value = *s++ };
				SIB sib = { .value = *s++ };

				DynamicOpcodeRVMControlByte controlByte = { .value = *s++ };
				
				int displacement = controlByte.hasDisplacementExpression ?
									ReadB4ExpressionValue(s, blockData) :
									ReadSigned32(s);
				int displacementBytes;
				if(modRM.mod == ModRM::Mod::IndirectDisplacement0 && sib.base == kSIBNoBase)
				{
					displacementBytes = 4;
				}
				else if(modRM.mod == ModRM::Mod::IndirectDisplacement0 && displacement == 0)
				{
					displacementBytes = 0;
				}
				else if(displacement == (int8_t)displacement)
				{
					modRM.mod = ModRM::Mod::IndirectDisplacement8;
					displacementBytes = 1;
				}
				else
				{
					modRM.mod = ModRM::Mod::IndirectDisplacement32;
					displacementBytes = 4;
				}
				
				if(controlByte.hasReg0Expression)
				{
					int8_t reg = ReadB1ExpressionValue(s, blockData);
					if(reg & 8) vex3B2 |= 0x80;	// Set .r bit.
				}
				if(controlByte.hasReg1Expression)
				{
					SkipExpressionValue(s);
				}
				if(controlByte.hasScaleExpression)
				{
					assert(modRM.rm == kModRMRmSIB);
					SkipExpressionValue(s);
				}
				if(controlByte.hasIndexExpression)
				{
					assert(modRM.rm == kModRMRmSIB);
					int8_t index = ReadB1ExpressionValue(s, blockData);
					assert(index != kSIBNoIndex);
					if(index & 8) vex3B2 |= 0x40;	// Set .x bit.
				}
				if(controlByte.hasBaseExpression)
				{
					int8_t base = ReadB1ExpressionValue(s, blockData);
					if(base & 8) vex3B2 |= 0x20;	// Set .b bit.

					if((base & 7) == kSIBNoBase && modRM.mod == ModRM::Mod::IndirectDisplacement0)
					{
						modRM.mod = ModRM::Mod::IndirectDisplacement8;
						displacementBytes = 1;
					}
					if((base & 7) == kModRMRmSIB)
					{
						modRM.rm = kModRMRmSIB;
					}
				}
				
				if((vex3B2 & 0x7f) != 1 || (vex3B3 & 0x80)) offset += 3;
				else offset += 2;
				int opcodeLength = controlByte.opcodeLengthM1+1;
				offset += opcodeLength;
				s += opcodeLength;
				++offset;
				if(modRM.rm == kModRMRmSIB) ++offset;
				offset += displacementBytes;
				CONTINUE;
			}
		{
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
			firstPassLabelOffsets.QueueLabelOffset(labelId, offset);
			CONTINUE;
		}
		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
		uint32_t labelId;
#pragma clang diagnostic pop
		CASE(Rel8LabelCondition):
			labelId = ReadUnsigned32(s);
			goto ProcessLabelCondition;
		CASE(Rel8ExpressionLabelCondition):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessLabelCondition:
			{
				uint32_t target = firstPassLabelOffsets.GetLastLabelOffsetIfExists(labelId);
				uint32_t index = *s++;
				uint32_t endOfLabelOffset = *s++ + offset;
				uint32_t jumpOffset = *s++;
				if(target != JitLabelOffsetQueue::NO_LABEL)
				{
					int32_t delta = target - endOfLabelOffset;
					if(delta != int8_t(delta)) offset += jumpOffset;
					else takeJump = true;
				}
				else
				{
					forwardLabelReferences[index+blockData->forwardLabelReferenceOffset] = endOfLabelOffset;
					s += jumpOffset;
				}
			}
			CONTINUE;
		}
		CASE(Rel8LabelForwardCondition):
			s += 4; // skip label
			goto ProcessForwardLabelCondition;
		CASE(Rel8ExpressionLabelForwardCondition):
			SkipExpressionValue(s);
		ProcessForwardLabelCondition:
			{
				uint32_t index = *s++;
				uint32_t endOfLabelOffset = *s++ + offset;
				uint32_t jumpOffset = *s++;
				forwardLabelReferences[index+blockData->forwardLabelReferenceOffset] = endOfLabelOffset;
				s += jumpOffset;
			}
			CONTINUE;
		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
		uint32_t labelId;
#pragma clang diagnostic pop
		CASE(Rel8LabelBackwardCondition):
			labelId = ReadUnsigned32(s);
			goto ProcessBackwardCondition;
		CASE(Rel8ExpressionLabelBackwardCondition):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessBackwardCondition:
			{
				uint32_t rel8Offset = *s++;
				uint32_t jumpOffset = *s++;
				uint32_t target = firstPassLabelOffsets.GetLastLabelOffset(labelId);
				int32_t delta = target - (offset + rel8Offset);
				if(delta != (int8_t)delta) offset += jumpOffset;
				else takeJump = true;
			}
			CONTINUE;
		}
		CASE(PatchLabel):
		CASE(PatchLabelBackward):
		CASE(PatchLabelForward):
			CONTINUE1(6);
			s += 4; 	// Skip label expression
			s += 2;		// Skip bytes, offset
			CONTINUE2;
		CASE(PatchExpressionLabel):
		CASE(PatchExpressionLabelBackward):
		CASE(PatchExpressionLabelForward):
			CONTINUE1(4);
			SkipExpressionValue(s); // Skip label expression
			s += 2;					// Skip bytes, offset
			CONTINUE2;
		CASE(PatchAbsoluteAddress):
			++s; // Skip delay
			CONTINUE;
#if !USE_GOTO_LABELS
		default:
			assert(!"Unhandled opcode");
#endif
		}
	}
}

uint8_t* SegmentAssembler::GenerateByteCode(uint8_t *__restrict p)
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
	const AppendAssemblyReference* pEnd = (AppendAssemblyReference*) buildData.end();
	const uint8_t* __restrict s = blockData->assemblerData;
	for(;;)
	{
#if USE_GOTO_LABELS
		CONTINUE;
#else
		switch((ActionType) *s++)
#endif
		{
		CASE(Return):
			blockData = blockData->GetNext();
			if(blockData == pEnd) return p;
			s = blockData->assemblerData;
			CONTINUE;
		CASE(Literal1):
			static_assert((int) ActionType::Literal1 == 1, "Expect Literal1 to have enum value 1");
			CONTINUE1(1);
			*p++ = *s++;
			CONTINUE2;
		CASE(Literal2): CONTINUE1(2); memcpy(p, s, 2); p += 2; s += 2; CONTINUE2;
		CASE(Literal3): CONTINUE1(3); memcpy(p, s, 4); p += 3; s += 3; CONTINUE2;
		CASE(Literal4): CONTINUE1(4); memcpy(p, s, 4); p += 4; s += 4; CONTINUE2;
		CASE(Literal5): CONTINUE1(5); memcpy(p, s, 8); p += 5; s += 5; CONTINUE2;
		CASE(Literal6): CONTINUE1(6); memcpy(p, s, 8); p += 6; s += 6; CONTINUE2;
		CASE(Literal7): CONTINUE1(7); memcpy(p, s, 8); p += 7; s += 7; CONTINUE2;
		CASE(Literal8): CONTINUE1(8); memcpy(p, s, 8); p += 8; s += 8; CONTINUE2;
		CASE(Literal9): CONTINUE1(9); memcpy(p, s, 12); p += 9; s += 9; CONTINUE2;
		CASE(Literal10): CONTINUE1(10); memcpy(p, s, 12); p += 10; s += 10; CONTINUE2;
		CASE(Literal11): CONTINUE1(11); memcpy(p, s, 12); p += 11; s += 11; CONTINUE2;
		CASE(Literal12): CONTINUE1(12); memcpy(p, s, 12); p += 12; s += 12; CONTINUE2;
		CASE(Literal13): CONTINUE1(13); memcpy(p, s, 16); p += 13; s += 13; CONTINUE2;
		CASE(Literal14): CONTINUE1(14); memcpy(p, s, 16); p += 14; s += 14; CONTINUE2;
		CASE(Literal15): CONTINUE1(15); memcpy(p, s, 16); p += 15; s += 15; CONTINUE2;
		CASE(Literal16): CONTINUE1(16); memcpy(p, s, 16); p += 16; s += 16; CONTINUE2;
		CASE(LiteralBlock):
			{
				uint32_t length = ReadUnsigned16(s);
				memcpy(p, s, length);
				s += length;
				p += length;
				CONTINUE;
			}
		CASE(Align):
			{
				static const uint8_t nop2[] = { 0x66, 0x90 };
				static const uint8_t nop3[] = { 0x0f, 0x1f, 0x00 };
				static const uint8_t nop4[] = { 0x0f, 0x1f, 0x40, 0x00 };
				static const uint8_t nop6[] = { 0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00 };
				static const uint8_t nop7[] = { 0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00 };
				static const uint8_t nop15[] = { 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };
				
				static const uint8_t *nops[] = {
					nop2+1, nop2, nop3, nop4, nop6+1,
					nop6, nop7, nop15+7, nop15+6, nop15+5,
					nop15+4, nop15+3, nop15+2, nop15+1, nop15
				};
				
				uint8_t alignmentM1 = *s++;
				uint32_t fillLength = (-(size_t) p) & alignmentM1;
				while(fillLength)
				{
					int nopLength = fillLength > 15 ? 15 : fillLength;
					const uint8_t *nop = nops[nopLength-1];
					memcpy(p, nop, nopLength);
					p += nopLength;
					fillLength -= nopLength;
				}
				CONTINUE;
			}
		CASE(Unalign):
			{
				uint8_t alignmentM1 = *s++;
				if(((size_t) p & alignmentM1) == 0) *p++ = 0x90;
				CONTINUE;
			}
		CASE(Alternate):
			++s;	// Skip maximumInstructionSize
			CONTINUE;
		CASE(EndAlternate):
			CONTINUE;
		CASE(Jump):
			s = s + *s + 1;
			CONTINUE;
		CASE(B1Expression):
			CONTINUE1(2)
			*p++ = ReadB1ExpressionValue(s, blockData);
			CONTINUE2;
		CASE(B2Expression):
			{
				CONTINUE1(2)
				int16_t v = ReadB2ExpressionValue(s, blockData);
				memcpy(p, &v, sizeof(v));
				p += sizeof(v);
				CONTINUE2;
			}
		CASE(B4Expression):
			{
				CONTINUE1(2)
				int32_t v = ReadB4ExpressionValue(s, blockData);
				memcpy(p, &v, sizeof(v));
				p += sizeof(v);
				CONTINUE2;
			}
		CASE(B8Expression):
			{
				CONTINUE1(2)
				int64_t v = ReadB8ExpressionValue(s, blockData);
				memcpy(p, &v, sizeof(v));
				p += sizeof(v);
				CONTINUE2;
			}
		CASE(B1DeltaExpression):
			{
				CONTINUE1(2)
				int64_t v = ReadB8ExpressionValue(s, blockData);
				int64_t delta = (v - int64_t(p)) + p[-1];
				assert(delta == (int8_t)delta);
				p[-1] = delta;
				CONTINUE2;
			}
		CASE(B4DeltaExpression):
			{
				CONTINUE1(2)
				int64_t v = ReadB8ExpressionValue(s, blockData);
				int32_t offset;
				memcpy(&offset, p-4, 4);
				int64_t delta = (v - int64_t(p)) + offset;
				assert(delta == (int32_t)delta);
				memcpy(p-4, &delta, 4);
				CONTINUE2;
			}
		CASE(B4AddExpression):
			{
				CONTINUE1(2)
				int32_t v = ReadB4ExpressionValue(s, blockData);
				int32_t oldValue;
				memcpy(&oldValue, p-4, 4);
				v += oldValue;
				assert(v == (int32_t)v);
				memcpy(p-4, &v, 4);
				CONTINUE2;
			}
		CASE(DataBlock):
			{
				uint32_t dataSize = ((const AppendDataReference*) blockData)->dataSize;
				const uint8_t *expressionData = (const uint8_t*) blockData + sizeof(AppendDataReference);
				memcpy(p, expressionData, dataSize);
				p += dataSize;
				CONTINUE;
			}
		CASE(DataPointer):
			{
				uint32_t dataSize = ((const AppendDataPointerReference*) blockData)->dataSize;
				const uint8_t *pData = ((const AppendDataPointerReference*) blockData)->pData;
				memcpy(p, pData, dataSize);
				p += dataSize;
				CONTINUE;
			}
		CASE(Imm0B1Condition):
			{
				int8_t v = ReadB1ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != 0) s += offset;
				CONTINUE;
			}
		CASE(Imm0B2Condition):
			{
				int16_t v = ReadB2ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != 0) s += offset;
				CONTINUE;
			}
		CASE(Imm0B4Condition):
			{
				int32_t v = ReadB4ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != 0) s += offset;
				CONTINUE;
			}
		CASE(Imm0B8Condition):
			{
				int64_t v = ReadB8ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != 0) s += offset;
				CONTINUE;
			}
		CASE(Imm8B2Condition):
			{
				int16_t v = ReadB2ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != (int8_t) v) s += offset;
				CONTINUE;
			}
		CASE(Imm8B4Condition):
			{
				int32_t v = ReadB4ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != (int8_t) v) s += offset;
				CONTINUE;
			}
		CASE(Imm8B8Condition):
			{
				int64_t v = ReadB4ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != (int8_t) v) s += offset;
				CONTINUE;
			}
		CASE(Imm16B4Condition):
			{
				int32_t v = ReadB4ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != (int16_t) v) s += offset;
				CONTINUE;
			}
		CASE(Imm16B8Condition):
			{
				int64_t v = ReadB8ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != (int16_t) v) s += offset;
				CONTINUE;
			}
		CASE(Imm32B8Condition):
			{
				int64_t v = ReadB8ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if(v != (int32_t) v) s += offset;
				CONTINUE;
			}
		CASE(UImm32B8Condition):
			{
				int64_t v = ReadB8ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				if((v >> 32) != 0) s += offset;
				CONTINUE;
			}
		CASE(Delta32Condition):
			{
				int64_t v = ReadB8ExpressionValue(s, blockData);
				uint32_t offset = *s++;
				int64_t currentP = int64_t(p);
				int64_t delta = v - currentP;
				if(int32_t(delta) != delta) s += offset;
				CONTINUE;
			}
		CASE(DynamicOpcodeO1):
			{
				uint8_t rex = *s++;
				int8_t reg = ReadB1ExpressionValue(s, blockData);
				if(reg & 8) rex |= 0x41;
				if(rex) *p++ = rex;
				*p++ = (reg&7) + *s++;
				CONTINUE;
			}
		CASE(DynamicOpcodeO2):
			{
				uint8_t rex = *s++;
				int8_t reg = ReadB1ExpressionValue(s, blockData);
				if(reg & 8) rex |= 0x41;
				if(rex) *p++ = rex;
				*p++ = *s++;
				*p++ = (reg&7) + *s++;
				CONTINUE;
			}
		CASE(DynamicOpcodeR):
			{
				uint8_t rex = *s++;
				uint8_t modRM = *s++;
				
				DynamicOpcodeRControlByte controlByte;
				controlByte.value = *s++;
				if(controlByte.hasRegExpression)
				{
					int8_t reg = ReadB1ExpressionValue(s, blockData);
					if(reg & 8) rex |= 0x44;	// Set rex and .r bit.
					modRM |= (reg&7) << 3;
				}
				if(rex) *p++ = rex;
				int opcodeLength = controlByte.opcodeLengthM1+1;
				memcpy(p, s, 4);
				p += opcodeLength;
				s += opcodeLength;
				*p++ = modRM;
				CONTINUE;
			}
		CASE(DynamicOpcodeRR):
			{
				uint8_t rex = *s++;
				uint8_t modRM = *s++;
				
				DynamicOpcodeRRControlByte controlByte;
				controlByte.value = *s++;
				if(controlByte.hasRegExpression)
				{
					int8_t reg = ReadB1ExpressionValue(s, blockData);
					if(reg & 8) rex |= 0x44;	// Set rex and .r bit.
					modRM |= (reg&7) << 3;
				}
				if(controlByte.hasRmExpression)
				{
					int8_t rm = ReadB1ExpressionValue(s, blockData);
					if(rm & 8) rex |= 0x41;		// Set rex and .b bit.
					modRM |= rm&7;
				}
				if(rex) *p++ = rex;
				int opcodeLength = controlByte.opcodeLengthM1+1;
				memcpy(p, s, 4);
				p += opcodeLength;
				s += opcodeLength;
				*p++ = modRM;
				CONTINUE;
			}
		CASE(DynamicOpcodeRM):
			{
				int rex = *s++;
				ModRM modRM = { .value = *s++ };
				SIB sib = { .value = *s++ };

				DynamicOpcodeRMControlByte controlByte = { .value = *s++ };
				
				int displacement = controlByte.hasDisplacementExpression ?
									ReadB4ExpressionValue(s, blockData) :
									ReadSigned32(s);
				int displacementBytes;
				if(modRM.mod == ModRM::Mod::IndirectDisplacement0 && sib.base == kSIBNoBase)
				{
					displacementBytes = 4;
				}
				else if(modRM.mod == ModRM::Mod::IndirectDisplacement0 && displacement == 0)
				{
					displacementBytes = 0;
				}
				else if(displacement == (int8_t)displacement)
				{
					modRM.mod = ModRM::Mod::IndirectDisplacement8;
					displacementBytes = 1;
				}
				else
				{
					modRM.mod = ModRM::Mod::IndirectDisplacement32;
					displacementBytes = 4;
				}
				
				if(controlByte.hasRegExpression)
				{
					int8_t reg = ReadB1ExpressionValue(s, blockData);
					modRM.reg = reg;
					if(reg & 8) rex |= 0x44;	// Set rex and .r bit.
				}
				if(controlByte.hasScaleExpression)
				{
					assert(modRM.rm == kModRMRmSIB);
					int8_t scale = ReadB1ExpressionValue(s, blockData);
					// Assert scale is valid.
					assert(scale == 1 || scale == 2 || scale == 4 || scale == 8);
					sib.scale = __builtin_ctz(scale);
				}
				if(controlByte.hasIndexExpression)
				{
					assert(modRM.rm == kModRMRmSIB);
					int8_t index = ReadB1ExpressionValue(s, blockData);
					assert(index != kSIBNoIndex);
					sib.index = index;
					if(index & 8) rex |= 0x42;	// Set rex and .x bit.
				}
				if(controlByte.hasBaseExpression)
				{
					int8_t base = ReadB1ExpressionValue(s, blockData);
					if(base & 8) rex |= 0x41;	// Set rex and .b bit.
					
					if((base & 7) == kSIBNoBase && modRM.mod == ModRM::Mod::IndirectDisplacement0)
					{
						modRM.mod = ModRM::Mod::IndirectDisplacement8;
						displacementBytes = 1;
					}
					if((base & 7) == kModRMRmSIB)
					{
						modRM.rm = kModRMRmSIB;
						sib.base = kModRMRmSIB;
					}
					else if(modRM.rm == kModRMRmSIB)
					{
						sib.base = base;
					}
					else
					{
						assert(modRM.rm == 0);
						modRM.rm = base;
					}
				}
				
				if(rex) *p++ = rex;
				int opcodeLength = controlByte.opcodeLengthM1+1;
				memcpy(p, s, 4);
				p += opcodeLength;
				s += opcodeLength;
				*p++ = modRM.value;
				if(modRM.rm == kModRMRmSIB) *p++ = sib.value;
				memcpy(p, &displacement, 4);
				p += displacementBytes;
				CONTINUE;
			}
		CASE(DynamicOpcodeRV):
			{
				uint8_t vex3B2 = *s++;
				uint8_t vex3B3 = *s++;
				ModRM modRM = { .value = *s++ };
				
				DynamicOpcodeRVControlByte controlByte;
				controlByte.value = *s++;
				if(controlByte.hasReg0Expression)
				{
					int8_t reg = ReadB1ExpressionValue(s, blockData);
					if(reg & 8) vex3B2 |= 0x80;	// Set .r bit.
					modRM.reg = reg;
				}
				if(controlByte.hasReg1Expression)
				{
					int8_t vvvv = ReadB1ExpressionValue(s, blockData);
					assert((vvvv >> 4) == 0);
					vex3B3 = vvvv << 3;
				}

				if((vex3B2 & 0x7f) != 1 || (vex3B3 & 0x80))
				{
					// 3 byte VEX required.
					*p++ = 0xc4;
					*p++ = vex3B2 ^ 0xe0;
					*p++ = vex3B3 ^ 0x78;
				}
				else
				{
					// 2 byte VEX required.
					*p++ = 0xc5;
					*p++ = vex3B2 ^ vex3B3 ^ 0xf9;
				}
				int opcodeLength = controlByte.opcodeLengthM1+1;
				memcpy(p, s, 4);
				p += opcodeLength;
				s += opcodeLength;
				*p++ = modRM.value;
				CONTINUE;
			}
		CASE(DynamicOpcodeRVR):
			{
				uint8_t vex3B2 = *s++;
				uint8_t vex3B3 = *s++;
				ModRM modRM = { .value = *s++ };
				
				DynamicOpcodeRVRControlByte controlByte;
				controlByte.value = *s++;
				if(controlByte.hasReg0Expression)
				{
					int8_t reg = ReadB1ExpressionValue(s, blockData);
					if(reg & 8) vex3B2 |= 0x80;	// Set .r bit.
					modRM.reg = reg;
				}
				if(controlByte.hasReg1Expression)
				{
					int8_t vvvv = ReadB1ExpressionValue(s, blockData);
					assert((vvvv >> 4) == 0);
					vex3B3 = vvvv << 3;
				}
				if(controlByte.hasReg2Expression)
				{
					int8_t rm = ReadB1ExpressionValue(s, blockData);
					if(rm & 8) vex3B2 |= 0x20;		// Set .b bit.
					modRM.rm = rm;
				}

				if((vex3B2 & 0x7f) != 1 || (vex3B3 & 0x80))
				{
					// 3 byte VEX required.
					*p++ = 0xc4;
					*p++ = vex3B2 ^ 0xe0;
					*p++ = vex3B3 ^ 0x78;
				}
				else
				{
					// 2 byte VEX required.
					*p++ = 0xc5;
					*p++ = vex3B2 ^ vex3B3 ^ 0xf9;
				}
				int opcodeLength = controlByte.opcodeLengthM1+1;
				memcpy(p, s, 4);
				p += opcodeLength;
				s += opcodeLength;
				*p++ = modRM.value;
				CONTINUE;
			}
		CASE(DynamicOpcodeRVM):
			{
				uint8_t vex3B2 = *s++;
				uint8_t vex3B3 = *s++;
				ModRM modRM = { .value = *s++ };
				SIB sib = { .value = *s++ };
				
				DynamicOpcodeRVMControlByte controlByte = { .value = *s++ };
				
				int displacement = controlByte.hasDisplacementExpression ?
									ReadB4ExpressionValue(s, blockData) :
									ReadSigned32(s);
				int displacementBytes;
				if(modRM.mod == ModRM::Mod::IndirectDisplacement0 && sib.base == kSIBNoIndex)
				{
					displacementBytes = 4;
				}
				else if(modRM.mod == ModRM::Mod::IndirectDisplacement0 && displacement == 0)
				{
					displacementBytes = 0;
				}
				else if(displacement == (int8_t)displacement)
				{
					modRM.mod = ModRM::Mod::IndirectDisplacement8;
					displacementBytes = 1;
				}
				else
				{
					modRM.mod = ModRM::Mod::IndirectDisplacement32;
					displacementBytes = 4;
				}
				
				if(controlByte.hasReg0Expression)
				{
					int8_t reg = ReadB1ExpressionValue(s, blockData);
					modRM.reg = reg;
					if(reg & 8) vex3B2 |= 0x80;	// Set .r bit.
				}
				if(controlByte.hasReg1Expression)
				{
					int8_t vvvv = ReadB1ExpressionValue(s, blockData);
					assert((vvvv >> 4) == 0);
					vex3B3 = vvvv << 3;
				}
				if(controlByte.hasScaleExpression)
				{
					assert(modRM.rm == kModRMRmSIB);
					int8_t scale = ReadB1ExpressionValue(s, blockData);
					// Assert scale is valid.
					assert(scale == 1 || scale == 2 || scale == 4 || scale == 8);
					sib.scale = __builtin_ctz(scale);
				}
				if(controlByte.hasIndexExpression)
				{
					assert(modRM.rm == kModRMRmSIB);
					int8_t index = ReadB1ExpressionValue(s, blockData);
					assert(index != kSIBNoIndex);
					sib.index = index;
					if(index & 8) vex3B2 |= 0x40;	// Set .x bit.
				}
				if(controlByte.hasBaseExpression)
				{
					int8_t base = ReadB1ExpressionValue(s, blockData);
					if(base & 8) vex3B2 |= 0x20;	// Set .b bit.

					if((base & 7) == kSIBNoBase && modRM.mod == ModRM::Mod::IndirectDisplacement0)
					{
						modRM.mod = ModRM::Mod::IndirectDisplacement8;
						displacementBytes = 1;
					}
					if((base & 7) == kModRMRmSIB)
					{
						modRM.rm = kModRMRmSIB;
						sib.base = kModRMRmSIB;
					}
					else if(modRM.rm == kModRMRmSIB)
					{
						sib.base = base;
					}
					else
					{
						assert(modRM.rm == 0);
						modRM.rm = base;
					}
				}
				
				if((vex3B2 & 0x7f) != 1 || (vex3B3 & 0x80))
				{
					// 3 byte VEX required.
					*p++ = 0xc4;
					*p++ = vex3B2 ^ 0xe0;
					*p++ = vex3B3 ^ 0x78;
				}
				else
				{
					// 2 byte VEX required.
					*p++ = 0xc5;
					*p++ = vex3B2 ^ vex3B3 ^ 0xf9;
				}
				int opcodeLength = controlByte.opcodeLengthM1+1;
				memcpy(p, s, 4);
				p += opcodeLength;
				s += opcodeLength;
				*p++ = modRM.value;
				if(modRM.rm == kModRMRmSIB) *p++ = sib.value;
				memcpy(p, &displacement, 4);
				p += displacementBytes;
				CONTINUE;
			}
		// Scope block for labelId
		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
			uint32_t labelId;
#pragma clang diagnostic pop
		CASE(Label):
			labelId = ReadUnsigned32(s);
			assert(!IsLabelIdNamed(labelId) || !labels.Contains(labelId));
			goto DequeueAndProcessLabel;
		CASE(ExpressionLabel):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		DequeueAndProcessLabel:
			firstPassLabelOffsets.DequeueLabel(labelId);

			JitForwardReferenceMapLookupResult lookup = unresolvedLabels.Find(labelId);
			if(lookup.reference)
			{
				JitForwardReferenceData *data = lookup.reference;
				JitForwardReferenceData *last;
				do
				{
					Patch(data->p, data->data, (int32_t) (size_t) p);
					last = data;
					data = data->next;
				} while(data);
				unresolvedLabels.Remove(lookup, last);
			}
		
			// Insert into map.
			labels.Set(labelId, p);
			CONTINUE;
		}
		// Scope block for labelId
		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
			uint32_t labelId;
			uint32_t index;
			uint8_t *target;
#pragma clang diagnostic pop
		CASE(Rel8LabelCondition):
			labelId = ReadUnsigned32(s);
			goto ProcessExpressionLabelCondition;
		CASE(Rel8ExpressionLabelCondition):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessExpressionLabelCondition:
			target = (uint8_t*) labels.GetIfExists(labelId);
			if(target != nullptr)
			{
				index = *s++;
				goto ProcessLabelBackwardConditionTarget;
			}
			else goto ProcessLabelForwardCondition;
		CASE(Rel8LabelForwardCondition):
			labelId = ReadUnsigned32(s);
			goto ProcessLabelForwardCondition;
		CASE(Rel8ExpressionLabelForwardCondition):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessLabelForwardCondition:
            {
                index = *s++;
                ++s; // skip rel8Offset
                uint32_t jumpOffset = *s++;
				uint32_t currentOffset = forwardLabelReferences[index+blockData->forwardLabelReferenceOffset];
				assert(currentOffset != 0);
				uint32_t target = firstPassLabelOffsets.GetFirstLabelOffset(labelId);
				int32_t delta = int32_t(target - currentOffset);
				if(delta != (int8_t) delta) s += jumpOffset;
			}
			CONTINUE;
		CASE(Rel8LabelBackwardCondition):
			labelId = ReadUnsigned32(s);
			goto ProcessLabelBackwardCondition;
		CASE(Rel8ExpressionLabelBackwardCondition):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessLabelBackwardCondition:
			target = (uint8_t*) labels.Get(labelId);
		ProcessLabelBackwardConditionTarget:
			uint32_t rel8Offset = *s++;
            uint32_t jumpOffset = *s++;
			int32_t delta = int32_t(target - p - rel8Offset);
			if(delta != (int8_t) delta) s += jumpOffset;
			CONTINUE;
		}
		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
			uint32_t labelId;
			uint8_t *target;
#pragma clang diagnostic pop
		CASE(PatchLabel):
			labelId = ReadUnsigned32(s);
			goto ProcessPatchLabel;
		CASE(PatchExpressionLabel):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessPatchLabel:
			target = (uint8_t*) labels.GetIfExists(labelId);
			if(target == nullptr) goto ProcessForwardLabel;
			goto ProcessBackwardsTarget;
		CASE(PatchLabelForward):
			labelId = ReadUnsigned32(s);
			goto ProcessForwardLabel;
		CASE(PatchExpressionLabelForward):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessForwardLabel:
			{
				uint8_t bytes = *s++;
				uint8_t offset = *s++;
				Patch(p-offset, bytes, -(int32_t) (size_t) p);
				unresolvedLabels.Add(labelId, p-offset, bytes);
			}
			CONTINUE;
		CASE(PatchLabelBackward):
			labelId = ReadUnsigned32(s);
			goto ProcessBackwardsLabel;
		CASE(PatchExpressionLabelBackward):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessBackwardsLabel:
			target = (uint8_t*) labels.Get(labelId);
		ProcessBackwardsTarget:
            {
                int32_t delta = int32_t(target - p);
                uint8_t bytes = *s++;
                uint8_t offset = *s++;
                Patch(p-offset, bytes, delta);
            }
			CONTINUE;
		}
		CASE(PatchAbsoluteAddress):
			{
				CONTINUE1(1)
				uint32_t delay = *s++;
				uint64_t v;
				memcpy(&v, p-delay, 8);
				v += (uint64_t) p - delay;
				memcpy(p-delay, &v, 8);
				CONTINUE2;
			}
#if !USE_GOTO_LABELS
		default:
			assert(!"Unhandled opcode");
#endif
		}
	}
}

//============================================================================

uint32_t SegmentAssembler::ReadUnsigned16(const uint8_t* __restrict &s)
{
	uint16_t v;
	memcpy(&v, s, 2);
	s += 2;
	return v;
}

void SegmentAssembler::SkipExpressionValue(const uint8_t* __restrict &s)
{
	s += 2;
}

int8_t SegmentAssembler::ReadB1ExpressionValue(const uint8_t* __restrict &s, const AppendAssemblyReference *reference)
{
	const uint8_t *expressionData = (const uint8_t*) reference;
	uint32_t offset = ReadUnsigned16(s);
	return expressionData[offset];
}

int32_t SegmentAssembler::ReadB2ExpressionValue(const uint8_t* __restrict &s, const AppendAssemblyReference *reference)
{
	const uint8_t *expressionData = (const uint8_t*) reference;
	uint32_t offset = ReadUnsigned16(s);
	int16_t result;
	memcpy(&result, expressionData + offset, sizeof(result));
	return result;
}

int32_t SegmentAssembler::ReadB4ExpressionValue(const uint8_t* __restrict &s, const AppendAssemblyReference *reference)
{
	const uint8_t *expressionData = (const uint8_t*) reference;
	uint32_t offset = ReadUnsigned16(s);
	int32_t result;
	memcpy(&result, expressionData + offset, sizeof(result));
	return result;
}

int64_t SegmentAssembler::ReadB8ExpressionValue(const uint8_t* __restrict &s, const AppendAssemblyReference *reference)
{
	const uint8_t *expressionData = (const uint8_t*) reference;
	uint32_t offset = ReadUnsigned16(s);
	int64_t result;
	memcpy(&result, expressionData + offset, sizeof(result));
	return result;
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

void* Assembler::Build()
{
	if(hasDataSegment)
	{
		uint32_t numberOfForwardLabelReferences = allLabelData.numberOfForwardLabelReferences
			+ dataSegment.allLabelData.numberOfForwardLabelReferences;
		uint32_t numberOfLabels = allLabelData.numberOfLabels
			+ dataSegment.allLabelData.numberOfLabels;

		forwardLabelReferences.Append(numberOfForwardLabelReferences);
		firstPassLabelOffsets.Reserve(numberOfLabels);
		labels.Reserve(numberOfLabels);
		unresolvedLabels.Reserve(numberOfForwardLabelReferences);

		dataSegment.forwardLabelReferences.StartUseBacking(forwardLabelReferences);
		dataSegment.firstPassLabelOffsets.StartUseBacking(firstPassLabelOffsets);
		dataSegment.labels.StartUseBacking(labels);
		dataSegment.unresolvedLabels.StartUseBacking(unresolvedLabels);

		dataSegment.ProcessByteCode();
		
		dataSegment.forwardLabelReferences.StopUseBacking(forwardLabelReferences);
		dataSegment.firstPassLabelOffsets.StopUseBacking(firstPassLabelOffsets);
		dataSegment.labels.StopUseBacking(labels);
		dataSegment.unresolvedLabels.StopUseBacking(unresolvedLabels);
	}
	else
	{
		forwardLabelReferences.Append(allLabelData.numberOfForwardLabelReferences);
		firstPassLabelOffsets.Reserve(allLabelData.numberOfLabels);
		labels.Reserve(allLabelData.numberOfLabels);
		unresolvedLabels.Reserve(allLabelData.numberOfForwardLabelReferences);

	}
	ProcessByteCode();
	assert(!unresolvedLabels.HasData() && "Not all references have been resolved");
	return programStart;
}

#endif // defined(__amd64__)

//============================================================================
