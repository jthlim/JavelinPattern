//============================================================================

#include "Javelin/Stream/DataBlockWriter.h"

//============================================================================

using namespace Javelin;

//============================================================================

DataBlockWriter::DataBlockWriter(size_t initialCapacity)
: dataBlock(initialCapacity)
{
}

DataBlockWriter::DataBlockWriter(DataBlockWriter&& a)
: dataBlock((DataBlock&&) a.dataBlock)
{
}

//============================================================================

size_t DataBlockWriter::WriteAvailable(const void *data, size_t dataSize)
{
	dataBlock.AppendCount(static_cast<const unsigned char*>(data), dataSize);
	return dataSize;
}

void DataBlockWriter::WriteBlock(const void* data, size_t dataSize)
{
	dataBlock.AppendCount(static_cast<const unsigned char*>(data), dataSize);
}

void DataBlockWriter::WriteByte(unsigned char c)
{
	dataBlock.Append(c);
}

//============================================================================

void DataBlockWriter::AdoptIfSmaller(DataBlockWriter& a)
{
	if(a.GetCount() < GetCount())
	{
		dataBlock = (DataBlock&&) a.dataBlock;
	}
}

//============================================================================
