//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Type/TypeData.h"
#include "Javelin/Template/Swap.h"
#include <memory.h>

//============================================================================

namespace Javelin
{
//============================================================================

#pragma pack(push, 1)
	
	template<typename T> struct NativeEndian
	{
	private:
		typedef typename TypeData<T>::MathematicalUpcast I;

	public:
		T	rawValue;

		NativeEndian() = default; 
		NativeEndian(I iData) : rawValue(iData) 				{ }
		constexpr NativeEndian(const NativeEndian& a) = default;
		constexpr NativeEndian(I iData, const RawInitialize*) :	rawValue(iData) { }
//		NativeEndian(const NativeEndian& a)						{ rawValue = a.rawValue;		}

		static constexpr NativeEndian Create(T value) 	{ return { value, RAW_INITIALIZE}; 	}
		
		I operator=(I a)								{ rawValue = a;	return a;				}
		operator I() const								{ return rawValue;						}
		NativeEndian& operator=(const NativeEndian& a) = default;
		
		NativeEndian& operator+=(I a)	{ rawValue += a;	return *this;	}
		NativeEndian& operator-=(I a)	{ rawValue -= a;	return *this;	}
		NativeEndian& operator*=(I a)	{ rawValue *= a;	return *this;	}
		NativeEndian& operator%=(I a)	{ rawValue %= a;	return *this;	}
		NativeEndian& operator/=(I a)	{ rawValue /= a;	return *this;	}
		NativeEndian& operator&=(I a)	{ rawValue &= a;	return *this;	}
		NativeEndian& operator|=(I a)	{ rawValue |= a;	return *this;	}
		NativeEndian& operator^=(I a)	{ rawValue ^= a;	return *this;	}
		NativeEndian& operator<<=(I a)	{ rawValue <<= a;	return *this;	}
		NativeEndian& operator>>=(I a)	{ rawValue >>= a;	return *this;	}

		// Post increment/decrement
		I operator++(int)				{ return rawValue++;	}
		I operator--(int)				{ return rawValue--;	}

		// Pre increment/decrement
		I operator++()					{ return ++rawValue;	}
		I operator--()					{ return --rawValue;	}
	};

//============================================================================

	template<int n> JINLINE void _ByteSwapHelper(unsigned char* c, int stm1)
	{
		Swap(c[n], c[stm1-n]);
		_ByteSwapHelper<n-1>(c, stm1);
	}
	template<> JINLINE void _ByteSwapHelper<0>(unsigned char* c, int stm1)
	{
		Swap(c[0], c[stm1]);
	}

