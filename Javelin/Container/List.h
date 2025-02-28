//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Allocators/StandardAllocator.h"
#include "Javelin/System/Assert.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class ListNodeBase
	{
	public:
		ListNodeBase*	previous;
		ListNodeBase*	next;

		JINLINE ListNodeBase()						{ SetIsolated(); }
		ListNodeBase(const ListNodeBase&) = delete;
		JINLINE ListNodeBase(ListNodeBase&& a)		{ next = (a.next == &a) ? this : a.next; previous = (a.previous == &a) ? this : a.previous; previous->next = this; next->previous = this; a.SetIsolated(); }

		void AdoptLinks(ListNodeBase& fromNode) 	{
														Remove();
														new(Placement(this)) ListNodeBase((ListNodeBase&&) fromNode);
													}
		
		void InsertAfter(ListNodeBase* newNode)		{
														newNode->next = next;
														newNode->previous = this;
														next->previous = newNode;
														next = newNode;
													}

		void ReplaceWith(ListNodeBase* newNode)		{
														newNode->next = next;
														newNode->previous = previous;
														previous->next = newNode;
														next->previous = newNode;
														SetIsolated();
													}
		
		void InsertBefore(ListNodeBase* newNode)	{ 
														newNode->next = this;
														newNode->previous = previous;
														previous->next = newNode;
														previous = newNode;
													}

		void Remove()								{ 
														previous->next = next;
														next->previous = previous;
													}
		
	protected:
		JINLINE ListNodeBase(const NoInitialize*)	{ }

	private:
		JINLINE void SetIsolated()					{ next = this; previous = this; }
			
		JEXPORT size_t GetCount(const ListNodeBase* sentinel) const;
		friend class ListBase;
	};

	template<typename T> class TypedListNode : public ListNodeBase
	{
	public:
		typedef T DataType;
		
		JINLINE TypedListNode() : ListNodeBase(NO_INITIALIZE)	{ }
		template<typename A>
		JINLINE TypedListNode(A&& a) : ListNodeBase(NO_INITIALIZE), data((A&&) a)	{ }

		JINLINE TypedListNode*	GetNext() const					{ return static_cast<TypedListNode*>(next); }
		JINLINE TypedListNode*	GetPrevious() const				{ return static_cast<TypedListNode*>(previous); }		

		T	data;
	};

	template<typename T, typename Allocator=StandardAllocator> class ListNode : public TypedListNode<T>, public Allocator
	{
	public:
		JINLINE ListNode()							{ }
		JINLINE ListNode*	GetNext()		const	{ return static_cast<ListNode*>(this->next);		}
		JINLINE ListNode*	GetPrevious()	const	{ return static_cast<ListNode*>(this->previous);	}

		template<typename A>
		JINLINE ListNode(A&& a) : TypedListNode<T>((A&&) a)	{ }
	};

//============================================================================

	template<typename NodeType> class ListIterator
	{
	public:
		JINLINE ListIterator()									{ }
		JINLINE ListIterator(NodeType* aNode)					{ node = aNode; }
		
		JINLINE ListIterator& operator++()						{ node = node->GetNext(); return *this; }
		JINLINE ListIterator& operator++(int)					{ ListIterator it(*this); node = node->GetNext(); return it; }
		JINLINE ListIterator& operator--()						{ node = node->GetPrevious(); return *this; }
		JINLINE ListIterator& operator--(int)					{ ListIterator it(*this); node = node->GetPevious(); return it; }
		
		JINLINE bool operator==(const ListIterator& a) const	{ return node == a.node; }
		JINLINE bool operator!=(const ListIterator& a) const	{ return node != a.node; }
		
		JINLINE typename NodeType::DataType& operator*() const	{ return node->data; }
		JINLINE typename NodeType::DataType* operator->() const	{ return &node->data; }
		
	private:
		NodeType*	node;
	};

