//============================================================================

#include "Javelin/Type/String.h"

#include "Javelin/Container/StackBuffer.h"
#include "Javelin/Cryptography/Crc64.h"
#include "Javelin/Math/MathFunctions.h"
#include "Javelin/Type/Character.h"
#include "Javelin/Type/Endian.h"
#include "Javelin/Type/Exception.h"
#include "Javelin/Type/Interval.h"
#include "Javelin/Type/Utf8Character.h"
#include "Javelin/Type/Utf8Pointer.h"
#include "Javelin/Stream/CountWriter.h"
#include "Javelin/Stream/IReader.h"
#include "Javelin/Stream/MemoryWriter.h"

#include "Javelin/Container/Tree.h"

#include <string.h>

//============================================================================

using namespace Javelin;

//============================================================================

//static Tree<StringData*>& GetStringTree()
//{
//	static Tree<StringData*> stringTree;
//	return stringTree;
//}
//
//class StringComparator
//{
//public:
//	static bool IsEqual(const char* a, const char *b) { return strcmp(a, b) == 0; }
//	static bool IsOrdered(const char* a, const char *b) { return strcmp(a, b) < 0; }
//};
//
//void DumpStrings()
//{
//	Table<const char*> strings;
//	for(const auto& it : GetStringTree())
//	{
//		strings.Append(it->Begin());
//	}
//	
//	strings.QuickSort<StringComparator>();
//	for(const auto& it : strings)
//	{
//		printf("%s\n", it);
//	}
//}

//============================================================================

class StringDataTextFormatter : public ICharacterWriter
{
public:
	StringDataTextFormatter(char* aP) : p(aP) { }
	
	virtual void JCALL WriteByte(unsigned char c) final                   	{ *p++ = c; }
	virtual void JCALL WriteString(const char* data, size_t count) final	{ CopyMemory(p, data, count); p += count; }
	
	char* GetDestination() const { return p; }
	
private:
	char* p;
};

//============================================================================

JINLINE void* JCALL StringData::operator new(size_t size, size_t extraBytes)
{
	JASSERT(size == sizeof(StringData));
	return ::operator new(sizeof(StringData)-sizeof(size_t)+extraBytes);
}

StringData::StringData()
{
	data = nullptr;
	length = 0;
	capacity = 0;
	
//	GetStringTree().Insert(this);
}

JINLINE StringData::StringData(StringData::InlineDataInitializer a)
{
	length = a.length;
	char* charDataLocation = GetInlineDataPointer();
	data = charDataLocation;
	charDataLocation[a.length] = '\0';

//	GetStringTree().Insert(this);
}

StringData::StringData(size_t initialCapacity)
{
	length = 0;
	capacity = initialCapacity;
	data = (char*) malloc(initialCapacity);

//	GetStringTree().Insert(this);
}

StringData::~StringData()
{
	JASSERT(data == nullptr || data[length] == '\0');
//	GetStringTree().Remove(this);
	if(data != GetInlineDataPointer()) free(data);
}

StringData::StringData(IReader& input)
{
	length = capacity = input.ReadValue<uint32_t>();
	data = (char*) malloc(capacity+1);
	data[capacity] = '\0';
	input.ReadBlock(data, capacity);
}

StringData* StringData::CreateFromAscii(const char* string, size_t numberOfBytes)
{
	return StackBuffer(numberOfBytes*2, [=](char* buffer) -> StringData*
	{
		char* p = buffer;
		const char* str = string;
		for(size_t i = 0; i < numberOfBytes; ++i)
		{
			unsigned char c = *reinterpret_cast<const unsigned char*>(str++);
			if(c < 0x80) *p++ = c;
			else
			{
				*p++ = 0xc0 | (c >> 6);
				*p++ = 0x80 | (c & 0x3f);
			}
		}
		size_t numberOfEncodedBytes = p - buffer;
		return CreateFromUtf8(buffer, numberOfEncodedBytes);
	});
}

StringData* StringData::CreateFromUtf8(const char* string, size_t numberOfBytes)
{
	StringData* p = CreateWithSize(numberOfBytes);
	CopyMemory(p->data, string, numberOfBytes);
	return p;
}

StringData* StringData::CreateHexStringFromBytes(const void* data, size_t numberOfBytes)
{
	size_t stringLength = numberOfBytes*2;
	StringData* p = CreateWithSize(stringLength);
	char* charDataLocation = p->GetInlineDataPointer();
	
	p->length = stringLength;
	p->data = charDataLocation;
	
	for(size_t i = 0; i < numberOfBytes; ++i)
	{
		unsigned char c = reinterpret_cast<const unsigned char*>(data)[i];
		
		*charDataLocation++ = String::LOWER_HEX_CHARACTERS[c>>4];
		*charDataLocation++ = String::LOWER_HEX_CHARACTERS[c&15];
	}
	*charDataLocation = '\0';
	
	return p;
}

