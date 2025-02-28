//===========================================================================

#include "Javelin/Type/VariableName.h"
#include "Javelin/Type/Character.h"

//===========================================================================

using namespace Javelin;

//===========================================================================

void VariableName::AddPart(const Utf8Pointer& pStart, const Utf8Pointer& pEnd)
{
	size_t numberOfBytes = (size_t) (pEnd.GetCharPointer() - pStart.GetCharPointer());
	if(numberOfBytes == 0) return;
	
	char* buffer;
	String resultString = String::CreateWithLength(buffer, numberOfBytes);

	Utf8Pointer pDst(buffer);
	Utf8Pointer pSrc(pStart);
	while(pSrc != pEnd)
	{
		*pDst++ = pSrc++->ToLower();
	}
	
	Append(resultString);
}

size_t VariableName::GetPartLength() const
{
	size_t length = 0;
	for(const String& s : *this) length += s.GetNumberOfBytes();
	return length;
}
	
void VariableName::InitWithCamelCase(const String& s)
{
	Utf8Pointer pStart = s.Begin();
	Utf8Pointer pEnd = s.End();
	
	Utf8Pointer p = pStart;
	
	bool lastWasDigit = false;
	
	if(p < pEnd) ++p;
	for(; p < pEnd; ++p)
	{
		bool isDigit = p->IsDigit();
		if(p->IsUpper() || (isDigit ^ lastWasDigit))
		{
			AddPart(pStart, p);
			pStart = p;
		}
		lastWasDigit = isDigit;
		
	}
	AddPart(pStart, p);
}

void VariableName::InitWithSeparator(const String& s, char separator)
{
	Utf8Pointer pStart = s.Begin();
	Utf8Pointer pEnd = s.End();
	
	Utf8Pointer p = pStart;
	for(; p < pEnd; ++p)
	{
		if(*p == separator)
		{
			AddPart(pStart, p);
			++p;
			pStart = p;
		}
		
	}
	AddPart(pStart, p);
}

//===========================================================================

String VariableName::AsLowerCamelCase() const
{
	if(GetCount() == 0) return String::EMPTY_STRING;
	
	char* buffer;
	String resultString = String::CreateWithLength(buffer, GetPartLength());
	
	CopyMemory(buffer, (*this)[0].GetData(), (*this)[0].GetNumberOfBytes());
	Utf8Pointer p { buffer + (*this)[0].GetNumberOfBytes() };
	
	for(size_t i = 1; i < GetCount(); ++i)
	{
		const String& s = (*this)[i];
		Utf8Pointer pSrc = s.Begin();
		*p++ = pSrc++->ToUpper();
		size_t numberOfBytes = s.End() - pSrc;
		CopyMemory(p.GetCharPointer(), pSrc.GetCharPointer(), numberOfBytes);
		p.OffsetByByteCount(numberOfBytes);
	}
	return resultString;
}

String VariableName::AsUpperCamelCase() const
{
	if(GetCount() == 0) return String::EMPTY_STRING;

	char* buffer;
	String resultString = String::CreateWithLength(buffer, GetPartLength());

	Utf8Pointer p{buffer};
	
	for(size_t i = 0; i < GetCount(); ++i)
	{
		const String& s = (*this)[i];
		Utf8Pointer pSrc = s.Begin();
		*p++ = pSrc++->ToUpper();
		size_t numberOfBytes = s.End() - pSrc;
		CopyMemory(p.GetCharPointer(), pSrc.GetCharPointer(), numberOfBytes);
		p.OffsetByByteCount(numberOfBytes);
	}
	return resultString;
}

String VariableName::AsLowerWithSeparator(char c) const
{
	if(GetCount() == 0) return String::EMPTY_STRING;

	char* p;
	String resultString = String::CreateWithLength(p, GetPartLength()+GetCount()-1);
	
	for(const String& s : *this)
	{
		CopyMemory(p, s.GetData(), s.GetNumberOfBytes());
		p += s.GetNumberOfBytes();
		*p++ = c;
	}
	*--p = '\0';
	return resultString;
}

String VariableName::AsUpperWithSeparator(char c) const
{
	if(GetCount() == 0) return String::EMPTY_STRING;

	char* buffer;
	String resultString = String::CreateWithLength(buffer, GetPartLength()+GetCount()-1);
	Utf8Pointer p{buffer};
	
	for(const String& s : *this)
	{
		for(Character c : s)
		{
			*p++ = Character::ToUpper(c);
		}
		*p++ = c;
	}
	*--p = '\0';
	return resultString;
}

//===========================================================================
