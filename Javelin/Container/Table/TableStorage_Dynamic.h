//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Template/Comparator.h"
#include "Javelin/Container/DataIntegrityPolicy.h"
#include "Javelin/Container/Table/TableIterator.h"
#include "Javelin/System/Assert.h"

//============================================================================

namespace Javelin
{
//============================================================================

	template<typename T> class DynamicTableAllocatePolicy_NewDelete
	{
	public:
		JINLINE static void* Allocate(size_t elementSize, size_t count)		{ return ::operator new(elementSize*count); }
		JINLINE static void	 Release(void* p)								{ ::operator delete(p);	}
		
		JINLINE static size_t GetRecommendedCapacity(size_t capacity)		{ return capacity; 	}
		JINLINE static constexpr size_t GetDefaultCapacity()				{ return 0; 		}
		JINLINE static void* GetDefaultAllocation()							{ return nullptr; 	}
		JINLINE static bool RequiresCopy(void* data)						{ return false; 	}

		static size_t GetExpandedTableCount(size_t oldTableSize)	
		{ 
			return oldTableSize + (oldTableSize >> 3) + 13;	
		}
	};

	template<size_t N> class DynamicTableAllocatePolicy_NewDeleteWithInlineStorage
	{
	public:
		template<typename T> class Policy
		{
		public:
			JINLINE void* Allocate(size_t elementSize, size_t count)		{ return count == N ? storage : ::operator new(elementSize*count); }
			JINLINE void  Release(void* p)									{ if(p != storage) ::operator delete(p);	}
			
			JINLINE static size_t GetRecommendedCapacity(size_t capacity)	{ return capacity < N ? N : capacity; 	}
			JINLINE static constexpr size_t GetDefaultCapacity()			{ return N; 							}
			JINLINE void* GetDefaultAllocation()							{ return storage;						}
			JINLINE bool RequiresCopy(void* data)							{ return storage == data; 				}
			
			static size_t GetExpandedTableCount(size_t oldTableSize)
			{
				return oldTableSize + (oldTableSize >> 3) + 13;
			}

		private:
			char storage[N*sizeof(T)];
		};
	};

//============================================================================

	template<template<typename> class AllocationPolicy = DynamicTableAllocatePolicy_NewDelete>
	class TableStorage_Dynamic
	{
	public:
		template<typename T, typename DataIntegrityPolicy = DataIntegrityPolicy_Automatic<T> > 
		class Implementation : private AllocationPolicy<T>
		{
		public:
			JINLINE	size_t	GetCount() 		const				{ return count;	}
			JINLINE	size_t	GetCapacity()	const				{ return capacity;	}

			void	SetCount(size_t size)						{
																	if(size < count)
																	{
																		DataIntegrityPolicy::Destroy(Begin()+size, count-size);
																		count = size;
																	}
																	else if(size > count)
																	{
																		if(size > capacity)
																		{
																			void* newData = AllocationPolicy<T>::Allocate(sizeof(T), size);
																			DataIntegrityPolicy::Relocate(reinterpret_cast<T*>(newData), reinterpret_cast<T*>(data), count);
																			AllocationPolicy<T>::Release(data);
																			DataIntegrityPolicy::CreateCount(reinterpret_cast<T*>(newData)+count, size-count);

																			data = newData;
																			capacity = size;
																		}
																		else
																		{
																			DataIntegrityPolicy::CreateCount(reinterpret_cast<T*>(data)+count, size-count);
																		}
																		count = size;
																	}
																}

			JINLINE	void	Reset() 							{ 
																	DataIntegrityPolicy::Destroy(Begin(), count);	
																	AllocationPolicy<T>::Release(data);
																	count	 = 0;
																	capacity = AllocationPolicy<T>::GetDefaultCapacity();
																	data	 = AllocationPolicy<T>::GetDefaultAllocation();
																}

			JINLINE	T&		Append()							{ PrepareAppend(1); T* p = &Begin()[count++]; DataIntegrityPolicy::Create(p); return *p; }

			template<typename... U>
			JINLINE void	Append(U&& ...a)					{
																	PrepareAppend(sizeof...(a));
																	AppendNoExpand((U&&) a...);
																}