StringData* StringData::CreateFromFormatString(const char* format, va_list arguments, size_t numberOfBytes)
{
	StringData* p = CreateWithSize(numberOfBytes);
	char* charDataLocation = p->GetInlineDataPointer();
	
	p->length = numberOfBytes;
	p->data = charDataLocation;
	charDataLocation[numberOfBytes] = '\0';
	
    StringDataTextFormatter stringData(p->data);
    stringData.VPrintF(format, arguments);
	JASSERT(p->data + p->length == stringData.GetDestination());
	
	return p;
}

StringData::StringData(const StringData& a)
{
//	GetStringTree().Insert(this);
	length = a.length;
	capacity = a.length+1;				// Don't copy capacity. It could be bogus.
	data = (char*) malloc(length);
	data[length] = '\0';
	CopyMemory(data, a.data, length);
}

StringData::StringData(StringData&& a)
{
	length = a.length;
	capacity = a.capacity;
	data = a.data;
	
	a.length = 0;
	a.capacity = 0;
	a.data = nullptr;
//	GetStringTree().Insert(this);
}

StringData* StringData::CreateFromUcs2(const unsigned short* string, size_t numberOfShorts)
{
	return StackBuffer(numberOfShorts*3+2, [=](char* buffer) -> StringData*
	{
		Utf8Pointer p{buffer};
		for(size_t i = 0; i < numberOfShorts; ++i)
		{
			// Cannot be combined! UTF8 only knows how to advance after assignment
			*p = string[i];
			++p;
		}
		
		size_t numberOfEncodedBytes = p.GetCharPointer() - buffer;
		return CreateFromUtf8(buffer, numberOfEncodedBytes);
	});
}

StringData* StringData::CreateFromUcs2(const AlternateEndian<unsigned short>* string, size_t numberOfShorts)
{
	return StackBuffer(numberOfShorts*3+2, [=](char* buffer) -> StringData*
	{
		Utf8Pointer p{buffer};
		for(size_t i = 0; i < numberOfShorts; ++i)
		{
			// Cannot be combined! UTF8 only knows how to advance after assignment
			*p = (unsigned short) string[i];
			++p;
		}
		
		size_t numberOfEncodedBytes = p.GetCharPointer() - buffer;
		return CreateFromUtf8(buffer, numberOfEncodedBytes);
	});
}

JINLINE StringData* StringData::CreateFromUcs2(const NativeEndian<unsigned short>* string, size_t numberOfShorts)
{
	return CreateFromUcs2((unsigned short*) string, numberOfShorts);
}

StringData* StringData::CreateFromUtf16(const unsigned short* string, size_t numberOfShorts)
{
	return StackBuffer(numberOfShorts*3+2, [=](char* buffer) -> StringData*
	{
		Utf8Pointer p{buffer};
		const unsigned short* pSource = string;
		const unsigned short* pEnd = string+numberOfShorts;
		
		while(pSource < pEnd)
		{
			unsigned short c1 = *pSource++;
			if(c1 < 0xd800)
			{
				// Cannot be combined! UTF8 only knows how to advance after assignment
				*p = c1;
				++p;
			}
			else if(c1 < 0xdc00)
			{
				if(pSource >= pEnd) JDATA_ERROR();
				unsigned short c2 = *pSource++;
				if(c2 < 0xdc00 || c2 > 0xe000) JDATA_ERROR();
				
				// Cannot be combined! UTF8 only knows how to advance after assignment
				*p = ((c1 << 10) ^ c2 ^ (0xd800 << 10 | 0xdc00)) + 0x10000;
				++p;
			}
			else if(c1 < 0xe000)
			{
				JDATA_ERROR();
			}
			else
			{
				// Cannot be combined! UTF8 only knows how to advance after assignment
				*p = c1;
				++p;
			}
		}
		
		size_t numberOfEncodedBytes = p.GetCharPointer() - buffer;
		return CreateFromUtf8(buffer, numberOfEncodedBytes);
	});
}

