//============================================================================

#include "Javelin/Container/List.h"
#include "Javelin/System/Assert.h"

//============================================================================

using namespace Javelin;

//============================================================================

size_t ListNodeBase::GetCount(const ListNodeBase* sentinel) const
{
	size_t count = 0;
	const ListNodeBase* p = this;
	
	while(p != sentinel)
	{
		++count;
		p = p->next;
	}
	
	return count;
}

ListNodeBase* ListBase::GetIndex(size_t index) const
{
	ListNodeBase* p = sentinel.next;
	JASSERT(p != &sentinel);
	
	while(index > 0)
	{
		p = p->next;
		--index;
		JASSERT(p != &sentinel);
	}
	
	return p;
}

//============================================================================

void ListBase::UnlinkAll()
{
	ListNodeBase* p = sentinel.next;
	
	while(p != &sentinel)
	{
		ListNodeBase* next = p->next;
		p->previous = p;
		p->next = p;
		p = next;
	}
}

//============================================================================
