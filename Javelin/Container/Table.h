//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Container/Table/TableStorage_Dynamic.h"
#include "Javelin/Container/Table/TableStorage_Static.h"
#include "Javelin/Cryptography/Crc64.h"
#include "Javelin/Math/HashFunctions.h"
#include "Javelin/Template/Range.h"
#include "Javelin/Template/Swap.h"
#include "Javelin/Template/Utility.h"
#include "Javelin/System/Assert.h"

//============================================================================

namespace Javelin
{
//============================================================================

	namespace Private
	{
		template<typename T> class TypedTableBase
		{
		protected:
			// Contains tests against a value, HasElement tests against a function
			template<typename F>
			static bool HasElement(const T* data, size_t tableSize, const F& f);

			template<typename F>
			static size_t CountElements(T* data, size_t tableSize, const F& f);

			template<typename F>
			static size_t Filter(T* data, size_t tableSize, F&& f);

			template<typename F>
			static void Perform(T* data, size_t tableSize, F&& f);
			
			template<typename M>
			static void Map(T* data, size_t tableSize, M&& m);

			static void SetAll(T* data, size_t tableSize, const T& value);
			
			template<typename R>
			static T Reduce(const T* data, size_t tableSize, const R& r);

			template<typename R, typename RT>
			static RT Reduce(const T* data, size_t tableSize, const R& r, RT initialValue);
			
			template<typename M, typename R>
			static auto MapReduce(const T* data, size_t tableSize, const M& m, const R& r) -> decltype(m(*(T*)0));

			template<typename M, typename R, typename RT>
			static RT MapReduce(const T* data, size_t tableSize, const M& m, const R& r, RT initialValue);
			
			template<typename Comparator>
			static bool IsOrdered(const T* data, size_t tableSize, const Comparator& c)
			{
				if(!tableSize) return true;
				while(--tableSize)
				{
					if(!c.IsOrdered(data[0], data[1])) return false;
					++data;
				}
				return true;
			}
			
			static bool IsLess(const T* a, const T* b, size_t count1, size_t count2);
			static bool IsEqual(const T* a, const T* b, size_t count);
			
			template<typename Comparator, typename U>
			static bool Contains(const T* data, size_t tableSize, U&& value, const Comparator& c)
			{
				for(size_t i = 0; i < tableSize; ++i)
				{
					if(c.IsEqual(data[i], value)) return true;
				}
				return false;
			}
			
			template<typename Comparator, typename U>
			static size_t FindIndexForValue(const T* data, size_t tableSize, U&& value, const Comparator& c)
			{
				for(size_t i = 0; i < tableSize; ++i)
				{
					if(c.IsEqual(data[i], value)) return i;
				}
				return TypeData<size_t>::Maximum();
			}

			// In an ordered table, find last index such that data[index] <= value
			template<typename Comparator, typename U>
			static size_t FindLastIndexBeforeValueInSortedTable(const T* data, size_t tableSize, U&& value, const Comparator& c);
			template<typename Comparator, typename U>
			static size_t FindLastIndexBeforeOrEqualToValueInSortedTable(const T* data, size_t tableSize, U&& value, const Comparator& c);
			template<typename Comparator, typename U>
			static size_t FindFirstIndexAfterValueInSortedTable(const T* data, size_t tableSize, U&& value, const Comparator& c);
			template<typename Comparator, typename U>
			static size_t FindFirstIndexAfterOrEqualToValueInSortedTable(const T* data, size_t tableSize, U&& value, const Comparator& c);

			// Table must be sorted!
			template<typename Comparator, typename U>
			static size_t FindInsertIndexForSortedTable(const T* data, size_t tableSize, U&& value, const Comparator& c);
			template<typename Comparator, typename U>
			static size_t FindIndexForValueInSortedTable(const T* data, size_t tableSize, U&& value, const Comparator& c);

			template<typename Comparator> static void BubbleSort(T *data, size_t tableSize, const Comparator& c);
			template<typename Comparator> static void HeapSort(T *data, size_t tableSize, const Comparator& c);
			template<typename Comparator> static void QuickSortSpan(T* lower, size_t spanLength, const Comparator& c);
			template<typename Comparator> static T&   QuickSelectSpan(T* lower, T *upper, T* position, const Comparator& c);
			template<typename Comparator> static void InsertionSort(T *data, size_t tableSize, const Comparator& c);

		private:
			template<typename Comparator> static void SiftDown(T *data, size_t start, size_t end, const Comparator& c);
			template<typename Comparator> static void Insert(T *data, size_t position, const Comparator& c);
		};
	}
	
//============================================================================