StringData* StringData::CreateFromUtf16(const AlternateEndian<unsigned short>* string, size_t numberOfShorts)
{
	return StackBuffer(numberOfShorts*3+2, [=](char* buffer) -> StringData*
	{
		Utf8Pointer p{buffer};
		const AlternateEndian<unsigned short>* pSource = string;
		const AlternateEndian<unsigned short>* pEnd = string+numberOfShorts;
		
		while(pSource < pEnd)
		{
			unsigned short c1 = *pSource++;
			if(c1 < 0xd800)
			{
				// Cannot be combined! UTF8 only knows how to advance after assignment
				*p = c1;
				++p;
			}
			else if(c1 < 0xdc00)
			{
				if(pSource >= pEnd) JDATA_ERROR();
				unsigned short c2 = *pSource++;
				if(c2 < 0xdc00 || c2 > 0xe000) JDATA_ERROR();
				
				// Cannot be combined! UTF8 only knows how to advance after assignment
				*p = ((c1 << 10) ^ c2 ^ (0xd800 << 10 | 0xdc00)) + 0x10000;
				++p;
			}
			else if(c1 < 0xe000)
			{
				JDATA_ERROR();
			}
			else
			{
				// Cannot be combined! UTF8 only knows how to advance after assignment
				*p = c1;
				++p;
			}
		}

		size_t numberOfEncodedBytes = p.GetCharPointer() - buffer;
		return CreateFromUtf8(buffer, numberOfEncodedBytes);
	});
}

JINLINE StringData* StringData::CreateFromUtf16(const NativeEndian<unsigned short>* string, size_t numberOfShorts)
{
	return CreateFromUtf16((unsigned short*) string, numberOfShorts);
}

StringData* StringData::CreateFromCharacter(const Character* string, size_t numberOfCharacters)
{
	return StackBuffer(numberOfCharacters*6+2, [=](char* buffer) -> StringData*
	{
		Utf8Pointer p{buffer};
		const Character* str = string;
		for(size_t i = 0; i < numberOfCharacters; ++i)
		{
			*p = *str++;
			++p;
		}
		size_t numberOfEncodedBytes = p.GetCharPointer() - buffer;
		return CreateFromUtf8(buffer, numberOfEncodedBytes);
	});
}

StringData* StringData::CreateWithSize(size_t numberOfBytes)
{
	return new(numberOfBytes+1) StringData(InlineDataInitializer{numberOfBytes});
}

StringData* StringData::CreateFromReader(IReader& input, size_t length)
{
	StringData* p = CreateWithSize(length);
	input.ReadBlock(p->data, length);
	return p;
}

IWriter& Javelin::operator<<(IWriter& output, const StringData& data)
{
	output << (uint32_t) data.length;
	output.WriteData(data);
	return output;
}

int StringData::Compare(const char* s) const
{
	return strcmp(data, s);
}

bool StringData::operator==(const StringData& a) const
{
	return length == a.length && memcmp(data, a.data, length) == 0;
}

bool StringData::operator<(const StringData& a) const
{
	return strcmp(data, a.data) < 0;
}

JINLINE size_t StringData::GetNewAllocationSize(size_t size)
{
	return size + (size >> 3) + 13;
}

void StringData::AppendCount(const char* p, size_t count)
{
	JASSERT(data == nullptr || capacity > 0);
	if(length + count > capacity)
	{
		capacity = GetNewAllocationSize(length+count);
		data = (char*) realloc(data, capacity);
	}
	CopyMemory(data+length, p, count);
	length += count;
}

void StringData::Append(char c)
{
	JASSERT(data == nullptr || capacity > 0);
	if(length >= capacity)
	{
		capacity = GetNewAllocationSize(capacity);
		data = (char*) realloc(data, capacity);
	}
	JASSERT(data != nullptr);
	data[length++] = c;
}

void StringData::SetTerminatingNull()
{
	if(length >= capacity)
	{
		capacity = GetNewAllocationSize(capacity);
		data = (char*) realloc(data, capacity);
	}
	data[length] = '\0';
}

//============================================================================

const char String::LOWER_HEX_CHARACTERS[] = "0123456789abcdef";
const char String::UPPER_HEX_CHARACTERS[] = "0123456789ABCDEF";

String::String() : Inherited(JS("")) { }
String::String(StringData* data) 											: Inherited(data)																	{ JASSERT(*GetEndOfData() == '\0'); }
String::String(const char* string) 											: Inherited(StringData::CreateFromUtf8(string, strlen(string)))						{ }
String::String(const char* string, size_t length)							: Inherited(StringData::CreateFromUtf8(string, length)) 							{ }
String::String(const Character* string)										: Inherited(StringData::CreateFromCharacter(string, Character::GetLength(string)))	{ }
String::String(const Character* string, size_t length)						: Inherited(StringData::CreateFromCharacter(string, length))						{ }
String::String(IReader& input)												: Inherited(new StringData(input)) 													{ }
String::String(IReader& input, size_t length)								: Inherited(StringData::CreateFromReader(input, length)) 							{ }

String String::CreateFromAscii(const char* string)
{
	return String(StringData::CreateFromAscii(string, strlen(string)));
}

String String::CreateFromAscii(const char* string, size_t length)
{
	return String(StringData::CreateFromAscii(string, length));
}

