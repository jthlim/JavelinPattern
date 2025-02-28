//============================================================================

#pragma once

//============================================================================

namespace Javelin
{
//============================================================================
	
	template<typename T> class TableIterator
	{
	public:
		JINLINE TableIterator()			{ p = nullptr;	}
		JINLINE TableIterator(T* aP)	{ p = aP;		}

		template<typename X>
		JINLINE TableIterator(const TableIterator<X>& a)		{ p = a.p;	}
		
		JINLINE bool operator==(const TableIterator& a) const { return p == a.p; }
		JINLINE bool operator!=(const TableIterator& a) const { return p != a.p; }
		
		JINLINE T& operator*()	const						{ return *p; }
        JINLINE T* operator->() const						{ return p;	 }
	
        JINLINE TableIterator&	operator++()            { ++p; return *this; }
        JINLINE T*				operator++(int)         { return p++; }
        JINLINE TableIterator&	operator--()            { --p; return *this; }
        JINLINE T*				operator--(int)         { return p--; }
		
        JINLINE TableIterator&	operator+=(int i)       { p += i; return *this; }
        JINLINE TableIterator&	operator-=(int i)       { p -= i; return *this; }
        JINLINE TableIterator&	operator+=(size_t i)    { p += i; return *this; }
        JINLINE TableIterator&	operator-=(size_t i)    { p -= i; return *this; }
		
        JINLINE T*				operator+(int i)        { return p + i; }
        JINLINE T*				operator-(int i)        { return p - i; }
		
		JINLINE TableIterator&	operator=(const TableIterator& a) { p = a.p; return *this; }
		
		JINLINE T*				GetPointer() const		{ return p; }
		
	protected:
		T*	p;

		template<typename> friend class TableIterator;
	};
	
//============================================================================
}
//============================================================================