	template<typename T,
			 class StoragePolicy = TableStorage_Dynamic<>,
			 class DataIntegrityPolicy = DataIntegrityPolicy_Automatic<T> > 
	class Table : public StoragePolicy::template Implementation<T, DataIntegrityPolicy>, private Private::TypedTableBase<T>
	{
	private:
		typedef typename StoragePolicy::template Implementation<T, DataIntegrityPolicy> ImplementationBase;
		
	public:
		Table() { }
		explicit Table(size_t initialCapacity) : ImplementationBase(initialCapacity) { }
		Table(const Table& a) : ImplementationBase(a) { }
		Table(Table&& a) JNOTHROW : ImplementationBase((ImplementationBase&&) a) { }
		Table(const T* data, size_t length) : ImplementationBase(data, length) { }
		JINLINE Table(const T* data, size_t length, const MakeReference* mr) : ImplementationBase(data, length, mr) { }

// These are inherited from ImplementationBase
// These lines are commented out because clang seems to think they're private, when they really aren't.
//
		using ImplementationBase::GetCount;
		using ImplementationBase::Append;
		using ImplementationBase::AppendCount;
		using ImplementationBase::InsertAtIndex;
		using ImplementationBase::RemoveIndex;
		using ImplementationBase::GetData;
		using ImplementationBase::Begin;
		using ImplementationBase::End;
		using ImplementationBase::operator[];
		
		JINLINE auto GetDomain() const -> decltype(Range(GetCount())) 	{ return Range(GetCount()); }
		
		JINLINE size_t		GetNumberOfBytes()		const	{ return GetCount() * sizeof(T); }
		JINLINE	bool		IsEmpty()				const	{ return GetCount() == 0; }
		JINLINE bool		HasData()				const	{ return GetCount() != 0; }

		JINLINE T&			Front()							{ JASSERT(GetCount() > 0); return (*this)[0]; 	}
		JINLINE const T&	Front()					const	{ JASSERT(GetCount() > 0); return (*this)[0]; 	}
		JINLINE	T&			Back(int i = -1)				{ JASSERT(GetCount() > 0); return (*this)[GetCount()+i]; }
		JINLINE	const T&	Back(int i = -1)		const	{ JASSERT(GetCount() > 0); return (*this)[GetCount()+i]; }
		
		template<typename U>
		JINLINE void		Push(U &&a)								{ this->Append((U&&) a); }

		template<typename U>
		JINLINE void		AppendUnique(U&& a)						{ if(!Contains(a)) Append(a); }
		
		template<typename Comparator=Less, typename U>
		JINLINE void		InsertSorted(U &&a, const Comparator& c = Comparator())
																	{ this->InsertAtIndex(Private::TypedTableBase<T>::FindInsertIndexForSortedTable(ImplementationBase::GetData(), GetCount(), a, c), (U&&) a); }
		
		JINLINE void		Pop()									{ this->RemoveBack(); }
		JINLINE void		Pop(size_t count)						{ for(size_t i = 0; i < count; ++i) this->RemoveBack(); }
		JINLINE void		PopValue(T& result)						{ JASSERT(GetCount() > 0); result = (T&&) ((*this)[GetCount()-1]); this->RemoveBack(); }
		
		template<typename U>
		JINLINE	void		Remove(U&& a)							{ this->RemoveIndex(FindIndexForValue(a));	}

		template<typename U>
		JINLINE bool		RemoveIfExists(U&& a)					{ size_t index = FindIndexForValue(a); if(index == TypeData<size_t>::Maximum()) return false; this->RemoveIndex(index); return true; }
		
		JINLINE	void		Remove(typename ImplementationBase::Iterator & i) { this->RemoveIndex(i-StoragePolicy::template Implementation<T>::Begin()); }

		void Reverse()												{ Javelin::Reverse(ImplementationBase::GetData(), GetCount());	}
		
		void Shuffle()												{ Javelin::Shuffle(ImplementationBase::GetData(), GetCount());	}

		template<typename F>
		bool HasElement(const F& f) const							{ return Private::TypedTableBase<T>::HasElement(ImplementationBase::GetData(), GetCount(), f); }