String String::CreateHexStringFromBytes(const void* data, size_t numberOfBytes)
{
	return String(StringData::CreateHexStringFromBytes(data, numberOfBytes));	
}

String String::CreateFromUcs2(const unsigned short* string, size_t length)
{
	return String(StringData::CreateFromUcs2(string, length));
}

String String::CreateFromUcs2(const BigEndian<unsigned short>* string, size_t length)
{
	return String(StringData::CreateFromUcs2(string, length));
}

String String::CreateFromUcs2(const LittleEndian<unsigned short>* string, size_t length)
{
	return String(StringData::CreateFromUcs2(string, length));
}

String String::CreateFromUtf16(const unsigned short* string, size_t length)
{
	return String(StringData::CreateFromUtf16(string, length));
}

String String::CreateFromUtf16(const BigEndian<unsigned short>* string, size_t length)
{
	return String(StringData::CreateFromUtf16(string, length));
}

String String::CreateFromUtf16(const LittleEndian<unsigned short>* string, size_t length)
{
	return String(StringData::CreateFromUtf16(string, length));
}

String String::CreateWithLength(char* &buffer, size_t length)
{
	StringData* stringData = StringData::CreateWithSize(length);
	buffer = const_cast<char*>(stringData->GetData());
	return String{stringData};
}

String String::Create(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
	
    String returnValue = VCreate(format, arguments);
	
    va_end(arguments);
	
    return returnValue;
}

String String::VCreate(const char* format, va_list arguments)
{
    CountWriter counter;
    counter.VPrintF(format, arguments);
	return String(StringData::CreateFromFormatString(format, arguments, counter.GetCount()));
}

void String::SPrintF(char* destination, const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    VSPrintF(destination, format, arguments);
    va_end(arguments);
}

void String::VSPrintF(char* destination, const char* format, va_list arguments)
{
	StringDataTextFormatter writer(destination);
	writer.VPrintF(format, arguments);
}

//============================================================================

void String::Reset()
{
	*this = EMPTY_STRING;
}

//============================================================================

String String::Concatenate(const String &a, const String &b)
{
	const StringData* pa = a.GetPointer();
	const StringData* pb = b.GetPointer();
	
	size_t length = a->GetCount() + b->GetCount();
	StringData* data = new StringData(length+1);
	data->AppendCountNoExpand(pa->GetData(), pa->GetCount());
	data->AppendCountNoExpand(pb->GetData(), pb->GetCount());
	data->SetTerminatingNullNoExpand();

    return String(data);
}

bool String::Split(const String& splitString, String* left, String* right) const
{
	const char* data = GetData();
	const char* find = strstr(data, splitString.GetData());
	if(!find) return false;
	if(left)
	{
		*left = String(data, find-data);
	}
	if(right)
	{
		const char* p = find + splitString.GetNumberOfBytes();
		*right = String(p, GetEndOfData() - p);
	}
	return true;
}

bool String::Split(char splitChar, String* left, String* right) const
{
	const char* data = GetData();
	const char* find = strchr(data, splitChar);
	if(!find) return false;
	if(left)
	{
		*left = String(data, find-data);
	}
	if(right)
	{
		const char* p = find + 1;
		*right = String(p, GetEndOfData() - p);
	}
	return true;
}

bool String::SplitLast(char splitChar, String* left, String* right) const
{
	const char* data = GetData();
	const char* find = strrchr(data, splitChar);
	if(!find) return false;
	if(left)
	{
		*left = String(data, find-data);
	}
	if(right)
	{
		const char* p = find + 1;
		*right = String(p, GetEndOfData() - p);
	}
	return true;
}

Table<String> String::Split(char splitChar) const
{
	const char* pString = GetData();
	
	// Count the number of splitChars.
	const char* pSearch = pString-1;
	size_t count = 0;
	do
	{
		pSearch = strchr(pSearch+1, splitChar);
		++count;
	} while(pSearch != nullptr);
	
	// And build the table result
	Table<String> result(count);
	pSearch = pString;
	for(;;)
	{
		const char* pNext = strchr(pSearch, splitChar);
		if(pNext == nullptr)
		{
			result.Append(String(pSearch, GetEndOfData()-pSearch));
			break;
		}
		else
		{
			result.Append(String(pSearch, pNext-pSearch));
			pSearch = pNext+1;
		}
	}
	
	return result;
}

//============================================================================

bool String::IsLower() const
{
	for(Character c : *this)
	{
		if(c != c.ToLower()) return false;
	}
	return true;
}

