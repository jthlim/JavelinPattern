//============================================================================

#pragma once
#include "Javelin/Allocators/StandardAllocator.h"
#include "Javelin/Container/DataIntegrityPolicy.h"
#include "Javelin/Container/Optional.h"
#include "Javelin/Math/BitUtility.h"
#include "Javelin/Math/HashFunctions.h"
#include "Javelin/System/Assert.h"
#include "Javelin/Template/Comparator.h"
#include "Javelin/Template/Memory.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	template<typename Storage> class OpenHashSetIteratePolicy
	{
	public:
		JINLINE static void OnCreate(Storage& s)
		{
		}
		
		template<typename HashType, typename U> class Iterator
		{
		public:
			Iterator(HashType& aHashSet, size_t aIndex) : hashSet(&aHashSet), index(aIndex) { }
			
			bool operator==(const Iterator& a) const 	{ return hashSet == a.hashSet && index == a.index; }
			bool operator!=(const Iterator& a) const	{ return hashSet != a.hashSet || index != a.index; }
			Iterator& operator++()						{ index = hashSet->FindNextUsedIndex(index+1); return *this;	}
			
			U* operator->() const 						{ return &hashSet->GetValueAtIndex(index); 	}
			U& operator*() const						{ return hashSet->GetValueAtIndex(index); 	}
			
			Storage& GetStorage()						{ return hashSet->GetStorage(index);		}
			
			void Remove()								{ hashSet->RemoveHashIndex(index); --index; }
			size_t GetIndex() const						{ return index; 							}
			
			HashType*	hashSet;
			size_t		index;
			
			static Iterator CreateBegin(HashType& h)	{ return Iterator(h, h.FindFirstUsedIndex()); 	}
			static Iterator CreateEnd(HashType& h)		{ return Iterator(h, h.GetCapacity());			}
		};
	};
	
	// There must be a size_t GetHash(T) available.
	template<typename T,
			 typename Storage=Optional<T>,
			 typename Comparator=Equal,
			 typename Alloc=StandardAllocator,
			 template<typename> class IteratePolicy=OpenHashSetIteratePolicy>
	class OpenHashSet : public Alloc, public IteratePolicy<Storage>
	{
	public:
		typedef Alloc Allocator;
		typedef T KeyType;
		typedef typename IteratePolicy<Storage>::template Iterator<OpenHashSet, T> Iterator;
		typedef typename IteratePolicy<Storage>::template Iterator<const OpenHashSet, const T> ConstIterator;

		OpenHashSet();
		OpenHashSet(size_t initialCapacity);
		OpenHashSet(const OpenHashSet& a);
		OpenHashSet(OpenHashSet&& a);
		~OpenHashSet();
		
		OpenHashSet& operator=(const OpenHashSet& a)	{ this->~OpenHashSet(); new(Placement(this)) OpenHashSet(a); return *this; }
		OpenHashSet& operator=(OpenHashSet&& a) 		{ this->~OpenHashSet(); new(Placement(this)) OpenHashSet((OpenHashSet&&) a); return *this; }
		
		bool IsEmpty() 			const 					{ return count == 0; }
		bool HasData() 			const 					{ return count != 0; }
		size_t GetCount()	 	const 					{ return count;		 }
		size_t GetCapacity()	const					{ return capacity;	 }

		void Reset()									{ this->~OpenHashSet(); new(Placement(this)) OpenHashSet; }
		
		JINLINE Iterator Begin()						{ return Iterator::CreateBegin(*this); 	}
		JINLINE Iterator End()							{ return Iterator::CreateEnd(*this);	}
		JINLINE ConstIterator Begin() const				{ return ConstIterator::CreateBegin(*this); }
		JINLINE ConstIterator End() const				{ return ConstIterator::CreateEnd(*this);	}

		template<typename A>
		JINLINE Iterator Find(A&& key)					{ return Iterator(*this, FindHashIndex(key)); }
		template<typename A>
		JINLINE ConstIterator Find(A&& key) const		{ return ConstIterator(*this, FindHashIndex(key)); }

		// Returns the object in the hash table for key. Inserts it if necessary.
		template<typename A>
		T& Get(const A& key);
		
		// Returns the object in the has table for the key. The object must exist.
		template<typename A>
		T& Get(const A& key) const;
		
		template<typename A>
		bool Contains(const A& key) const;

		// Returns true if a new element has been added
		template<typename A>
		bool Put(const A& key);
		
		// This is a special case to help construct hash results in place. This is especially useful for Map
		// Clang stopped compiling this out-of-line when IteratorPolicy was introduced!
		template<typename KeyType, typename... ExtraConstructorArguments>
		T& Insert(KeyType&& key, ExtraConstructorArguments&&... v)
		{
			// Why is this done inline? Because of a clang bug not able to bind the name otherwise!
			if(++count >= threshold) ExpandCapacityAndRehash();
			
			size_t index = GetHashIndex(key);
			while(values[index].HasValue())
			{
				JASSERT(!Comparator::IsEqual((const T&) values[index], key));
				index = (index+1) & mask;
			}
			
			values[index].Create((KeyType&&) key, (ExtraConstructorArguments&&) v...);
			IteratePolicy<Storage>::OnCreate(values[index]);
			return values[index];
		}

		template<typename A>
		void Remove(const A& key);
		
		void Remove(const Iterator& it)					{ RemoveHashIndex(it.GetIndex()); }
		
		void Remove(const Storage& entry);
		
		template<typename A>
		bool RemoveIfExists(const A& key);

		// These should be private.. gotta figure out how to make them such again
		size_t FindFirstUsedIndex() const;
		size_t FindNextUsedIndex(size_t i) const;
		void RemoveHashIndex(size_t holeIndex);
		const Storage& GetStorage(size_t index) const	{ return values[index]; }
		Storage& GetStorage(size_t index)				{ return values[index]; }
		T& GetValueAtIndex(size_t index) 				{ return values[index]; }
		const T& GetValueAtIndex(size_t index) const 	{ return values[index]; }

	protected:
		template<typename A>
		JINLINE size_t GetHashIndex(const A& key) const;
		
		JINLINE void AllocateBuffers();
		void ExpandCapacityAndRehash();
		
		JINLINE void VerifyCount() const
		{
#if JBUILDCONFIG_DEBUG
			// If this fails, then most likely it's using MapStorage_OpenHashSetEmptyValueNullPointer
			// And at some stage, there has been a write of a nullptr, or a read on a non-existent value.
			size_t actualCount = 0;
			for(size_t i = 0; i < capacity; ++i) if(values[i].HasValue()) ++actualCount;
			JASSERT(count == actualCount);
#endif
		}
		
		template<typename A>
		size_t FindHashIndex(A&& key) const;
		
		size_t		capacity;
		size_t		count;
		size_t		mask;
		size_t		threshold;
		Storage*	values;
		
		void MoveEntryFromOldHash(Storage& entry);
	};

