//============================================================================

#pragma once
#include "Javelin/Stream/IWriter.h"
#include "Javelin/Type/DataBlock.h"

//============================================================================

namespace Javelin 
{
//============================================================================
	
	class String;
	
//============================================================================

	class DataBlockWriter : public IWriter
	{
	public:
		JEXPORT DataBlockWriter(size_t initialCapacity=1024);
		JEXPORT DataBlockWriter(DataBlockWriter&& a);
	
		JEXPORT size_t JCALL WriteAvailable(const void *data, size_t dataSize) final;
		JEXPORT void   JCALL WriteBlock(const void* data, size_t dataSize) final;
		JEXPORT void   JCALL WriteByte(unsigned char c) final;

		size_t					GetCount()			const	{ return dataBlock.GetCount();			}
		size_t					GetNumberOfBytes()	const	{ return dataBlock.GetNumberOfBytes(); 	}
		const unsigned char*	GetData()			const	{ return dataBlock.GetData();			}
		void					Reset()						{ dataBlock.Reset();					}
		
		DataBlock& GetBuffer() 							 	{ return dataBlock; 					}
		const DataBlock& GetBuffer() 				const 	{ return dataBlock; 					}
		
		void operator=(DataBlockWriter&& a)					{ dataBlock = (DataBlock&&) a.dataBlock; }
		void operator=(const DataBlockWriter&) = delete;

		void AdoptIfSmaller(DataBlockWriter& a);
		
	protected:
		DataBlock	dataBlock;
	};

//============================================================================
} // namespace Javelin
//============================================================================
