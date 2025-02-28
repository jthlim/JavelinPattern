//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/System/Assert.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	class TreeType_RedBlack
	{
	public:
		class NodeBase
		{
		public:
			NodeBase*	left;
			NodeBase*	right;
			NodeBase*	parent;
			bool		isRed;
			
			JEXPORT static NodeBase	sentinel;
			
			bool	IsEmpty()	const 								{ return this == &sentinel; }
			bool	IsUsed()	const 								{ return this != &sentinel; }

			friend bool IsEmpty(const NodeBase* p)					{ return p == &sentinel;	}
			friend bool IsUsed(const NodeBase* p)					{ return p != &sentinel;	}
			
			void	SetParent(NodeBase* newParent)					{ parent = newParent; }
			
			JINLINE static constexpr size_t GetNumberOfChildren()	{ return 2; }
			JINLINE NodeBase* GetChild(size_t i)					{ JASSERT(i < 2); return (&left)[i]; }
			JINLINE void SetChild(size_t i, NodeBase* p)			{ JASSERT(i < 2); (&left)[i] = p; }
			JINLINE const NodeBase* GetChild(size_t i) const		{ JASSERT(i < 2); return (&left)[i]; }
						
			JEXPORT static size_t JCALL CountAllChildren(const NodeBase* p);
			
			JEXPORT static NodeBase* JCALL GetLeftMostNode(const NodeBase* p);
			JEXPORT static NodeBase* JCALL GetRightMostNode(const NodeBase* p);
			
			JEXPORT static NodeBase* JCALL GetIteratorNext(const NodeBase* p);
			JEXPORT static NodeBase* JCALL GetIteratorPrevious(const NodeBase* p);
			
		protected:
			// This will fixup the Left, Right and IsRed members.
			// Data, and Parent members must already be setup.
			JEXPORT void JCALL FixupInsert(NodeBase* &root);
			
			// This will remove the node from the tree. It will *not* free the memory
			// associated with the node.
			JEXPORT void JCALL RemoveFromTree(NodeBase* &root);
			
		private:
			void ChangeParentToNewChild(NodeBase* &root, NodeBase* newChild) const;
			
			void RotateLeft(NodeBase* &root);
			void RotateRight(NodeBase* &root);
		};
		
	protected:
		static NodeBase* GetInitialRoot()	{ return &NodeBase::sentinel; }
	};
	
//============================================================================
} // namespace Javelin
//===========================================================================e