//============================================================================
//============================================================================

	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy> template<typename A>
		size_t OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::GetHashIndex(const A& key) const
	{
		size_t h = GetHash(key);
		
		// Make sure the object we're testing has the same hash key.
		// If this doesn't match, cast the key to T before calling into OpenHashSet
		JASSERT((CheckEqualHash<T, A>::IsEqual(h, key)));
		
		// These are the last steps in the murmur hash
		size_t hash = h;
		hash ^= hash >> 16;
		hash *= 0x85ebca6b;
		hash ^= hash >> 13;
		hash *= 0xc2b2ae35;
		hash ^= hash >> 16;
		
		return hash & mask;
	}
	
	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy>
	OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::OpenHashSet()
	{
		capacity = 0;
		mask = 0;
		count = 0;
		threshold = 0;
		values = nullptr;
	}
	
	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy>
		OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::OpenHashSet(size_t initialCapacity)
	{
		if(initialCapacity < 4) initialCapacity = 4;
		size_t setSize = (initialCapacity + initialCapacity * 2) >> 1;	// setSize = 1.5 * initialCapacity
		
		capacity = BitUtility::RoundUpToPowerOf2(setSize);
		mask = capacity-1;
		count = 0;
		threshold = (capacity + capacity * 2) >> 2;	// threshold = 0.75 * capacity
		AllocateBuffers();
	}

	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy>
	OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::OpenHashSet(const OpenHashSet& a)
	{
		capacity = a.capacity;
		count = a.count;
		mask = a.mask;
		threshold = a.threshold;
		AllocateBuffers();
		
		for(const auto& it : a) Insert(it);
	}
	
	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy>
		OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::OpenHashSet(OpenHashSet&& a) : IteratePolicy<Storage>((IteratePolicy<Storage>&&)a)
	
	{
		capacity = a.capacity;
		count = a.count;
		mask = a.mask;
		threshold = a.threshold;
		values = a.values;
		
		a.capacity = 0;
		a.count = 0;
		a.mask = 0;
		a.threshold = 0;
		a.values = nullptr;
	}
	
	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy>
		OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::~OpenHashSet()
	{
		DataIntegrityPolicy_Automatic<Storage>::template DeleteCount<Storage, Allocator>(values, capacity);
	}

	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy>
		JINLINE void OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::AllocateBuffers()
	{
		values = DataIntegrityPolicy_Automatic<Storage>::template NewCount<Storage, Allocator>(capacity);
	}

	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy> template<typename A>
		T& OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::Get(const A& key)
	{
		// Speculatively rehash if we're near the threshold
		if(count + 1 >= threshold) ExpandCapacityAndRehash();
		
		size_t index = GetHashIndex(key);
		while(true)
		{
			if(values[index].IsEmpty())
			{
				++count;
				T& result = values[index].Create(key);
				IteratePolicy<Storage>::OnCreate(values[index]);
				return result;
			}
			if(Comparator::IsEqual((const T&) values[index], key)) return values[index];
			index = (index+1) & mask;
		}
	}

	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy> template<typename A>
		T& OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::Get(const A& key) const
	{
        JASSERT(values != nullptr);
		
		size_t index = GetHashIndex(key);
		while(true)
		{
            JASSERT(values[index].HasValue());
			if(Comparator::IsEqual((const T&) values[index], key)) return values[index];
			index = (index+1) & mask;
		}
	}

	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy> template<typename A>
		bool OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::Contains(const A& key) const
	{
		if(!values) return false;
		
		size_t index = GetHashIndex(key);
		while(true)
		{
			if(values[index].IsEmpty()) return false;
			if(Comparator::IsEqual((const T&) values[index], key)) return true;
			index = (index + 1) & mask;
		}
	}

	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy> template<typename A>
		bool OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::Put(const A& key)
	{
		// Speculatively rehash if we're near the threshold
		if(count + 1 >= threshold) ExpandCapacityAndRehash();

		size_t index = GetHashIndex(key);
		while(values[index].HasValue())
		{
			if(Comparator::IsEqual((const T&) values[index], key)) return false;
			index = (index+1) & mask;
		}

		++count;
		values[index].Create(key);
		IteratePolicy<Storage>::OnCreate(values[index]);
		return true;
	}

