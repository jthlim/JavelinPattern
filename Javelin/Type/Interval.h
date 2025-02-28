//============================================================================
//
// There are two ways of representing an interval
//
//  min, max
//  midPoint, extent
//
// This class uses min/max representation as it can accurately represent
// integer intervals
//
//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Template/Utility.h"
#include "Javelin/Type/TypeData.h"

//============================================================================

namespace Javelin
{
//============================================================================

	template<typename T, bool MAX_INCLUSIVE = TypeData<T>::IS_FLOAT> class Interval
	{
	public:
		T	min;
		T	max;

		// Constructors
		Interval() { }
		constexpr Interval(const T &iMin, const T &iMax) : min(iMin), max(iMax)	{ }

		// Using false means an empty interval. Using true means spanning the entire interval
		explicit constexpr Interval(bool span) : min(span ? TypeData<T>::Minimum() : TypeData<T>::Zero()), max(span ? TypeData<T>::Maximum() : TypeData<T>::Zero()) { }

		JINLINE void JCALL SetEmpty()										{ max = min;							}
		JINLINE bool JCALL IsEmpty() const									{ return min == max;					}

		JINLINE void JCALL Set(const T &iMin, const T &iMax) 				{ min = iMin; max = iMax;				}
		JINLINE void JCALL SetAll() 										{ min = TypeData<T>::Minimum(); max = TypeData<T>::Maximum(); }
		JINLINE void JCALL SetMinMax(const T& value)						{ min = value; max = value;				}
		JINLINE void JCALL SetInvalid();

		typename TypeData<T>::DeltaType GetSize() const						{ return max - min;						}
		void SetSize(const typename TypeData<T>::DeltaType &a)				{ max = min + a;						}

		// For pointers
		T GetData() const													{ return min;							}
		size_t GetNumberOfBytes() const										{ return size_t(max) - size_t(min);		}

		typename TypeData<T>::MathematicalUpcast GetMidPoint() const		{ return (min+max) / 2;					}

		// Comparison operators
		bool operator==(const Interval &a) const							{ return min == a.min && max == a.max;	}
		bool operator!=(const Interval &a) const							{ return min != a.min || max != a.max;  }
		
		// Translation operators
		template<typename U> JINLINE Interval operator+(const U &a) const	{ return { min+a, max+a };				}
		template<typename U> JINLINE Interval operator-(const U &a) const	{ return { min-a, max-a };				}
		JINLINE friend Interval operator+(const T &a, const Interval &b)	{ return { a+b.min, a+b.max };			}
		JINLINE friend Interval operator-(const T &a, const Interval &b)	{ return { a-b.min, a-b.max };			}
		JINLINE Interval& operator+=(const T &a)							{ *this = *this + a; return *this;		}
		JINLINE Interval& operator-=(const T &a)							{ *this = *this - a; return *this;		}

		// Scale operators
		template<typename S> JINLINE Interval operator*(S a) const			{ return { min*a, max*a };				}
		template<typename S> JINLINE Interval operator/(S a) const			{ return { min/a, max/a };				}
							 JINLINE Interval operator/(float a) const		{ return (*this) * (1.0f / a);			}

		template<typename S> 
			JINLINE friend Interval operator*(S a, const Interval &b)		{ return { a*b.min, a*b.max };			}
		template<typename S> JINLINE Interval& operator*=(S a)				{ min *= a; max *= a; return *this;	}
		template<typename S> JINLINE Interval& operator/=(S a)				{ min /= a; max /= a; return *this;	}
							 JINLINE Interval& operator/=(float a)			{ return (*this) *= (1.0f / a);			}

		template<typename S> void ScaleWithPivot(S a, const T& pivot)		{ T size = max-min; min = (1-a)*pivot + a*min; max = a*size+min; }
		
		bool Contains(const Interval &a) const								{ return min <= a.min && a.max <= max;	}
		bool Contains(const T &a) const										{ return (*this) % a;					}

		bool IsValid() const;

		T Clamp(T a) const													{ return a < min ? min : a > max ? max : a; }
			
		// Union of two intervals
		Interval	operator|(const Interval &a) const;
		Interval&	operator|=(const Interval &a);
		Interval&	operator|=(const T &a);

		// This will even take the union of empty intervals
		friend Interval Union(const Interval& a, const Interval& b)			{ return { Minimum(a.min, b.min), Maximum(a.max, b.max) };	}

		// Intersection of two intervals
		Interval	operator&(const Interval &a) const;
		Interval&	operator&=(const Interval &a);

		// Array access operators
		const T& operator[](int n) const									{ return (&min)[n];						}
			  T& operator[](int n)											{ return (&min)[n];						}

		// Tests for overlap
		inline bool operator%(const Interval &b) const;

		// Tests if the value lies in the interval
		inline bool operator%(const T &b) const;
		inline bool operator&(const T &b) const								{ return (*this) % b;					}

		friend inline bool operator%(const T &b, const Interval &i)			{ return i%b;							}
		friend inline bool operator&(const T &b, const Interval &i)			{ return i%b;							}

		// Interval retargetting methods.
		// eg. Used to convert a pixel on a screen <-> time in TimeInterval
		T ConvertValueToRange(const T& value, const Interval& newRange)		{ return (value-min) * newRange.GetSize() / GetSize() + newRange.min; 	}
		T ConvertValueToRange(const T& value, const T& max)					{ return (value-min) * max / GetSize(); 								}
		T ConvertValueFromRange(const T& value, const Interval& oldRange)	{ return (value-oldRange.min) * GetSize() / oldRange.GetSize() + min; 	}
		T ConvertValueFromRange(const T& value, const T& oldMax)			{ return value * GetSize() / oldMax + min;								}
		
		// Iterator support for intervals with types that define ++
		T Begin()		{ return min; }
		T Begin() const { return min; }
		
		JINLINE T GetEnd() const;
			
		T End()			{ return GetEnd(); }
		T End() const	{ return GetEnd(); }
		
		JINLINE bool ApproximateEquals(const Interval& a, float ACCEPTABLE_ERROR = 0.001f) const 	{ return min.ApproximateEquals(a.min) && max.ApproximateEquals(a.max); }
	};

//============================================================================
//============================================================================