	template<typename T> struct AlternateEndian
	{
	private:
		typedef typename TypeData<T>::MathematicalUpcast I;

	public:
		T	rawValue;

		JINLINE static I ByteSwap(I a)
		{
			union
			{
				T				x;
				unsigned char	c[sizeof(T)];
			} u;
			u.x = a;
			_ByteSwapHelper<sizeof(T)/2-1>(u.c, sizeof(T)-1);
			return u.x;
		}
		JINLINE static I	GetValue(const T* rawValue)		{ T v; memcpy(&v, rawValue, sizeof(v)); return ByteSwap(v);	}
		JINLINE static void SetValue(T* rawValue, I value)	{ T v = ByteSwap(value); memcpy(rawValue, &v, sizeof(v));	}

		AlternateEndian() = default;
		AlternateEndian(I iData)							{ SetValue(&rawValue, iData);	}
		constexpr AlternateEndian(const AlternateEndian& a) = default;
		constexpr AlternateEndian(I aValue, const RawInitialize*) : rawValue(aValue) { }

		static constexpr AlternateEndian Create(T value);

		I operator=(I a)									{ SetValue(&rawValue, a); return a;		}
		operator I() const									{ return GetValue(&rawValue);			}
		AlternateEndian& operator=(const AlternateEndian& a) = default;
		
		AlternateEndian& operator+=(I a)					{ *this = *this + a;	return *this;	}
		AlternateEndian& operator-=(I a)					{ *this = *this - a;	return *this;	}
		AlternateEndian& operator*=(I a)					{ *this = *this * a;	return *this;	}
		AlternateEndian& operator%=(I a)					{ *this = *this % a;	return *this;	}
		AlternateEndian& operator/=(I a)					{ *this = *this / a;	return *this;	}
		AlternateEndian& operator&=(I a)					{ *this = *this & a;	return *this;	}
		AlternateEndian& operator|=(I a)					{ *this = *this | a;	return *this;	}
		AlternateEndian& operator^=(I a)					{ *this = *this ^ a;	return *this;	}
		AlternateEndian& operator<<=(I a)					{ *this = *this << a;	return *this;	}
		AlternateEndian& operator>>=(I a)					{ *this = *this >> a;	return *this;	}

		AlternateEndian& operator&=(const AlternateEndian& a)	{ rawValue &= a.rawValue; return *this;	}
		AlternateEndian& operator|=(const AlternateEndian& a)	{ rawValue |= a.rawValue; return *this;	}
		AlternateEndian& operator^=(const AlternateEndian& a)	{ rawValue ^= a.rawValue; return *this;	}
		
		// Post increment/decrement
		I operator++(int)	{ I i = GetValue(&rawValue); SetValue(&rawValue, i+1); return i; }
		I operator--(int)	{ I i = GetValue(&rawValue); SetValue(&rawValue, i-1); return i; }

		// Pre increment/decrement
		I operator++()		{ I i = GetValue(&rawValue)+1; SetValue(&rawValue, i); return i; }
		I operator--()		{ I i = GetValue(&rawValue)-1; SetValue(&rawValue, i); return i; }
	};

#pragma pack(pop)
	
//============================================================================

#if defined(JCOMPILER_GCC) || defined(JCOMPILER_CLANG)

	template<typename RETURN_TYPE, typename STORE_TYPE, int N> struct BitSwap;
    template<> struct BitSwap<int, short, 2>                 { JINLINE static int Swap(int a) { return (short) __builtin_bswap16(a); } };
	template<typename R, typename S> struct BitSwap<R, S, 2> { JINLINE static R Swap(R a) { return __builtin_bswap16(a); } };
	template<typename R, typename S> struct BitSwap<R, S, 4> { JINLINE static R Swap(R a) { return __builtin_bswap32(a); } };
	template<typename R, typename S> struct BitSwap<R, S, 8> { JINLINE static R Swap(R a) { return __builtin_bswap64(a); } };
		
	template<> JINLINE int AlternateEndian<short>::ByteSwap(int a)												{ return BitSwap<int, short, sizeof(short)>::Swap(a);                           }
	template<> JINLINE unsigned AlternateEndian<unsigned short>::ByteSwap(unsigned a)							{ return BitSwap<unsigned, unsigned short, sizeof(short)>::Swap(a);             }
	template<> JINLINE int AlternateEndian<int>::ByteSwap(int a)												{ return BitSwap<int, int, sizeof(int)>::Swap(a);                               }
	template<> JINLINE unsigned AlternateEndian<unsigned>::ByteSwap(unsigned a)									{ return BitSwap<unsigned, unsigned, sizeof(unsigned)>::Swap(a);                }
	template<> JINLINE long AlternateEndian<long>::ByteSwap(long a)												{ return BitSwap<long, long, sizeof(long)>::Swap(a);                            }
	template<> JINLINE unsigned long AlternateEndian<unsigned long>::ByteSwap(unsigned long a)					{ return BitSwap<unsigned long, unsigned long, sizeof(unsigned long)>::Swap(a); }
	template<> JINLINE long long AlternateEndian<long long>::ByteSwap(long long a)								{ return BitSwap<long long, long long, sizeof(long long)>::Swap(a); 			}
	template<> JINLINE unsigned long long AlternateEndian<unsigned long long>::ByteSwap(unsigned long long a)	{ return BitSwap<unsigned long long, unsigned long long, sizeof(unsigned long long)>::Swap(a); 	}
	
