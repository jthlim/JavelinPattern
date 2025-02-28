//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Container/Table.h"
#include "Javelin/Template/Memory.h"
#include "Javelin/Type/SmartPointer.h"
#include "Javelin/Type/Utf8Pointer.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	template<typename T> struct AlternateEndian;
	template<typename T> struct BigEndian;
	template<typename T> struct LittleEndian;
	template<typename T> struct NativeEndian;
	template<typename T, bool b> class Interval;
	class IWriter;
	class IReader;

//============================================================================

	class StringData : public SingleAssignSmartObject
	{
	public:
		StringData();
		StringData(size_t initialCapacity);
		StringData(const StringData& a);
		StringData(StringData&& a);
		StringData(IReader& input);
		~StringData();

		static StringData* CreateFromAscii(const char* string, size_t numberOfBytes);
		static StringData* CreateFromUtf8(const char* string, size_t numberOfBytes);
		static StringData* CreateFromUtf16(const AlternateEndian<unsigned short>* string, size_t numberOfShorts);
		static StringData* CreateFromUtf16(const NativeEndian<unsigned short>* string, size_t numberOfShorts);
		static StringData* CreateFromUtf16(const unsigned short* string, size_t numberOfShorts);
		static StringData* CreateFromUcs2(const AlternateEndian<unsigned short>* string, size_t numberOfShorts);
		static StringData* CreateFromUcs2(const NativeEndian<unsigned short>* string, size_t numberOfShorts);
		static StringData* CreateFromUcs2(const unsigned short* string, size_t numberOfShorts);
		static StringData* CreateFromCharacter(const Character* string, size_t numberOfCharacters);
//		static StringData* CreateFromLiteral(const char* string, size_t numberOfBytes);
		static StringData* CreateFromFormatString(const char* format, va_list arguments, size_t numberOfBytes);
		static StringData* CreateHexStringFromBytes(const void* data, size_t numberOfBytes);
		static StringData* CreateWithSize(size_t numberOfBytes);
		static StringData* CreateFromReader(IReader& input, size_t length);
		
		friend IWriter& operator<<(IWriter& output, const StringData& data);

		size_t		GetCount()			const		{ return length; 		}
		size_t		GetNumberOfBytes()	const		{ return length; 		}
		
		bool		HasData()			const		{ return length != 0;	}
		const char*	GetData()			const		{ return data; 			}
		bool		IsEmpty()			const		{ return length == 0;	}
		char*		Begin()					 		{ return data; 			}
		const char*	Begin()				const 		{ return data; 			}
		char*		End()					 		{ return data+length;	}
		const char*	End()				const 		{ return data+length;	}

		void		AppendCount(const char* p, size_t count);
		void		AppendCountNoExpand(const char* p, size_t count)	{ JASSERT(length + count <= capacity); CopyMemory(data+length, p, count); length += count; }
		void		Append(char c);
		void		AppendNoExpand(char c)								{ JASSERT(length < capacity); data[length++] = c; }
		void		SetTerminatingNull();
		void 		SetTerminatingNullNoExpand()						{ JASSERT(length < capacity); data[length] = '\0'; }
		
		bool 		operator<(const StringData& a) const;
		bool		operator==(const StringData& a) const;
		int			Compare(const char* s) const;
		
		void operator=(const StringData&) = delete;
		
		static void* operator new(size_t size)								{ return ::operator new(size); }
		static void operator delete(void* p)								{ ::operator delete(p); }
		
	private:
		char*		data;
		size_t		length;
		size_t		capacity;

		char* GetInlineDataPointer() 										{ return (char*) &capacity; }
		
		struct InlineDataInitializer
		{
			size_t	length;
		};
		StringData(InlineDataInitializer a);
		
		static void* JCALL operator new(size_t size, size_t extraBytes);
		static void JCALL operator delete(void *p, size_t extraBytes)		{ ::operator delete(p); }
		static size_t GetNewAllocationSize(size_t size);
	};
	
