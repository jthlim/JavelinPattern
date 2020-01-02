#pragma once
#include <stdlib.h>

//============================================================================

namespace Javelin
{
//============================================================================

	/**
	 * The JitMemoryManager needs to manage executable memory allocations
	 * and memory map/mprotect status since some systems are W^X.
	 */
	class JitMemoryManager
	{
	public:
		constexpr JitMemoryManager() { }
		
		virtual void* Allocate(size_t length) = 0;
		virtual void Release(void *p) = 0;
		
		virtual void EndWrite(void *data) { }
		virtual void Shrink(void *p, size_t length) { }
	};

//============================================================================

	class SimpleJitMemoryManager final : public JitMemoryManager
	{
	public:
		static SimpleJitMemoryManager* GetInstance();
		
		static void StaticInstanceRelease(void *);

		virtual void* Allocate(size_t length) final;
		virtual void Release(void *p) final;
		
		virtual void EndWrite(void *data) final;
		
	private:
		SimpleJitMemoryManager() { }
	};
	
//============================================================================
	
	class SimpleJitDataMemoryManager final : public JitMemoryManager
	{
	public:
		static SimpleJitDataMemoryManager* GetInstance();
		
		static void StaticInstanceRelease(void *);

		virtual void* Allocate(size_t length) final;
		virtual void Release(void *p) final;
		
	private:
		SimpleJitDataMemoryManager() { }
	};
	
//============================================================================
} // namespace Javelin
//============================================================================
