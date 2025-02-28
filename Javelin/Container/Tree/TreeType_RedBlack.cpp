//============================================================================

#include "Javelin/Container/Tree.h"
#include "Javelin/Template/Swap.h"

//============================================================================

using namespace Javelin;

//============================================================================

TreeType_RedBlack::NodeBase TreeType_RedBlack::NodeBase::sentinel =
{
	&sentinel,
	&sentinel,
	nullptr,
	false
};

//============================================================================

size_t TreeType_RedBlack::NodeBase::CountAllChildren(const NodeBase* p) 
{
	if(p->IsEmpty()) return 0;
	return 1 + CountAllChildren(p->left) + CountAllChildren(p->right);
}

void TreeType_RedBlack::NodeBase::ChangeParentToNewChild(TreeType_RedBlack::NodeBase* &root, TreeType_RedBlack::NodeBase* newChild) const
{
	newChild->parent = parent;
	if(parent)
	{
		if(parent->right == this)	parent->right = newChild;
		else 						parent->left = newChild;
	}
	else
	{
		root = newChild;
	}
}

void TreeType_RedBlack::NodeBase::RotateLeft(TreeType_RedBlack::NodeBase* &root)
{
	TreeType_RedBlack::NodeBase* p = right;
	right = p->left;

	if(p->left->IsUsed()) p->left->parent = this;

	ChangeParentToNewChild(root, p);

	p->left = this;
	parent = p;
}

void TreeType_RedBlack::NodeBase::RotateRight(TreeType_RedBlack::NodeBase* &root)
{
	TreeType_RedBlack::NodeBase* p = left;
	left = p->right;
	if(p->right->IsUsed()) p->right->parent = this;

	ChangeParentToNewChild(root, p);

	p->right = this;
	parent = p;
}

//===========================================================================

void TreeType_RedBlack::NodeBase::FixupInsert(NodeBase* &root)
{
	left	= &sentinel;
	right	= &sentinel;
	isRed	= true;
	
	NodeBase* node = this;
	
	while(node != root && node->parent->isRed)
	{
        if(node->parent == node->parent->parent->left) 
		{
            NodeBase *uncle = node->parent->parent->right;
            if (uncle->isRed)
			{
                node->parent->isRed = false;
                uncle->isRed = false;
                node->parent->parent->isRed = true;
                node = node->parent->parent;
            } 
			else 
			{
                if(node == node->parent->right) 
				{
                    node = node->parent;
					node->RotateLeft(root);
                }
				
                node->parent->isRed = false;
                node->parent->parent->isRed = true;
				node->parent->parent->RotateRight(root);
            }
        } 
		else 
		{
            NodeBase *uncle = node->parent->parent->left;
            if(uncle->isRed) 
			{
                node->parent->isRed = false;
                uncle->isRed = false;
                node->parent->parent->isRed = true;
                node = node->parent->parent;
            } 
			else 
			{
                if(node == node->parent->left) 
				{
                    node = node->parent;
					node->RotateRight(root);
                }
                node->parent->isRed = false;
                node->parent->parent->isRed = true;
				node->parent->parent->RotateLeft(root);
            }
        }
	}
	
	root->isRed = false;
}

//===========================================================================

