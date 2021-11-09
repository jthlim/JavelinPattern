//============================================================================

#if 1 // defined(__riscv)

//============================================================================

#include "Javelin/Assembler/riscv/Assembler.h"

#include "Javelin/Assembler/JitMemoryManager.h"
#include "Javelin/Assembler/riscv/ActionType.h"
#include <assert.h>
#include <algorithm>
#include <stdint.h>

//============================================================================

#if DEBUG
  #define USE_GOTO_LABELS	0
#else
  #define USE_GOTO_LABELS	1
#endif

//============================================================================

using namespace Javelin;
using namespace Javelin::riscvAssembler;

//============================================================================

size_t Assembler::misa = 0x20000004;

//============================================================================

SegmentAssembler::SegmentAssembler(JitMemoryManager &aMemoryManager)
: memoryManager(aMemoryManager)
{
	buildData.Reserve(8192);
}

//============================================================================

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

inline int32_t SegmentAssembler::ReadSigned16(const uint8_t* &s)
{
    int16_t result;
    memcpy(&result, s, sizeof(result));
    s += sizeof(result);
    return result;
}

inline int32_t SegmentAssembler::ReadSigned32(const uint8_t* &s)
{
	int32_t result;
	memcpy(&result, s, sizeof(result));
	s += sizeof(result);
	return result;
}

inline uint32_t SegmentAssembler::ReadUnsigned32(const uint8_t* &s)
{
	uint32_t result;
	memcpy(&result, s, sizeof(result));
	s += sizeof(result);
	return result;
}

