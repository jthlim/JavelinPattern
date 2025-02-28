//============================================================================

#pragma once
#include "Javelin/Stream/IWriter.h"

//============================================================================

namespace Javelin 
{
//============================================================================

	class MemoryWriter : public IWriter
	{
	public:
		JEXPORT MemoryWriter(void* destination, size_t maximumLength);
	
		// From IWriter
		JEXPORT bool   JCALL CanSeek() const final;
		JEXPORT void   JCALL Seek(long long offset, SeekType whence = SeekType::Current) final;
		using IWriter::Seek;
		JEXPORT long long JCALL GetOffset() const final;
		JEXPORT long long JCALL GetLength() const final;
		JEXPORT long long JCALL GetRemainingLength() const final;

		JEXPORT bool   JCALL IsWritable() const final;
		JEXPORT size_t JCALL WriteAvailable(const void *data, size_t dataSize) final;
		JEXPORT void   JCALL WriteBlock(const void *data, size_t dataSize) final;
		JEXPORT void   JCALL WriteByte(unsigned char c) final;
		
	private:
		unsigned char*	base;
		size_t			position;
		size_t			dataLength;
	};

//============================================================================
} // namespace Javelin
//============================================================================