			JINLINE	void	AppendCount(const T* p, size_t n)	{ PrepareAppend(n); DataIntegrityPolicy::InitialCopy(&Begin()[count], p, n); count += n; }
			JINLINE T*		AppendCount(size_t n)				{ PrepareAppend(n); T* r = Begin(); DataIntegrityPolicy::CreateCount(r, n); count += n; return r; }
			
			template<typename U>
			JINLINE	void	AppendNoExpand(U &&a)						{
																			JASSERT(count < capacity);
																			JASSERT((void*) &a < Begin() || (void*) &a >= End());
																			DataIntegrityPolicy::Create(&Begin()[count++], (U&&) a);
																		}

			template<typename U, typename V, typename... W>
			JINLINE	void	AppendNoExpand(U &&u, V &&v, W&& ...w)		{ AppendNoExpand((U&&) u); AppendNoExpand((V&&) v, (W&&) w...); }

			JINLINE	void	AppendCountNoExpand(const T* p, size_t n)	{ JASSERT(count+n <= capacity); DataIntegrityPolicy::InitialCopy(&Begin()[count], p, n); count += n; }
			JINLINE	void	AppendCountNoExpand(size_t n)				{ JASSERT(count+n <= capacity); DataIntegrityPolicy::CreateCount(&Begin()[count], n); count += n; }

			template<typename U>
			JINLINE void	InsertAtIndex(size_t i, U &&a) 		{
																	JASSERT(i <= count);
																	PrepareAppend(1);
																	DataIntegrityPolicy::CopyBackwards(Begin()+i+1, Begin()+i, count++ - i);
																	DataIntegrityPolicy::CreateOnZombie(&Begin()[i], (U&&) a);
																}
		
			JINLINE	void	RemoveIndex(size_t i)				{
																	JASSERT(i < count);
																	DataIntegrityPolicy::Copy(Begin()+i, Begin()+i+1, --count-i);
																	DataIntegrityPolicy::Destroy(Begin()+count);
																}

			JINLINE void	RemoveCount(size_t i, size_t numElements)	{
																			JASSERT(i+numElements <= count);
																			count -= numElements;
																			DataIntegrityPolicy::Copy(Begin()+i, Begin()+i+numElements, count-i);
																			DataIntegrityPolicy::Destroy(Begin()+count, numElements);
																		}
			
			
			JINLINE	void	RemoveBack()						{
																	JASSERT(count);
																	DataIntegrityPolicy::Destroy(Begin() + --count);
																}

			JINLINE	T*&			GetData()						{ return reinterpret_cast<T*&>(data);		}
			JINLINE	const T*	GetData() const					{ return reinterpret_cast<const T*>(data); 	}
			
			JINLINE	T*			Begin()							{ return reinterpret_cast<T*>(data);		}
			JINLINE	const T*	Begin()	const					{ return reinterpret_cast<const T*>(data); 	}

			JINLINE	T*			End()							{ return Begin() + count; }
			JINLINE	const T*	End()	const					{ return Begin() + count; }

			JINLINE	T&			operator[](size_t i)			{ JASSERT(i < GetCount()); return reinterpret_cast<T*>(data)[i]; }
			JINLINE	const T&	operator[](size_t i)	const	{ JASSERT(i < GetCount()); return reinterpret_cast<const T*>(data)[i]; }

			typedef T						ElementType;
			typedef TableIterator<T>		Iterator;
			typedef TableIterator<const T>	ConstIterator;

			void		Reserve(size_t newCapacity)					{
																		if(newCapacity < capacity) return;

																		void* newData = AllocationPolicy<T>::Allocate(sizeof(T), newCapacity);
																		DataIntegrityPolicy::Relocate(reinterpret_cast<T*>(newData), reinterpret_cast<T*>(data), count);
																		AllocationPolicy<T>::Release(data);

																		data = newData;
																		capacity = newCapacity;
																	}

		protected:
			JINLINE void Destroy()									{
																		DataIntegrityPolicy::Destroy(Begin(), count);
																		if(capacity != 0) AllocationPolicy<T>::Release(data);
																	}
			~Implementation()										{ Destroy(); }
			
			
			constexpr Implementation() : count(0), capacity(AllocationPolicy<T>::GetDefaultCapacity()), data(AllocationPolicy<T>::GetDefaultAllocation()) { }