	template<> constexpr AlternateEndian<unsigned short> AlternateEndian<unsigned short>::Create(unsigned short value)
	{
		return { (unsigned int) ((value & 0xff) << 8) | ((value & 0xff00) >> 8), RAW_INITIALIZE };
	}

	template<> constexpr AlternateEndian<signed short> AlternateEndian<signed short>::Create(signed short value)
	{
		return { ((value & 0xff) << 8) | ((value & 0xff00) >> 8), RAW_INITIALIZE };
	}

	template<> constexpr AlternateEndian<unsigned int> AlternateEndian<unsigned int>::Create(unsigned int value)
	{
		return { ((value & 0xff) << 24) | ((value & 0xff00) << 8) | ((value & 0xff0000) >> 8) | ((value & 0xff000000) >> 24), RAW_INITIALIZE };
	}
	
#elif defined(JASM_MSC_X86)
	template<> JINLINE float AlternateEndian<float>::ByteSwap(float f)
	{
		__asm
		{
			Mov		EAX, f
			BSwap	EAX
			Mov		f, EAX
		}
		return f;
	}

	template<> JINLINE int AlternateEndian<short>::ByteSwap(int a)
	{
		__asm
		{
			Mov		EAX, a
			BSwap	EAX
			SAR		EAX, 16
		}
	}
	
	template<> JINLINE unsigned AlternateEndian<unsigned short>::ByteSwap(unsigned a)
	{
		__asm
		{
			Mov		EAX, a
			BSwap	EAX
			ShR		EAX, 16
		}
	}

	template<> JINLINE int AlternateEndian<int>::ByteSwap(int a)
	{
		__asm
		{
			Mov		EAX, a
			BSwap	EAX
		}
	}

	template<> JINLINE unsigned AlternateEndian<unsigned>::ByteSwap(unsigned a)
	{
		__asm
		{
			Mov		EAX, a
			BSwap	EAX
		}
	}

	template<> JINLINE long AlternateEndian<long>::ByteSwap(long a)
	{
		__asm
		{
			Mov		EAX, a
			BSwap	EAX
		}
	}
	
	template<> JINLINE unsigned long AlternateEndian<unsigned long>::ByteSwap(unsigned long a)
	{
		__asm
		{
			Mov		EAX, a
			BSwap	EAX
		}
	}	
#elif defined(JASM_GNUC_X86)
	template<> JINLINE int AlternateEndian<short>::ByteSwap(int a)
	{
		register int returnValue = a;
		asm("bswap  %0\n"
			"sar	$16, %0\n"
			: "+r" (returnValue)
			);
		return returnValue;
	}
	
	template<> JINLINE unsigned AlternateEndian<unsigned short>::ByteSwap(unsigned a)
	{
		register unsigned returnValue = a;
		asm("bswap  %0\n"
			"shr	$16, %0\n"
			: "+r" (returnValue)
			);
		return returnValue;
	}

	template<> JINLINE int AlternateEndian<int>::ByteSwap(int a)
	{
		register int returnValue = a;
		asm("bswap  %0\n"
			: "+r" (returnValue)
			);
		return returnValue;
	}

	template<> JINLINE unsigned AlternateEndian<unsigned>::ByteSwap(unsigned a)
	{
		register unsigned returnValue = a;
		asm("bswap  %0\n"
			: "+r" (returnValue)
			);
		return returnValue;
	}

	template<> JINLINE long AlternateEndian<long>::ByteSwap(long a)
	{
		register long returnValue = a;
		asm("bswap  %0\n"
			: "+r" (returnValue)
			);
		return returnValue;
	}
	
	template<> JINLINE unsigned long AlternateEndian<unsigned long>::ByteSwap(unsigned long a)
	{
		register unsigned long returnValue = a;
		asm("bswap  %0\n"
			: "+r" (returnValue)
			);
		return returnValue;
	}	
#elif defined(JASM_GNUC_X86_64)
	template<> JINLINE int AlternateEndian<short>::ByteSwap(int a)
	{
		register int returnValue = a;
		asm("bswapl  %0\n"
			"sarl	$16, %0\n"
			: "+r" (returnValue)
			);
		return returnValue;
	}
	
