//============================================================================

#pragma once
#include "Javelin/Assembler/JitVector.h"
#include "Javelin/Assembler/JitForwardReferenceMap.h"
#include "Javelin/Assembler/JitLabelId.h"
#include "Javelin/Assembler/JitLabelMap.h"
#include "Javelin/Assembler/JitMemoryManager.h"
#include "Javelin/Assembler/arm64/ActionType.h"
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
		
		void AppendData(const void *data, uint32_t byteSize) { memcpy(AppendData(byteSize), data, byteSize); }
		void AppendDataPointer(const void *data, uint32_t byteSize);

		void* GetNamedLabelAddress(const char *label) const   { return labels.GetIfExists(GetLabelIdForNamed(label)); }
		void* GetNumericLabelAddress(uint32_t label) const    { return labels.GetIfExists(GetLabelIdForNumeric(label)); }
		void* GetExpressionLabelAddress(uint32_t label) const { return labels.GetIfExists(GetLabelIdForExpression(label)); }

		void Reset()										  { buildData.Clear(); aggregateData = { }; }
		
		static bool IsValidBitmask32(uint64_t v)			  { return IsValidBitmask64(v | (v << 32)); }
		static bool IsValidBitmask64(uint64_t v);
		
	private:
		// Any changes to this need to be reflected in:
		//  - Assembler::arm64::Action::WriteExpressionOffset
		//  - Assembler::arm64::Assembler::Write
		struct AppendAssemblyReference
		{
			const uint8_t* assemblerData;
			uint32_t referenceSize;
			
			const AppendAssemblyReference* GetNext() const { return (AppendAssemblyReference*) (size_t(this) + referenceSize); }
		};
		static_assert(sizeof(AppendAssemblyReference) == 16, "Expect AppendAssemblyReference to be 16 bytes. Update Assembler::arm64::Assembler::Write if this has changed");

		// Deriving from AppendAssemblyReference doesn't pack dataSize next to blockSize.
		struct AppendByteReference /* : public AppendAssemblyReference */
		{
			const uint8_t* assemblerData;
			uint32_t referenceSize;
			uint32_t dataSize;
		};

		struct AppendDataPointerReference : public AppendByteReference
		{
			const uint8_t *pData;
		};

		JitVector<uint8_t> buildData;

		struct AggregateData
		{
			uint32_t byteCodeSize;
			uint32_t numberOfLabels;
			uint32_t numberOfForwardLabelReferences;
		};
		
		AggregateData aggregateData = { };
		
		// Map of label id -> offset
		JitLabelMap labels;
		
		// Unresolved named labels
		JitForwardReferenceMap unresolvedLabels;
		
		uint8_t *programStart;
		JitMemoryManager &memoryManager;

		void ProcessLabelData(uint32_t labelData);
		
		void ProcessByteCode();
		uint8_t *GenerateByteCode(uint8_t* p);

		static int32_t ReadSigned16(const uint8_t* &s);
		static uint32_t ReadUnsigned16(const uint8_t* &s);
		static uint32_t ReadUnsigned32(const uint8_t* &s);
		static void SkipExpressionValue(const uint8_t* &s);
		static void Patch(uint8_t *p, uint32_t encoding, int64_t delta);
		uint32_t LogicalOpcodeValue(uint64_t v);
		int8_t ReadB1ExpressionValue(const uint8_t *&s, const AppendAssemblyReference *blockData);
		int32_t ReadB2ExpressionValue(const uint8_t *&s, const AppendAssemblyReference *blockData);
		int32_t ReadB4ExpressionValue(const uint8_t *&s, const AppendAssemblyReference *blockData);
		int64_t ReadB8ExpressionValue(const uint8_t *&s, const AppendAssemblyReference *blockData);
		
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