	namespace Private
	{
		template<typename T, bool MAX_INCLUSIVE> class IntervalHelper;
		template<typename T> class IntervalHelper<T, false>
		{
		public:
			static inline bool Test(const Interval<T, false>& a, const T& b)					{ return a.min <= b && b < a.max;	}
			// Intersects if max(min, b.min) < min(Right, b.Right)
			JINLINE static bool Test(const Interval<T, false>& a, const Interval<T, false>& b)	{ return a.min < b.max && b.min < a.max;	}
			JINLINE static bool IsValid(const Interval<T, false>& a)							{ return a.min < a.max; }
			JINLINE static T GetEnd(const Interval<T, false>& a) 								{ return a.max; }
		};
		
		template<typename T> class IntervalHelper<T, true>
		{
		public:
			JINLINE static bool Test(const Interval<T, true>& a, const T& b)					{ return a.min <= b && b <= a.max;	}
			// Intersects if max(min, b.min) <= min(Right, b.Right)
			JINLINE static bool Test(const Interval<T, true>& a, const Interval<T, true>& b)	{ return a.min <= b.max && b.min <= a.max;	}
			JINLINE static bool IsValid(const Interval<T, true>& a)								{ return a.min <= a.max; }
			JINLINE static T GetEnd(const Interval<T, false>& a) 								{ return a.max+1; 		 }
		};
	}

	template<typename T, bool MAX_INCLUSIVE> inline bool Interval<T, MAX_INCLUSIVE>::operator%(const Interval<T, MAX_INCLUSIVE> &b) const
	{
		return Private::IntervalHelper<T, MAX_INCLUSIVE>::Test(*this, b);
	}

	template<typename T, bool MAX_INCLUSIVE> inline bool Interval<T, MAX_INCLUSIVE>::operator%(const T &b) const
	{
		return Private::IntervalHelper<T, MAX_INCLUSIVE>::Test(*this, b);
	}

	template<typename T, bool MAX_INCLUSIVE> inline bool Interval<T, MAX_INCLUSIVE>::IsValid() const
	{
		return Private::IntervalHelper<T, MAX_INCLUSIVE>::IsValid(*this);
	}

