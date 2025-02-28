//============================================================================

#include "Javelin/Container/Tree/TreeType_AVL.h"
#include "Javelin/Template/Utility.h"

//============================================================================

using namespace Javelin;

//============================================================================

void TreeType_AVL::NodeBase::UpdateHeight()
{
	int leftHeight = GetHeight((NodeBase*) left);
	int rightHeight = GetHeight((NodeBase*) right);
	height = Maximum(leftHeight, rightHeight) + 1;
}

//============================================================================

void TreeType_AVL::NodeBase::ChangeParentToNewChild(TreeType_AVL::NodeBase* &root, TreeType_AVL::NodeBase* newChild) const
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

//============================================================================

void TreeType_AVL::NodeBase::FixupInsert(NodeBase* &root)
{
	left = nullptr;
	right = nullptr;
	height = 0;
	
	NodeBase* grandChild;
	NodeBase* child = nullptr;
	NodeBase* node = this;
	int processHeight = 0;
	
	while(node->parent)
	{
		grandChild = child;
		child = node;
		node = (NodeBase*) node->parent;

		++processHeight;
		if(node->height >= processHeight)
		{
			root->CheckConsistency();
			return;
		}
		node->height = processHeight;
		
		int delta = GetHeight((NodeBase*) node->left) - GetHeight((NodeBase*) node->right);
		JASSERT(-2 <= delta && delta <= 2);
		
		if(delta == 2)
		{
			// Left has greater depth by 2.
			// Check if it's a left rotation or a double left rotation
			if(grandChild == child->left)
			{
				// LL Rotation
				// NodeBase* T1 = grandChild;
				NodeBase* T2 = (NodeBase*) child->right;

				JASSERT(GetHeight(T2) == GetHeight((NodeBase*) node->right));

				child->right = node;
				node->left = T2;
				node->ChangeParentToNewChild(root, child);
				if(T2) T2->parent = node;
				node->parent = child;
				node->height = node->height - 2;

				root->CheckConsistency();
				return;
			}
			else 
			{
				// LR Rotation
				// NodeBase* T1 = (NodeBase*) child->left;
				NodeBase* T2 = (NodeBase*) grandChild->left;
				NodeBase* T3 = (NodeBase*) grandChild->right;
				// NodeBase* T4 = (NodeBase*) node->right;

				child->right = T2;
				node->left = T3;
				grandChild->left = child;
				grandChild->right = node;
				node->ChangeParentToNewChild(root, grandChild);
				if(T2) T2->parent = child;
				if(T3) T3->parent = node;
				node->parent = grandChild;
				child->parent = grandChild;

				node->height = grandChild->height;
				grandChild->height = child->height;
				child->height = node->height;

				root->CheckConsistency();
				return;
			}
		}
		else if(delta == -2)
		{
			if(grandChild == child->right)
			{
				// RR Rotation
				// NodeBase* T1 = grandChild;
				NodeBase* T2 = (NodeBase*) child->left;

				JASSERT(GetHeight(T2) == GetHeight((NodeBase*) node->left));

				child->left = node;
				node->right = T2;
				node->ChangeParentToNewChild(root, child);
				if(T2) T2->parent = node;
				node->parent = child;
				node->height = node->height - 2;

				root->CheckConsistency();
				return;
			}
			else 
			{
				// RL Rotation
				// NodeBase* T1 = (NodeBase*) child->right;
				NodeBase* T2 = (NodeBase*) grandChild->right;
				NodeBase* T3 = (NodeBase*) grandChild->left;
				// NodeBase* T4 = (NodeBase*) node->left;

				child->left = T2;
				node->right = T3;
				grandChild->right = child;
				grandChild->left = node;
				node->ChangeParentToNewChild(root, grandChild);
				if(T2) T2->parent = child;
				if(T3) T3->parent = node;
				node->parent = grandChild;
				child->parent = grandChild;
				
				node->height = grandChild->height;
				grandChild->height = child->height;
				child->height = node->height;
				
				root->CheckConsistency();
				return;
			}
		}
	}
}

//============================================================================

#if JCHECK_AVL_CONSISTENCY

void TreeType_AVL::NodeBase::CheckConsistency() const
{
	int leftHeight = ((NodeBase*) left)->GetHeight();
	int rightHeight = ((NodeBase*) right)->GetHeight();

	int delta = leftHeight - rightHeight;
	JASSERT(-2 < delta && delta < 2);
	
	JASSERT(leftHeight == height-1 || rightHeight == height-1);
	JASSERT(leftHeight < height && rightHeight < height);
	
	if(left)
	{
		JASSERT( ((NodeBase*) left)->parent == this );
		((NodeBase*) left)->CheckConsistency();
	}
	if(right)
	{
		JASSERT( ((NodeBase*) right)->parent == this );
		((NodeBase*) right)->CheckConsistency();
	}
}

#endif

//============================================================================
