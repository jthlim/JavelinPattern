//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

namespace Javelin
{
//============================================================================

	namespace Private
	{
		template<size_t N> JINLINE void InlineCopyMemory(void* destination, const void* source, size_t count)
		{
			memcpy(destination, source, count*N);
		}

		template<size_t N> JINLINE void InlineClearMemory(void* data, size_t count)
		{
			memset(data, 0, count*N);
		}

		template<size_t N> struct InlineSetMemorySizeType;
		template<> struct InlineSetMemorySizeType<1> { typedef uint8_t  Type; };
		template<> struct InlineSetMemorySizeType<2> { typedef uint16_t Type; };
		template<> struct InlineSetMemorySizeType<4> { typedef uint32_t Type; };
		template<> struct InlineSetMemorySizeType<8> { typedef uint64_t Type; };

		JINLINE void InlineSetMemory(void* data, uint8_t c, size_t count)
		{
			memset(data, c, count);
		}
		
#if defined(JASM_GNUC_X86) || defined(JASM_GNUC_X86_64)
		template<> JINLINE void InlineCopyMemory<4>(void* destination, const void* source, size_t count)
		{
			asm("rep movsl"
				: "+D"(destination), "+S"(source), "+c"(count)
                :
				: "memory");
		}

//		template<> JINLINE void InlineClearMemory<4>(void* data, size_t count)
//		{
//			asm("rep stosl"
//				: "+D"(data), "+c"(count)
//				: "a"(0)
//				: "memory");
//		}

		#define FUNCTION_INLINE_SET_MEMORY_2_DEFINED
		JINLINE void InlineSetMemory(void* data, uint16_t v, size_t count)
		{
			asm("rep stosw"
				: "+D"(data), "+c"(count)
				: "a"(v)
				: "memory");
		}

		#define FUNCTION_INLINE_SET_MEMORY_4_DEFINED
		JINLINE void InlineSetMemory(uint32_t* data, uint32_t v, size_t count)
		{
			asm("rep stosl"
				: "+D"(data), "+c"(count)
				: "a"(v)
				: "memory");
		}
#endif

#if defined(JASM_GNUC_X86_64)
		template<> JINLINE void InlineCopyMemory<8>(void* destination, const void* source, size_t count)
		{
			asm("rep movsq"
				: "+D"(destination), "+S"(source), "+c"(count)
				:
				: "memory");
		}

//		template<> JINLINE void InlineClearMemory<8>(void* data, size_t count)
//		{
//			asm("rep stosq"
//				: "+D"(data), "+c"(count)
//				: "a"(0)
//				: "memory");
//		}

		#define FUNCTION_INLINE_SET_MEMORY_8_DEFINED
		JINLINE void InlineSetMemory(uint64_t* data, uint64_t v, size_t count)
		{
			asm("rep stosq"
				: "+D"(data), "+c"(count)
				: "a"(v)
				: "memory");
		}
#endif
		
#if !defined(FUNCTION_INLINE_SET_MEMORY_2_DEFINED)
		JINLINE void InlineSetMemory(uint16_t* d, uint16_t v, size_t count)
		{
			for(size_t i = 0; i < count; ++i)
			{
				d[i] = v;
			}
		}
#endif
		
#if !defined(FUNCTION_INLINE_SET_MEMORY_4_DEFINED)
		JINLINE void InlineSetMemory(uint32_t* d, uint32_t v, size_t count)
		{
			for(size_t i = 0; i < count; ++i)
			{
				d[i] = v;
			}
		}
#endif
		
#if !defined(FUNCTION_INLINE_SET_MEMORY_8_DEFINED)
		JINLINE void InlineSetMemory(uint64_t* d, uint64_t v, size_t count)
		{
			for(size_t i = 0; i < count; ++i)
			{
				d[i] = v;
			}
		}
#endif
	}
	
	template<typename T> JINLINE void CopyMemory(T *destination, const T *source, int count)
	{
		Private::InlineCopyMemory<sizeof(T)>(destination, source, count);
	}

	template<typename T> JINLINE void CopyMemory(T *destination, const T *source, size_t count)
	{
		Private::InlineCopyMemory<sizeof(T)>(destination, source, count);
	}
	
	template<typename T> JINLINE void MoveMemory(T *destination, const T *source, int count)
	{
		memmove((void*) destination, (void*) source, count*sizeof(T));
	}

	template<typename T> JINLINE void MoveMemory(T *destination, const T *source, size_t count=1)
	{
		memmove((void*) destination, (void*) source, count*sizeof(T));
	}

	template<typename T> JINLINE void ClearMemory(T& a)
	{
		memset(&a, 0, sizeof(T));
	}
	
	template<typename T> JINLINE void ClearMemory(T *a, int count)
	{
		Private::InlineClearMemory<sizeof(T)>(a, count);
	}

	template<typename T> JINLINE void ClearMemory(T *a, size_t count)
	{ 
		Private::InlineClearMemory<sizeof(T)>(a, count);
	}

	template<typename T> JINLINE void SetMemory(T *a, T b, size_t count)
	{
		Private::InlineSetMemory((typename Private::InlineSetMemorySizeType<sizeof(T)>::Type*) a, (typename Private::InlineSetMemorySizeType<sizeof(T)>::Type) b, count);
	}

	template<typename T> JINLINE int CompareMemory(const T *a, const T *b, int count)
	{
		return memcmp(a, b, count*sizeof(T));
	}

	template<typename T> JINLINE int CompareMemory(const T *a, const T *b, size_t count=1)
	{
		return memcmp(a, b, count*sizeof(T));
	}

//============================================================================
} // namespace Javelin
//============================================================================