//============================================================================
	
	class String : public SmartPointer<StringData, SmartPointerOwnershipPolicy_SingleAssignSmartObjectPointerAlwaysValid>
	{
	private:
#if defined(JCOMPILER_CLANG) || defined(JCOMPILER_MSVC)
		typedef SmartPointer Inherited;
#else
		typedef SmartPointer<StringData, SmartPointerOwnershipPolicy_SingleAssignSmartObjectPointerAlwaysValid> Inherited;
#endif
	
		class TemporaryString
		{
		public:
			constexpr TemporaryString(const char* aData, size_t aCount)	: stringData((StringData*) &counter), counter(1), data(aData), count(aCount) { }
			~TemporaryString() 				{ JASSERT(counter == 1); }
			
			operator String&() const 		{ return *(String*) this; }
			
		private:
			StringData*	stringData;
			int			counter;
			const char*	data;
			size_t		count;
		};

	public:
		JEXPORT String();
		JEXPORT explicit String(StringData* data);
		JEXPORT explicit String(const char* utf8String);
		JEXPORT String(const char* utf8String, size_t numberOfBytes);
		JEXPORT String(Utf8Pointer p, size_t numberOfBytes) : String(p.GetCharPointer(), numberOfBytes) { }
		JEXPORT explicit String(const Character* string);
		template<typename T, bool b> JINLINE String(const Interval<const T*, b>& i) : String((const char*) i.GetData(), i.GetNumberOfBytes()) { }
		JEXPORT String(const Character* string, size_t numberOfCharacters);
		JEXPORT String(IReader& input);
		JEXPORT String(IReader& input, size_t length);
		
		JEXPORT static String CreateHexStringFromBytes(const void* data, size_t numberOfBytes);
		JEXPORT static String CreateFromAscii(const char* string);
		JEXPORT static String CreateFromAscii(const char* string, size_t numberOfBytes);
		JEXPORT static String CreateFromUtf8(const char* utf8String) 						{ return String(utf8String); 				}
		JEXPORT static String CreateFromUtf8(const char* utf8String, size_t numberOfBytes)	{ return String(utf8String, numberOfBytes); }
		JEXPORT static String CreateFromUcs2(const LittleEndian<unsigned short>* string, size_t numberOfShorts);
		JEXPORT static String CreateFromUcs2(const BigEndian<unsigned short>* string, size_t numberOfShorts);
		JEXPORT static String CreateFromUcs2(const unsigned short* string, size_t numberOfShorts);
		JEXPORT static String CreateFromUtf16(const LittleEndian<unsigned short>* string, size_t numberOfShorts);
		JEXPORT static String CreateFromUtf16(const BigEndian<unsigned short>* string, size_t numberOfShorts);
		JEXPORT static String CreateFromUtf16(const unsigned short* string, size_t numberOfShorts);

		JEXPORT static String JCALL CreateWithLength(char* &buffer, size_t length);
		
		static const TemporaryString MakeReferenceTo(const char* data) 					{ return TemporaryString(data, strlen(data)); }
		static const TemporaryString MakeReferenceTo(const char* data, size_t length) 	{ JASSERT(data[length] == '\0'); return TemporaryString(data, length); }

		JEXPORT static String JVCALL Create(const char* format, ...);
        JEXPORT static String JVCALL VCreate(const char* format, va_list arguments);
		JEXPORT static void   JVCALL SPrintF(char* destination, const char* format, ...);
		JEXPORT static void   JVCALL VSPrintF(char* destination, const char* format, va_list arguments);

		// These are overridden so that we don't get warnings about taking addresses of temporary object when we want to PrintF("%A", &temporaryResult());
		JINLINE String* operator&() 			{ return this; }
		JINLINE const String* operator&() const { return this; }
		
		JEXPORT void JCALL Reset();
		
		JINLINE bool HasData() const										{ return GetPointer()->HasData();	}
		JINLINE bool IsEmpty() const										{ return GetPointer()->IsEmpty();	}

		JINLINE size_t GetNumberOfBytes() const								{ return GetPointer()->GetCount();				}
		JINLINE size_t GetNumberOfCharacters() const						{ return Begin().GetNumberOfCharacters();		}

		JINLINE const char* GetData() const									{ return (const char*) GetPointer()->Begin(); 	}
		JINLINE const char* GetEndOfData() const							{ return (const char*) GetPointer()->End(); 	}
		JINLINE Utf8Pointer Begin() const									{ return Utf8Pointer(GetPointer()->Begin());	}
		JINLINE Utf8Pointer End() const										{ return Utf8Pointer(GetPointer()->End());		}

		// For iterating char data, eg. for(const char c : string.RawBytes()
		JINLINE const StringData& RawBytes() const							{ return *GetPointer(); }
		
		JEXPORT bool JCALL Contains(Character c) const;
		JEXPORT bool JCALL Contains(const String& s) const;
		
		JEXPORT bool operator==(const String& a) const;
		JEXPORT bool operator< (const String& a) const;
		JEXPORT bool operator==(const char* a) const;
		JEXPORT bool operator< (const char* a) const;
		JEXPORT bool operator> (const char* a) const;
		
		JINLINE bool operator!=(const String& a) const						{ return !(*this == a); }
		JINLINE bool operator> (const String& a) const						{ return  (a < *this); }
		JINLINE bool operator<=(const String& a) const						{ return !(a < *this); }
		JINLINE bool operator>=(const String& a) const						{ return !(*this < a); }

		JINLINE bool operator!=(const char* a) const						{ return !(*this == a); }
		JINLINE bool operator<=(const char* a) const						{ return !(*this > a); }
		JINLINE bool operator>=(const char* a) const						{ return !(*this < a); }

		// 7-bit test
		JEXPORT bool  JCALL	IsAscii() const;
		
		JEXPORT bool  JCALL IsInteger() const;
		JEXPORT int   JCALL AsInteger() const;
		JEXPORT int   JCALL AsIntegerFromHex() const;
		JEXPORT bool  JCALL IsFloat() const;
		JEXPORT float JCALL AsFloat() const;
		JEXPORT static unsigned JCALL AsHex(const char* p, size_t length);
		
		JEXPORT bool JCALL BeginsWith(char c) const;
		JEXPORT bool JCALL BeginsWith(const String& a) const;
		JEXPORT bool JCALL EndsWith(const String& a) const;
		
		JEXPORT const char* JCALL AsUtf8String() const						{ return GetData(); }

		JEXPORT static String Concatenate(const String &a, const String &b);
		
		// left or right can be null, in which case the result is not written
		JEXPORT bool Split(const String& splitString, String* left, String* right) const;
		JEXPORT bool Split(char splitChar, String* left, String* right) const;
		JEXPORT bool SplitLast(char splitChar, String* left, String* right) const;
		
		JEXPORT Table<String> Split(char splitChar) const;
		
		JEXPORT bool JCALL IsLower() const;
		JEXPORT friend String JCALL ToLower(const String& string);
        JEXPORT friend String JCALL ToUpper(const String& string);

		JEXPORT String SubString(int startCharacterIndex) const;
		JEXPORT String SubString(int startCharacterIndex, int endCharacterIndex) const;
		
		JEXPORT String CreateJsonString() const;
		JEXPORT String CreateYamlString() const;

		enum class UrlEncode
		{
			SPACE_UNCHANGED,
			SPACE_TO_PLUS,
			SPACE_TO_PERCENT,
		};
		
		JEXPORT friend String UrlEncode(const String& s, UrlEncode urlEncode);
		JEXPORT friend String UrlDecode(const String& s, bool convertPlusses);
		JEXPORT friend String UrlDecode(const String& s)					{ return UrlDecode(s, true); }

		JEXPORT size_t JCALL GetHash() const;
		JINLINE friend size_t GetHash(const String& s)						{ return s.GetHash(); }
		
		// result MUST be able to hold GetNumberOfCharacters() characters. No terminating null provided.
		JEXPORT void JCALL ToCharacters(Character* result) const;
		
		// The default SmartPointer will write an object reference number.
		// We want strings to be written raw without references
		        friend IWriter& operator<<(IWriter& output, const String& a)	{ return output << *a.GetPointer(); }
		JEXPORT friend IReader& operator>>(IReader& input, String& a);
		
		JEXPORT friend size_t JCALL Levenshtein(const String& a, const String& b);
		JEXPORT friend size_t JCALL DamerauLevenshtein(const String& a, const String& b);
		
		JEXPORT static const String EMPTY_STRING;
		JEXPORT static const char LOWER_HEX_CHARACTERS[];
		JEXPORT static const char UPPER_HEX_CHARACTERS[];

	private:
		Utf8Pointer JCALL GetPointerToCharacterIndex(int i) const;
	};

