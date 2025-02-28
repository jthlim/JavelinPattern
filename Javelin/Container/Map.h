//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Container/OpenHashSet.h"
#include "Javelin/Container/TreeSet.h"
#include "Javelin/Type/TypeData.h"

//============================================================================

namespace Javelin
{
//============================================================================

	template<typename K, typename V> struct MapData
	{
		const K key;
		V		value;
		
		template<typename KeyType>
		MapData(const KeyType& k) : key(k), value(V()) { }
		
		template<typename KeyType, typename ValueType>
		MapData(KeyType&& k, ValueType&& v) : key((KeyType&&) k), value((ValueType&&) v) { }
		
		MapData(MapData& a) : key(a.key), value(a.value) { }
		MapData(const MapData& a) : key(a.key), value(a.value) { }
		MapData(MapData&& a) : key((K&&) a.key), value((V&&) a.value) { }

		MapData& operator=(MapData&& a) { this->~MapData(); new(Placement(this)) MapData((MapData&&) a); return *this; }
		
		template<typename A> bool operator==(const A& k) const { return key == k; }
		template<typename A> bool operator!=(const A& k) const { return key != k; }
		template<typename A> bool operator< (const A& k) const { return key <  k; }
		template<typename A> bool operator<=(const A& k) const { return key <= k; }
		template<typename A> bool operator> (const A& k) const { return key >  k; }
		template<typename A> bool operator>=(const A& k) const { return key >= k; }

		bool operator==(const MapData& a) const { return key == a.key; }
		bool operator!=(const MapData& a) const { return key != a.key; }
		bool operator< (const MapData& a) const { return key <  a.key; }
		bool operator<=(const MapData& a) const { return key <= a.key; }
		bool operator> (const MapData& a) const { return key >  a.key; }
		bool operator>=(const MapData& a) const { return key >= a.key; }
		
		friend size_t GetHash(const MapData& m) { return GetHash(m.key); }
	};
	
	template<typename K, typename V> struct TypeData< MapData<K, V> >
	{
		enum { IS_INTEGRAL			= false };
		enum { IS_FLOAT				= false };
		enum { IS_POD				= TypeData<K>::IS_POD && TypeData<V>::IS_POD };
		enum { IS_BITWISE_COPY_SAFE	= TypeData<K>::IS_BITWISE_COPY_SAFE && TypeData<V>::IS_BITWISE_COPY_SAFE };
		enum { IS_POINTER			= false };

		typedef MapData<K, V>			NonPointerType;
		typedef const MapData<K, V>&	ParameterType;
		typedef MapData<K, V>&& 		MoveType;
	};
	
//============================================================================

	template<typename T> class DefaultMapComparator : public DefaultTreeComparator<T> { };

	template<typename V> struct OptionalStorage_Automatic::Policy<MapData<String, V>> : public OptionalStorage_NullPointer::Policy<MapData<String, V>> { };
	template<typename K, typename V> struct OptionalStorage_Automatic::Policy<MapData<K*, V>> : public OptionalStorage_NullPointer::Policy<MapData<K*, V>> { };

//============================================================================

	template<typename D, typename Comparator, typename Allocator> class DefaultMapStorage : protected OpenHashSet<D, Optional<D>, Comparator, Allocator>
	{
	private:
		typedef OpenHashSet<D, Optional<D>, Comparator, Allocator> Inherited;
		
	public:
		DefaultMapStorage() { }
		DefaultMapStorage(size_t initialCapacity) : Inherited(initialCapacity) { }
		
		using Inherited::operator new;
		using Inherited::operator delete;
	};

	template<typename D, typename Comparator, typename Allocator> class MapStorage_OpenHashSet : protected OpenHashSet<D, Optional<D>, Comparator, Allocator>
	{
	private:
		typedef OpenHashSet<D, Optional<D>, Comparator, Allocator> Inherited;
		
	public:
		MapStorage_OpenHashSet() { }
		MapStorage_OpenHashSet(size_t initialCapacity) : Inherited(initialCapacity) { }
		
		using Inherited::operator new;
		using Inherited::operator delete;
	};
	
	template<typename OptionalStorage> class MapStorage_OpenHashSetWithOptionalStorage
	{
	public:
		template<typename D, typename Comparator, typename Allocator> class Type : protected OpenHashSet<D, Optional<D, OptionalStorage>, Comparator, Allocator>
		{
		private:
			typedef OpenHashSet<D, Optional<D, OptionalStorage>, Comparator, Allocator> Inherited;
			
		public:
			Type() { }
			Type(size_t initialCapacity) : Inherited(initialCapacity) { }
			
			using Inherited::operator new;
			using Inherited::operator delete;
		};
	};
	
