//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Math/Random.h"
#include "Javelin/Type/TypeData.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	namespace Private
	{
		template<bool IS_BITWISE_COPY_SAFE, typename T> struct SwapClass
		{
			static void DoSwap(T& a, T& b)
			{
				T c((T&&) a);
				a = (T&&) b;
				b = (T&&) c;
			}
		};
		
		template<typename T> struct SwapClass<true, T>
		{
			static void DoSwap(T& a, T& b)
			{
#pragma clang diagnostic push			
#pragma clang diagnostic ignored "-Wdynamic-class-memaccess"
				char c[sizeof(T)];
				memcpy(&c, &a, sizeof(T));
				memcpy(&a, &b, sizeof(T));
				memcpy(&b, &c, sizeof(T));
#pragma clang diagnostic pop
			}
		};
	} // namespace Private
	
	template<typename T> void Swap(T& a, T& b)					{ Private::SwapClass< TypeData<T>::IS_BITWISE_COPY_SAFE, T>::DoSwap(a, b);	}
	
	template<typename T> void SwapByBitwiseCopy(T& a, T& b)		{ Private::SwapClass<true, T>::DoSwap(a, b);	}
	
	void SwapBlock(void* a, void* b, size_t length);
	template<typename T> void Swap(T* a, T* b, size_t count)	{ SwapBlock(a, b, count*sizeof(T)); }
	
	template<typename T> void Shuffle(T* begin, size_t count)	{
																	while(count > 1)
																	{
																		size_t swapIndex = Random::instance.Generate(count);
																		if(swapIndex != 0)
																		{
																			Swap(begin[0], begin[swapIndex]);
																		}
																		--count;
																		++begin;
																	}
																}
	
	template<typename T> void Reverse(T* begin, size_t count)	{
																	T* end = begin + (count - 1);
																	while(begin < end) Swap(*begin++, *end--);
																}
	
//============================================================================
}
//============================================================================