//============================================================================

	class ListBase
	{
	public:
		ListBase() { }
		ListBase(ListBase&& a) : sentinel((ListNodeBase&&) a.sentinel) { }
		
		JINLINE size_t JCALL GetCount() const		{ return sentinel.next->GetCount(&sentinel); }
		JINLINE bool JCALL IsEmpty()	const		{ return sentinel.next == &sentinel; }
		JINLINE bool JCALL HasData()	const		{ return sentinel.next != &sentinel; }

	protected:
		JEXPORT ListNodeBase* JCALL GetIndex(size_t index) const;
		
		// Used by IntrusiveList
		JEXPORT void JCALL UnlinkAll();
			
		ListNodeBase	sentinel;
	};

	template<typename T, typename Allocator=StandardAllocator> class List : public ListBase
	{
	private:
		typedef ListBase Inherited;
		typedef ListNode<T, Allocator>	NodeType;

	public:
		typedef ListIterator< TypedListNode<T> > Iterator;
		typedef ListIterator< TypedListNode<const T> > ConstIterator;
		
		List()										{ }
		List(const List& l);
		void operator=(const List& l);
		~List()										{ DestroyAll(); }

		JINLINE void Reset()						{ DestroyAll(); sentinel.next = &sentinel; sentinel.previous = &sentinel; }
		
		JINLINE T& Append()							{ NodeType* p = new NodeType; sentinel.InsertBefore(p); return p->data; }
		JINLINE T& PushBack()						{ NodeType* p = new NodeType; sentinel.InsertBefore(p); return p->data; }
		JINLINE T& PushFront()						{ NodeType* p = new NodeType; sentinel.InsertAfter(p); return p->data; }

		template<typename A>
		JINLINE void Append(A &&a)					{ sentinel.InsertBefore(new NodeType((A&&) a)); }
		template<typename A>
		JINLINE void PushBack(A &&a)				{ sentinel.InsertBefore(new NodeType((A&&) a)); }
		template<typename A>
		JINLINE void PushFront(A &&a)				{ sentinel.InsertAfter(new NodeType((A&&) a)); }

		using Inherited::GetCount;
		using Inherited::HasData;
		using Inherited::IsEmpty;
		
		JINLINE T& operator[](size_t i)				{ return static_cast<TypedListNode<T>*>(GetIndex(i))->data; }
		JINLINE const T& operator[](size_t i) const	{ return static_cast<TypedListNode<T>*>(GetIndex(i))->data; }

		JINLINE void Pop()							{ 
														JASSERT(HasData()); 
														NodeType* p = (NodeType*) sentinel.previous;
														p->Remove();
														delete p;
													}
		JINLINE void PopFront()						{
														JASSERT(HasData());
														NodeType* p = (NodeType*) sentinel.next;
														p->Remove();
														delete p;
													}
		
		JINLINE T& Front() const					{ JASSERT(HasData()); return ((NodeType*) sentinel.next)->data; }
		JINLINE T& Back() const						{ JASSERT(HasData()); return ((NodeType*) sentinel.previous)->data; }
		
		template<typename A>
		JINLINE T& Find(const A& a)					{
														NodeType* p = static_cast<NodeType*>(sentinel.next);
														while(true)
														{
															JASSERT(p != &sentinel);
															if(p->data == a) return p->data;
															p = p->GetNext();
														}
													}
		template<typename A>
		JINLINE const T& Find(const A& a) const		{ return const_cast<List*>(this)->Find(a); }

		JINLINE Iterator Begin()					{ return (TypedListNode<T>*) sentinel.next; }
		JINLINE Iterator End()						{ return (TypedListNode<T>*) &sentinel; }
		JINLINE Iterator ReverseBegin()				{ return (TypedListNode<T>*) sentinel.previous; }
		JINLINE Iterator ReverseEnd()				{ return (TypedListNode<T>*) &sentinel; }

		JINLINE ConstIterator Begin()	const		{ return (TypedListNode<const T>*) sentinel.next; }
		JINLINE ConstIterator End()		const		{ return (TypedListNode<const T>*) &sentinel; }
		JINLINE ConstIterator ReverseBegin() const	{ return (TypedListNode<const T>*) sentinel.previous; }
		JINLINE ConstIterator ReverseEnd()	 const	{ return (TypedListNode<const T>*) &sentinel; }

	private:
		void JCALL DestroyAll();
	};

//============================================================================
	
	template<typename T, typename Allocator> void List<T,Allocator>::DestroyAll()
	{
		NodeType* p = (NodeType*) sentinel.next;
		while(p != &sentinel)
		{
			NodeType* pNext = (NodeType*) p->GetNext();
			delete p;
			p = pNext;
		}
	}

//============================================================================
} // namespace Javelin
//===========================================================================e
