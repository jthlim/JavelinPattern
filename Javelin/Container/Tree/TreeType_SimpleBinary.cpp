//============================================================================

#include "Javelin/Container/Tree.h"

//============================================================================

using namespace Javelin;

//============================================================================

size_t TreeType_SimpleBinary::NodeBase::CountAllChildren(const NodeBase* p)
{
	if(IsEmpty(p)) return 0;
	return 1 + CountAllChildren(p->left) + CountAllChildren(p->right);
}

//============================================================================
