//============================================================================

#include "Javelin/Stream/StringWriter.h"
#include "Javelin/Template/Utility.h"

//============================================================================

using namespace Javelin;

//============================================================================

StringWriter::StringWriter()
: stringData(1024)
{
}

StringWriter::~StringWriter()
{
	// By default, StringData's destructor will check that the
	// string has a terminating null character as a sanity check.
	//
	// Since StringWriter directly writes to StringData, we can't
	// be sure that there is a terminating null. Force it to be one
	// so that the check can remain in general for debug/release builds.
#if defined(JBUILDCONFIG_DEBUG) || defined(JBUILDCONFIG_RELEASE)
	stringData.SetTerminatingNull();
#endif
}

//============================================================================

String StringWriter::GetString()
{
	stringData.SetTerminatingNull();
	return String(new StringData(Move(stringData)));
}

//============================================================================

void StringWriter::WriteByte(unsigned char c)
{
	stringData.Append(c);
}

size_t StringWriter::WriteAvailable(const void *data, size_t dataSize)
{
	stringData.AppendCount((const char*) data, dataSize);
	return dataSize;
}

void StringWriter::WriteBlock(const void *data, size_t dataSize)
{
	stringData.AppendCount((const char*) data, dataSize);
}

//============================================================================
