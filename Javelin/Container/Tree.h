//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Allocators/StandardAllocator.h"
#include "Javelin/Container/Tree/TreeType_AVL.h"
#include "Javelin/Container/Tree/TreeType_Binary.h"
#include "Javelin/Container/Tree/TreeType_RedBlack.h"
#include "Javelin/Container/Tree/TreeType_SimpleBinary.h"
#include "Javelin/Template/Comparator.h"
#include "Javelin/Template/Range.h"

//============================================================================

namespace Javelin
{
//============================================================================

	typedef TreeType_RedBlack TREE_TYPE_DEFAULT;

//============================================================================
	
	template<typename T> class DefaultTreeComparator : public Less { };
	
//============================================================================

	template<typename T, typename NodeBase, typename Comparator>
	class TreeTypeNode : public NodeBase
	{
	private:
		typedef NodeBase Inherited;
		
	public:
		template<typename A> TreeTypeNode(const A& a) : data(a) { }
		template<typename A, typename B> TreeTypeNode(const A& a, const B& b) : data(a, b) { }
		
		T	data;		

		template<typename A> TreeTypeNode* Find(const A& a) const;
		template<typename A> const TreeTypeNode* Lookup(const A& a) const;
		
		template<typename A> TreeTypeNode* FindWithLowerBound(const A &a);
        template<typename A> TreeTypeNode* FindWithUpperBound(const A &a);

		void RecurseDump(size_t depth) const;
		T& Insert(NodeBase* &root);
	};
	
	template<typename T, typename NodeBase, typename Comparator, typename Allocator>
	class TreeNode : public TreeTypeNode<T, NodeBase, Comparator>, public Allocator
	{
	private:
		typedef TreeTypeNode<T, NodeBase, Comparator> Inherited;
		
	public:	
		template<typename A> TreeNode(const A& a) : Inherited(a) { }
		template<typename A, typename B> TreeNode(const A& a, const B& b) : Inherited(a, b) { }
		
		static void RecurseDelete(TreeNode* p);
		void Remove(NodeBase* &root)		{ Inherited::RemoveFromTree(root); delete this; }
		
		template<typename A> static TreeNode* FindNodeOrCreate(NodeBase* &root, const A& a);
	};
	
//============================================================================
	
	template<typename NodeBase>
	class TreeIteratorBase
	{
	public:
		JINLINE TreeIteratorBase()				{ p = nullptr;	}
		JINLINE TreeIteratorBase(NodeBase* aP)	{ p = aP;		}
		
		JINLINE bool operator==(const TreeIteratorBase& a) const { return p == a.p; }
		JINLINE bool operator!=(const TreeIteratorBase& a) const { return p != a.p; }
		
		NodeBase*	p;
	};
	
	template<typename T, typename NodeType, typename NodeBase>
	class TreeIterator : public TreeIteratorBase<NodeBase>
	{
	private:
		typedef TreeIteratorBase<NodeBase> Inherited;
		
	public:
		JINLINE TreeIterator() { }
		JINLINE TreeIterator(NodeType* p) : Inherited(p) { }
		
		// Pre increment/decrement
		JINLINE TreeIterator& operator++()		{ this->p = (NodeType*) this->p->GetIteratorNext(); return *this; }
		JINLINE TreeIterator& operator--()		{ this->p = (NodeType*) this->p->GetIteratorPrevious(); return *this; }
		
		// Post increment/decrement
		JINLINE TreeIterator operator++(int)	{ TreeIterator a(*this); this->p = (NodeType*) this->p->GetIteratorNext(); return a; }
		JINLINE TreeIterator operator--(int)	{ TreeIterator a(*this); this->p = (NodeType*) this->p->GetIteratorPrevious(); return a; }
		
		JINLINE T& operator*() const			{ return static_cast<NodeType*>(this->p)->data; }
		JINLINE T* operator->() const			{ return &static_cast<NodeType*>(this->p)->data; }
	};
	
//============================================================================

	template<typename T, 
			 typename TreeType = TREE_TYPE_DEFAULT, 
			 typename Comparator = DefaultTreeComparator<T>,
			 typename Allocator = StandardAllocator>
	class Tree : private TreeType
	{
	protected:
		typedef TreeNode<T, typename TreeType::NodeBase, Comparator, Allocator> Node;

	public:
		Tree()															{ root = (Node*) TreeType::GetInitialRoot(); }
		Tree(const Tree& a)												{ root = RecurseCopyNode(a.root); }
		Tree(Tree&& a) JNOTHROW 										{ root = a.root; a.root = (Node*) TreeType::GetInitialRoot(); }
		~Tree()															{ Node::RecurseDelete(root); }