		template<typename F>
		size_t CountElements(const F& f)							{ return Private::TypedTableBase<T>::CountElements(ImplementationBase::GetData(), GetCount(), f); }
		
		template<typename F>
		void Filter(F&& filter)										{ ImplementationBase::SetCount( Private::TypedTableBase<T>::Filter(ImplementationBase::GetData(), GetCount(), filter)); }

		template<typename F> 
		void Perform(F&& f)											{ Private::TypedTableBase<T>::Perform(ImplementationBase::GetData(), GetCount(), f); }

		void SetAll(const T& value)									{ Private::TypedTableBase<T>::SetAll(ImplementationBase::GetData(), GetCount(), value); }
		
		template<typename M> 
		void Map(M&& map)											{ Private::TypedTableBase<T>::Map(ImplementationBase::GetData(), GetCount(), map); }
		
		template<typename R>
		T Reduce(const R& r) const									{ return Private::TypedTableBase<T>::Reduce(ImplementationBase::GetData(), GetCount(), r); }

		template<typename R, typename RT>
		RT Reduce(const R& r, RT initialValue) const				{ return Private::TypedTableBase<T>::Reduce(ImplementationBase::GetData(), GetCount(), r, initialValue); }
		
		template<typename M, typename R>
		auto MapReduce(const M& m, const R& r) const -> decltype(m(*(T*)0)) { return Private::TypedTableBase<T>::MapReduce(ImplementationBase::GetData(), GetCount(), m, r); }

		template<typename M, typename R, typename RT>
		RT MapReduce(const M& m, const R& r, RT initialValue) const	{ return Private::TypedTableBase<T>::MapReduce(ImplementationBase::GetData(), GetCount(), m, r, initialValue); }
		
		Table& operator=(const Table& a)							{ ImplementationBase::operator=(a); return *this; }
		Table& operator=(Table&& a)	JNOTHROW 						{ ImplementationBase::operator=((ImplementationBase&&) a); return *this; }
		
		T FindMaximum() const										{ return Reduce([](const T& a, const T& b) { return Maximum(a, b); }); }
		T FindMinimum() const										{ return Reduce([](const T& a, const T& b) { return Minimum(a, b); }); }
	
		friend size_t GetHash(const Table& a)						{ size_t hash = ~size_t(0); for(const T& x : a) { uint64_t v = GetHash(x); hash = Crc64Iteration(hash,  &v, sizeof(v)); } return ~hash; }
	
		bool operator==(const Table& a) const
		{
			if(GetCount() != a.GetCount()) return false;
			return Private::TypedTableBase<T>::IsEqual(ImplementationBase::Begin(), a.Begin(), GetCount());
		}

		bool operator<(const Table& a) const
		{
			return Private::TypedTableBase<T>::IsLess(ImplementationBase::Begin(), a.Begin(), GetCount(), a.GetCount());
		}
		
		JINLINE bool JCALL operator!=(const Table& a) const { return !(*this == a); }
		JINLINE bool JCALL operator>(const Table& a) const  { return (a < *this); }
		JINLINE bool JCALL operator<=(const Table& a) const  { return !(a < *this); }
		JINLINE bool JCALL operator>=(const Table& a) const  { return !(*this < a); }

		template<typename Comparator = Less>
			bool IsOrdered(const Comparator& c = Comparator()) const	
		{ 
			return Private::TypedTableBase<T>::IsOrdered(ImplementationBase::Begin(), GetCount(), c); 
		}

		template<typename U>
		size_t FindIndexForValue(U&& value) const
		{ 
			return Private::TypedTableBase<T>::FindIndexForValue(ImplementationBase::GetData(), GetCount(), value, Equal()); 
		}

		template<typename Comparator = Less, typename U>
		size_t FindIndexForValueInSortedTable(U&& value, const Comparator& c = Comparator()) const
		{
			return Private::TypedTableBase<T>::FindIndexForValueInSortedTable(ImplementationBase::GetData(), GetCount(), value, c);
		}

		template<typename U>
			bool Contains(U&& value) const
		{
			return Private::TypedTableBase<T>::Contains(ImplementationBase::GetData(), GetCount(), value, Equal()); 
		}

		template<typename Comparator = Less, typename U>
		size_t FindLastIndexBeforeValueInSortedTable(U&& value, const Comparator& c = Comparator()) const
		{
			return Private::TypedTableBase<T>::FindLastIndexBeforeValueInSortedTable(ImplementationBase::GetData(), GetCount(), value, c);
		}
		
