//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Container/DataIntegrityPolicy.h"
#include "Javelin/Container/Table/TableIterator.h"
#include "Javelin/System/Assert.h"

//============================================================================

namespace Javelin
{
//============================================================================

	template<size_t N> class TableStorage_Static
	{
	public:
		template<typename T, typename DataIntegrityPolicy = DataIntegrityPolicy_Automatic<T> > 
		class Implementation
		{
		public:
			JINLINE	size_t		GetCount() 				const		{ return count;	}
			JINLINE	size_t		GetCapacity()			const		{ return N;		}

			JINLINE	void		Reset() 							{ DataIntegrityPolicy::Destroy(Begin(), count); count = 0; }

			JINLINE	T&			Append()							{ JASSERT(count < N); T* p = &Begin()[count++]; DataIntegrityPolicy::Create(p); return *p; }
			
			template<typename U>
			JINLINE	void		Append(U &&a)						{ JASSERT(count < N); DataIntegrityPolicy::Create(&Begin()[count++], (U&&) a);	}

			JINLINE	void		AppendCount(const T* p, size_t c)	{ JASSERT(count+c <= N); DataIntegrityPolicy::InitialCopy(&Begin()[count], p, c); count += c;	}
			JINLINE T*			AppendCount(size_t n)				{ JASSERT(count+n <= N); T* r = Begin(); DataIntegrityPolicy::CreateCount(r, n); count += n; return r; }

			template<typename U>
			JINLINE void		InsertAtIndex(size_t i, U &&a) 		{
																		JASSERT(i <= count);
																		JASSERT(count < N);
																		DataIntegrityPolicy::CopyBackwards(Begin()+i+1, Begin()+i, count++ - i);
																		DataIntegrityPolicy::CreateOnZombie(&Begin()[i], (U&&) a);
																	}

			JINLINE	void		RemoveIndex(size_t i)				{
																		JASSERT(i < count);
																		DataIntegrityPolicy::Copy(Begin()+i, Begin()+i+1, --count-i);
																		DataIntegrityPolicy::Destroy(Begin()+count);
																	}

			JINLINE	void		RemoveBack()						{
																		JASSERT(count);
																		DataIntegrityPolicy::Destroy(Begin() + --count);
																	}

			JINLINE	T*			GetData()							{ return reinterpret_cast<T*>(data); 		}
			JINLINE	const T*	GetData() const						{ return reinterpret_cast<const T*>(data); 	}
	
			JINLINE	T*			Begin()								{ return reinterpret_cast<T*>(data); 		}
			JINLINE	const T*	Begin()	const						{ return reinterpret_cast<const T*>(data); 	}

			JINLINE	T*			End()								{ return Begin() + count; }
			JINLINE	const T*	End()	const						{ return Begin() + count; }

			JINLINE	T&			operator[](size_t i)				{ JASSERT(i < GetCount()); return reinterpret_cast<T*>(data)[i]; }
			JINLINE	const T&	operator[](size_t i)	const		{ JASSERT(i < GetCount()); return reinterpret_cast<const T*>(data)[i]; }
			
			typedef T						ElementType;
			typedef TableIterator<T>		Iterator;
			typedef TableIterator<const T>	ConstIterator;

		protected:
			Implementation()										{ count = 0;	}
			~Implementation()										{ DataIntegrityPolicy::Destroy(Begin(), count); }
			
		private:
			char	data[sizeof(T)*N];
			size_t	count;
		};
	};

//============================================================================
} // namespace Javelin
//============================================================================