String Javelin::ToLower(const String& string)
{
	// This assumes that upper case representation of Utf8 strings take the same number of bytes as lower case!
    size_t length = string.GetNumberOfBytes();
	StringData* data = new StringData(length+1);
	
	for(Utf8Pointer p = string.Begin(); p != string.End(); ++p)
	{
		Utf8Character c((*p).ToLower());
		data->AppendCountNoExpand(c.GetData(), c.GetNumberOfBytes());
	}
	data->SetTerminatingNullNoExpand();
	
    return String(data);
}

String Javelin::ToUpper(const String& string)
{
	// This assumes that upper case representation of Utf8 strings take the same number of bytes as lower case!
    size_t length = string.GetNumberOfBytes();
	StringData* data = new StringData(length+1);
	
	for(Utf8Pointer p = string.Begin(); p != string.End(); ++p)
	{
		Utf8Character c((*p).ToUpper());
		data->AppendCountNoExpand(c.GetData(), c.GetNumberOfBytes());
	}
	data->SetTerminatingNullNoExpand();
	
    return String(data);
}

//============================================================================

bool String::Contains(Character c) const
{
	Utf8Character utf8(c);
	if(utf8.GetNumberOfBytes() == 1) return strchr(GetData(), c) != nullptr;
	else return strstr(GetData(), utf8.AsString()) != nullptr;
}

bool String::Contains(const String& s) const
{
	return strstr(GetData(), s.GetData()) != nullptr;
}

bool String::operator==(const char* s) const
{
	return GetPointer()->Compare(s) == 0;
}

bool String::operator<(const char* s) const
{
	return GetPointer()->Compare(s) < 0;
}

bool String::operator>(const char* s) const
{
	return GetPointer()->Compare(s) > 0;
}

bool String::operator==(const String& a) const
{
	const StringData* s1 = GetPointer();
	const StringData* s2 = a.GetPointer();
	
	return *s1 == *s2;
}

bool String::operator<(const String& a) const
{
	const StringData* s1 = GetPointer();
	const StringData* s2 = a.GetPointer();
	return *s1 < *s2;
}

//============================================================================

bool String::IsAscii() const
{
    const unsigned char* p = (const unsigned char*) GetData();
    const unsigned char* pEnd = (const unsigned char*) GetEndOfData();

	while(p < pEnd)
	{
		if(*p >= 0x80) return false;
		++p;
	}
	return true;
}

bool String::IsInteger() const
{
    const char* p = GetData();
    const char* pEnd = GetEndOfData();
	
    if(p == pEnd) return false;
	
    if(*p == '-' || *p == '+') p++;
	while('0' <= *p && *p <= '9') ++p;
	return p == pEnd;
}

int String::AsInteger() const
{
    const char* p = GetData();
    JASSERT(*GetEndOfData() == '\0');
	
    bool isNegative = false;
	
    if(*p == '-')
    {
        isNegative = true;
        ++p;
    }
	else if(*p == '+')
	{
		++p;
	}
	
    int value = 0;
    while('0' <= *p && *p <= '9')
    {
        value = value * 10 + (*p++ - '0');
    }
    return isNegative ? -value : value;
}

int String::AsIntegerFromHex() const
{
    const char* p = GetData();
    const char* pEnd = GetEndOfData();
	
    bool isNegative = false;
	
    if(*p == '-')
    {
        isNegative = true;
        ++p;
    }
	else if(*p == '+')
	{
		++p;
	}
	
    int value = 0;
	while(p < pEnd)
	{
		int x = Character::GetHexValue(*p++);
		if(x == -1) break;
		value = value * 16 + x;
	}
    return isNegative ? -value : value;
}

static int SkipDigits(const char* &p)
{
	int countSkipped = 0;
	while('0' <= *p && *p <= '9')
	{
		++countSkipped;
		++p;
	}
	return countSkipped;
}

bool String::IsFloat() const
{
	const char* pStart = GetData();
    const char* p = pStart;
    const char* pEnd = GetEndOfData();
	JASSERT(*pEnd == '\0');
	
    if(p == pEnd) return false;
	
    if(*p == '-' || *p == '+') ++p;
	
    while('0' <= *p && *p <= '9') ++p;
	
	if(*p == '.')
	{
		++p;
		while('0' <= *p && *p <= '9') ++p;
	}
	if((*p == 'e' || *p == 'E') && p != pStart)
	{
		++p;
		if(*p == '-' || *p == '+') ++p;
		
		while('0' <= *p && *p <= '9') ++p;
	}
	
	return p == pEnd;
}