		template<typename Comparator = Less, typename U>
			size_t FindLastIndexBeforeOrEqualToValueInSortedTable(U&& value, const Comparator& c = Comparator()) const
		{
			return Private::TypedTableBase<T>::FindLastIndexBeforeOrEqualToValueInSortedTable(ImplementationBase::GetData(), GetCount(), value, c);
		}

		template<typename Comparator = Less, typename U>
		size_t FindFirstIndexAfterValueInSortedTable(U&& value, const Comparator& c = Comparator()) const
		{
			return Private::TypedTableBase<T>::FindFirstIndexAfterValueInSortedTable(ImplementationBase::GetData(), GetCount(), value, c);
		}
		
		template<typename Comparator = Less, typename U>
		size_t FindFirstIndexAfterOrEqualToValueInSortedTable(U&& value, const Comparator& c = Comparator()) const
		{
			return Private::TypedTableBase<T>::FindFirstIndexAfterOrEqualToValueInSortedTable(ImplementationBase::GetData(), GetCount(), value, c);
		}
		
		template<typename Comparator = Less>
		void BubbleSort(const Comparator& c = Comparator())
		{ 
			Private::TypedTableBase<T>::BubbleSort(ImplementationBase::GetData(), GetCount(), c); 
		}
		
		template<typename Comparator = Less>
		void QuickSort(const Comparator& c = Comparator())
		{ 
			Private::TypedTableBase<T>::QuickSortSpan(ImplementationBase::GetData(), GetCount(), c); 
		}

		template<typename Comparator = Less>
		void HeapSort(const Comparator& c = Comparator())
		{
			Private::TypedTableBase<T>::HeapSort(ImplementationBase::GetData(), GetCount(), c);
		}

		template<typename Comparator = Less>
		void InsertionSort(const Comparator& c = Comparator())
		{
			Private::TypedTableBase<T>::InsertionSort(ImplementationBase::GetData(), GetCount(), c);
		}
	};

	template<typename T, 
			 int n,
			 class DataIntegrityPolicy = DataIntegrityPolicy_Automatic<T> > 
	class StaticTable : public Table<T, TableStorage_Static<n>, DataIntegrityPolicy >
	{
	};

//============================================================================

	namespace Private
	{
		template<typename T> template<typename F>
		bool TypedTableBase<T>::HasElement(const T* data, size_t tableSize, const F& f)
		{
			for(size_t i = 0; i < tableSize; ++i)
			{
				if(f(data[i])) return true;
			}
			return false;
		}

		template<typename T> template<typename F>
		size_t TypedTableBase<T>::CountElements(T* data, size_t tableSize, const F& f)
		{
			size_t count = 0;
			for(size_t i = 0; i < tableSize; ++i)
			{
				if(f(data[i])) ++count;
			}
			return count;
		}
		
		template<typename T> template<typename F>
		size_t TypedTableBase<T>::Filter(T* data, size_t tableSize, F&& f)
		{
			T* p = data;
			T* pEnd = data + tableSize;
			
			while(p < pEnd)
			{
				if(!f(*p)) break;
				++p;
			}
			
			T* pOut = p++;
			
			while(p < pEnd)
			{
				if(f(*p)) *pOut++ = *p;
				++p;
			}
			
			return pOut - data;
		}

		template<typename T> template<typename F>
		void TypedTableBase<T>::Perform(T* data, size_t tableSize, F&& f)
		{
			for(size_t i = 0; i < tableSize; ++i)
			{
				f(data[i]);
			}
		}

		template<typename T>
		void TypedTableBase<T>::SetAll(T* data, size_t tableSize, const T& value)
		{
			for(size_t i = 0; i < tableSize; ++i)
			{
				data[i] = value;
			}
		}

		template<typename T> template<typename M>
		void TypedTableBase<T>::Map(T* data, size_t tableSize, M&& m)
		{
			for(size_t i = 0; i < tableSize; ++i)
			{
				data[i] = m(data[i]);
			}
		}
		
		template<typename T> template<typename R>
		T TypedTableBase<T>::Reduce(const T* data, size_t tableSize, const R& r)
		{
			JASSERT(tableSize > 0);
			T result = data[0];
			
			for(size_t i = 1; i < tableSize; ++i)
			{
				result = r(result, data[i]);
			}
			
			return result;
		}
		
