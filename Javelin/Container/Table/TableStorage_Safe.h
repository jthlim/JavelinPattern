//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Container/SafeIterator.h"
#include "Javelin/Container/Table/TableIterator.h"
#include "Javelin/System/Assert.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	template<typename StoragePolicy>
	class TableStorage_Safe
	{
	public:
		template<typename T, typename DataIntegrityPolicy > 
		class Implementation : private StoragePolicy::template Implementation<T, DataIntegrityPolicy>
		{
		private:
			typedef typename StoragePolicy::template Implementation<T, DataIntegrityPolicy> Inherited;
			
		public:
			typedef typename Inherited::ElementType					ElementType;
			typedef SafeIterator<typename Inherited::Iterator>		Iterator;
			typedef SafeIterator<typename Inherited::ConstIterator> ConstIterator;
			
			using Inherited::GetCount;
			using Inherited::GetCapacity;
			using Inherited::operator[];
			
			JINLINE void Reset()								{ Inherited::Reset(); Iterator::SetAll(activeIterators, Inherited::GetData()-1); }
			
			JINLINE void Reserve(size_t size)					{ void* oldBase = Inherited::GetData(); Inherited::Reserve(size); AdjustIteratorsForRelocate(oldBase); }
			
			JINLINE	T&		Append()							{ void* oldBase = Inherited::GetData(); T& result = Inherited::Append(); AdjustIteratorsForRelocate(oldBase); return result; }
				
			template<typename U>
			JINLINE	void	Append(U &&a)						{ void* oldBase = Inherited::GetData(); Inherited::Append((U&&) a); AdjustIteratorsForRelocate(oldBase); }

			JINLINE	void	RemoveIndex(size_t i)				{
																	AdjustIteratorsForRemove(Inherited::GetData()+i);
																	Inherited::RemoveIndex(i);
																}
			JINLINE	void	RemoveBack()						{
																	AdjustIteratorsForRemove(Inherited::End()-1);
																	Inherited::RemoveBack();
																}
			
			using Inherited::GetData;
			
			JINLINE	Iterator		Begin()						{ return Iterator(Inherited::Begin(), activeIterators); }
			JINLINE	ConstIterator	Begin()	const				{ return ConstIterator(Inherited::Begin(), activeIterators); }

			using Inherited::End;
			
		protected:
			Implementation()										{ activeIterators = nullptr; }
			Implementation(const Implementation& a) : Inherited(a)	{ activeIterators = nullptr; }
			~Implementation()										{ Iterator::MarkInvalid(activeIterators); }

		private:
			Iterator*	activeIterators;
			
			void AdjustIteratorsForRelocate(void* oldBase);
			void AdjustIteratorsForRemove(void* removePosition);
		};
	};

//============================================================================

	template<typename StoragePolicy> template<typename T, typename DataIntegrityPolicy > 
	void TableStorage_Safe<StoragePolicy>::Implementation<T, DataIntegrityPolicy>::AdjustIteratorsForRelocate(void* oldBase)
	{
		void* newBase = Inherited::GetData();
		size_t offset = (size_t) newBase - (size_t) oldBase;
		if(offset)
		{
			Iterator* list = activeIterators;
			while(list)
			{
				*list = (T*) ((size_t) list->GetPointer() + offset);
				list = list->next;
			}
		}
	}

	template<typename StoragePolicy> template<typename T, typename DataIntegrityPolicy > 
	void TableStorage_Safe<StoragePolicy>::Implementation<T, DataIntegrityPolicy>::AdjustIteratorsForRemove(void* removePosition)
	{
		Iterator* list = activeIterators;
		while(list)
		{
			if(list->GetPointer() >= removePosition) --(*list);
			list = list->next;
		}
	}
	
//============================================================================
} // namespace Javelin
//============================================================================