float String::AsFloat() const
{
    const char* p = GetData();
	JASSERT(*GetEndOfData() == '\0');
	
	int sign = 1;
    if(*p == '-')
    {
		sign = -1;
        ++p;
    }
	if(*p == '+')
	{
		++p;
	}

	int digits = 0;
	int value = 0;
	int exponent = 0;
    while('0' <= *p && *p <= '9')
    {
        value = value * 10 + (*p++ - '0');
		if(value)
		{
			if(++digits >= 9)
			{
				exponent = SkipDigits(p);
				if(*p == '.') p++;
				SkipDigits(p);
				goto DoExponent;
			}
		}
    }
	
	if(*p == '.')
	{
		p++;
		while('0' <= *p && *p <= '9')
		{
			--exponent;
			value = value * 10 + (*p++ - '0');
			if(value)
			{
				if(++digits >= 9)
				{
					SkipDigits(p);
					break;
				}
			}
		}
	}
	
DoExponent:
	if(*p == 'e' || *p == 'E')
	{
		++p;
		int exponentSign = 1;
		if(*p == '-')
		{
			exponentSign = -1;
			++p;
		}
		else if(*p == '+')
		{
			++p;
		}
		
		int evalue = 0;
		while('0' <= *p && *p <= '9')
		{
			evalue = evalue * 10 + (*p++ - '0');
		}
		
		exponent += exponentSign * evalue;
	}

	return sign * value * Math::Pow<float>(10, exponent);
}

unsigned String::AsHex(const char* p, size_t length)
{
	unsigned value = 0;
	do
	{
		int hexValue = Character::GetHexValue(*p++);
		JASSERT(hexValue != -1);
		value = (value << 4) | hexValue;
	} while(--length != 0);
	return value;
}

//============================================================================

String String::CreateJsonString() const
{
	return StackBuffer(GetNumberOfBytes()*6 + 2, [=](char* buffer) -> String
	{
		char* p = buffer;
		*p++ = '"';
		
		for(const unsigned char c : RawBytes())
		{
			switch(c)
			{
			case '"':
				*p++ = '\\';
				*p++ = '"';
				break;
				
			case '\\':
				*p++ = '\\';
				*p++ = '\\';
				break;
				
				// I don't see any need to escape '/'.. so let's not -- makes the output more readable too
				//		case '/':
				//			*p++ = '\\';
				//			*p++ = '/';
				//			break;
				//
			case '\b':
				*p++ = '\\';
				*p++ = 'b';
				break;
				
			case '\f':
				*p++ = '\\';
				*p++ = 'f';
				break;
				
			case '\n':
				*p++ = '\\';
				*p++ = 'n';
				break;
				
			case '\r':
				*p++ = '\\';
				*p++ = 'r';
				break;
				
			case '\t':
				*p++ = '\\';
				*p++ = 't';
				break;
				
			default:
				if(c < 32)
				{
					*p++ = '\\';
					*p++ = 'u';
					*p++ = '0';	// Since c < 32, upper 8 bits are going to be 00
					*p++ = '0';
					*p++ = LOWER_HEX_CHARACTERS[(c>> 4) & 0xf];
					*p++ = LOWER_HEX_CHARACTERS[ c      & 0xf];
				}
				else
				{
					*p++ = c;
				}
			}
		}
		*p++ = '"';
		
		return String(buffer, p-buffer);
	});
}

String String::CreateYamlString() const
{
	if(GetNumberOfBytes() == 0)
	{
		return JS("''");
	}
	
	// If it contains '\r', '\n' -> use double quoted representation
	// Also put in strings that contain "'" (not required by spec.. choice made)
	if(Contains('\r') || Contains('\n') || Contains('\''))
	{
		return CreateJsonString();
	}

	bool useSingleQuote = false;

	// Special string words need to be quoted so that they're decoded appropriately
	if(*this == JS("false") || *this == JS("true") || *this == JS("null"))
	{
		useSingleQuote = true;
	}
	// If we start or end with whitespace, or start with an indicator character
	else
	{
		const char* pBegin = GetData();
		const char* pEnd = GetEndOfData();

		if(pBegin[0] == ' '
		   || pBegin[0] == '!'
		   || pBegin[0] == '\t'
		   || pBegin[0] == ':'
		   || pBegin[0] == '?'
		   || pBegin[0] == '-'
		   || pEnd[-1] == ' '
		   || pEnd[-1] == '\t')
		{
			useSingleQuote = true;
		}
		else
		{
			// Check that we don't contain:
			//		"- "
			//		": "
			//		"? "
			//		" #"
			// or any of: ",[]{}"
			
			const char* p = pBegin;
			while(p < pEnd)
			{
				switch(*p)
				{
				case ',':
				case '[':
				case ']':
				case '{':
				case '}':
					useSingleQuote = true;
					goto Exit;
					
				case ' ':
					// We can access p[-1] safely because it passed the pBegin[0] == ' ' check before.
					if(p[-1] == '-'
					   || p[-1] == ':'
					   || p[-1] == '?')
					{
						useSingleQuote = true;
						goto Exit;
					}
					if(p+1 < pEnd && p[1] == '#')
					{
						useSingleQuote = true;
						goto Exit;
					}
					break;
				}
				++p;
			}
		}
	}

Exit:
	if(useSingleQuote) return String::Create("'%A'", this);
	else return *this;
}