		template<typename T> template<typename R, typename RT>
		RT TypedTableBase<T>::Reduce(const T* data, size_t tableSize, const R& r, RT initialValue)
		{
			T result = initialValue;
			
			for(size_t i = 0; i < tableSize; ++i)
			{
				result = r(result, data[i]);
			}
			
			return result;
		}
		
		template<typename T> template<typename M, typename R>
		auto TypedTableBase<T>::MapReduce(const T* data, size_t tableSize, const M& m, const R& r) -> decltype(m(*(T*)0))
		{
			JASSERT(tableSize > 0);
			decltype(m(*(T*)0)) result = m(data[0]);
			
			for(size_t i = 1; i < tableSize; ++i)
			{
				result = r(result, m(data[i]));
			}
			
			return result;
		}
		
		template<typename T> template<typename M, typename R, typename RT>
		RT TypedTableBase<T>::MapReduce(const T* data, size_t tableSize, const M& m, const R& r, RT initialValue)
		{
			T result = initialValue;
			
			for(size_t i = 0; i < tableSize; ++i)
			{
				result = r(result, m(data[i]));
			}
			
			return result;
		}

		template<typename T> template<typename Comparator, typename U>
		size_t TypedTableBase<T>::FindLastIndexBeforeValueInSortedTable(const T* data, size_t tableSize, U&& value, const Comparator& c)
		{
			size_t lower = 0;
			size_t upper = tableSize;
			while(lower+1 < upper)
			{
				size_t mid = (lower+upper)>>1;
				const T& entry = data[mid];
				if(c.IsEqual(value, entry)) upper = mid;
				else if(c.IsOrdered(value, entry)) upper = mid;
				else lower = mid;
			}
			
			if(upper < tableSize)
			{
				if(c.IsOrdered(data[upper], value)) return upper;
			}
			
			return lower;
		}
		
		template<typename T> template<typename Comparator, typename U>
		size_t TypedTableBase<T>::FindLastIndexBeforeOrEqualToValueInSortedTable(const T* data, size_t tableSize, U&& value, const Comparator& c)
		{
			size_t lower = 0;
			size_t upper = tableSize;
			while(lower+1 < upper)
			{
				size_t mid = (lower+upper)>>1;
				const T& entry = data[mid];
				if(c.IsEqual(value, entry)) lower = mid;
				else if(c.IsOrdered(value, entry)) upper = mid;
				else lower = mid;
			}
			
			if(upper < tableSize)
			{
				if(!c.IsOrdered(value, data[upper])) return upper;
			}
			
			return lower;
		}

		template<typename T> template<typename Comparator, typename U>
		size_t TypedTableBase<T>::FindInsertIndexForSortedTable(const T* data, size_t tableSize, U&& value, const Comparator& c)
		{
			size_t lower = 0;
			size_t upper = tableSize;	// Exclusive!
			
			while(lower < upper)
			{
				size_t mid = (lower+upper) >> 1;
				if(c.IsEqual(value, data[mid])) return mid;
				if(c.IsOrdered(value, data[mid])) upper = mid;
				else lower = mid+1;
			}
			
			return lower;
		}
		
		template<typename T> template<typename Comparator, typename U>
		size_t TypedTableBase<T>::FindIndexForValueInSortedTable(const T* data, size_t tableSize, U&& value, const Comparator& c)
		{
			size_t lower = 0;
			size_t upper = tableSize;	// Exclusive!
			
			while(lower < upper)
			{
				size_t mid = (lower+upper) >> 1;
				if(c.IsEqual(value, data[mid])) return mid;
				if(c.IsOrdered(value, data[mid])) upper = mid;
				else lower = mid+1;
			}
			
			return TypeData<size_t>::Maximum();
		}

