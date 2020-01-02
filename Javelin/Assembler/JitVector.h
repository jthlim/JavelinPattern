//============================================================================

#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

//============================================================================

namespace Javelin
{
//===========================================================================

	class JitVectorBase
	{
	public:
		JitVectorBase() = default;
		~JitVectorBase() { free(data); }

	protected:
		uint32_t offset = 0;
		uint32_t capacity = 0;
		uint8_t *data = nullptr;
		
		void* Append(uint32_t appendSize) 	{
												uint32_t newOffset = offset + appendSize;
												if(newOffset <= capacity)
												{
													uint8_t *result = data + offset;
													offset = newOffset;
													return result;
												}
												return ExpandAndAppend(appendSize);
											}
		void Reserve(uint32_t reserveSize);

	private:
		__attribute__((noinline)) void* ExpandAndAppend(uint32_t appendSize);
		
		JitVectorBase(const JitVectorBase&) = delete;
		void operator=(const JitVectorBase&) = delete;
	};

	template<typename T> class JitVector : private JitVectorBase
	{
	public:
		// These are only made public so that the inline assembler can access them.
		using JitVectorBase::offset;
		using JitVectorBase::capacity;
		using JitVectorBase::data;

		T& Append()								{ return *(T*) JitVectorBase::Append(sizeof(T)); }
		T* Append(uint32_t length)				{ return (T*) JitVectorBase::Append(length * sizeof(T)); }
		void AppendValue(T a) 					{ Append() = a; }

		void Reserve(uint32_t size) 			{ JitVectorBase::Reserve(size * sizeof(T)); }
		void Clear()							{ offset = 0; }

		uint32_t GetNumberOfBytes() const		{ return offset; }
		T& GetElementAtByteIndex(uint32_t i)	{ return *(T*) (data + i); }
		
		T* begin() 								{ return (T*) data; }
		T* end() 								{ return (T*) (data + offset); }
		
		T& operator[](uint32_t i)				{ return *((T*) data + i); }
		const T& operator[](uint32_t i) const	{ return *((T*) data + i); }
		
		void StartUseBacking(JitVector &a)
		{
			offset = a.offset;
			capacity = a.capacity;
			data = a.data;
		}
		void StopUseBacking(JitVector &a) {
			assert(capacity == a.capacity);
			assert(data == a.data);

			a.offset = offset;
			
			data = nullptr;
		}

	};

//============================================================================
} // namespace Javelin
//============================================================================
