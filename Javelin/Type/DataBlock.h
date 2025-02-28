//============================================================================

#pragma once
#include "Javelin/Container/Table.h"
#include "Javelin/Type/SmartPointer.h"
#include "Javelin/Type/String.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	class IReader;
	class IWriter;
	class Url;

	class TemporaryDataBlock;
	class FileMappedDataBlock;
	
	class DataBlock : public Table<unsigned char>, public SingleAssignSmartObject
	{
	private:
		typedef Table<unsigned char> Inherited;
		
	public:
		DataBlock() { }
		DataBlock(const DataBlock& data) : Inherited(data) { }
		DataBlock(DataBlock&& data) : Inherited((Inherited&&) data) { }
		DataBlock(size_t initialCapacity) : Inherited(initialCapacity) { }
		DataBlock(int initialCapacity) : Inherited((size_t) initialCapacity) { }
		DataBlock(IReader& input, size_t size);
		DataBlock(const void* data, size_t length);
		template<typename T>
			JINLINE DataBlock(const T& data) : DataBlock(data.GetData(), data.GetNumberOfBytes()) { }
		virtual ~DataBlock() { }
		
		template<typename T> static DataBlock FromHex(const T& a)	{ return FromHex(a.GetData(), a.GetNumberOfBytes()); }
		static DataBlock FromHex(const void* data, size_t length);
        static size_t FromHex(void* result, size_t resultLength, const void* data, size_t dataLength);
        JINLINE String JCALL AsHex() const                          { return String::CreateHexStringFromBytes(GetData(), GetNumberOfBytes()); }
        
		static DataBlock FromBase64(const String& string);
		static size_t FromBase64(void* result, size_t resultLength, const void* data, size_t dataLength);
		JINLINE String JCALL AsBase64() const						{ return AsBase64(GetData(), GetCount()); }
		static String AsBase64(const void* data, size_t length);
		
		void SetFromReader(IReader& reader, size_t size);
		
		void AppendData(const void* data, size_t size)				{ Inherited::AppendCount((const unsigned char*) data, size); 	}
		void AppendDataNoExpand(const void* data, size_t size)		{ Inherited::AppendCountNoExpand((const unsigned char*) data, size); 	}
		template<typename T> void AppendData(const T &d)			{ Inherited::AppendCount((const unsigned char*) d.GetData(), d.GetNumberOfBytes()); }
		void AppendFromReader(IReader& reader, size_t size);
		
		JEXPORT friend IWriter& JCALL operator<<(IWriter& writer, const DataBlock& data);
		
		JEXPORT friend size_t GetHash(const DataBlock& a);
		
		DataBlock& operator=(DataBlock&& a)							{ Table<unsigned char>::operator=((DataBlock&&) a); return *this; }
		DataBlock& operator=(const DataBlock& a)					{ Table<unsigned char>::operator=(a); return *this; }

		static const TemporaryDataBlock 	MakeReferenceTo(const void* data, size_t length);

		JEXPORT static const DataBlock EMPTY_DATA_BLOCK;

	protected:
		JINLINE DataBlock(const void* data, size_t length, const MakeReference* mr) : Inherited((const unsigned char*) data, length, mr) { }
	};

	class TemporaryDataBlock : public DataBlock
	{
	private:
		typedef DataBlock Inherited;
		
	public:
		TemporaryDataBlock(const void* data, size_t length);
		~TemporaryDataBlock();
	};

	JEXPORT String JCALL ToString(const DataBlock& a);

//============================================================================
} // namespace Javelin
//============================================================================