	// Warning!!
	// This is dangerous, but has storage benefits
	// 1. If you assign null to the value, it will cause the element to be considered empty. This will lead to memory leaks if the key requires allocations, and can also corrupt the hash set's data
	// 2. If you access an empty key, and this causes the creation of an entry, this can also lead to memory leaks
	//
	// As a precaution against this, JBUILDCONFIG_DEBUG builds will verify the element count in the destructor.
	template<typename D, typename Comparator, typename Allocator> class MapStorage_OpenHashSetEmptyValueNullPointer : protected OpenHashSet<D, Optional<D, OptionalStorage_NullPointerMember<JCLASS_MEMBER((&D::value))>>, Comparator, Allocator>
	{
	private:
		typedef OpenHashSet<D, Optional<D, OptionalStorage_NullPointerMember<JCLASS_MEMBER((&D::value))>>, Comparator, Allocator> Inherited;
		
	public:
		MapStorage_OpenHashSetEmptyValueNullPointer() { }
		MapStorage_OpenHashSetEmptyValueNullPointer(size_t initialCapacity) : Inherited(initialCapacity) { }
		~MapStorage_OpenHashSetEmptyValueNullPointer() { Inherited::VerifyCount(); }
		
		using Inherited::operator new;
		using Inherited::operator delete;
	};

	template<typename D, typename Comparator, typename Allocator> class MapStorage_RBTree : protected TreeSet<D, TreeType_RedBlack, Comparator, Allocator>
	{
	private:
		typedef TreeSet<D, TreeType_RedBlack, Comparator, Allocator> Inherited;

	public:
		using Inherited::DumpNodes;
		using Inherited::FindWithLowerBound;
		using Inherited::FindWithUpperBound;
	};
	
	template<typename D, typename Comparator, typename Allocator> class MapStorage_AVLTree : protected TreeSet<D, TreeType_AVL, Comparator, Allocator>
	{
	private:
		typedef TreeSet<D, TreeType_AVL, Comparator, Allocator> Inherited;
		
	public:
		using Inherited::DumpNodes;
		using Inherited::FindWithLowerBound;
		using Inherited::FindWithUpperBound;
	};
	
//============================================================================
	
	template<typename K,
			 typename V,
			 template<typename, typename, typename> class StorageType = DefaultMapStorage,
			 typename Comparator = DefaultMapComparator< MapData<K, V> >,
			 typename Alloc = StandardAllocator
			>
	class Map : public StorageType< MapData<K, V>, Comparator, Alloc >
	{
	private:
		typedef StorageType< MapData<K, V>, Comparator, Alloc > Inherited;
		
	public:
		typedef Alloc Allocator;
		typedef MapData<K, V> Data;
		typedef typename Inherited::Iterator Iterator;
		typedef typename Inherited::ConstIterator ConstIterator;

		Map() { }
		Map(size_t initialCapacity) : Inherited(initialCapacity) { }
		Map(const Map& a) : Inherited(a) { }
		Map(Map&& a) JNOTHROW : Inherited((Inherited&&) a) { }
	
		Map& operator=(const Map& a)						{ Inherited::operator=(a); return *this; }
		Map& operator=(Map&& a) JNOTHROW					{ Inherited::operator=((Inherited&&) a); return *this; }
	
		using Inherited::Insert;
		template<typename KeyType>
		V& Insert(KeyType&& k)								{ return Inherited::Insert((KeyType&&) k).value; }

		using Inherited::Put;
	
		template<typename A>
		V& operator[](const A& k)							{ return Inherited::Get(k).value; }

		template<typename A>
		const V& operator[](const A& k) const				{ return Inherited::Get(k).value; }

		template<typename A>
		const V& Lookup(const A& k) const					{ return Inherited::Get(k).value; }
	
		using Inherited::Begin;
		using Inherited::End;

		using Inherited::Contains;
		using Inherited::Find;
		
// Available for Maps using a Tree container
//		using Inherited::DumpNodes;
//		using Inherited::FindWithLowerBound;
//		using Inherited::FindWithUpperBound;
		
		using Inherited::IsEmpty;
		using Inherited::HasData;
		using Inherited::Reset;
		
		using Inherited::GetCount;
		using Inherited::Remove;
		using Inherited::RemoveIfExists;
	};

//============================================================================
} // namespace Javelin
//===========================================================================e