	template<> JINLINE unsigned AlternateEndian<unsigned short>::ByteSwap(unsigned a)
	{
		register unsigned returnValue = a;
		asm("bswapl  %0\n"
			"shrl	$16, %0\n"
			: "+r" (returnValue)
			);
		return returnValue;
	}

	template<> JINLINE int AlternateEndian<int>::ByteSwap(int a)
	{
		register int returnValue = a;
		asm("bswapl  %0\n"
			: "+r" (returnValue)
			);
		return returnValue;
	}

	template<> JINLINE unsigned AlternateEndian<unsigned>::ByteSwap(unsigned a)
	{
		register unsigned returnValue = a;
		asm("bswapl  %0\n"
			: "+r" (returnValue)
			);
		return returnValue;
	}

	template<> JINLINE long AlternateEndian<long>::ByteSwap(long a)
	{
		register long returnValue = a;
		asm("bswapq  %0\n"
			: "+r" (returnValue)
			);
		return returnValue;
	}
	
	template<> JINLINE unsigned long AlternateEndian<unsigned long>::ByteSwap(unsigned long a)
	{
		register unsigned long returnValue = a;
		asm("bswapq  %0\n"
			: "+r" (returnValue)
			);
		return returnValue;
	}
	
#elif defined(JASM_GNUC_PPC) || defined(JASM_GNUC_PPC64)
	template<> JINLINE int AlternateEndian<short>::GetValue(const short* data)
	{
		register int returnValue;
		asm volatile("lhbrx %0, 0, %1\n"
					 "slwi  %0, %0, 16\n"
					 : "=r" (returnValue)
					 : "r" (data)
					 );
		return returnValue >> 16;
	}
	
	template<> JINLINE void AlternateEndian<short>::SetValue(short *data, int value)
	{
		asm volatile("sthbrx %0, 0, %1\n"
					 : // No outputs
					 : "r" (value), "r" (data)
					 : "memory"
					 );
	}
	
	template<> JINLINE unsigned AlternateEndian<unsigned short>::GetValue(const unsigned short* data)
	{
		register unsigned returnValue;
		asm volatile("lhbrx %0, 0, %1\n"
					 : "=r" (returnValue)
					 : "r" (data)
					 );
		return returnValue;
	}
	
	template<> JINLINE void AlternateEndian<unsigned short>::SetValue(unsigned short *data, unsigned value)
	{
		asm volatile("sthbrx %0, 0, %1\n"
					 : // No outputs
					 : "r" (value), "r" (data)
					 : "memory"
					 );
	}

	template<> JINLINE int AlternateEndian<int>::GetValue(const int* data)
	{
		register int returnValue;
		asm volatile("lwbrx %0, 0, %1\n"
					 : "=r" (returnValue)
					 : "r" (data)
					 );
		return returnValue;
	}

	template<> JINLINE void AlternateEndian<int>::SetValue(int *data, int value)
	{
		asm volatile("stwbrx %0, 0, %1\n"
					 : // No outputs
					 : "r" (value), "r" (data)
					 : "memory"
					 );
	}

	template<> JINLINE unsigned AlternateEndian<unsigned>::GetValue(const unsigned* data)
	{
		register unsigned returnValue;
		asm volatile("lwbrx %0, 0, %1\n"
					 : "=r" (returnValue)
					 : "r" (data)
					 );
		return returnValue;
	}

	template<> JINLINE void AlternateEndian<unsigned>::SetValue(unsigned *data, unsigned value)
	{
		asm volatile("stwbrx %0, 0, %1\n"
					 : // No outputs
					 : "r" (value), "r" (data)
					 : "memory"
					 );
	}

	template<> JINLINE long AlternateEndian<long>::GetValue(const long* data)
	{
		register long returnValue;
		asm volatile("lwbrx %0, 0, %1\n"
					 : "=r" (returnValue)
					 : "r" (data)
					 );
		return returnValue;
	}
	