		Tree& operator=(const Tree& a)									{ Node::RecurseDelete(root); root = RecurseCopyNode(a.root); return *this; }
		Tree& operator=(Tree&& a) JNOTHROW								{ Node::RecurseDelete(root); root = a.root; a.root = (Node*) TreeType::GetInitialRoot(); return *this; }
		
		typedef TreeIterator<T, Node, typename TreeType::NodeBase> Iterator;
		typedef TreeIterator<const T, const Node, const typename TreeType::NodeBase> ConstIterator;
		
		JINLINE void Reset()											{ RecurseDelete(root); root = (Node*) TreeType::GetInitialRoot(); }
		
		void DumpNodes() const											{ root->RecurseDump(0); }
		
		JINLINE Iterator Begin()										{ return Iterator((Node*) root->GetLeftMostNode()); }
		JINLINE Iterator End()											{ return Iterator(nullptr); }
		JINLINE ConstIterator Begin() const								{ return ConstIterator((Node*) root->GetLeftMostNode()); }
		JINLINE ConstIterator End() const								{ return ConstIterator(nullptr); }
		
		JINLINE bool IsEmpty() const									{ return IsEmpty(root); }
		JINLINE bool HasData() const									{ return !IsEmpty(root); }
		
		template<typename A>
		JINLINE Iterator Find(const A& a)								{ return Iterator(FindNode(a)); }

		template<typename A>
		JINLINE ConstIterator Find(const A& a) const					{ return ConstIterator(FindNode(a)); }
		
		
		// Returns an iterator to the first item greater than or equal to a
        template<typename A>
		Iterator FindWithLowerBound(const A &a)							{ return Iterator((Node*) root->FindWithLowerBound(a)); }
		
		// Returns an iterator to the first item less than or equal to a
        template<typename A>
		Iterator FindWithUpperBound(const A &a)							{ return Iterator((Node*) root->FindWithUpperBound(a)); }

		JINLINE size_t	GetCount() const								{ return Node::CountAllChildren(root); }
		
		template<typename A> JINLINE void Insert(const A& a)			{ InsertNode(new Node(a)); }
		template<typename A> JINLINE void Remove(const A& a)			{ ((Node*) root->Lookup(a))->Remove(root); }
							 JINLINE void Remove(const Iterator& it)	{ ((Node*) it.p)->Remove(root); }
		template<typename A> JINLINE bool RemoveIfExists(const A& a)	{ Node* p = ((Node*) root->Find(a)); if(p) p->Remove(root); return p != nullptr; }
		
		template<typename A> JINLINE bool Contains(const A& a) const	{ return FindNode(a) != nullptr; }
		
	protected:
        template<typename A>
		JINLINE Node* FindNode(const A &a) const						{ return static_cast<Node*>(root->Find(a)); }

        template<typename A>
		JINLINE Node* FindNodeOrCreate(const A &a)						{ return static_cast<Node*>(Node::FindNodeOrCreate((typename TreeType::NodeBase*&) root, a)); }
		
		JINLINE void InsertNode(Node* newNode)							{ newNode->Insert((typename TreeType::NodeBase*&) root); }
		
	private:
		Node*	root;
		
		static Node* RecurseCopyNode(Node* from)
		{
			if(Node::IsEmpty(from)) return (Node*) TreeType::GetInitialRoot();
			
			Node* newNode = new Node(*from);
			for(size_t i = 0; i < from->GetNumberOfChildren(); ++i)
			{
				Node* child = RecurseCopyNode( (Node*) from->GetChild(i));
				child->SetParent(newNode);
				newNode->SetChild(i, child);
			}
			return newNode;					  
		}
	};

//============================================================================
	
//	template<typename T, typename NodeBase, typename Comparator>
//	void TreeTypeNode<T, NodeBase, Comparator>::RecurseDump(size_t depth) const
//	{
//		if(this->IsEmpty()) return;
//		
//		for(size_t i = 0; i < depth; ++i) printf(" ");
//		printf("%p\n", this);
//		
//		for(size_t i = 0; i < this->GetNumberOfChildren(); ++i)
//		{
//			static_cast<const TreeTypeNode*>(this->GetChild(i))->RecurseDump(depth+1);
//		}
//	}	

