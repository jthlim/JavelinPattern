//============================================================================

#pragma once
#include "Javelin/Stream/IWriter.h"

//============================================================================

namespace Javelin 
{
//============================================================================

	class NullWriter : public IWriter
	{
	public:
		void JCALL Release()				{ }

		size_t JCALL WriteAvailable(const void *data, size_t dataSize) final;
		void JCALL WriteBlock(const void *data, size_t dataSize) final;
		void JCALL WriteByte(unsigned char a) final;

		void JCALL  VPrintF(const char *format, va_list arguments) final;
	};

//============================================================================

	JEXPORT extern NullWriter NullOutput;

//============================================================================
} // namespace Javelin
//============================================================================
