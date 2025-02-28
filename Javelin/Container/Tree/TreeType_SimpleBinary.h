//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/System/Assert.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	class TreeType_SimpleBinary
	{
	public:
		class NodeBase
		{
		public:
			NodeBase*	left;
			NodeBase*	right;

			friend bool	IsEmpty(const NodeBase* p)					{ return p == nullptr; }
			friend bool	IsUsed(const NodeBase* p) 					{ return p != nullptr; }

			void	SetParent(NodeBase*)							{ }
			
			JINLINE static constexpr size_t GetNumberOfChildren()	{ return 2; }
			JINLINE NodeBase* GetChild(size_t i)					{ JASSERT(i < 2); return (&left)[i]; }
			JINLINE void SetChild(size_t i, NodeBase* p)			{ JASSERT(i < 2); (&left)[i] = p; }
			JINLINE const NodeBase* GetChild(size_t i) const		{ JASSERT(i < 2); return (&left)[i]; }
			
			JEXPORT static size_t JCALL CountAllChildren(const NodeBase* p);
			
		protected:
			void FixupInsert(NodeBase*) 							{ left = nullptr; right = nullptr; }
		};

	protected:
		static NodeBase* GetInitialRoot()	{ return nullptr; }
	};
	
//============================================================================
} // namespace Javelin
//===========================================================================e