String Javelin::UrlEncode(const String& s, String::UrlEncode encode)
{
	return StackBuffer(s.GetNumberOfBytes()*3, [=](char* buffer) -> String
	{
		char* p = buffer;
		
		int percentEncodeThreshold = (encode == String::UrlEncode::SPACE_TO_PERCENT) ? ' '+1 : ' ';
		const int spaceToPlusCharacter = (encode == String::UrlEncode::SPACE_TO_PLUS) ? ' ' : TypeData<int>::Maximum();
		const int plusEncodeCharacter = (encode == String::UrlEncode::SPACE_TO_PLUS) ? '+' : TypeData<int>::Maximum();
		
		for(const unsigned char c : s.RawBytes())
		{
			if(c < percentEncodeThreshold || c == '%' || c == plusEncodeCharacter)
			{
				*p++ = '%';
				*p++ = String::LOWER_HEX_CHARACTERS[(c>> 4) & 0xf];
				*p++ = String::LOWER_HEX_CHARACTERS[ c      & 0xf];
			}
			else if(c == spaceToPlusCharacter)
			{
				*p++ = '+';
			}
			else
			{
				*p++ = c;
			}
		}

		return String(buffer, p-buffer);
	});
}

String Javelin::UrlDecode(const String& s, bool convertPlusses)
{
	return StackBuffer(s.GetNumberOfBytes(), [=](char* buffer) -> String
	{
		char* p = buffer;
		const char convertPlusCharacter = convertPlusses ? ' ' : '+';

		const unsigned char* pSource = (const unsigned char*) s.GetData();
		const unsigned char* pEnd = (const unsigned char*) s.GetEndOfData();
		
		while(pSource < pEnd)
		{
			unsigned char c = *pSource++;
			switch(c)
			{
			case '%':
				*p++ = (Character::GetHexValue(pSource[0]) << 4) | Character::GetHexValue(pSource[1]);
				pSource += 2;
				break;
					
			case '+':
				*p++ = convertPlusCharacter;
				break;
					
			default:
				*p++ = c;
			}
		}
		return String(buffer, p-buffer);
	});
}

//============================================================================

bool String::BeginsWith(char c) const
{
	return HasData() > 0 && *GetData() == c;
}

bool String::BeginsWith(const String& a) const
{
	size_t lengthA = a.GetNumberOfBytes();
	return GetNumberOfBytes() >= lengthA && memcmp(GetData(), a.GetData(), lengthA) == 0;
}

bool String::EndsWith(const String& a) const
{
	size_t length = GetNumberOfBytes();
	size_t lengthA = a.GetNumberOfBytes();
	
	return length >= lengthA && memcmp(GetData() + (length-lengthA), a.GetData(), lengthA) == 0;
}

Utf8Pointer String::GetPointerToCharacterIndex(int i) const
{
	Utf8Pointer p = Begin();
	Utf8Pointer pEnd = End();
	
	if(i >= 0)
	{
		while(i > 0 && p < pEnd)
		{
			++p;
			--i;
		}
		return p;
	}
	else
	{
		while(pEnd > p && i < 0)
		{
			--pEnd;
			++i;
		}
		return pEnd;
	}
}

String String::SubString(int startCharacterIndex) const
{
	Utf8Pointer p = GetPointerToCharacterIndex(startCharacterIndex);
	if(p == Begin()) return *this;
	if(p >= End()) return String::EMPTY_STRING;

	return String{p.GetCharPointer(), static_cast<size_t>(GetEndOfData() - p.GetCharPointer())};
}

String String::SubString(int startCharacterIndex, int endCharacterIndex) const
{
	Utf8Pointer start = GetPointerToCharacterIndex(startCharacterIndex);
	Utf8Pointer end = GetPointerToCharacterIndex(endCharacterIndex);
	
	if(start == Begin() && end == End()) return *this;
	if(start >= end) return String::EMPTY_STRING;
	
	return String{start.GetCharPointer(), static_cast<size_t>(end.GetCharPointer() - start.GetCharPointer())};
}

//============================================================================

IReader& Javelin::operator>>(IReader& input, String& a)
{
	a = String(new StringData(input));
	return input;
}

//============================================================================

void String::ToCharacters(Character* result) const
{
	for(Character c : *this) *result++ = c;
}

