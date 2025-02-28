//============================================================================

#pragma once
#include "Javelin/Stream/IWriter.h"
#include "Javelin/Type/String.h"

//============================================================================

namespace Javelin 
{
//============================================================================

	class StringWriter : public IWriter
	{
	private:
		StringData	stringData;

	public:
		JEXPORT StringWriter();
		JEXPORT ~StringWriter();

		// Getting the string will Reset the written stream!
		JEXPORT String JCALL GetString();
		
		JINLINE size_t JCALL GetCount() const { return stringData.GetCount(); }

		// From IWriter
		JEXPORT size_t JCALL WriteAvailable(const void *data, size_t dataSize) final;
		JEXPORT void   JCALL WriteByte(unsigned char c) final;
		JEXPORT void   JCALL WriteBlock(const void *data, size_t dataSize) final;
	};

//============================================================================
} // namespace Javelin
//============================================================================