			Implementation(const T* p, size_t length)				{
																		count	 = length;
																		capacity = AllocationPolicy<T>::GetRecommendedCapacity(length);
																		if(capacity != 0)
																		{
																			data = AllocationPolicy<T>::Allocate(sizeof(T), capacity);
																			DataIntegrityPolicy::InitialCopy(Begin(), p, length);
																		}
																		else
																		{
																			data = nullptr;
																		}
																	}


			Implementation(const T* p, size_t length, const MakeReference*)
																	{
																		count		= length;
																		capacity	= 0;
																		data		= (void*) p;
																	}
			
			void _ForceNoopDestructor()								{
																		capacity	= 0;
																	}
			
			Implementation(const Implementation& a)					{
																		count	 = a.count;
																		capacity = a.capacity;
																		if(a.capacity != 0)
																		{
																			data = AllocationPolicy<T>::Allocate(sizeof(T), capacity);
																			DataIntegrityPolicy::InitialCopy(Begin(), a.Begin(), count);
																		}
																		else
																		{
																			data = nullptr;
																		}
																	}

			Implementation(Implementation&& a) JNOTHROW				{
																		count		= a.count;
																		capacity	= a.capacity;
																		if(a.AllocationPolicy<T>::RequiresCopy(a.data))
																		{
																			data = AllocationPolicy<T>::GetDefaultAllocation();
																			DataIntegrityPolicy::Relocate(Begin(), a.Begin(), count);
																		}
																		else
																		{
																			data = a.data;
																		}
																		a.count		= 0;
																		a.capacity	= a.AllocationPolicy<T>::GetDefaultCapacity();
																		a.data		= a.AllocationPolicy<T>::GetDefaultAllocation();
																	}
			
			Implementation(size_t initialCapacity)					{
																		count	 = 0;
																		capacity = AllocationPolicy<T>::GetRecommendedCapacity(initialCapacity);
																		data	 = capacity == 0 ? nullptr : AllocationPolicy<T>::Allocate(sizeof(T), capacity);
																	}

			Implementation& operator=(const Implementation& a)		{
																		Destroy();

																		count = a.count;
																		capacity = a.capacity;
																		if(a.capacity != 0)
																		{
																			data = AllocationPolicy<T>::Allocate(sizeof(T), capacity);
																			DataIntegrityPolicy::InitialCopy(Begin(), a.Begin(), count);
																		}
																		else
																		{
																			data = nullptr;
																		}

																		return (*this);
																	}

			Implementation& operator=(Implementation&& a) JNOTHROW	{
																		Destroy();
																
																		count		= a.count;
																		capacity	= a.capacity;
				
																		if(a.AllocationPolicy<T>::RequiresCopy(a.data))
																		{
																			data = AllocationPolicy<T>::GetDefaultAllocation();
																			DataIntegrityPolicy::Relocate(Begin(), a.Begin(), count);
																		}
																		else
																		{
																			data = a.data;
																		}
				
																		a.count		= 0;
																		a.capacity	= a.AllocationPolicy<T>::GetDefaultCapacity();
																		a.data		= a.AllocationPolicy<T>::GetDefaultAllocation();
																		
																		return (*this);
																	}

			
			JINLINE void PrepareAppend(size_t expand)				{
																		if(count+expand > capacity)
																		{
																			ExpandData(expand);
																		}
																	}
			
		private:
			size_t	count;
			size_t	capacity;
			void*	data;
			
			void	ExpandData(size_t expand);
		};
	};

	template<template<typename> class AllocationPolicy>
	template<typename T, typename DataIntegrityPolicy>
	JNOINLINE void TableStorage_Dynamic<AllocationPolicy>::Implementation<T, DataIntegrityPolicy>::ExpandData(size_t expand)
	{
		size_t newCapacity = AllocationPolicy<T>::GetExpandedTableCount(count+expand);
		
		void* newData = AllocationPolicy<T>::Allocate(sizeof(T), newCapacity);
		DataIntegrityPolicy::Relocate(reinterpret_cast<T*>(newData), reinterpret_cast<T*>(data), count);
		AllocationPolicy<T>::Release(data);
		
		data = newData;
		capacity = newCapacity;
	}

//============================================================================
} // namespace Javelin
//============================================================================