void SegmentAssembler::Patch(uint8_t* p, RelEncoding encoding, intptr_t delta)
{
    switch(encoding)
    {
    case RelEncoding::BDelta:
        {
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
            static_assert(sizeof(Opcode) == 4, "Opcode should be 4 bytes");
            Opcode opcode;
            memcpy(&opcode, p, 4);
            
            int32_t rel = opcode.imm4_1 << 1
                          | opcode.imm10_5 << 5
                          | opcode.imm11 << 11
                          | opcode.imm12 << 12;

            rel += delta;
            assert((rel & 1) == 0);
            assert(rel >> 12 == 0 || rel >> 12 == -1);

            opcode.imm4_1 = rel >> 1;
            opcode.imm10_5 = rel >> 5;
            opcode.imm11 = rel >> 11;
            opcode.imm12 = rel >> 12;
            
            memcpy(p, &opcode, 4);
        }
        break;
            
    case RelEncoding::JDelta:
        {
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
            static_assert(sizeof(Opcode) == 4, "Opcode should be 4 bytes");
            Opcode opcode;
            memcpy(&opcode, p, 4);
            
            int32_t rel = opcode.imm10_1 << 1
                          | opcode.imm11 << 11
                          | opcode.imm19_12 << 12
                          | opcode.imm20 << 20;

            rel += delta;
            assert((rel & 1) == 0);
            assert(rel >> 20 == 0 || rel >> 20 == -1);
            opcode.imm10_1 = rel >> 1;
            opcode.imm11 = rel >> 11;
            opcode.imm19_12 = rel >> 12;
            opcode.imm20 = rel >> 20;
            
            memcpy(p, &opcode, 4);
        }
        break;
            
    case RelEncoding::Auipc:
        {
            uint32_t opcode;
            memcpy(&opcode, p, 4);
            assert((delta & 3) == 0);
            delta = opcode + (uint32_t(delta) << 3);
            opcode = (opcode & ~0xffffe0) | (delta & 0xffffe0);
            memcpy(p, &opcode, 4);
        }
        break;
         
    case RelEncoding::CBDelta:
        {
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
            Opcode opcode;
            memcpy(&opcode, p, 2);
            
            int32_t rel = opcode.imm2_1 << 1
                            | opcode.imm4_3 << 3
                            | opcode.imm5 << 5
                            | opcode.imm7_6 << 6
                            | opcode.imm8 << 8;

            rel += delta;
            
            assert((rel & 1) == 0);
            assert(rel >> 8 == 0 || rel >> 8 == -1);

            opcode.imm2_1 = rel >> 1;
            opcode.imm4_3 = rel >> 3;
            opcode.imm5 = rel >> 5;
            opcode.imm7_6 = rel >> 6;
            opcode.imm8 = rel >> 8;

            memcpy(p, &opcode, 2);
        }
        break;

    case RelEncoding::CJDelta:
        {
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
            Opcode opcode;
            memcpy(&opcode, p, 2);
            
            int32_t rel = opcode.imm3_1 << 1
                            | opcode.imm4 << 4
                            | opcode.imm5 << 5
                            | opcode.imm6 << 6
                            | opcode.imm7 << 7
                            | opcode.imm9_8 << 8
                            | opcode.imm10 << 10
                            | opcode.imm11 << 11;

            rel += delta;
            
            assert((rel & 1) == 0);
            assert(rel >> 11 == 0 || rel >> 11 == -1);

            opcode.imm3_1 = rel >> 1;
            opcode.imm4 = rel >> 4;
            opcode.imm5 = rel >> 5;
            opcode.imm6 = rel >> 6;
            opcode.imm7 = rel >> 7;
            opcode.imm9_8 = rel >> 8;
            opcode.imm10 = rel >> 10;
            opcode.imm11 = rel >> 11;

            memcpy(p, &opcode, 2);
        }
        break;
            
    case RelEncoding::Imm12:
        {
            uint32_t opcode;
            memcpy(&opcode, p, 4);
// TODO
//                      uint32_t rel = (opcode >> 10) & 0xfff;
//
//                      if(int64_t(p) + delta == 0) rel = int32_t(delta);
//                      else rel += (int64_t(p) + delta);
//                      opcode = (opcode & ~0x3ffc00) | ((rel << 10) & 0x3ffc00);
//                      memcpy(p, &opcode, 4);
        }
        break;
            
    case RelEncoding::Rel64:
        {
            int64_t rel;
            memcpy(&rel, p, 8);
            rel += delta;
            memcpy(p, &rel, 8);
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
	
	const  uint8_t* s = blockData->assemblerData;
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
        CASE(Literal4):  CONTINUE1(4);  offset += 4;  s += 4;  CONTINUE2;
        CASE(Literal6):  CONTINUE1(6);  offset += 6;  s += 6;  CONTINUE2;
        CASE(Literal8):  CONTINUE1(8);  offset += 8;  s += 8;  CONTINUE2;
        CASE(Literal10): CONTINUE1(10); offset += 10; s += 10; CONTINUE2;
        CASE(Literal12): CONTINUE1(12); offset += 12; s += 12; CONTINUE2;
        CASE(Literal14): CONTINUE1(14); offset += 14; s += 14; CONTINUE2;
        CASE(Literal16): CONTINUE1(16); offset += 16; s += 16; CONTINUE2;
        CASE(Literal18): CONTINUE1(18); offset += 18; s += 18; CONTINUE2;
        CASE(Literal20): CONTINUE1(20); offset += 20; s += 20; CONTINUE2;
        CASE(Literal22): CONTINUE1(22); offset += 22; s += 22; CONTINUE2;
        CASE(Literal24): CONTINUE1(24); offset += 24; s += 24; CONTINUE2;
        CASE(Literal26): CONTINUE1(26); offset += 26; s += 26; CONTINUE2;
        CASE(Literal28): CONTINUE1(28); offset += 28; s += 28; CONTINUE2;
        CASE(Literal30): CONTINUE1(30); offset += 30; s += 30; CONTINUE2;
        CASE(Literal32): CONTINUE1(32); offset += 32; s += 32; CONTINUE2;

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
			offset += 4;
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
        CASE(MaskedPatchB1Opcode):
        CASE(UnsignedPatchB1Opcode):
        CASE(SignedPatchB1Opcode):
        CASE(MaskedPatchB2Opcode):
        CASE(UnsignedPatchB2Opcode):
        CASE(SignedPatchB2Opcode):
        CASE(MaskedPatchB4Opcode):
        CASE(UnsignedPatchB4Opcode):
        CASE(SignedPatchB4Opcode):
            // Skip:
            // 1 byte: Number of bits
            // 1 byte: bitOffset
            // 1 byte: valueShift
            // 2 bytes: Expression offset
            // 2 bytes: Write offset
            s += 7;
            CONTINUE;
        CASE(CIValuePatchB1Opcode):
            // Skip:
            // 2 bytes: expression offset
            // 2 bytes: Write offset
            s += 4;
            CONTINUE;
        CASE(RepeatPatch2BOpcode):
        CASE(RepeatPatch4BOpcode):
            // Skip write offset
            s += 2;
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
        CASE(SimmCondition):
            {
                int64_t v = ReadB4ExpressionValue(s, blockData);
                int bitCountMinus1 = *s++;
                uint32_t offset = *s++;
                v >>= bitCountMinus1;
                if(v != 0 && v != -1) s += offset;
                CONTINUE;
            }
        CASE(UimmCondition):
            {
                uint64_t v = ReadB4ExpressionValue(s, blockData);
                int bitCount = *s++;
                uint32_t offset = *s++;
                if((v >> bitCount) != 0) s += offset;
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
		CASE(DeltaLabelCondition):
			labelId = ReadUnsigned32(s);
			goto ProcessLabelCondition;
		CASE(DeltaExpressionLabelCondition):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessLabelCondition:
			{
				uint32_t target = firstPassLabelOffsets.GetLastLabelOffsetIfExists(labelId);
				uint32_t index = *s++;
                int bitShift = *s++;
				uint32_t jumpOffset = *s++;
				if(target != JitLabelOffsetQueue::NO_LABEL)
				{
					int32_t delta = target - offset;
                    delta >>= bitShift;
					if(delta != 0 && delta != 1) offset += jumpOffset;
					else takeJump = true;
				}
				else
				{
					forwardLabelReferences[index+blockData->forwardLabelReferenceOffset] = offset;
					s += jumpOffset;
				}
			}
			CONTINUE;
		}
		CASE(DeltaLabelForwardCondition):
			s += 4; // skip label
			goto ProcessForwardLabelCondition;
		CASE(DeltaExpressionLabelForwardCondition):
			SkipExpressionValue(s);
		ProcessForwardLabelCondition:
			{
				uint32_t index = *s++;
                ++s;   // Skip bit shift.
				uint32_t jumpOffset = *s++;
				forwardLabelReferences[index+blockData->forwardLabelReferenceOffset] = offset;
				s += jumpOffset;
			}
			CONTINUE;
		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
		uint32_t labelId;
#pragma clang diagnostic pop
		CASE(DeltaLabelBackwardCondition):
			labelId = ReadUnsigned32(s);
			goto ProcessBackwardCondition;
		CASE(DeltaExpressionLabelBackwardCondition):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessBackwardCondition:
			{
                int bitShift = *s++;
				uint32_t rel8Offset = *s++;
				uint32_t jumpOffset = *s++;
				uint32_t target = firstPassLabelOffsets.GetLastLabelOffset(labelId);
				int32_t delta = target - (offset + rel8Offset);
				if((delta >> bitShift) != -1) offset += jumpOffset;
				else takeJump = true;
			}
			CONTINUE;
		}
		CASE(PatchLabel):
		CASE(PatchLabelBackward):
		CASE(PatchLabelForward):
			CONTINUE1(7);
            s += 4;                 // Skip labelId
            s += 1;                 // Skip relEncoding
			s += 2;		            // Skip offset
			CONTINUE2;
        CASE(PatchExpression):
		CASE(PatchExpressionLabel):
		CASE(PatchExpressionLabelBackward):
		CASE(PatchExpressionLabelForward):
			CONTINUE1(5);
			SkipExpressionValue(s); // Skip label expression
            s += 1;                 // Skip relEncoding
			s += 2;					// Skip offset
			CONTINUE2;
		CASE(PatchAbsoluteAddress):
            s += 2;                 // Skip offset
			CONTINUE;
#if !USE_GOTO_LABELS
		default:
			assert(!"Unhandled opcode");
#endif
		}
	}
}

