//============================================================================

#include "Javelin/Stream/StandardWriter.h"
#include <unistd.h>
#include <sys/uio.h>

//============================================================================

using namespace Javelin;

//============================================================================

StandardWriter::~StandardWriter()
{
	Flush();
}

//============================================================================

inline void StandardWriter::QueueCharacter(unsigned char c)
{
	buffer[bufferPosition++] = c;

	if(bufferPosition >= BUFFER_SIZE || c == '\n')
	{
		write(fd, buffer, bufferPosition);
		bufferPosition = 0;
	}
}

//============================================================================

void StandardWriter::WriteByte(unsigned char a)
{
	QueueCharacter(a);
}

size_t StandardWriter::WriteAvailable(const void *data, size_t dataSize)
{
	if(bufferPosition != 0)
	{
		iovec vec[2];
		vec[0].iov_base = buffer;
		vec[0].iov_len = bufferPosition;
		vec[1].iov_base = (void*) data;
		vec[1].iov_len = dataSize;
		writev(fd, vec, 2);
		bufferPosition = 0;
	}
	else
	{
		write(fd, data, dataSize);
	}

	return dataSize;
}

//============================================================================

void StandardWriter::Flush()
{
	if(bufferPosition)
	{
		write(fd, buffer, bufferPosition);
		bufferPosition = 0;
	}
}

//============================================================================
