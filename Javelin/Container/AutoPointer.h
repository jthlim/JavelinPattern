//===========================================================================
//
// Simpler than a SmartObject, the AutoPointer automatically
// deletes objects created with new when they go out of scope.
// 
//===========================================================================

#pragma once 
#include "Javelin/JavelinBase.h"

//===========================================================================

namespace Javelin
{
//===========================================================================

	template<typename T> class AutoPointer
	{
	public:
		AutoPointer()							{ p = nullptr;	}
		AutoPointer(T *ip)						{ p = ip;		}
		~AutoPointer()							{ delete p;		}

		AutoPointer& operator=(T *ip)			{ delete p; p = ip;	return *this; }
		AutoPointer& operator=(AutoPointer& a)	{ delete p; p = a.p; a.p = nullptr; return *this; }

		bool IsValid() const					{ return p != nullptr; }
		
		T*		 operator->() const				{ return p;		}
		T&		 operator*() const				{ return *p;	}
		operator T*() const						{ return p; 	}

		T* GetPointer() const					{ return p; 	}
		T* RelinquishPointer()					{ T *r = p; p = nullptr; return r;	}
		void Release()							{ delete p; p = nullptr; }

	private:
		T*	p;
	};

//===========================================================================

	template<typename T> class AutoArrayPointer
	{
	public:
		AutoArrayPointer()									{ p = nullptr;		}
		AutoArrayPointer(T *ip)								{ p = ip;			}
		AutoArrayPointer(size_t count)						{ p = new T[count];	}
		~AutoArrayPointer()									{ delete [] p;	}

		AutoArrayPointer& operator=(T *ip)					{ delete [] p; p = ip; return *this; }
		AutoArrayPointer& operator=(AutoArrayPointer& a)	{ delete [] p; p = a.p; a.p = nullptr; return *this; }

		bool IsValid() const								{ return p != nullptr; }
		void SetSize(size_t count)							{ delete [] p; p = new T[count]; }

		T* operator+(size_t count)							{ return p + count; }
		T* operator+(int count)								{ return p + count; }

		// Probably shouldn't be using operator- on AutoArrayPointer, so don't provide it!
		
		T* operator->() const								{ return p;		}
		T& operator*() const								{ return *p;	}
		operator T*() const									{ return p; 	}
		
		const T& operator[](int i) const					{ return p[i];	}
		T& operator[](int i)								{ return p[i];	}
		const T& operator[](size_t i) const					{ return p[i];	}
		T& operator[](size_t i)								{ return p[i];	}

		T* GetPointer() const								{ return p; }
		T* RelinquishPointer()								{ T *r = p; p = nullptr; return r;	}
		void Release()										{ delete [] p; p = nullptr; }
		
	private:
		T*	p;
	};

//===========================================================================
	
	template<typename T, size_t N> class AutoArrayPointerWithInlineStore
	{
	public:
		AutoArrayPointerWithInlineStore()				{ p = store; }
		AutoArrayPointerWithInlineStore(size_t count)	{ p = (count <= N) ? store : new T[count]; }
		~AutoArrayPointerWithInlineStore()				{ if(p != store) delete [] p; }

		T* operator+(size_t count)						{ return p + count; }
		T* operator+(int count)							{ return p + count; }
		
		T* operator->() const							{ return p;		}
		T& operator*() const							{ return *p;	}
		operator T*() const								{ return p; 	}
		
		const T& operator[](int i) const				{ return p[i];	}
		T& operator[](int i)							{ return p[i];	}
		const T& operator[](size_t i) const				{ return p[i];	}
		T& operator[](size_t i)							{ return p[i];	}

		T* GetPointer() const							{ return p; }
		void Release()									{ if(p != store) delete [] p; p = store; }

	private:
		T*	p;
		T	store[N];
	};
	
//===========================================================================
} // namespace Javelin
//===========================================================================
