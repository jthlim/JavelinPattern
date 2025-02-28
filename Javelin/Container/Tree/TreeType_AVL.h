//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/System/Assert.h"
#include "Javelin/Container/Tree/TreeType_Binary.h"

//============================================================================

namespace Javelin
{
//============================================================================

#define JCHECK_AVL_CONSISTENCY 0
	
	class TreeType_AVL : public TreeType_Binary
	{
	public:
		class NodeBase : public TreeType_Binary::NodeBase
		{
		protected:
			// This will fixup the Left, Right and height members.
			// Data, and Parent members must already be setup.
			JEXPORT void JCALL FixupInsert(NodeBase* &root);
			
			// This will remove the node from the tree. It will *not* free the memory
			// associated with the node.
			JEXPORT void JCALL RemoveFromTree(NodeBase* &root);
			
		private:
			int			height;
			
			void JCALL UpdateHeight();
			void JCALL ChangeParentToNewChild(NodeBase* &root, NodeBase* newChild) const;

#if JCHECK_AVL_CONSISTENCY
			void JCALL CheckConsistency() const;
#else
			JINLINE void JCALL CheckConsistency() const { }
#endif
			static int JCALL GetHeight(const NodeBase *node) { return node ? node->height : -1; }
		};
	};
	
//============================================================================
} // namespace Javelin
//===========================================================================e
