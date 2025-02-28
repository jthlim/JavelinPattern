//============================================================================
//
// Example:
//
// class IRenderable
// {
//     ...
//     IntrusiveListNode siblings;
//     ...
// };
//
//
// IntrusiveList<IRenderable, &IRenderable::siblings> renderList;
//
// for(auto it = renderList.Begin(); it != renderList.End(); ++it)
// {
//     ...
// }
//
//============================================================================

#pragma once
#include "Javelin/Container/List.h"
#include "Javelin/Type/TypeData.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class IntrusiveListNode : public ListNodeBase
	{
	public:
		IntrusiveListNode() { }
		IntrusiveListNode(const IntrusiveListNode&) = delete;
		IntrusiveListNode(IntrusiveListNode&& a) : ListNodeBase((ListNodeBase&&) a) { }
		IntrusiveListNode(const NoInitialize*) : ListNodeBase(NO_INITIALIZE) { }
		~IntrusiveListNode()										{ ListNodeBase::Remove(); }
		
		void Remove()												{ ListNodeBase::Remove(); next = this; previous = this; }
		
		bool IsIsolated() const										{ return next == this; }
		bool HasNext() const										{ return next != this; }
		bool HasPrevious() const									{ return previous != this; }
		
		template<typename T, IntrusiveListNode T::*MEMBER>
		JINLINE T* GetNext() const									{ return (T*) ((size_t) next - (((size_t) &(((T*) 1)->*MEMBER)) - 1)); }
		
		template<typename T, IntrusiveListNode T::*MEMBER>
		JINLINE T* GetPrevious() const								{ return (T*) ((size_t) previous - (((size_t) &(((T*) 1)->*MEMBER)) - 1)); }
		
		JINLINE IntrusiveListNode*	GetNext() const					{ return static_cast<IntrusiveListNode*>(next); }
		JINLINE IntrusiveListNode*	GetPrevious() const				{ return static_cast<IntrusiveListNode*>(previous); }		
	};

	template<typename T, IntrusiveListNode T::*MEMBER, typename ACCESS_TYPE=T > class IntrusiveListIterator
	{
	private:
		typedef IntrusiveListNode NodeType;
		
	public:
		JINLINE IntrusiveListIterator()									{ }
		JINLINE IntrusiveListIterator(NodeType* aNode)					{ node = aNode; }
		JINLINE IntrusiveListIterator(T* t)								{ node = &(t->*MEMBER); }
		
		JINLINE IntrusiveListIterator& operator++()						{ node = node->GetNext(); return *this; }
		JINLINE IntrusiveListIterator& operator++(int)					{ IntrusiveListIterator it(*this); node = node->GetNext(); return it; }
		JINLINE IntrusiveListIterator& operator--()						{ node = node->GetPrevious(); return *this; }
		JINLINE IntrusiveListIterator& operator--(int)					{ IntrusiveListIterator it(*this); node = node->GetPrevious(); return it; }
		
		JINLINE bool operator==(const T* a) const						{ return GetBaseAddress() == a; }
		JINLINE bool operator!=(const T* a) const						{ return GetBaseAddress() != a; }
		JINLINE bool operator==(const IntrusiveListNode* p) const		{ return node == p;				}
		JINLINE bool operator!=(const IntrusiveListNode* p) const		{ return node != p;				}
		JINLINE bool operator==(const IntrusiveListIterator& a) const	{ return node == a.node; 		}
		JINLINE bool operator!=(const IntrusiveListIterator& a) const	{ return node != a.node; 		}
		
		JINLINE ACCESS_TYPE& operator*() const							{ return *GetBaseAddress(); }
		JINLINE ACCESS_TYPE* operator->() const							{ return GetBaseAddress(); }

		JINLINE void operator=(T* t)									{ node = &(t->*MEMBER); }

		// Used for SafeIntrusiveList
		JINLINE const void* GetListNodePointer() const					{ return node;	}
		
	private:
		JINLINE ACCESS_TYPE* GetBaseAddress() const 					{ return (ACCESS_TYPE*) ((size_t) node - (((size_t) &(((ACCESS_TYPE*) 1)->*MEMBER)) - 1)); }
		
		NodeType*	node;
	};

	template<typename T, IntrusiveListNode T::*MEMBER> class IntrusiveList : public ListBase
	{
	private:
		typedef ListBase Inherited;
		
	public:
		typedef IntrusiveListIterator<T, MEMBER, T> Iterator;
		typedef IntrusiveListIterator<T, MEMBER, const T> ConstIterator;
		
		JINLINE IntrusiveList() 					{ }
		IntrusiveList(const IntrusiveList&) = delete;
		JINLINE IntrusiveList(IntrusiveList&& a) : ListBase((ListBase&&) a) { }
		JINLINE ~IntrusiveList()					{ UnlinkAll(); }
		
		JINLINE void Reset()						{ UnlinkAll(); sentinel.next = &sentinel; sentinel.previous = &sentinel; }
		
		JINLINE T& Front()							{ JASSERT(HasData()); return *Iterator(((IntrusiveListNode&) sentinel).GetNext()); }
		JINLINE T& Back()							{ JASSERT(HasData()); return *Iterator(((IntrusiveListNode&) sentinel).GetPrevious()); }
		
		JINLINE void Append(T* object)				{ sentinel.InsertBefore(&(object->*MEMBER)); }
		JINLINE void PushBack(T* object)			{ sentinel.InsertBefore(&(object->*MEMBER)); }
		JINLINE void PushFront(T* object)			{ sentinel.InsertAfter(&(object->*MEMBER)); }
		JINLINE static void Remove(T* object)		{ (object->*MEMBER).Remove(); }

		JINLINE void MoveToFront(T* object)			{ Remove(object); PushFront(object); }
		JINLINE void MoveToBack(T* object)			{ Remove(object); PushBack(object);  }
		
		using Inherited::GetCount;
		using Inherited::HasData;
		using Inherited::IsEmpty;
		
		T&		operator[](size_t index)					{ return *Iterator((IntrusiveListNode*) GetIndex(index)); 		}
		JINLINE const T& operator[](size_t index) const		{ return *ConstIterator((IntrusiveListNode*) GetIndex(index)); 	}
		size_t FindIndexForNode(const T* node) const;
		
		JINLINE Iterator Begin()					{ return ((IntrusiveListNode&) sentinel).GetNext(); }
		JINLINE Iterator End()						{ return &(IntrusiveListNode&) sentinel; }
		JINLINE Iterator ReverseBegin()				{ return ((IntrusiveListNode&) sentinel).GetPrevious(); }
		JINLINE Iterator ReverseEnd()				{ return &(IntrusiveListNode&) sentinel; }

		JINLINE ConstIterator Begin() const			{ return ((IntrusiveListNode&) sentinel).GetNext(); }
		JINLINE ConstIterator End()	  const			{ return &(IntrusiveListNode&) sentinel; }
		JINLINE ConstIterator ReverseBegin() const	{ return ((IntrusiveListNode&) sentinel).GetPrevious(); }
		JINLINE ConstIterator ReverseEnd()	 const	{ return &(IntrusiveListNode&) sentinel; }

		template<typename F>
		bool HasElement(const F& f) const			{ for(const auto& it : *this) { if(f(it)) return true; } return false; }

		template<typename F>
		void Perform(const F& f) 					{ for(auto& it : *this) f(it); }
	};
	
//============================================================================
//============================================================================

	template<typename T, IntrusiveListNode T::*MEMBER>
		size_t IntrusiveList<T, MEMBER>::FindIndexForNode(const T* node) const
	{
		size_t index = 0;
		ConstIterator it = Begin();
		const IntrusiveListNode* p = &(node->*MEMBER);
		while(it != End())
		{
			if(it == p) return index;
			++index;
			++it;
		}
		return TypeData<size_t>::Maximum();
	}
	
//============================================================================
}
//============================================================================