		template<typename T> template<typename C>
		void TypedTableBase<T>::QuickSortSpan(T *lower, size_t spanLength, const C& c)
		{
			if(spanLength <= 1) return;
			
			T* upper = lower + (spanLength-1);
			if(spanLength == 2)
			{
				if(!c.IsOrdered(*lower, *upper)) Swap(*lower, *upper);
				return;
			}
			
			// Select pivot
			T &pivotValue = *lower;
			T* middle = lower + (spanLength >> 1);
			
			if(c.IsOrdered(pivotValue, *middle))
			{
				if(c.IsOrdered(*middle, *upper))
				{
					Swap(pivotValue, *middle);
				}
				else if(c.IsOrdered(pivotValue, *upper))
				{
					Swap(pivotValue, *upper);
				}
			}
			else
			{
				if(c.IsOrdered(*upper, *middle))
				{
					Swap(pivotValue, *middle);
				}
				else if(c.IsOrdered(*upper, pivotValue))
				{
					Swap(pivotValue, *upper);
				}
			}
			
			// Do the Partition	
			T* left   = lower;
			T* right  = upper;
			while(1)
			{
				while(c.IsOrdered(pivotValue, *right) && right > left) { --right; }
				
				do
				{
					if(++left >= right) goto EndOfLoop;
				} while(c.IsOrdered(*left, pivotValue));
				
				Swap(*left, *right--);
			}
			
		EndOfLoop:
			Swap(pivotValue, *right);
			T* pivot = right;
			
			// And recurse
			QuickSortSpan(lower, pivot-lower, c);
			QuickSortSpan(pivot+1, upper-pivot, c);		
		}
		
		template<typename T> template<typename C> 
		void TypedTableBase<T>::BubbleSort(T *data, size_t tableSize, const C& c)
		{
			while(tableSize > 1)
			{
				--tableSize;

				bool noSwap = true;
				
				for(size_t j = 0; j < tableSize; ++j)
				{
					if(!c.IsOrdered(data[j], data[j+1]))
					{
						Swap(data[j], data[j+1]);
						noSwap = false;
					}
				}

				if(noSwap) return;
			}
		}

		template<typename T> template<typename C>
		void TypedTableBase<T>::HeapSort(T *data, size_t tableSize, const C& c)
		{
			if(tableSize <= 1) return;
			
			// First heapify
			size_t start = tableSize/2-1;
			while(true)
			{
				SiftDown(data, start, tableSize, c);
				if(start == 0) break;
				--start;
			}
			
			// Then pop off the heap!
			size_t end = tableSize-1;
			while(end > 0)
			{
				Swap(data[end], data[0]);
				SiftDown(data, 0, end, c);
				--end;
			}
		}
		
		template<typename T> template<typename C>
		void TypedTableBase<T>::SiftDown(T *data, size_t start, size_t end, const C& c)
		{
			size_t root = start;
			while(root*2 + 1 < end)
			{
				size_t child = root*2 + 1;	// left child
				size_t swap = root;
				if(c.IsOrdered(data[swap], data[child])) swap = child;
				if(child+1 < end && c.IsOrdered(data[swap], data[child+1])) swap = child+1;
				
				if(swap == root) return;
				
				Swap(data[swap], data[root]);
				root = swap;
			}
		}

		template<typename T> template<typename C>
		void TypedTableBase<T>::InsertionSort(T *data, size_t tableSize, const C& c)
		{
			for(size_t i = 1; i < tableSize; ++i)
			{
				Insert(data, i, c);
			}
		}

		template<typename T> template<typename C>
		void TypedTableBase<T>::Insert(T *data, size_t position, const C& c)
		{
			T value(data[position]);
			while(position > 0 && c.IsOrdered(value, data[position-1]))
			{
				data[position] = (T&&) data[position-1];
				--position;
			}
			data[position] = (T&&) value;
		}
		
		template<typename T>
		bool TypedTableBase<T>::IsLess(const T* a, const T* b, size_t count1, size_t count2)
		{
			size_t count = Minimum(count1, count2);
			for(size_t i = 0; i < count; ++i)
			{
				if(a[i] != b[i])
				{
					return a[i] < b[i];
				}
			}
			return count1 < count2;
		}
		
		template<typename T>
		bool TypedTableBase<T>::IsEqual(const T* a, const T* b, size_t count)
		{
			while(count)
			{
				if(*a++ != *b++) return false;
				--count;
			}
			return true;
		}

		
		// Optimization specializations
		template<> JINLINE bool TypedTableBase<unsigned char>::IsLess(const unsigned char* a, const unsigned char* b, size_t count1, size_t count2)
		{
			size_t count = Minimum(count1, count2);
			int result = memcmp(a, b, count);
			if(result != 0) return result < 0;
			return count1 < count2;
		}
		
		template<> JINLINE bool TypedTableBase<unsigned char>::IsEqual(const unsigned char* a, const unsigned char* b, size_t count)
		{
			return memcmp(a, b, count) == 0;
		}
	}
	
//============================================================================
} // namespace Javelin
//============================================================================
