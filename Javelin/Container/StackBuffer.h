//============================================================================

#pragma once
#include "Javelin/Container/AutoPointer.h"
#include "Javelin/Type/Function.h"

//============================================================================

namespace Javelin
{
//============================================================================

	template<typename T>
	auto JINLINE StackBuffer(size_t numberOfElements, T&& callback) -> typename Function<T>::ReturnType
	{
		static const size_t MAX_STACK_BUFFER = 64*1024;
		typedef typename TypeData<typename Function<T>::template Parameter<0>::Type>::NonPointerType BufferType;
		
		AutoArrayPointer<BufferType> allocated;
		BufferType* buffer;
		
		if(JLIKELY(numberOfElements < MAX_STACK_BUFFER/sizeof(BufferType)))
		{
			buffer = (BufferType*) alloca(numberOfElements*sizeof(BufferType));
		}
		else
		{
			allocated.SetSize(numberOfElements);
			buffer = allocated;
		}
		
		return callback(buffer);
	}

//============================================================================
} // namespace Javelin
//===========================================================================e
