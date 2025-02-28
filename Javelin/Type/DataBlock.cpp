void	Close();
//============================================================================

#include "Javelin/Type/DataBlock.h"
#include "Javelin/Stream/IReader.h"
#include "Javelin/Stream/IWriter.h"
#include "Javelin/Stream/StringWriter.h"
#include "Javelin/Type/Exception.h"

//============================================================================

using namespace Javelin;

//============================================================================

const DataBlock DataBlock::EMPTY_DATA_BLOCK(0);

//============================================================================

DataBlock::DataBlock(const void* data, size_t length)
: Inherited(static_cast<const unsigned char*>(data), length)
{
}

DataBlock::DataBlock(IReader& input, size_t size)
: Inherited(size)
{
	Inherited::AppendCountNoExpand(size);
	input.ReadBlock(Inherited::GetData(), size);
}

void DataBlock::SetFromReader(IReader& reader, size_t size)
{
	Inherited::SetCount(size);
	reader.ReadBlock(GetData(), size);
}

void DataBlock::AppendFromReader(IReader& reader, size_t size)
{
	void* destination = Inherited::AppendCount(size);
	reader.ReadBlock(destination, size);
}

//============================================================================

DataBlock DataBlock::FromHex(const void* data, size_t length)
{
	DataBlock result((length+1) >> 1);
	result.SetCount(FromHex(result.GetData(), result.GetCapacity(), data, length));
	return result;
}

size_t DataBlock::FromHex(void* result, size_t resultLength, const void* data, size_t dataLength)
{
	unsigned char* pStart = (unsigned char*) result;
	unsigned char* p = pStart;
	unsigned char* pEnd = pStart + resultLength;

	unsigned char* pSource = (unsigned char*) data;
	unsigned char* pSourceEnd = pSource + dataLength;

	int bitShift = 4;
	int value = 0;

	while(pSource < pSourceEnd)
	{
		int characterValue = Character::GetHexValue(*pSource++);
		if(characterValue < 0) continue;

		value |= characterValue << bitShift;
		bitShift -= 4;
		if(bitShift >= 0) continue;

		*p++ = value;
		JASSERT(p <= pEnd);

		bitShift = 4;
		value = 0;
	}

	JVERIFY(bitShift == 4);

	return p-pStart;
}

//============================================================================

IWriter& Javelin::operator<<(IWriter& writer, const DataBlock& data)
{
	writer.WriteData(data);
	return writer;
}

//============================================================================

TemporaryDataBlock::TemporaryDataBlock(const void* data, size_t length)
: Inherited(data, length, MAKE_REFERENCE)
{
}

TemporaryDataBlock::~TemporaryDataBlock()
{
	_ForceNoopDestructor();
}

const TemporaryDataBlock DataBlock::MakeReferenceTo(const void* data, size_t length)
{
	return TemporaryDataBlock(data, length);
}

//============================================================================

String Javelin::ToString(const DataBlock& a)
{
	StringWriter buffer;
	buffer.WriteByte('\n');
	buffer.DumpHex(a);
	return buffer.GetString();
}

//============================================================================

size_t Javelin::GetHash(const DataBlock& a)
{
	return Crc64(a.GetData(), a.GetNumberOfBytes());
}

//============================================================================