size_t Javelin::Levenshtein(const String& a, const String& b)
{
	if(a == b) return 0;
	
	size_t numberOfCharactersA = a.GetNumberOfCharacters();
	size_t numberOfCharactersB = b.GetNumberOfCharacters();
	if(numberOfCharactersA == 0) return numberOfCharactersB;
	if(numberOfCharactersB == 0) return numberOfCharactersA;
	
	AutoArrayPointerWithInlineStore<size_t, 1000> vStore(2*(numberOfCharactersB+1));
	size_t* v0 = vStore;
	size_t* v1 = v0 + numberOfCharactersB+1;
	
	for(size_t i = 0; i <= numberOfCharactersB; ++i) v1[i] = i;

	AutoArrayPointerWithInlineStore<Character, 1000> charactersStore(numberOfCharactersA+numberOfCharactersB);
	Character* charactersA = charactersStore;
	Character* charactersB = charactersA + numberOfCharactersA;
	a.ToCharacters(charactersA);
	b.ToCharacters(charactersB);
	
	for(size_t i = 0; i < numberOfCharactersA; ++i)
	{
		Swap(v0, v1);
		v1[0] = i + 1;
		for(size_t j = 0; j < numberOfCharactersB; ++j)
		{
			size_t cost = (charactersA[i] == charactersB[j]) ? 0 : 1;
			v1[j+1] = Minimum(v1[j] + 1, v0[j+1] + 1, v0[j] + cost);
		}
	}
	
    return v1[numberOfCharactersB];
}

size_t Javelin::DamerauLevenshtein(const String& a, const String& b)
{
	if(a == b) return 0;
	
	size_t numberOfCharactersA = a.GetNumberOfCharacters();
	size_t numberOfCharactersB = b.GetNumberOfCharacters();
	if(numberOfCharactersA == 0) return numberOfCharactersB;
	if(numberOfCharactersB == 0) return numberOfCharactersA;
	
	AutoArrayPointerWithInlineStore<size_t, 1000> vStore(3*(numberOfCharactersB+1));
	size_t* v0 = vStore;
	size_t* v1 = v0 + numberOfCharactersB+1;
	size_t* v2 = v1 + numberOfCharactersB+1;
	
	for(size_t i = 0; i <= numberOfCharactersB; ++i) v2[i] = i;
	
	AutoArrayPointerWithInlineStore<Character, 1000> charactersStore(numberOfCharactersA+numberOfCharactersB);
	Character* charactersA = charactersStore;
	Character* charactersB = charactersA + numberOfCharactersA;
	a.ToCharacters(charactersA);
	b.ToCharacters(charactersB);
	
	for(size_t i = 0; i < numberOfCharactersA; ++i)
	{
		size_t* vtemp = v0;
		v0 = v1;
		v1 = v2;
		v2 = vtemp;

		v2[0] = i + 1;
		for(size_t j = 0; j < numberOfCharactersB; ++j)
		{
			size_t cost = (charactersA[i] == charactersB[j]) ? 0 : 1;
			v2[j+1] = Minimum(v2[j] + 1, v1[j+1] + 1, v1[j] + cost);
			
			if(i >= 1 && j >= 1 && charactersA[i] == charactersB[j-1] && charactersA[i-1] == charactersB[j])
			{
				v2[j+1] = Minimum(v2[j+1], v0[j-1] + cost);
			}
		}
	}
	
    return v2[numberOfCharactersB];
}

//============================================================================

size_t String::GetHash() const
{
	const StringData* p = GetPointer();
	return (size_t) Crc64(*p);
}

//============================================================================

String Javelin::ToString(bool b)				{ return b ? JS("true") : JS("false"); }
String Javelin::ToString(const char* s)			{ return String{s};					}
String Javelin::ToString(char a) 				{ return String::Create("'%c'", a); }
String Javelin::ToString(unsigned char a)		{ return String::Create("'%c'", a);	}
String Javelin::ToString(int a)					{ return String::Create("%d", a);	}
String Javelin::ToString(unsigned a)			{ return String::Create("%u", a);	}
String Javelin::ToString(long long a)			{ return String::Create("%D", a);	}
String Javelin::ToString(unsigned long long a)	{ return String::Create("%U", a);	}
String Javelin::ToString(float a)				{ return String::Create("%f", a);	}
String Javelin::ToString(double a)				{ return String::Create("%g", a);	}
String Javelin::ToString(const String& a)		{ return a;							}
String Javelin::ToString(const void *p)			{ return String::Create("%p", p);	}
String Javelin::ToString(const decltype(nullptr)&) { return JS("nullptr"); }

String Javelin::ToString(long a)				{ return sizeof(long) == sizeof(int) ? ToString((int) a) : ToString((long long) a); }
String Javelin::ToString(unsigned long a)		{ return sizeof(long) == sizeof(int) ? ToString((unsigned) a) : ToString((unsigned long long) a); }

//============================================================================
