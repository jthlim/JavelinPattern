//============================================================================

#include "Javelin/Container/Tree.h"

//============================================================================

using namespace Javelin;

//============================================================================

TreeType_Binary::NodeBase* TreeType_Binary::NodeBase::GetLeftMostNode(const NodeBase* node)
{
	if(IsEmpty(node)) return nullptr;
	while(IsUsed(node->left)) node = static_cast<NodeBase*>(node->left);
	return const_cast<NodeBase*>(node);
}

TreeType_Binary::NodeBase* TreeType_Binary::NodeBase::GetRightMostNode(const NodeBase* node)
{
	if(IsEmpty(node)) return nullptr;
	while(IsUsed(node->right)) node = static_cast<NodeBase*>(node->right);
	return const_cast<NodeBase*>(node);
}

TreeType_Binary::NodeBase* TreeType_Binary::NodeBase::GetIteratorNext(const NodeBase* p)
{
	if(IsUsed(p->right))
	{
		p = static_cast<NodeBase*>(p->right);
		while(IsUsed(p->left)) p = static_cast<NodeBase*>(p->left);
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

TreeType_Binary::NodeBase* TreeType_Binary::NodeBase::GetIteratorPrevious(const NodeBase* p)
{
	if(IsUsed(p->left))
	{
		p = static_cast<NodeBase*>(p->left);
		while(IsUsed(p->right)) p = static_cast<NodeBase*>(p->right);
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

//============================================================================
