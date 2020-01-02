//============================================================================
//
// SimpleJitMemoryManager inserts a 8-byte header to memory allocated in
// discrete pages.
//
//============================================================================

#include "Javelin/Assembler/JitMemoryManager.h"
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <unordered_map>

//============================================================================

using namespace Javelin;

//============================================================================

SimpleJitMemoryManager* SimpleJitMemoryManager::GetInstance()
{
	static SimpleJitMemoryManager instance;
	return &instance;
}

SimpleJitDataMemoryManager* SimpleJitDataMemoryManager::GetInstance()
{
	static SimpleJitDataMemoryManager instance;
	return &instance;
}

//============================================================================

struct SimpleJitMemoryManagerAllocationHeader
{
	size_t mapRegionLength;
};

//============================================================================

static size_t GetMapRegionLengthForLength(size_t length)
{
	static size_t pageSize = getpagesize();
	return (length + pageSize-1) & -pageSize;
}

//============================================================================

void* SimpleJitMemoryManager::Allocate(size_t length)
{
#if defined(__i386__) || defined(__amd64__)
	int mode = PROT_READ|PROT_WRITE|PROT_EXEC;
#else
	int mode = PROT_READ|PROT_WRITE;
#endif
	
	size_t mapRegionLength = GetMapRegionLengthForLength(length + sizeof(SimpleJitMemoryManagerAllocationHeader));
	void *mapRegion = mmap(0, mapRegionLength, mode, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	assert(mapRegion != MAP_FAILED);
	SimpleJitMemoryManagerAllocationHeader* header = (SimpleJitMemoryManagerAllocationHeader*) mapRegion;
	header->mapRegionLength = mapRegionLength;
	return header + 1;
}

void SimpleJitMemoryManager::EndWrite(void *data)
{
#if !defined(__i386__) && !defined(__amd64__)
	SimpleJitMemoryManagerAllocationHeader* header = (SimpleJitMemoryManagerAllocationHeader*) data - 1;
	mprotect(header, header->mapRegionLength, PROT_READ|PROT_EXEC);
#endif
}

void SimpleJitMemoryManager::Release(void *p)
{
	SimpleJitMemoryManagerAllocationHeader* header = (SimpleJitMemoryManagerAllocationHeader*) p - 1;
	munmap(header, header->mapRegionLength);
}

void SimpleJitMemoryManager::StaticInstanceRelease(void *p)
{
	GetInstance()->Release(p);
}

//============================================================================

void* SimpleJitDataMemoryManager::Allocate(size_t length)
{
#if defined(__arm64__)
	size_t mapRegionLength = GetMapRegionLengthForLength(length + sizeof(SimpleJitMemoryManagerAllocationHeader));
	void *mapRegion = mmap(0, mapRegionLength, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	assert(mapRegion != MAP_FAILED);
	SimpleJitMemoryManagerAllocationHeader* header = (SimpleJitMemoryManagerAllocationHeader*) mapRegion;
	header->mapRegionLength = mapRegionLength;
	return header + 1;
#else
	return malloc(length);
#endif
}

void SimpleJitDataMemoryManager::Release(void *p)
{
#if defined(__arm64__)
	SimpleJitMemoryManagerAllocationHeader* header = (SimpleJitMemoryManagerAllocationHeader*) p - 1;
	munmap(header, header->mapRegionLength);
#else
	free(p);
#endif
}

void SimpleJitDataMemoryManager::StaticInstanceRelease(void *p)
{
	GetInstance()->Release(p);
}

//============================================================================
