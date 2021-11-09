//============================================================================

#pragma once
#include "Javelin/Assembler/JitVector.h"
#include "Javelin/Assembler/JitForwardReferenceMap.h"
#include "Javelin/Assembler/JitLabelId.h"
#include "Javelin/Assembler/JitLabelMap.h"
#include "Javelin/Assembler/JitLabelOffsetQueue.h"
#include "Javelin/Assembler/JitMemoryManager.h"
#include "Javelin/Assembler/x64/ActionType.h"
#include <stdint.h>
#include <string.h>

//============================================================================

namespace Javelin
{
//============================================================================

	class SegmentAssembler
	{
	public:
		SegmentAssembler(JitMemoryManager &aMemoryManager);
		
		void AppendInstructionData(uint32_t blockByteCodeSize, const uint8_t *s);
		void* AppendInstructionData(uint32_t blockByteCodeSize, const uint8_t *s, uint32_t referenceAndDataLength);
		void* AppendInstructionData(uint32_t blockByteCodeSize, const uint8_t *s, uint32_t referenceAndDataLength, uint32_t labelData);

		// Appends an instruction to append bytes of byteSize.
		// Returns a region of memory that the caller should populate byteSize bytes.
		void* AppendData(uint32_t byteSize);
		
		void AppendData(const void *data, uint32_t byteSize)  { memcpy(AppendData(byteSize), data, byteSize); }
		void AppendDataPointer(const void *data, uint32_t byteSize);

		// Returns address to start of assembly.
		// After this, the Assembler class is not required and can be destroyed.
		void* Build();
		
        void* GetLabelIdAddress(uint32_t labelId) const       { return labels.GetIfExists(labelId); }
		void* GetNamedLabelAddress(const char *label) const   { return labels.GetIfExists(GetLabelIdForNamed(label)); }
		void* GetNumericLabelAddress(uint32_t label) const    { return labels.GetIfExists(GetLabelIdForNumeric(label)); }
		void* GetExpressionLabelAddress(uint32_t label) const { return labels.GetIfExists(GetLabelIdForExpression(label)); }

		void Reset()										  { buildData.Clear(); allLabelData = { }; }

	private:
		// Any changes to this need to be reflected in:
		//  - Assembler::x64::Action::WriteExpressionOffset
		//  - Assembler::x64::Assembler::Write
		struct AppendAssemblyReference
		{
			uint16_t referenceSize;
			uint16_t blockByteCodeSize;
			uint32_t forwardLabelReferenceOffset;
			const uint8_t* assemblerData;

			const AppendAssemblyReference* GetNext() const { return (AppendAssemblyReference*) (size_t(this) + referenceSize); }
		};

		struct AppendDataReference : public AppendAssemblyReference
		{
			uint32_t dataSize;
		};
		
		struct AppendDataPointerReference : public AppendDataReference
		{
			const uint8_t *pData;
		};

		JitVector<uint8_t> buildData;

		struct LabelData
		{
			uint32_t numberOfLabels;
			uint32_t numberOfForwardLabelReferences;
		};
		LabelData allLabelData = { };
		
		// Mapping of label id -> max offset.
		JitLabelOffsetQueue firstPassLabelOffsets;
		
		// Map of label id -> offset
		JitLabelMap labels;
		
		// [index] -> max offset.
		JitVector<uint32_t> forwardLabelReferences;

		// Unresolved named labels
		JitForwardReferenceMap unresolvedLabels;
		
		uint8_t *programStart;
		JitMemoryManager &memoryManager;

		void ProcessLabelData(uint32_t labelData);
		
		void ProcessByteCode();
		uint32_t PrepareGenerateByteCode();
		uint8_t *GenerateByteCode(uint8_t* __restrict p);

		static int32_t ReadSigned32(const uint8_t* __restrict &s);
		static uint32_t ReadUnsigned32(const uint8_t* __restrict &s);
		static uint32_t ReadUnsigned16(const uint8_t* __restrict &s);
		static void SkipExpressionValue(const uint8_t* __restrict &s);
		static void Patch(uint8_t* __restrict p, uint32_t bytes, int32_t delta);
		int8_t ReadB1ExpressionValue(const uint8_t* __restrict &s, const AppendAssemblyReference *reference);
		int32_t ReadB2ExpressionValue(const uint8_t* __restrict &s, const AppendAssemblyReference *reference);
		int32_t ReadB4ExpressionValue(const uint8_t* __restrict &s, const AppendAssemblyReference *reference);
		int64_t ReadB8ExpressionValue(const uint8_t* __restrict &s, const AppendAssemblyReference *reference);

		friend class Assembler;
	};

//============================================================================

class Assembler : public SegmentAssembler
{
public:
	Assembler(JitMemoryManager *codeSegmentMemoryManager = SimpleJitMemoryManager::GetInstance(),
			  JitMemoryManager *dataSegmentMemoryManager = nullptr);
	~Assembler();
	
	// Returns address to start of assembly.
	// After this, the Assembler class is not required and can be destroyed.
	void* Build();

	void* GetCodeSegment()	 { return programStart; }
	void* GetDataSegment()	 { return dataSegment.programStart; }

	SegmentAssembler& GetDataSegmentAssembler() { return dataSegment; }

private:
	bool hasDataSegment = false;
	union
	{
		SegmentAssembler dataSegment;
	};
};

//============================================================================
} // namespace Javelin
//============================================================================
