/****************************************************************************

  The TextFormatter classes interpret the following substitutions

    %A:		Javelin::String*
	%s:		const char*
	%d/%i:	int_32t
	%u:		uint32_t
 	%D:		int64_t
 	%U:		uint64_t
	%c:		Character
	%C:		char or hex escaped value
	%f:		float
	%F:		"Truncated Float"
	%p:		Pointer <architecture independent>
 	%r:		Repeated Character, ie. PrintF("%r", 'c', (size_t) 15) prints 'c' 15 times
	%x:		Hex value
    %z:		size_t
	%%:		'%'
    %@:		Javelin::ITextFormatter*

 ***************************************************************************/

#pragma once
#include "Javelin/Container/Map.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class String;
	class TextFormatData;

//============================================================================

	class JABSTRACT ICharacterWriter
	{
	public:
		virtual ~ICharacterWriter() 	{ }

		virtual void JCALL Flush()		{ }
		virtual void JCALL Close()		{ }

		virtual void JCALL WriteByte(unsigned char c) = 0;
		JEXPORT virtual void JCALL WriteBlock(const void *data, size_t numberOfBytes);

		JEXPORT void JCALL WriteString(const char *data);
		JEXPORT void JCALL WriteString(const String& string);
		
		// This dispatches to the respective VPrintF functions
		// Use a const void* instead of const char* so that the single parameter override
		// can be used without ambiguity.
		// In debug mode, assert on printing "%%" -> force call to use WriteString instead as the optimized path does not substitute
		JEXPORT void JVCALL PrintF(const void* format, ...);
		template<size_t N> JINLINE void JCALL PrintF(const char (&string)[N]) { JASSERT(memmem(string, N-1, "%%", 2) == nullptr); WriteBlock(string, N-1); }
		
		JEXPORT virtual void JCALL VPrintF(const char* format, va_list arguments);
		
		JEXPORT void JVCALL AggregatedPrintF(const char* format, ...);
		JEXPORT void JCALL AggregatedVPrintF(const char* format, va_list Arguments);

		JEXPORT void JCALL DumpHex(const void* data, size_t length);
		JEXPORT void JCALL DumpCStructure(const void* data, size_t length, const String& name=DEFAULT_VARIABLE_NAME);
		
		// For use with any type that supports .GetData() and .GetNumberOfBytes()
		// eg. String, DataBlock, Table<>, Tuple, Javelin/Cryptography/Hash functions
		template<typename T> JINLINE void JCALL WriteData(const T& d)		{ WriteBlock((char*) d.GetData(), d.GetNumberOfBytes()); }
		template<typename T> JINLINE void JCALL DumpHex(const T& data)		{ DumpHex(data.GetData(), data.GetNumberOfBytes()); }
		template<typename T> JINLINE void JCALL DumpCStructure(const T& data, const String& name=DEFAULT_VARIABLE_NAME)		{ DumpCStructure(data.GetData(), data.GetNumberOfBytes(), name); }
		
		static const String DEFAULT_VARIABLE_NAME;
	};

//============================================================================
	
	class JABSTRACT ITextFormatter
	{
	public:
		JEXPORT void JCALL Register(char c) const;
		
		virtual void JCALL FormatText(TextFormatData &context) const = 0;
	};
	
//============================================================================

	class TextFormatData
	{
	private:
		friend class ITextFormatter;

		typedef Map<char, const ITextFormatter*, MapStorage_OpenHashSetEmptyValueNullPointer> FormatMap;
		static FormatMap formatMap;

	public:
		unsigned char				flags;
		char						fillCharacter;
		unsigned short				count;
		va_list&					arguments;
		const char*&				controlString;
		ICharacterWriter&			output;
		const ITextFormatter*		formatter;

		TextFormatData(va_list &aArguments, ICharacterWriter &aOutput, const char* &aControlString);

		enum Flag
		{
			LeftAlign	= 1,
			Dot			= 2,
			Comma		= 4,
			Space		= 8,
		};

		void WriteByte(char c) const								{ output.WriteByte(c); }
		void WriteString(const char* data, size_t length) const		{ output.WriteBlock(data, length); }
		void WriteQueue(const char* data, size_t length) const;
		
		template<typename T>
			JINLINE void WriteData(const T& data) const				{ output.WriteData(data); }
	};

//============================================================================
} // namespace Javelin
//============================================================================
