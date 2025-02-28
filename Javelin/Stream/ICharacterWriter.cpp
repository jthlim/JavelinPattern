		//============================================================================

#include "Javelin/Stream/ICharacterWriter.h"
#include "Javelin/Type/String.h"
#include <ctype.h>

//============================================================================

using namespace Javelin;

//============================================================================

TextFormatData::FormatMap TextFormatData::formatMap(32);

const String ICharacterWriter::DEFAULT_VARIABLE_NAME = JS("value");

//============================================================================

TextFormatData::TextFormatData(va_list &aArguments, ICharacterWriter &aOutput, const char* &aControlString)
: arguments(aArguments), controlString(aControlString), output(aOutput)
{
	count				= 0;
	flags				= 0;
	fillCharacter		= ' ';

	while(1)
	{
		char c = *++controlString;
		
		switch(c)
		{
		case '\0':
			formatter = nullptr;
			return;

		case '-':
			flags |= Flag::LeftAlign;
			break;

		case '.':
			flags |= Flag::Dot;
			break;

		case ',':
			flags |= Flag::Comma;
			break;

		case ' ':
			flags |= Flag::Space;
			break;

		case '0':
			if(count == 0)
			{
				fillCharacter = '0';
				break;
			}

		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			count = count*10 + c - '0';
			break;

		default:
			{
				FormatMap::Iterator it = formatMap.Find(c);
				if(it != formatMap.End())
				{
					formatter = it->value;
					return;
				}
			}
			break;
		}
	}
}

//============================================================================

void TextFormatData::WriteQueue(const char* head, size_t queueSize) const
{
	int fillCount = (int) count - static_cast<int>(queueSize);
	if(fillCount > 0)
	{
		char fillData[fillCount];
		
		if(flags & Flag::LeftAlign)
		{
			memset(fillData, ' ', fillCount);
			output.WriteBlock(head, queueSize);
			output.WriteBlock(fillData, fillCount);
		}
		else
		{
			memset(fillData, fillCharacter, fillCount);
			output.WriteBlock(fillData, fillCount);
			output.WriteBlock(head, queueSize);
		}
	}
	else
	{
		output.WriteBlock(head, queueSize);
	}
}

//============================================================================

void ICharacterWriter::WriteBlock(const void *data, size_t length)
{
	const char* p = (const char*) data;
	while(length)
	{
		WriteByte(*p++);
		--length;
	}
}

void ICharacterWriter::WriteString(const char *data)
{
	WriteBlock(data, strlen(data));
}

void ICharacterWriter::WriteString(const String& string)
{
	WriteData(string);
}

//============================================================================

void JVCALL ICharacterWriter::PrintF(const void *format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	VPrintF((const char*) format, arguments);
	va_end(arguments);
}

//============================================================================

void ICharacterWriter::AggregatedPrintF(const char* format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	AggregatedVPrintF(format, arguments);
	va_end(arguments);
}

void ICharacterWriter::AggregatedVPrintF(const char* format, va_list arguments)
{
	const String& string = String::VCreate(format, arguments);
	WriteString(string);
}

//============================================================================

void ICharacterWriter::DumpHex(const void* data, size_t length)
{
	const unsigned char* p = static_cast<const unsigned char*>(data);
	
	size_t i = 0;
	while(i < length)
	{
		if(i % 16 == 0)
		{
			PrintF("%08x: ", i);
		}
		PrintF(i % 8 == 0 ? "  %02x" : " %02x", p[i]);
		++i;
		
		if(i % 16 == 0)
		{
			PrintF("   |");
			const unsigned char* x = p + i - 16;
			for(int a : Range(16))
			{
				WriteByte(' ' <= x[a] && x[a] < 0x7f ? x[a] : '.');
			}
			PrintF("|\n");
		}
	}
	
	size_t remainder = i % 16;
	if(remainder != 0)
	{
		static const char SPACES[] = "                                                 |\n";
		size_t filler = (16 - remainder) * 3 + 4;
		if(remainder <= 8) filler++;
		WriteBlock((SPACES+sizeof(SPACES)-2) - filler, filler);
		const unsigned char* x = p + i - remainder;
		for(size_t a : Range(remainder))
		{
			WriteByte(' ' <= x[a] && x[a] < 0x7f ? x[a] : '.');
		}
		size_t endCharacterCount = 18-remainder;
		WriteBlock((SPACES+sizeof(SPACES)-1) - endCharacterCount, endCharacterCount);
	}
}

void ICharacterWriter::DumpCStructure(const void* data, size_t length, const String& name)
{
	PrintF("const unsigned char %A[%z] =\n", &name, length);
	PrintF("{\n");
	
	const unsigned char* p = static_cast<const unsigned char*>(data);
	size_t i = 0;
	while(i < length)
	{
		if(i % 16 == 0) PrintF("\t");
		PrintF(i % 16 == 0 ? "\t0x%02x, " :
			   i % 8  == 0 ? " 0x%02x, " :
			                 "0x%02x, ", p[i]);
		++i;
		
		if(i % 16 == 0)
		{
			PrintF(" // %08x: |", i-16);
			const unsigned char* x = p + i - 16;
			for(int a : Range(16))
			{
				WriteByte(' ' <= x[a] && x[a] < 0x7f ? x[a] : '.');
			}
			PrintF("|\n");
		}
	}

	size_t remainder = i % 16;
	if(remainder != 0)
	{
		size_t filler = (16 - remainder) * 6 + ((remainder <= 8) ? 1 : 0);
		PrintF("%r // %08x: |", ' ', filler, i-remainder);
		const unsigned char* x = p + i - remainder;
		for(size_t a : Range(remainder))
		{
			WriteByte(' ' <= x[a] && x[a] < 0x7f ? x[a] : '.');
		}
		PrintF("%r|\n", ' ', 16-remainder);
	}

	PrintF("};\n");
}

//============================================================================

#ifndef va_copy
	#define va_copy(dst, src)		dst = src
#endif

void ICharacterWriter::VPrintF(const char *format, va_list aArguments)
{
	va_list arguments;
	va_copy(arguments, aArguments);

	while(*format)
	{
		if(*format == '%')
		{
			TextFormatData formatData(arguments, *this, format);
			if(!formatData.formatter) return;

			formatData.formatter->FormatText(formatData);
		}
		else
		{
			WriteByte(*format);
		}

		++format;
	}
}

//============================================================================

void ITextFormatter::Register(char c) const
{
	TextFormatData::formatMap.Insert(c, this);
}

//============================================================================
