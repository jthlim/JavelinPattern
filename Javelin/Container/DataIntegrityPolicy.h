//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Template/Memory.h"
#include "Javelin/Template/Range.h"
#include "Javelin/Type/TypeData.h"

//============================================================================

namespace Javelin
{
//============================================================================


	// This represents data that has no destructor or constructor
	class DataIntegrityPolicy_BitwiseCopyPODType
	{
	public:
		// Create default object
		template<typename T, typename Allocator>
			JINLINE static T* New()
			{
				return (T*) Allocator::operator new(sizeof(T));
			}
		
		template<typename T, typename Allocator>
			JINLINE static T* NewCount(size_t count)
			{
				return (T*) Allocator::operator new[](count*sizeof(T));
			}
		
		template<typename T, typename Allocator>
			JINLINE static T* NewCount(const T* source, size_t count)
			{
				T* p = (T*) Allocator::operator new[](count*sizeof(T));
				CopyMemory(p, source, count);
				return p;
			}
		
		template<typename T, typename Allocator>
			JINLINE static void Delete(T* p)
			{
				Allocator::operator delete(p);
			}
		
		template<typename T, typename Allocator>
			JINLINE static void DeleteCount(T* p, size_t count)
			{
				Allocator::operator delete[](p);
			}
		
		template<typename T>
			JINLINE static void Create(T* p)
			{
			}

		template<typename T>
			JINLINE static void CreateCount(T* p, size_t n)
			{
			}

		template<typename T, typename U>
			JINLINE static void Create(T* p, U &&a)
			{
				*p = (U&&) a;
			}

		template<typename T, typename U>
			JINLINE static void CreateOnZombie(T* p, U &&a)
			{
				*p = (U&&) a;
			}

		template<typename T>
			JINLINE static void Relocate(T* destination, const T* source, size_t count)
			{
				CopyMemory(destination, source, count);
			}

		// Copy source into initialized destination
		template<typename T>
			JINLINE static void Copy(T* destination, const T* source, size_t count)
			{
				MoveMemory(destination, source, count);
			}

		// Copy source into initialized destination
		template<typename T>
			JINLINE static void CopyBackwards(T* destination, const T* source, size_t count)
			{
				MoveMemory(destination, source, count);
			}

		// Copy source into uninitialized destination
		template<typename T>
			JINLINE static void InitialCopy(T* destination, const T* source, size_t count)
			{
				CopyMemory(destination, source, count);
			}

		template<typename T>
			JINLINE static void Destroy(T* p)
			{
			}

		template<typename T>
			JINLINE static void Destroy(T* p, size_t count)
			{
			}
	};

//============================================================================

	// Objects that need to be constructed and destructed, but can be memcpy to a new location
#pragma clang diagnostic push			
#pragma clang diagnostic ignored "-Wdynamic-class-memaccess"
	class DataIntegrityPolicy_BitwiseCopyObject : public DataIntegrityPolicy_BitwiseCopyPODType
	{
	public:
		// Create default object
		template<typename T, typename Allocator>
			JINLINE static T* New()
			{
				T* p = (T*) Allocator::operator new(sizeof(T));
				Create(p);
				return p;
			}
		
		template<typename T, typename Allocator>
			JINLINE static T* NewCount(size_t count)
			{
				T* p = (T*) Allocator::operator new[](count*sizeof(T));
				CreateCount(p, count);
				return p;
			}

		template<typename T, typename Allocator>
			JINLINE static T* NewCount(const T* source, size_t count)
			{
				T* p = (T*) Allocator::operator new[](count*sizeof(T));
				for(size_t i : Range(count)) new(Placement(p+i)) T(source[i]);
				return p;
			}
			
		template<typename T, typename Allocator>
			JINLINE static void Delete(T* p)
			{
				p->~T();
				Allocator::operator delete(p);
			}
		
		template<typename T, typename Allocator>
			JINLINE static void DeleteCount(T* p, size_t count)
			{
				Destroy(p, count);
				Allocator::operator delete[](p);
			}
		
		template<typename T>
			JINLINE static void Create(T* p)
			{
				new(Placement(p)) T;
			}

		template<typename T>
			JINLINE static void CreateCount(T* p, size_t n)
			{
				for(size_t i : Range(n)) new(Placement(p+i)) T;
			}

		template<typename T, typename U>
			JINLINE static void Create(T* p, U &&a)
			{
				new(Placement(p)) T((U&&) a);
			}
		
		template<typename T, typename U>
			JINLINE static void CreateOnZombie(T* p, U &&a)
			{
				new(Placement(p)) T((U&&) a);
			}

		// Copy source into uninitialized destination
		template<typename T>
			JINLINE static void Relocate(T* destination, const T* source, size_t count)
			{
				CopyMemory(destination, source, count);
			}

		// Copy source into initialized destination
		template<typename T, typename U>
			static void Copy(T* destination, U* source, size_t count)
			{
				for(size_t i : Range(count))
				{
					destination[i] = source[i];
				}
			}

		template<typename T, typename U>
			static void CopyBackwards(T* destination, U* source, size_t count)
			{
				for(size_t i = count; i > 0; --i)
				{
					destination[i-1] = source[i-1];
				}
			}

		template<typename T>
			static void InitialCopy(T* destination, const T* source, size_t count)
			{
				for(size_t i : Range(count))
				{
					new(Placement(destination+i)) T(source[i]);
				}
			}

		template<typename T>
		static void InitialCopy(T* destination, T* source, size_t count)
		{
			for(size_t i : Range(count))
			{
				new(Placement(destination+i)) T(source[i]);
			}
		}
		
		template<typename T>
			JINLINE static void Destroy(T* p)
			{
				p->~T();
			}

		template<typename T>
			static void Destroy(T* p, size_t count)
			{
				while(count) { p[--count].~T(); }
			}
	};
#pragma clang diagnostic pop

//============================================================================

	// Data requires object move semantics
	class DataIntegrityPolicy_Object : public DataIntegrityPolicy_BitwiseCopyObject
	{
	public:
		template<typename T, typename U>
			JINLINE static void CreateOnZombie(T* p, U &&a)
			{
				*p = (U&&) a;
			}

		template<typename T, typename U>
			static void Relocate(T* destination, U* source, size_t count)
			{
				for(size_t i : Range(count))
				{
					new(Placement(destination+i)) T((T&&) source[i]);
				}
				Destroy(source, count);
			}
	};

//============================================================================

	template<typename T, bool isBitwiseCopySafe> class DataIntegrityPolicy_AutomaticObjectType : public DataIntegrityPolicy_BitwiseCopyObject { };
	template<typename T> class DataIntegrityPolicy_AutomaticObjectType<T, false> : public DataIntegrityPolicy_Object { };

	template<typename T, bool isPODType> class DataIntegrityPolicy_AutomaticPODType : public DataIntegrityPolicy_BitwiseCopyPODType { };
	template<typename T> class DataIntegrityPolicy_AutomaticPODType<T, false> : public DataIntegrityPolicy_AutomaticObjectType<T, TypeData<T>::IS_BITWISE_COPY_SAFE> { };

	template<typename T> class DataIntegrityPolicy_Automatic : public DataIntegrityPolicy_AutomaticPODType<T, TypeData<T>::IS_POD> { };

//============================================================================
} // namespace Javelin
//============================================================================
