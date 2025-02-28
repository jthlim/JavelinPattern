//============================================================================

#include "Javelin/Stream/CountWriter.h"

//============================================================================

using namespace Javelin; 

//============================================================================

size_t CountWriter::WriteAvailable(const void*, size_t dataSize)	
{ 
	count += dataSize;
	return dataSize; 
}

void CountWriter::WriteBlock(const void*, size_t dataSize)
{
	count += dataSize;
}

void CountWriter::WriteByte(unsigned char)
{
	++count;
}

//============================================================================