	template<> JINLINE void AlternateEndian<long>::SetValue(long *data, long value)
	{
		asm volatile("stwbrx %0, 0, %1\n"
					 : // No outputs
					 : "r" (value), "r" (data)
					 : "memory"
					 );
	}
	
	template<> JINLINE unsigned long AlternateEndian<unsigned long>::GetValue(const unsigned long* data)
	{
		register unsigned long returnValue;
		asm volatile("lwbrx %0, 0, %1\n"
					 : "=r" (returnValue)
					 : "r" (data)
					 );
		return returnValue;
	}
	
	template<> JINLINE void AlternateEndian<unsigned long>::SetValue(unsigned long *data, unsigned long value)
	{
		asm volatile("stwbrx %0, 0, %1\n"
					 : // No outputs
					 : "r" (value), "r" (data)
					 : "memory"
					 );
	}
#else
	#if !defined(JASM_NONE)
		#warning "No platform specific AlternateEndian ByteSwap defined"
	#endif

	template<> JINLINE int AlternateEndian<short>::ByteSwap(int a)								{ int i = ((a<<24) & 0xFF000000) | ((a<<8) & 0xFF0000); return i >> 16; }
	template<> JINLINE unsigned AlternateEndian<unsigned short>::ByteSwap(unsigned a)			{ return ((a<<8) & 0xFF00) | ((a>>8) & 0xFF);	}
	template<> JINLINE int AlternateEndian<int>::ByteSwap(int a)								{ return ((a<<24) & 0xFF000000) | ((a << 8) & 0xFF0000) | ((a >> 8) & 0xFF00) | ((a >> 24) & 0xFF);	}
	template<> JINLINE unsigned AlternateEndian<unsigned>::ByteSwap(unsigned a)					{ return ((a<<24) & 0xFF000000) | ((a << 8) & 0xFF0000) | ((a >> 8) & 0xFF00) | ((a >> 24) & 0xFF);	}
	template<> JINLINE long AlternateEndian<long>::ByteSwap(long a)								{ return ((a<<24) & 0xFF000000) | ((a << 8) & 0xFF0000) | ((a >> 8) & 0xFF00) | ((a >> 24) & 0xFF);	}
	template<> JINLINE unsigned long AlternateEndian<unsigned long>::ByteSwap(unsigned long a)	{ return ((a<<24) & 0xFF000000) | ((a << 8) & 0xFF0000) | ((a >> 8) & 0xFF00) | ((a >> 24) & 0xFF);	}
#endif

//============================================================================
	
#if defined(JENDIAN_LITTLE)
	template<typename T> struct LittleEndian : public NativeEndian<T>
	{
		using NativeEndian<T>::NativeEndian;
		using NativeEndian<T>::operator=;

		static constexpr LittleEndian Create(T value) 	{ return { value, RAW_INITIALIZE}; 	}
	};

	template<typename T> struct BigEndian : public AlternateEndian<T>
	{
		using AlternateEndian<T>::AlternateEndian;
		using AlternateEndian<T>::operator=;

		static constexpr BigEndian Create(T value) { return { AlternateEndian<T>::Create(value).rawValue, RAW_INITIALIZE }; }
	};
#elif defined(JENDIAN_BIG)
	template<typename T> struct LittleEndian : public AlternateEndian<T>
	{
		using AlternateEndian<T>::AlternateEndian;
		using AlternateEndian<T>::operator=;
		
		static constexpr LittleEndian Create(T value) { return { AlternateEndian<T>::Create(value).rawValue, RAW_INITIALIZE }; }
	};
	
	template<typename T> struct BigEndian : public NativeEndian<T>
	{
		using NativeEndian<T>::NativeEndian;
		using NativeEndian<T>::operator=;

		static constexpr BigEndian Create(T value) 	{ return { value, RAW_INITIALIZE}; 	}
	};
#else
	#error "Fatal error - Endian not defined!"
#endif
	
//============================================================================

	template<typename T> struct TypeData< NativeEndian<T> > : public TypeData<T>	{ };
	template<typename T> struct TypeData< AlternateEndian<T> > : public TypeData<T>	{ };

//============================================================================
} // namespace Javelin
//============================================================================
