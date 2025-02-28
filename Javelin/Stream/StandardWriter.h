//============================================================================

#pragma once
#include "Javelin/Stream/IWriter.h"

//============================================================================

namespace Javelin 
{
//============================================================================

	class StandardWriter : public IWriter
	{
	public:
		StandardWriter(int aFd) : fd(aFd), bufferPosition(0) { }
		~StandardWriter();
        
		// From IWriter
		void JCALL Release()	{ }
        
		void   JCALL WriteByte(unsigned char a) final;
		size_t JCALL WriteAvailable(const void *data, size_t dataSize) final;
        
		void  JCALL Flush();

	private:
		enum { BUFFER_SIZE = 512 };

		int fd;

		int				bufferPosition;
		unsigned char	buffer[BUFFER_SIZE];

		inline void QueueCharacter(unsigned char c);
	};

//============================================================================

	JEXPORT extern StandardWriter	StandardOutput;
	JEXPORT extern StandardWriter	StandardError;

//============================================================================
} // namespace Javelin
//============================================================================
