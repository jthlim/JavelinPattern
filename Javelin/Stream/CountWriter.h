//============================================================================

#pragma once
#include "Javelin/Stream/IWriter.h"

//============================================================================

namespace Javelin 
{
//============================================================================

	class CountWriter : public IWriter
	{
	public:
		CountWriter() { count = 0; }
		
		size_t JCALL WriteAvailable(const void *data, size_t dataSize) final;
		void JCALL WriteBlock(const void *data, size_t dataSize) final;
		void JCALL WriteByte(unsigned char a) final;
		
		size_t GetCount() const { return count; }
		
	private:
		size_t count;
	};

//============================================================================
} // namespace Javelin
//============================================================================
