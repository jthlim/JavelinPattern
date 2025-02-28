//============================================================================

#include "Javelin/Stream/ICharacterReader.h"
#include "Javelin/Stream/StringWriter.h"
#include "Javelin/Type/Exception.h"

//============================================================================

using namespace Javelin;

//============================================================================

String ICharacterReader::ReadLine()
{
	StringWriter stringWriter;

	try
	{
		while(true)
		{
			unsigned char c = ReadByte();
			if(c == '\n' || c == '\0') break;
			if(c == '\r')
			{
				c = ReadByte();
				if(c == '\n' || c == '\0') break;
				stringWriter.WriteByte('\r');
			}
			stringWriter.WriteByte(c);
		}
	}
	catch(const StreamException &e)
	{
		if(stringWriter.GetCount() == 0 || !e.IsEndOfStream()) throw;
	}

	return stringWriter.GetString();
}

String ICharacterReader::ReadAllAsString()
{
	StringWriter stringWriter;

	try
	{
		while(true) stringWriter.WriteByte(ReadByte());
	}
	catch(const StreamException &e)
	{
		if(!e.IsEndOfStream()) throw;
	}

	return stringWriter.GetString();
}

//============================================================================
