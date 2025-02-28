//============================================================================

#pragma once
#include "Javelin/Container/Table.h"
#include "Javelin/Stream/ICharacterReader.h"
#include "Javelin/Stream/SeekType.h"
#include "Javelin/Type/Endian.h"
#include "Javelin/Type/String.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class DataBlock;
	class Url;
	class Variant;

//============================================================================

	class JABSTRACT IReader : public ICharacterReader
	{
	public:
    IReader() { }
		~IReader() { }

		static JEXPORT IReader* JCALL Create(const Url& url);
		static JEXPORT IReader* JCALL Create(const Url& url, const Variant& parameters);

		// JEXPORT virtual Url JCALL GetUrl() const;

		virtual void JCALL Close()								{ }
		virtual void JCALL Release()							{ delete this;	}

		// Block devices support Seek, Offset, Length
		virtual bool JCALL CanSeek() const						{ return false;	}

		// Seek to a position in a stream.
		virtual void JCALL Seek(long long offset, SeekType whence = SeekType::Current);
		JINLINE void JCALL Seek(SeekType whence)				{ Seek(0, whence);	}

		// Returns the current offset in the stream
		virtual long long JCALL GetOffset() const				{ return 0;	}

		// Returns the total length of the stream
		JEXPORT virtual long long JCALL GetLength() const;
		JEXPORT virtual long long JCALL GetRemainingLength() const;

		// Read related functions.
		virtual bool JCALL IsReadable() const					{ return true;	}
		virtual size_t JCALL ReadAvailable(void *data, size_t dataSize) = 0;
		JEXPORT virtual void JCALL ReadBlock(void *data, size_t dataSize);
		JEXPORT DataBlock ReadBlock(size_t dataSize);

		template<typename T> JINLINE void ReadCount(T *a, size_t n)	{ ReadBlock(a, sizeof(T)*n);	}
		template<typename T> JINLINE void Read(T &a)				{ ReadBlock(&a, sizeof(T));		}
		template<typename T> JINLINE T Read()						{ T a; ReadBlock(&a, sizeof(T)); return a; }
		template<typename T> JINLINE T ReadValue()					{ return T(*this); 				}
		JEXPORT virtual unsigned char JCALL ReadByte() override;

		JEXPORT virtual String JCALL ReadAllAsString() override;
		JEXPORT virtual DataBlock JCALL ReadAll();

		// Operator overloaded data access.
		JINLINE IReader& JCALL operator>>(char &c)					{ Read(c); return *this;	}
		JINLINE IReader& JCALL operator>>(signed char &c)			{ Read(c); return *this;	}
		JINLINE IReader& JCALL operator>>(unsigned char &c)			{ Read(c); return *this;	}
		JINLINE IReader& JCALL operator>>(bool &b)					{ b = (ReadByte() != 0); return *this; }

#if defined(JENDIAN_LITTLE)
		JINLINE IReader& JCALL operator>>(short &s)					{ Read(s); return *this;	}
		JINLINE IReader& JCALL operator>>(unsigned short &s)		{ Read(s); return *this;	}
		JINLINE IReader& JCALL operator>>(int &i)					{ Read(i); return *this;	}
		JINLINE IReader& JCALL operator>>(unsigned &u)				{ Read(u); return *this;	}
		JINLINE IReader& JCALL operator>>(long &l)					{ Read(l); return *this;	}
		JINLINE IReader& JCALL operator>>(unsigned long &u)			{ Read(u); return *this;	}
		JINLINE IReader& JCALL operator>>(long long &l)				{ Read(l); return *this;	}
		JINLINE IReader& JCALL operator>>(unsigned long long &u)	{ Read(u); return *this;	}
		JINLINE IReader& JCALL operator>>(float &f)					{ Read(f); return *this;	}
		JINLINE IReader& JCALL operator>>(double &d)				{ Read(d); return *this;	}
#elif defined(JENDIAN_BIG)
		JINLINE IReader& JCALL operator>>(short &s)					{ LittleEndian<short> a;				Read(a); s = a; return *this;	}
		JINLINE IReader& JCALL operator>>(unsigned short &s)		{ LittleEndian<unsigned short> a;		Read(a); s = a; return *this;	}
		JINLINE IReader& JCALL operator>>(int &i)					{ LittleEndian<int> a;					Read(a); i = a; return *this;	}
		JINLINE IReader& JCALL operator>>(unsigned &u)				{ LittleEndian<unsigned> a;				Read(a); u = a; return *this;	}
		JINLINE IReader& JCALL operator>>(long &l)					{ LittleEndian<long> a;					Read(a); l = a; return *this;	}
		JINLINE IReader& JCALL operator>>(unsigned long &u)			{ LittleEndian<unsigned long> a;		Read(a); u = a; return *this;	}
		JINLINE IReader& JCALL operator>>(long long &l)				{ LittleEndian<long long> a;			Read(a); l = a; return *this;	}
		JINLINE IReader& JCALL operator>>(unsigned long long &u)	{ LittleEndian<unsigned long long> a;	Read(a); u = a; return *this;	}
		JINLINE IReader& JCALL operator>>(float &f)					{ LittleEndian<float> a; 				Read(a); f = a; return *this;	}
		JINLINE IReader& JCALL operator>>(double &d)				{ LittleEndian<double> a; 				Read(a); d = a; return *this;	}
#else
	#error Endian not defined!
#endif
		void* GetObjectReferenceId(int referenceId) const;
		void  SetObjectReferenceId(int referenceId, void* obj);

	private:
		typedef Table<void*, TableStorage_Dynamic<DynamicTableAllocatePolicy_NewDeleteWithInlineStorage<8>::Policy>> ReferenceList;
		ReferenceList referenceList;
	};

//============================================================================

	class IReaderFactory
	{
	public:
		virtual bool CanCreate(const Url& url, const Variant& parameters) const = 0;
		virtual IReader* Create(const Url& url, const Variant& parameters) const = 0;

		void Register()	const { factoryList.Append(this); }

	protected:
		static Table<const IReaderFactory*> factoryList;
		friend class IReader;
	};

//============================================================================

	// Will cause link errors if accidentally used
	template<> JINLINE void IReader::ReadCount(void* a, size_t n);

	#define DELCARE_IREADER_READ_VALUE(T) template<> JINLINE T IReader::ReadValue<T>()	{ T a; *this >> a; return a; }
		DELCARE_IREADER_READ_VALUE(bool)
		DELCARE_IREADER_READ_VALUE(char)
		DELCARE_IREADER_READ_VALUE(signed char)
		DELCARE_IREADER_READ_VALUE(unsigned char)
		DELCARE_IREADER_READ_VALUE(short)
		DELCARE_IREADER_READ_VALUE(unsigned short)
		DELCARE_IREADER_READ_VALUE(int)
		DELCARE_IREADER_READ_VALUE(unsigned int)
		DELCARE_IREADER_READ_VALUE(long)
		DELCARE_IREADER_READ_VALUE(unsigned long)
		DELCARE_IREADER_READ_VALUE(long long)
		DELCARE_IREADER_READ_VALUE(unsigned long long)
		DELCARE_IREADER_READ_VALUE(float)
		DELCARE_IREADER_READ_VALUE(double)
	#undef DELCARE_IREADER_READ_VALUE

//============================================================================
} // namespace Javelin
//============================================================================
