//============================================================================

#include "Javelin/Stream/IWriter.h"
#include "Javelin/Stream/NullWriter.h"
#include "Javelin/Type/String.h"
#include "Javelin/Type/Exception.h"

//============================================================================

using namespace Javelin;

//============================================================================

Table<const IWriterFactory*> IWriterFactory::factoryList;

//============================================================================

IWriter::IWriter()
{
	nextObjectReferenceId = 0;
}

IWriter::~IWriter()
{
}

//============================================================================

void IWriter::Seek(long long, SeekType)
{
	throw StreamException(StreamExceptionType::CannotSeek);
}

//============================================================================

long long IWriter::GetLength() const
{
	IWriter& writer = const_cast<IWriter&>(*this);
	long long position = writer.GetOffset();
	writer.Seek(SeekType::End);
	long long length = writer.GetOffset();
	writer.Seek(position, SeekType::Start);
	return length;
}

long long IWriter::GetRemainingLength() const
{
	return GetLength() - GetOffset();
}

//============================================================================

void IWriter::WriteByte(unsigned char a)
{
	Write(a);
}

//============================================================================

void IWriter::WriteBlock(const void *data, size_t dataSize)
{
	const unsigned char *cData = static_cast<const unsigned char*>(data);

	while(dataSize)
	{
		if(!IsWritable()) throw StreamException(StreamExceptionType::CannotWrite);
		size_t bytesWritten = WriteAvailable(cData, dataSize);

		cData		+= bytesWritten;
		dataSize	-= bytesWritten;
	}
}

//============================================================================

bool IWriter::WriteObjectReferenceId(void* data)
{
	if(!data)
	{
		*this << 0;
		return false;
	}

	const Map<void*, int>::Iterator mapData = objectReferenceIdMap.Find(data);
	if( mapData != objectReferenceIdMap.End() )
	{
		*this << mapData->value;
		return false;
	}
	else
	{
		objectReferenceIdMap.Insert(data, ++nextObjectReferenceId);
		*this << nextObjectReferenceId;
		return true;
	}
}

void IWriter::PurgeObjectReferenceId(void* data)
{
	objectReferenceIdMap.Remove(data);
}

//============================================================================
