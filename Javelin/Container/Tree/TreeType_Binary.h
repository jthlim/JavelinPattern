//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/System/Assert.h"
#include "Javelin/Container/Tree/TreeType_SimpleBinary.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	class TreeType_Binary : public TreeType_SimpleBinary
	{
	public:
		class NodeBase : public TreeType_SimpleBinary::NodeBase
		{
		public:
			NodeBase*	parent;

			void	SetParent(NodeBase* p)		{ parent = p; }			

			JEXPORT static NodeBase* JCALL GetLeftMostNode(const NodeBase* node);
			JEXPORT static NodeBase* JCALL GetRightMostNode(const NodeBase* node);
			
			JEXPORT static NodeBase* JCALL GetIteratorNext(const NodeBase* node);
			JEXPORT static NodeBase* JCALL GetIteratorPrevious(const NodeBase* node);
		};

	};
	
//============================================================================
} // namespace Javelin
//===========================================================================e
