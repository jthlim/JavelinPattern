/****************************************************************************

  Lock classes 

  The goal of the lock class is to provide transparent synchronized access
  to member functions of any class. Example usage is:

    Lock< Table<int>, Mutex > lockedTable;
	Lock< Table<int>, CriticalSection > lockedTable;
	Lock< Table<int> > lockedTable;
 
	lockedTable->Append(4);

  The -> operator is overloaded such that the Append function from Table is 
  called between synchronization start and end. 
  
  Access can also be performed using:

    lockedTable.BeginLock();
	lockedTable.Append(4);
	lockedTable.QuickSort();
	lockedTable.EndLock();

  Or:
	Sentry<> s(lockedTable);
	lockedTable.Append(4);
	lockedTable.QuickSort();
	
****************************************************************************/

#pragma once
#include "Javelin/Thread/CriticalSection.h"

//============================================================================

namespace Javelin
{
//============================================================================

	template<typename T=CriticalSection, typename L=T> class Sentry
	{
	public:
		inline Sentry(volatile const T& aT) : t(const_cast<T*>(&aT)), lock(*const_cast<L*>(&aT))	{ lock.BeginLock();	}
		inline Sentry(volatile const T& aT, L& aLock)	: t(const_cast<T*>(&aT)), lock(aLock)		{ lock.BeginLock();	}
		inline ~Sentry()																	{ lock.EndLock();	}

		T*	operator->()	{ return &t; }
		T&	operator*()		{ return *t; }

	private:
		T*	t;
		L&	lock;
	};

//============================================================================

	namespace Private
	{
		template<typename Lock> class LockAccessor
		{
		private:
			Lock*	p;

		public:
			inline LockAccessor(volatile Lock *l)	{ p = const_cast<Lock*>(l); p->BeginLock();	}
			inline ~LockAccessor()					{ p->EndLock();				}

			inline Lock* operator->() const			{ return p;					}
		};
	}

	template<typename T, typename L=CriticalSection> class Lock : public T, public L
	{
	public:
		typedef T Type;
		
		template<typename... U> Lock(U&&... u) : T(u...) { }

		Lock& operator=(const Lock &b)				
		{ 
			b.BeginLock();
			this->BeginLock();
			this->T::operator=(b);
			this->EndLock();
			b.EndLock(); 
			return *this;
		}

		inline const Private::LockAccessor< Lock<T, L> >		operator->() volatile		{ return Private::LockAccessor< Lock<T, L> >(this);			}
		inline const Private::LockAccessor< const Lock<T, L> >	operator->() const volatile	{ return Private::LockAccessor< const Lock<T, L> >(this);	}
	};

//============================================================================
} // namespace Javelin
//============================================================================