//	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy> template<typename KeyType, typename... ExtraConstructorArguments>
//		T& OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::Insert(KeyType&& key, ExtraConstructorArguments&&... v)
//	{
//		if(++count >= threshold) ExpandCapacityAndRehash();
//		
//		size_t index = GetHashIndex(key);
//		while(values[index].HasValue())
//		{
//			JASSERT(!Comparator::IsEqual((const T&) values[index], key));
//			index = (index+1) & mask;
//		}
//		
//		values[index].Create((KeyType&&) key, (ExtraConstructorArguments&&) v...);
//		IteratePolicy<Storage>::OnCreate(values[index]);
//		return values[index];
//	}
	
	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy> template<typename A>
		void OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::Remove(const A& key)
	{
		size_t index = GetHashIndex(key);
		while(true)
		{
			JASSERT(values[index].HasValue());
			if(Comparator::IsEqual((const T&) values[index], key)) break;
			index = (index+1) & mask;
		}
		RemoveHashIndex(index);
	}
	
	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy>
		void OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::Remove(const Storage& entry)
	{
		if(values <= &entry && &entry < values+capacity) RemoveHashIndex(&entry - values);
		else Remove<T>(entry);
	}
	
	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy> template<typename A>
		bool OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::RemoveIfExists(const A& key)
	{
		if(!values) return false;

		size_t index = GetHashIndex(key);
		while(true)
		{
			if(values[index].IsEmpty()) return false;
			if(Comparator::IsEqual((const T&) values[index], key)) break;
			index = (index+1) & mask;
		}
		RemoveHashIndex(index);
		return true;
	}

	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy>
		void OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::RemoveHashIndex(size_t holeIndex)
	{
		JASSERT(holeIndex < capacity);
		
		size_t offset = 1;
		size_t scanToEnd = (holeIndex+1) & mask;
		while(values[scanToEnd].HasValue())
		{
			size_t hashIndex = GetHashIndex((const T&) values[scanToEnd]);
		
			if(((scanToEnd - hashIndex) & mask) >= offset)
			{
				values[holeIndex] = (Storage&&) values[scanToEnd];
				holeIndex = scanToEnd;
				offset = 0;
			}
			
			++offset;
			scanToEnd = (scanToEnd + 1) & mask;
		}
		--count;
		values[holeIndex].SetEmpty();
	}

	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy>
		JINLINE void OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::MoveEntryFromOldHash(Storage& entry)
	{
		T& value = (T&) entry;
		size_t index = GetHashIndex(value);
		while(values[index].HasValue())
		{
			// Should be no collisions when rehashing!
			JASSERT(!Comparator::IsEqual((const T&) values[index], value));
			index = (index + 1) & mask;
		}
		values[index].Create((T&&) value);
		IteratePolicy<Storage>::OnCreate(values[index]);
	}

	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy>
		void OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::ExpandCapacityAndRehash()
	{
		OpenHashSet newSet(capacity);
		newSet.count = count;
		
		Iterator it = Begin();
		const Iterator endIt = End();
		while(it != endIt)
		{
			Iterator next = it;
			++next;
			newSet.MoveEntryFromOldHash(it.GetStorage());
			it = next;
		}
		
		*this = (OpenHashSet&&) newSet;
	}

	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy> template<typename A>
		size_t OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::FindHashIndex(A&& value) const
	{
		if(!count) return capacity;
		
		size_t index = GetHashIndex(value);
		
		while(true)
		{
			if(values[index].IsEmpty()) return capacity;
			if(Comparator::IsEqual((const T&) values[index], value)) return index;
			index = (index+1) & mask;
		}
	}
	
	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy>
		size_t OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::FindFirstUsedIndex() const
	{
		for(size_t i = 0; i < capacity; ++i)
		{
			if(values[i].HasValue()) return i;
		}
		return capacity;
	}

	template<typename T, typename Storage, typename Comparator, typename Alloc, template<typename> class IteratePolicy>
		size_t OpenHashSet<T, Storage, Comparator, Alloc, IteratePolicy>::FindNextUsedIndex(size_t i) const
	{
		for(;i < capacity; ++i)
		{
			if(values[i].HasValue()) return i;
		}
		return capacity;
	}
	
//============================================================================
} // namespace Javelin
//===========================================================================e