	template<typename T, typename NodeBase, typename Comparator> template<typename A>
	TreeTypeNode<T, NodeBase, Comparator>* TreeTypeNode<T, NodeBase, Comparator>::FindWithLowerBound(const A &a)
	{
		TreeTypeNode* process = this;
		TreeTypeNode* found = nullptr;
		
		while(IsUsed(process))
		{
			if(Comparator::IsEqual(process->data, a)) return process;
			else
			{
				if(Comparator::IsOrdered(process->data, a)) process = (TreeTypeNode*) process->right;
				else
				{
					found = process;
					process = (TreeTypeNode*) process->left;
				}
			}
		}
		
		return found;
	}

	template<typename T, typename NodeBase, typename Comparator> template<typename A>
	TreeTypeNode<T, NodeBase, Comparator>* TreeTypeNode<T, NodeBase, Comparator>::FindWithUpperBound(const A &a)
	{
		TreeTypeNode* process = this;
		TreeTypeNode* found = nullptr;
		
		while(IsUsed(process))
		{
			if(Comparator::IsEqual(process->data, a)) return process;
			else
			{
				if(Comparator::IsOrdered(process->data, a))
				{
					found = process;
					process = (TreeTypeNode*) process->right;
				}
				else
				{
					process = (TreeTypeNode*) process->left;
				}
			}
		}
		
		return found;
	}

	template<typename T, typename NodeBase, typename Comparator>
	T& TreeTypeNode<T, NodeBase, Comparator>::Insert(NodeBase* &root)
	{
		JASSERT(this->GetNumberOfChildren() == 2);
		
		NodeBase*  parent = nullptr;
		NodeBase** current = &root;
		
		while(IsUsed(*current))
		{
			parent = *current;
			if(Comparator::IsOrdered(data, ((TreeTypeNode*) parent)->data)) current = (NodeBase**) &parent->left;
			else															current = (NodeBase**) &parent->right;
		}
		
		this->SetParent(parent);
		*current = this;
		
		this->FixupInsert(root);
		
		return data;
	}
	
	
	template<typename T, typename NodeBase, typename Comparator> template<typename A>
	TreeTypeNode<T, NodeBase, Comparator>* TreeTypeNode<T, NodeBase, Comparator>::Find(const A &a) const
	{
		// Find only works when there are 2 children per node
		JASSERT(this->GetNumberOfChildren() == 2);
		
		TreeTypeNode* p = const_cast<TreeTypeNode*>(this);
		while(IsUsed(p))
		{
			if(Comparator::IsEqual(p->data, a)) return p;
			else
			{
				if(Comparator::IsOrdered(p->data, a)) p = (TreeTypeNode*) p->right;
				else                                  p = (TreeTypeNode*) p->left;
			}
		}
		
		return nullptr;
	}

	template<typename T, typename NodeBase, typename Comparator> template<typename A>
	const TreeTypeNode<T, NodeBase, Comparator>* TreeTypeNode<T, NodeBase, Comparator>::Lookup(const A &a) const
	{
		// Find only works when there are 2 children per node
		JASSERT(this->GetNumberOfChildren() == 2);
		
		const TreeTypeNode* p = this;
		while(true)
		{
			JASSERT(IsUsed(p));
			
			if(Comparator::IsEqual(p->data, a)) return p;
			else
			{
				if(Comparator::IsOrdered(p->data, a)) p = (TreeTypeNode*) p->right;
				else                                  p = (TreeTypeNode*) p->left;
			}
		}
	}
	
	template<typename T, typename NodeBase, typename Comparator, typename Allocator> 
	void TreeNode<T, NodeBase, Comparator, Allocator>::RecurseDelete(TreeNode* p)
	{
		if(IsEmpty(p)) return;
		
		for(size_t i : Range(p->GetNumberOfChildren()))
		{
			RecurseDelete(static_cast<TreeNode*>(p->GetChild(i)));
		}
		
		delete p;
	}
	
	template<typename T, typename NodeBase, typename Comparator, typename Allocator> template<typename A> 
	TreeNode<T, NodeBase, Comparator, Allocator>* TreeNode<T, NodeBase, Comparator, Allocator>::FindNodeOrCreate(NodeBase* &root, const A& a)
	{
		TreeNode*  parent = nullptr;
		TreeNode** current = (TreeNode**) &root;
		
		while(IsUsed(*current))
		{
			if(Comparator::IsEqual((*current)->data, a)) return *current;

			parent = *current;
			if(Comparator::IsOrdered(((TreeNode*) parent)->data, a)) current = (TreeNode**) &parent->right;
			else													 current = (TreeNode**) &parent->left;
		}
		
		TreeNode* newNode = new TreeNode(a);
		newNode->SetParent(parent);
		*current = newNode;
		
		newNode->FixupInsert(root);
		
		return newNode;
	}
	
	
//============================================================================
} // namespace Javelin
//===========================================================================e