void TreeType_RedBlack::NodeBase::RemoveFromTree(NodeBase* &root)
{
	JASSERT(this);
	
	// A removable node is a node with which does not have 2 children
	NodeBase* removableNode;	
	NodeBase* removableNodeChild;
	
	// Stage 1 - determine which node needs to be replaced
	NodeBase* node = this;
	if(node->left->IsEmpty())
	{
		removableNode = node;
		removableNodeChild = node->right;
	}
	else if(node->right->IsEmpty())
	{
		removableNode = node;
		removableNodeChild = node->left;
	}
	else
	{
		removableNode = node->right;
		while(removableNode->left->IsUsed()) removableNode = removableNode->left;
		
		// As we do not know the data or datasize involved with each node, we have 
		// to mess with pointers to swap the RemovableNode and Node in the table.
		Swap(removableNode->left, node->left);
		Swap(removableNode->isRed, node->isRed);
		
		if(node->right == removableNode)
		{
			removableNode->parent = node->parent;
			node->parent = removableNode;
			node->right = removableNode->right;
			removableNode->right = node;
		}
		else
		{
			Swap(removableNode->right, node->right);
			Swap(removableNode->parent, node->parent);
		}
		
		if(removableNode->parent)
		{
			if(removableNode->parent->left == node) removableNode->parent->left = removableNode;
			else removableNode->parent->right = removableNode;
		}
		else
		{
			root = removableNode;
		}
		
		// And fixup child pointers
		removableNode->left->parent = removableNode;
		removableNode->right->parent = removableNode;
		
		Swap(removableNode, node);
		removableNodeChild = removableNode->right;
	}
	
	// Stage 2 - unlink the node to be replaced and bind in child instead
	removableNodeChild->parent = removableNode->parent;
	if(removableNode->parent)
	{
		if(removableNode->parent->left == node) removableNode->parent->left = removableNodeChild;
		else removableNode->parent->right = removableNodeChild;
	}
	else
	{
		root = removableNodeChild;
	}
	
	if(removableNode->isRed) return;
	
	// Stage 3 - restore Red-Black property.
	node = removableNodeChild;
	while(node != root && !node->isRed)
	{
		if(node == node->parent->left)
		{
			NodeBase *sibling = node->parent->right;
			
			if(sibling->isRed)
			{
				sibling->isRed = false;
				node->parent->isRed = true;
				node->parent->RotateLeft(root);
				sibling = node->parent->right;
			}
			
			if(!sibling->left->isRed && !sibling->right->isRed)
			{
				sibling->isRed = true;
				node = node->parent;
			}
			else
			{
				if(!sibling->right->isRed)
				{
					sibling->left->isRed = false;
					sibling->isRed = true;
					sibling->RotateRight(root);
					sibling = node->parent->right;
				}
				
				sibling->isRed = node->parent->isRed;
				node->parent->isRed = false;
				sibling->right->isRed = false;
				node->parent->RotateLeft(root);
				root->isRed = false;
				return;
			}
		}
		else
		{
			NodeBase *sibling = node->parent->left;
			
			if(sibling->isRed)
			{
				sibling->isRed = false;
				node->parent->isRed = true;
				node->parent->RotateRight(root);
				sibling = node->parent->left;
			}
			
			if(!sibling->right->isRed && !sibling->left->isRed)
			{
				sibling->isRed = true;
				node = node->parent;
			}
			else
			{
				if(!sibling->left->isRed)
				{
					sibling->right->isRed = false;
					sibling->isRed = true;
					sibling->RotateLeft(root);
					sibling = node->parent->left;
				}
				
				sibling->isRed = node->parent->isRed;
				node->parent->isRed = false;
				sibling->left->isRed = false;
				node->parent->RotateRight(root);
				root->isRed = false;
				return;
			}
		}
	}
	
	node->isRed = false;
}

//===========================================================================

TreeType_RedBlack::NodeBase* TreeType_RedBlack::NodeBase::GetLeftMostNode(const NodeBase* node)
{
	if(node->IsEmpty()) return nullptr;
	while(node->left->IsUsed()) node = node->left;
	return const_cast<NodeBase*>(node);
}

TreeType_RedBlack::NodeBase* TreeType_RedBlack::NodeBase::GetRightMostNode(const NodeBase* node)
{
	if(node->IsEmpty()) return nullptr;
	while(node->right->IsUsed()) node = node->right;
	return const_cast<NodeBase*>(node);
}

TreeType_RedBlack::NodeBase* TreeType_RedBlack::NodeBase::GetIteratorNext(const NodeBase* p)
{
	if(p->right->IsUsed())
	{
		p = p->right;
		while(p->left->IsUsed()) p = p->left;
	}
	else
	{
		const NodeBase* last;
		do
		{
			last = p;
			p = p->parent;
		} while(p && last == p->right);
	}
	return const_cast<NodeBase*>(p);
}

TreeType_RedBlack::NodeBase* TreeType_RedBlack::NodeBase::GetIteratorPrevious(const NodeBase* p)
{
	if(p->left->IsUsed())
	{
		p = p->left;
		while(p->right->IsUsed()) p = p->right;
	}
	else
	{
		const NodeBase* last;
		do
		{
			last = p;
			p = p->parent;
		} while(p && last == p->left);
	}
	return const_cast<NodeBase*>(p);
}

//===========================================================================
