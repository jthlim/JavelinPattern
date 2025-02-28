//============================================================================

#pragma once
#include "Javelin/Stream/ICharacterWriter.h"
#include "Javelin/Container/Map.h"
#include "Javelin/Container/Table.h"
#include "Javelin/Stream/SeekType.h"
#include "Javelin/Type/Endian.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class Url;
	class Variant;

//============================================================================

	class JABSTRACT IWriter : public ICharacterWriter
	{
	public:
    JEXPORT IWriter();
		JEXPORT ~IWriter();

		static JEXPORT IWriter* JCALL Create(const Url& url);
		static JEXPORT IWriter* JCALL Create(const Url& url, const Variant& parameters);

		// JEXPORT virtual Url JCALL GetUrl() const;

		virtual void JCALL Release()					{ delete this;	}

		// Block devices support Seek, Offset, Length
		virtual bool JCALL CanSeek() const				{ return false;	}

		// Seek to a position in a stream.
		virtual void JCALL Seek(long long offset, SeekType whence = SeekType::Current);
		JINLINE void JCALL Seek(SeekType whence)						{ Seek(0, whence); }

		// Returns the current offset in the stream
		virtual long long JCALL GetOffset() const						{ return 0;	}

		// Returns the total length of the stream
		JEXPORT virtual long long JCALL GetLength() const;

		JEXPORT virtual long long JCALL GetRemainingLength() const;

		// Write related functions.
		virtual bool JCALL IsWritable() const								{ return true;	}
		virtual size_t JCALL WriteAvailable(const void *data, size_t dataSize) = 0;
		JEXPORT virtual void JCALL WriteBlock(const void *data, size_t dataSize) override;

		using ICharacterWriter::WriteData;

		template<typename T> JINLINE void WriteCount(const T *a, size_t n)	{ WriteBlock(a, sizeof(T)*n);	}
		template<typename T> JINLINE void Write(const T &a)					{ WriteBlock(&a, sizeof(T));	}
		template<typename T> JINLINE void WriteValue(const T &a)			{ *this << a; }

		// From ICharacterWriter
		JEXPORT void JCALL WriteByte(unsigned char a) override;

		// Chaining write operators
		inline IWriter&	JCALL operator<<(char c)				{ Write(c); return *this;		}
		inline IWriter&	JCALL operator<<(signed char c)			{ Write(c); return *this;		}
		inline IWriter&	JCALL operator<<(unsigned char c)		{ Write(c); return *this;		}
		inline IWriter& JCALL operator<<(bool b)				{ WriteByte(b);	return *this;	}

#if defined(JENDIAN_LITTLE)
		inline IWriter& JCALL operator<<(short s)				{ Write(s); return *this;	}
		inline IWriter& JCALL operator<<(unsigned short s)		{ Write(s); return *this;	}
		inline IWriter& JCALL operator<<(int i)					{ Write(i); return *this;	}
		inline IWriter& JCALL operator<<(unsigned u)			{ Write(u); return *this;	}
		inline IWriter& JCALL operator<<(long l)				{ Write(l); return *this;	}
		inline IWriter& JCALL operator<<(unsigned long u)		{ Write(u); return *this;	}
		inline IWriter& JCALL operator<<(long long l)			{ Write(l); return *this;	}
		inline IWriter& JCALL operator<<(unsigned long long u)	{ Write(u); return *this;	}
		inline IWriter& JCALL operator<<(float f)				{ Write(f); return *this;	}
		inline IWriter& JCALL operator<<(double d)				{ Write(d); return *this;	}
#elif defined(JENDIAN_BIG)
		inline IWriter& JCALL operator<<(short s)				{ LittleEndian<short> a(s);					Write(a); return *this;	}
		inline IWriter& JCALL operator<<(unsigned short s)		{ LittleEndian<unsigned short> a(s);		Write(a); return *this;	}
		inline IWriter& JCALL operator<<(int i)					{ LittleEndian<int> a(i);					Write(a); return *this;	}
		inline IWriter& JCALL operator<<(unsigned u)			{ LittleEndian<unsigned> a(u);				Write(a); return *this;	}
		inline IWriter& JCALL operator<<(long l)				{ LittleEndian<long> a(l);					Write(a); return *this;	}
		inline IWriter& JCALL operator<<(unsigned long u)		{ LittleEndian<unsigned long> a(u);			Write(a); return *this;	}
		inline IWriter& JCALL operator<<(long long l)			{ LittleEndian<long long> a(l);				Write(a); return *this;	}
		inline IWriter& JCALL operator<<(unsigned long long u)	{ LittleEndian<unsigned long long> a(u);	Write(a); return *this;	}
		inline IWriter& JCALL operator<<(float f)				{ LittleEndian<float> a(f);					Write(a); return *this;	}
		inline IWriter& JCALL operator<<(double d)				{ LittleEndian<double> a(d);				Write(a); return *this;	}
#else
	#error Endian not defined!
#endif

		// Returns true if the object's content needs to be written
		bool WriteObjectReferenceId(void* data);
		void PurgeObjectReferenceId(void* data);

	private:
		int				nextObjectReferenceId;
		Map<void*, int>	objectReferenceIdMap;
	};

//============================================================================

	class IWriterFactory
	{
	public:
		virtual bool CanCreate(const Url& url, const Variant& parameters) const = 0;
		virtual IWriter* Create(const Url& url, const Variant& parameters) const = 0;

		void Register()	const { factoryList.Append(this); }

	protected:
		static Table<const IWriterFactory*> factoryList;
		friend class IWriter;
	};

//============================================================================

	// Will cause link errors if accidentally used
	template<> JINLINE void IWriter::WriteCount(const void* a, size_t n);

//============================================================================

} // namespace Javelin
//============================================================================
