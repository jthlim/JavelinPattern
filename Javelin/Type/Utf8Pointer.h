//===========================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Data/Utf8DecodeTable.h"
#include "Javelin/Type/Character.h"
#include "Javelin/Type/Utf8Character.h"

//===========================================================================

namespace Javelin
{
//===========================================================================
	
	class Utf8Pointer
	{
	private:
		// This is required because writing *p++ = x needs to advance p after
		// the write has occurred to *p since there can be a varying number
		// of bytes required to store x
		class PostIncrementHelper
		{
		public:
			PostIncrementHelper(Utf8Pointer& aP) : active(true), p(aP) { }
			PostIncrementHelper(const PostIncrementHelper& a) = delete;
			PostIncrementHelper(PostIncrementHelper&& a) : active(true), p(a.p)  { a.active = false; }
			~PostIncrementHelper() 								{ if(active) ++p; }
			
			JINLINE Utf8Character& operator*()	const			{ return *p; }
			JINLINE operator Utf8Pointer&() 	const 			{ return p; }
			JINLINE Utf8Character* operator->()					{ return (Utf8Character*) p.p;	}
			
		private:
			bool 			active;
			Utf8Pointer& 	p;
		};
		
	public:
		Utf8Pointer()									{ p = nullptr;				}
		Utf8Pointer(const Utf8Character *ip)			{ p = ip;					}
		explicit Utf8Pointer(const char *ip)			{ p = (const Utf8Character*) ip;	}
		explicit Utf8Pointer(const unsigned char *ip)	{ p = (const Utf8Character*) ip;	}

		// Tests if the pointer is a valid UTF8 stream
		JEXPORT bool JCALL IsValid()		const;

		JEXPORT size_t JCALL GetNumberOfCharacters()	const;

		JINLINE Utf8Pointer& operator+=(int n)					{ if(n >= 0) SeekForwards(n); else SeekBackwards(-n); return *this; }
		JINLINE Utf8Pointer& operator-=(int n)					{ if(n >= 0) SeekBackwards(n); else SeekForwards(-n); return *this; }
		JINLINE Utf8Pointer& operator+=(size_t n)				{ SeekForwards(n); return *this; }
		JINLINE Utf8Pointer& operator-=(size_t n)				{ SeekBackwards(n); return *this; }
		
		JINLINE Utf8Pointer JCALL operator+(int n) const		{ Utf8Pointer r(*this); r += n; return r; }
		JINLINE Utf8Pointer JCALL operator-(int n) const		{ Utf8Pointer r(*this); r -= n; return r; }
		JINLINE Utf8Pointer JCALL operator+(size_t n) const		{ Utf8Pointer r(*this); r += n; return r; }
		JINLINE Utf8Pointer JCALL operator-(size_t n) const		{ Utf8Pointer r(*this); r -= n; return r; }

		JINLINE PostIncrementHelper JCALL operator++(int)		{ return *this; }
		JINLINE Utf8Pointer JCALL operator--(int)				{ Utf8Pointer r{*this}; operator--(); return r; }

		// Pre increment/decrement operators
		JINLINE Utf8Pointer& JCALL operator++()					{ OffsetByByteCount(p->GetNumberOfBytes()); return *this; }
		JEXPORT Utf8Pointer& JCALL operator--();

		size_t 		GetNumberOfBytes() const					{ return strlen((const char*) p); }
		Utf8Pointer	GetPointerToEndOfString() const				{ return Utf8Pointer((const char*) p + GetNumberOfBytes()); }
		
		void OffsetByByteCount(ssize_t byteCount)				{
																	const unsigned char *process = reinterpret_cast<const unsigned char*>(p);
																	p = reinterpret_cast<const Utf8Character*>(process + byteCount);
																}
		char* GetCharPointer() 									{ return (char*) p;				}
		const char* GetCharPointer() const						{ return (const char*) p;		}
		JINLINE Utf8Character& operator*() 						{ return (Utf8Character&) *p;	}
		JINLINE const Utf8Character& operator*() const			{ return *p;					}
		JINLINE Utf8Character* operator->()						{ return (Utf8Character*) p;	}
		JINLINE const Utf8Character* operator->() const			{ return p;						}
		JINLINE Character JCALL operator[](int n) const			{ return *(*this+n);			}
		
		friend size_t operator-(const Utf8Pointer& a, const Utf8Pointer& b);
		
		JINLINE bool friend operator==(const Utf8Pointer& a, const Utf8Pointer& b)	{ return a.p == b.p; }
		JINLINE bool friend operator!=(const Utf8Pointer& a, const Utf8Pointer& b)	{ return a.p != b.p; }
		JINLINE bool friend operator<(const Utf8Pointer& a, const Utf8Pointer& b)	{ return a.p <  b.p; }
		JINLINE bool friend operator<=(const Utf8Pointer& a, const Utf8Pointer& b)	{ return a.p <= b.p; }
		JINLINE bool friend operator>(const Utf8Pointer& a, const Utf8Pointer& b)	{ return a.p >  b.p; }
		JINLINE bool friend operator>=(const Utf8Pointer& a, const Utf8Pointer& b)	{ return a.p >= b.p; }

	private:
		const Utf8Character *p;

		JEXPORT void JCALL SeekForwards(size_t n);
		JEXPORT void JCALL SeekBackwards(size_t n);
	};

//===========================================================================
} // namespace Javelin
//===========================================================================