uint8_t* SegmentAssembler::GenerateByteCode(uint8_t *p)
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
	const uint8_t* s = blockData->assemblerData;
#if !USE_GOTO_LABELS
    uint32_t opcodeValue;
#endif
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
        CASE(Literal4): CONTINUE1(4); memcpy(p, s, 4); p += 4; s += 4; CONTINUE2;
        CASE(Literal6): CONTINUE1(6); memcpy(p, s, 8); p += 6; s += 6; CONTINUE2;
        CASE(Literal8): CONTINUE1(8); memcpy(p, s, 8); p += 8; s += 8; CONTINUE2;
        CASE(Literal10): CONTINUE1(10); memcpy(p, s, 12); p += 10; s += 10; CONTINUE2;
        CASE(Literal12): CONTINUE1(12); memcpy(p, s, 12); p += 12; s += 12; CONTINUE2;
        CASE(Literal14): CONTINUE1(14); memcpy(p, s, 16); p += 14; s += 14; CONTINUE2;
        CASE(Literal16): CONTINUE1(16); memcpy(p, s, 16); p += 16; s += 16; CONTINUE2;
        CASE(Literal18): CONTINUE1(18); memcpy(p, s, 20); p += 18; s += 18; CONTINUE2;
        CASE(Literal20): CONTINUE1(20); memcpy(p, s, 20); p += 20; s += 20; CONTINUE2;
        CASE(Literal22): CONTINUE1(22); memcpy(p, s, 24); p += 22; s += 22; CONTINUE2;
        CASE(Literal24): CONTINUE1(24); memcpy(p, s, 24); p += 24; s += 24; CONTINUE2;
        CASE(Literal26): CONTINUE1(26); memcpy(p, s, 28); p += 26; s += 26; CONTINUE2;
        CASE(Literal28): CONTINUE1(28); memcpy(p, s, 28); p += 28; s += 28; CONTINUE2;
        CASE(Literal30): CONTINUE1(30); memcpy(p, s, 32); p += 30; s += 30; CONTINUE2;
        CASE(Literal32): CONTINUE1(32); memcpy(p, s, 32); p += 32; s += 32; CONTINUE2;
                
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
                CONTINUE1(1);
                uint8_t alignmentM1 = *s++;
                if (size_t(p) & 1) *p++ = 0;
                if (size_t(p) & 2)
                {
                    const uint16_t opcode = 0x0001;
                    memcpy(p, &opcode, 2);
                    p += 2;
                }
                while(size_t(p) & alignmentM1)
                {
                    const uint32_t opcode = 0x00000013;
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
                    const uint32_t opcode = 0x00000013;
                    memcpy(p, &opcode, 4);
                    p += 4;
                }
                CONTINUE2;
            }
		CASE(Alternate):
			++s;	// Skip maximumInstructionSize
			CONTINUE;
		CASE(EndAlternate):
			CONTINUE;
		CASE(Jump):
			s = s + *s + 1;
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
            }
            goto ProcessPatch4BOpcode;
        CASE(UnsignedPatchB1Opcode):
            {
                int bitMask = *s++;
                int bitOffset = *s++;
                int valueShift = *s++;
                uint8_t value = ReadB1ExpressionValue(s, blockData);
                assert((value & ((1<<valueShift)-1)) == 0);
                assert((value & ~bitMask) == 0);
                opcodeValue = value >> valueShift << bitOffset;
            }
            goto ProcessPatch4BOpcode;
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
            }
            goto ProcessPatch4BOpcode;
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
            }
            goto ProcessPatch4BOpcode;
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
            }
            goto ProcessPatch4BOpcode;
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
            }
            goto ProcessPatch4BOpcode;
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
            }
            goto ProcessPatch4BOpcode;
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
            }
            goto ProcessPatch4BOpcode;
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
            }
            goto ProcessPatch4BOpcode;
        CASE(CIValuePatchB1Opcode):
            {
                // Skip:
                // 2 bytes: expression offset
                // 2 bytes: Write offset
                int32_t value = ReadB1ExpressionValue(s, blockData);
                assert(-32 <= value && value < 32);
                opcodeValue = (value << 2) & 0x107c;
            }
            goto ProcessPatch2BOpcode;
        CASE(RepeatPatch2BOpcode):
        ProcessPatch2BOpcode:
            {
                int offset = ReadSigned16(s);
                uint16_t opcode;
                memcpy(&opcode, p+offset, 2);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconditional-uninitialized"
                opcode |= opcodeValue;
#pragma clang diagnostic pop
                memcpy(p+offset, &opcode, 2);
            }
            CONTINUE;
        CASE(RepeatPatch4BOpcode):
        ProcessPatch4BOpcode:
            {
                int offset = ReadSigned16(s);
                uint32_t opcode;
                memcpy(&opcode, p+offset, 4);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconditional-uninitialized"
                opcode |= opcodeValue;
#pragma clang diagnostic pop
                memcpy(p+offset, &opcode, 4);
            }
            CONTINUE;