//============================================================================
	
// This definition of JS has the following properties:
//	No memory allocations required
//  Release builds collapse the entire function, with no global guards for static variable accesses
//  The StringData is correctly defined in read-write memory so that reference count increments/decrements are valid
//  The string contents is defined right next to the structure so that cache misses are minimized.
//	Note that this also places the strings in read-write memory. If this is a concern, the alternative can be used below
	
	namespace Private
	{
		template<int N> struct RawStringInline
		{
			int*		stringData;
			mutable int	counter;
			const char*	data;
			size_t		count;
//			size_t		capacity;			// Don't bother storing capacity. It'll never be inspected for compile-time strings
			char		storage[N];
		};
	}

	#define JS(x)	([]() -> const Javelin::String& 								\
	{ 																				\
		static constexpr Javelin::Private::RawStringInline<sizeof(x)> rawString 	\
			{ &rawString.counter, 1, &rawString.storage[0], sizeof(x)-1, x };		\
		return *reinterpret_cast<const Javelin::String*>(&rawString); 				\
	})()

	// Other definitions that required some run time memory, no C++11 lambda trickery.
	//#define JS(x) String::CreateFromLiteral(x, sizeof(x)-1)
	//template<int N> JINLINE String JCALL JS(const char (&string)[N]) { return String::CreateFromLiteral(string, N-1); }

	// This places the string in read-only memory, and separates the String object from the StringData
	//
	//	namespace Private
	//	{
	// 		struct RawStringData
	//		{
	//			int			counter;
	//			const char*	data;
	//			size_t		count;
	//			size_t		capacity;
	//		};
	//	}
	//
	//	#define JS(x)	([]() -> const Javelin::String& 										\
	//	{ 																						\
	//		static Javelin::Private::RawStringData rawData{ 1, x, sizeof(x)-1, (size_t) -1 };	\
	//		static const Javelin::Private::RawStringData* stringProxy = &rawData; 				\
	//		return *reinterpret_cast<const Javelin::String*>(&stringProxy); 					\
	//	})()
	
//============================================================================

	String ToString(bool b);
	String ToString(const char* s);
	String ToString(char a);
	String ToString(unsigned char a);
	String ToString(int a);
	String ToString(unsigned a);
	String ToString(long long a);
	String ToString(long a);
	String ToString(unsigned long a);
	String ToString(unsigned long long a);
	String ToString(float a);
	String ToString(double a);
	String ToString(const String& a);
	template<size_t N> String ToString(const char (&p)[N])		{ return String(p);	}
	String ToString(const void* p);
	String ToString(const decltype(nullptr)&);
	template<typename T> String ToString(Interval<T> a) 		{ return String::Create("(%A,%A)", &ToString(a.min), &ToString(a.max)); }

//============================================================================
} // namespace Javelin
//============================================================================
