//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Container/Tree.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	template<typename T, typename TreeType, typename Comparator=DefaultTreeComparator<T>, typename Alloc=StandardAllocator> class TreeSet : private Tree<T, TreeType, Comparator, Alloc>
	{
	private:
		typedef Tree<T, TreeType, Comparator, Alloc> Inherited;
		typedef typename Inherited::Node NodeType;
		
	public:
		typedef typename Inherited::Iterator Iterator;
		typedef typename Inherited::ConstIterator ConstIterator;
		
		TreeSet() { }
		TreeSet(const TreeSet& a) : Inherited(a) { }
		TreeSet(TreeSet&& a) JNOTHROW : Inherited((Inherited&&) a) { }
		
		TreeSet& operator=(const TreeSet& t) 	{ Inherited::operator=(t); return *this; }
		TreeSet& operator=(TreeSet&& t) 		{ Inherited::operator=((Inherited&&) t); return *this; }
		
		using Inherited::Begin;
		using Inherited::End;
		using Inherited::Find;
		
		using Inherited::IsEmpty;
		using Inherited::HasData;
		using Inherited::Reset;
		using Inherited::GetCount;
		using Inherited::Contains;
		using Inherited::Remove;
		using Inherited::RemoveIfExists;

		using Inherited::DumpNodes;
		using Inherited::FindWithLowerBound;
		using Inherited::FindWithUpperBound;
		
		// This is a special case to help Map generate better code
		template<typename KeyType, typename ValueType>
		void Insert(const KeyType &k, const ValueType& v)	{ JASSERT(Contains(k) == false); this->InsertNode(new NodeType(k, v)); }
		
		template<typename KeyType>
		T& Insert(const KeyType &k)							{ JASSERT(Contains(k) == false); NodeType* node = new NodeType(k); this->InsertNode(node); return node->data; }
		
		template<typename A>
		T& Get(const A& value)								{ return Inherited::FindNodeOrCreate(value)->data;	}

		template<typename A>
		T& Get(const A& value) const						{ return Inherited::FindNode(value)->data;			}

		// Lookup is always const
		template<typename A>
		T& Lookup(const A& value) const						{ return Get(value); }
		
		template<typename A>
		bool Put(const A& value);
	};
	
//============================================================================
} // namespace Javelin
//===========================================================================e