#if USE_GOTO_LABELS
        }
#endif
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
        CASE(SimmCondition):
            {
                int64_t v = ReadB4ExpressionValue(s, blockData);
                int bitCount = *s++;
                uint32_t offset = *s++;
                v >>= bitCount;
                if(v != 0 && v != -1) s += offset;
                CONTINUE;
            }
        CASE(UimmCondition):
            {
                uint64_t v = ReadB4ExpressionValue(s, blockData);
                int bitCount = *s++;
                uint32_t offset = *s++;
                if((v >> bitCount) != 0) s += offset;
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
					Patch(data->p, (RelEncoding) data->data, (intptr_t) p - (intptr_t) data->p);
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
		CASE(DeltaLabelCondition):
			labelId = ReadUnsigned32(s);
			goto ProcessExpressionLabelCondition;
		CASE(DeltaExpressionLabelCondition):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessExpressionLabelCondition:
			target = (uint8_t*) labels.GetIfExists(labelId);
			if(target != nullptr)
			{
				index = *s++;
				goto ProcessLabelBackwardConditionTarget;
			}
			else goto ProcessLabelForwardCondition;
		CASE(DeltaLabelForwardCondition):
			labelId = ReadUnsigned32(s);
			goto ProcessLabelForwardCondition;
		CASE(DeltaExpressionLabelForwardCondition):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessLabelForwardCondition:
            {
                index = *s++;
                int bitShift = *s++;
                uint32_t jumpOffset = *s++;
				uint32_t currentOffset = forwardLabelReferences[index+blockData->forwardLabelReferenceOffset];
				assert(currentOffset != 0);
				uint32_t target = firstPassLabelOffsets.GetFirstLabelOffset(labelId);
				int32_t delta = int32_t(target - currentOffset);
				if((delta >> bitShift) != 0) s += jumpOffset;
			}
			CONTINUE;
		CASE(DeltaLabelBackwardCondition):
			labelId = ReadUnsigned32(s);
			goto ProcessLabelBackwardCondition;
		CASE(DeltaExpressionLabelBackwardCondition):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessLabelBackwardCondition:
			target = (uint8_t*) labels.Get(labelId);
		ProcessLabelBackwardConditionTarget:
            int bitShift = *s++;
			uint32_t jumpOffset = *s++;
			int32_t delta = int32_t(target - p);
			if((delta >> bitShift) != -1) s += jumpOffset;
			CONTINUE;
		}
		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
			uint32_t labelId;
			const uint8_t *target;
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
                JitForwardReferenceData *refData = unresolvedLabels.Add(labelId);
                RelEncoding encoding = (RelEncoding) *s++;
                uint8_t *patchAddress = p + ReadSigned16(s);
                refData->data = (uint32_t) encoding;
                refData->p = patchAddress;
			}
			CONTINUE;
        CASE(PatchExpression):
            target = (const uint8_t*) ReadB8ExpressionValue(s, blockData);
            goto ProcessBackwardsTarget;
		CASE(PatchLabelBackward):
			labelId = ReadUnsigned32(s);
			goto ProcessBackwardsLabel;
		CASE(PatchExpressionLabelBackward):
			labelId = GetLabelIdForExpression(ReadB4ExpressionValue(s, blockData));
		ProcessBackwardsLabel:
			target = (uint8_t*) labels.Get(labelId);
		ProcessBackwardsTarget:
			RelEncoding encoding = (RelEncoding) *s++;
            int32_t offset = ReadSigned16(s);
            uint8_t *patchAddress = p + offset;
            int32_t delta = int32_t(target - patchAddress);
			Patch(patchAddress, encoding, delta);
			CONTINUE;
		}
		CASE(PatchAbsoluteAddress):
			{
				CONTINUE1(2)
                int32_t offset = ReadSigned16(s);
				uint64_t v;
				memcpy(&v, p+offset, 8);
				v += (uint64_t) p+offset;
				memcpy(p+offset, &v, 8);
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

uint32_t SegmentAssembler::ReadUnsigned16(const uint8_t* &s)
{
	uint16_t v;
	memcpy(&v, s, 2);
	s += 2;
	return v;
}

void SegmentAssembler::SkipExpressionValue(const uint8_t* &s)
{
	s += 2;
}

int8_t SegmentAssembler::ReadB1ExpressionValue(const uint8_t* &s, const AppendAssemblyReference *reference)
{
	const uint8_t *expressionData = (const uint8_t*) reference;
	uint32_t offset = ReadUnsigned16(s);
	return expressionData[offset];
}

int32_t SegmentAssembler::ReadB2ExpressionValue(const uint8_t* &s, const AppendAssemblyReference *reference)
{
	const uint8_t *expressionData = (const uint8_t*) reference;
	uint32_t offset = ReadUnsigned16(s);
	int16_t result;
	memcpy(&result, expressionData + offset, sizeof(result));
	return result;
}

int32_t SegmentAssembler::ReadB4ExpressionValue(const uint8_t* &s, const AppendAssemblyReference *reference)
{
	const uint8_t *expressionData = (const uint8_t*) reference;
	uint32_t offset = ReadUnsigned16(s);
	int32_t result;
	memcpy(&result, expressionData + offset, sizeof(result));
	return result;
}

int64_t SegmentAssembler::ReadB8ExpressionValue(const uint8_t* &s, const AppendAssemblyReference *reference)
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

#endif // defined(__riscv)

//============================================================================