	template<typename T, bool MAX_INCLUSIVE> inline T Interval<T, MAX_INCLUSIVE>::GetEnd() const
	{
		return Private::IntervalHelper<T, MAX_INCLUSIVE>::GetEnd(*this);
	}

//============================================================================
// Union operators

	template<typename T, bool MAX_INCLUSIVE> Interval<T, MAX_INCLUSIVE> Interval<T, MAX_INCLUSIVE>::operator|(const Interval<T, MAX_INCLUSIVE> &a) const
	{
		if(IsEmpty()) return a;
		if(a.IsEmpty()) return (*this);

		return Interval<T, MAX_INCLUSIVE>( Minimum(min, a.min), Maximum(max, a.max) );
	}

	template<typename T, bool MAX_INCLUSIVE> Interval<T, MAX_INCLUSIVE>& Interval<T, MAX_INCLUSIVE>::operator|=(const Interval<T, MAX_INCLUSIVE> &a)
	{
		if(!a.IsEmpty())
		{
			if(IsEmpty()) (*this) = a;
			else
			{
				SetMinimum(min, a.min);
				SetMaximum(max, a.max);
			}
		}
		return *this;
	}

	template<typename T, bool MAX_INCLUSIVE> Interval<T, MAX_INCLUSIVE>& Interval<T, MAX_INCLUSIVE>::operator|=(const T &a)
	{
		if(IsEmpty())
		{
			min = a;
			max = a;
		}
		else 
		{
			SetMinimum(min, a);
			SetMaximum(max, a);
		}
		return *this;
	}
	
//============================================================================
// Intersection operators

	template<typename T, bool MAX_INCLUSIVE> Interval<T, MAX_INCLUSIVE> Interval<T, MAX_INCLUSIVE>::operator&(const Interval<T, MAX_INCLUSIVE> &a) const
	{
		if(!IsValid())   return *this;
		if(!a.IsValid()) return a;

		Interval<T, MAX_INCLUSIVE> newInterval;
		newInterval.min = Maximum(min, a.min);
		newInterval.max = Minimum(max, a.max);

		return newInterval;
	}

	template<typename T, bool MAX_INCLUSIVE> Interval<T, MAX_INCLUSIVE>& Interval<T, MAX_INCLUSIVE>::operator&=(const Interval<T, MAX_INCLUSIVE> &a)
	{
		if(IsValid())
		{
			if(a.IsValid())
			{
				SetMaximum(min, a.min);
				SetMinimum(max, a.max);
			}
			else
			{
				*this = a;
			}
		}
		return *this;
	}

//============================================================================

	// Override the float case.
	template<> JINLINE bool Interval<float>::IsEmpty() const
	{
		return *reinterpret_cast<const unsigned*>(&max) == TypeData<unsigned>::Maximum();
	}
	
	template<> JINLINE void Interval<float>::SetEmpty()
	{
		*reinterpret_cast<unsigned*>(&max) = TypeData<unsigned>::Maximum();
	}

	// Floating Point optimization overrides
	template<> template<typename S> Interval<float> Interval<float>::operator/(S a) const
	{
		return (*this) * (1.0f / a);
	}

	template<> template<typename S> Interval<float>& Interval<float>::operator/=(S a)
	{
		return (*this) *= (1.0f / a);
	}

//============================================================================
	
	template<typename T, bool b> struct TypeData< Interval<T, b> >
	{
		static constexpr Interval<T, b> Zero()	{ return Interval<T,b>(TypeData<T>::Zero(), TypeData<T>::Zero());	}
		
		enum { IS_POD				= TypeData<T>::IS_POD				};
		enum { IS_BITWISE_COPY_SAFE	= TypeData<T>::IS_BITWISE_COPY_SAFE };
		enum { IS_POINTER			= false								};

		typedef Interval<T, b> NonPointerType;
		typedef const Interval<T,b>& ParameterType;
	};

//============================================================================

	template<> JINLINE void Interval<float>::SetInvalid()
	{
		min = NAN;
		max = NAN;
	}

//============================================================================
} // namespace Javelin
//============================================================================
