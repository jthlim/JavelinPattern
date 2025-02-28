//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Math/HashFunctions.h"
#include "Javelin/Template/Sequence.h"
#include "Javelin/Stream/IReader.h"
#include "Javelin/Type/Function.h"
#include "Javelin/Type/TypeData.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	class IReader;
	class IWriter;
	
//============================================================================

	namespace Private
	{
		template<int n, typename T> struct TupleTypeHelper	{ typedef typename TupleTypeHelper<n-1, typename T::Inherited>::Type Type; };
		template<typename T> struct TupleTypeHelper<0, T>	{ typedef typename T::ValueType Type; };
		
		template<int n, typename RETURN, typename T> struct TupleValueHelper
		{
			JINLINE static RETURN Get(T* a) { return TupleValueHelper<n-1, RETURN, typename T::Inherited>::Get(a); }
		};
		
		template<typename RETURN, typename T> struct TupleValueHelper<0, RETURN, T>
		{
			JINLINE static RETURN Get(T* a) { return a->GetValue(); }
		};

		template<typename FUNCTION, typename P, int ...S>
		JINLINE auto CallFunction(FUNCTION&& function, const P& p, Sequence<S...>) -> decltype(function( p.template Get<S>()... ))
		{
			return function( p.template Get<S>()... );
		}

		template<typename T, typename FUNCTION, typename P, int ...S>
		JINLINE auto CallMethod(T* obj, FUNCTION&& function, const P& p, Sequence<S...>) -> decltype((obj->*function)( p.template Get<S>()... ))
		{
			return (obj->*function)( p.template Get<S>()... );
		}
	}
	
//============================================================================

	template<typename... T> class Tuple;
	
	template<> class Tuple<>
	{
	public:
		JINLINE Tuple() = default;
		JINLINE Tuple(IReader&) { }
		
		JINLINE Tuple operator+(const Tuple& b) const 									{ return {}; }
		JINLINE Tuple operator-(const Tuple& b) const 									{ return {}; }
		JINLINE Tuple operator*(const Tuple& b) const 									{ return {}; }
		JINLINE Tuple operator/(const Tuple& b) const 									{ return {}; }
		
		template<typename A> JINLINE Tuple operator*(const A& k) const 					{ return {}; }
		template<typename A> JINLINE friend Tuple operator*(const A& k, const Tuple& b) { return {}; }
		
		JINLINE friend size_t GetHash(const Tuple&) 			{ return 0; }
		JINLINE static size_t GetNumberOfElements()				{ return 0; }
		JINLINE void SetAll()									{ }

		template<typename A> JINLINE const A& Get() const;

		JINLINE bool operator==(const Tuple& other) const		{ return true;	}
		JINLINE bool operator!=(const Tuple& other) const		{ return false; }
		JINLINE bool operator<(const Tuple& other) const		{ return false; }
		JINLINE bool operator<=(const Tuple& other) const		{ return true;	}
		JINLINE bool operator>(const Tuple& other) const		{ return false; }
		JINLINE bool operator>=(const Tuple& other) const		{ return true;	}

		JINLINE friend IWriter& operator<<(IWriter& output, const Tuple&)		{ return output; }

		template<typename FUNCTION>
		JINLINE auto Call(FUNCTION&& function) const -> decltype(function())	{ return function(); }
		
		const void* GetData() const { return this; }
		
	protected:
		template<typename T> static constexpr bool HasType() 	{ return false; }
	};
	
	template<typename T, typename... U> class Tuple<T, U...> : public Tuple<U...>
	{
	public:
		typedef T			ValueType;
		typedef Tuple<U...> Inherited;
		
	protected:
		template<typename T2> static constexpr bool HasType() 	{ return Helper<T2>::HasType(); }
		
	private:
		JINLINE Tuple(typename TypeData<T>::ParameterType aA, const Inherited& u) : Inherited(u), a(aA) { }
		
		template<typename T2, typename DUMMY=void> struct Helper
		{
			static const T2& Get(const Tuple& t)				{ return t.Inherited::template Get<T2>();	}
			static void Set(Tuple& t, const T2& v)				{ return t.Inherited::template Set<T2>(v);	}
			static constexpr bool HasType()						{ return Inherited::template HasType<T2>();	}
		};

		template<typename DUMMY> struct Helper<T, DUMMY>
		{
			static const T& Get(const Tuple& t)
			{
				static_assert(Inherited::template HasType<T>() == false, "More that one member of specified type");
				return t.a;
			}

			static void Set(Tuple& t, const T& a)
			{
				static_assert(Inherited::template HasType<T>() == false, "More that one member of specified type");
				t.a = a;
			}

			static constexpr bool HasType()
			{
				return true;
			}
		};

	public:
		JINLINE Tuple() = default;
		JINLINE Tuple(typename TypeData<T>::ParameterType aA, typename TypeData<U>::ParameterType... u) : Inherited(u...), a(aA) { }
		Tuple(IReader& input) : Inherited(input), a(input.ReadValue<T>()) { }

		// Get/Set by index.
		template<int n> struct ElementType													{ typedef typename Private::TupleTypeHelper<n, Tuple>::Type Type; };
		template<int n> JINLINE typename ElementType<n>::Type& Get()						{ return Private::TupleValueHelper<n, typename ElementType<n>::Type&, Tuple>::Get(this); }
		template<int n> JINLINE const typename ElementType<n>::Type& Get() const			{ return Private::TupleValueHelper<n, const typename ElementType<n>::Type&, Tuple>::Get(const_cast<Tuple*>(this)); }
		template<int n> JINLINE void Set(const typename ElementType<n>::Type& value)		{ Private::TupleValueHelper<n, typename ElementType<n>::Type&, Tuple>::Get(this) = value; }

		// Get/Set by type
		template<typename T2> JINLINE const T2& Get() const									{ return Helper<T2>::Get(*this); 	}
		template<typename T2> JINLINE void Set(const T2& a) 								{ Helper<T2>::Set(*this, a); 		}

		// Set full Tuple
		template<typename T2, typename... U2> JINLINE void SetAll(T2&& aA, U2&&... aB)		{ a = aA; Inherited::SetAll(aB...); }

		JINLINE Tuple operator+(const Tuple& b) const										{ return {a+b.a, *(const Inherited*) this + (const Inherited&) b}; }
		JINLINE Tuple operator-(const Tuple& b) const										{ return {a-b.a, *(const Inherited*) this - (const Inherited&) b}; }
		JINLINE Tuple operator*(const Tuple& b) const										{ return {a*b.a, *(const Inherited*) this * (const Inherited&) b}; }
		JINLINE Tuple operator/(const Tuple& b) const										{ return {a/b.a, *(const Inherited*) this / (const Inherited&) b}; }

		template<typename A> JINLINE Tuple operator*(const A& k) const						{ return {a*k, *(const Inherited*) this * k};	}
		template<typename A> JINLINE friend Tuple operator*(const A& k, const Tuple& b)		{ return {k*b.a, k * (const Inherited&) b}; 	}
		
		JINLINE static size_t GetNumberOfBytes()  											{ return sizeof(Tuple); 	}
		JINLINE static size_t GetNumberOfElements()											{ return 1+sizeof...(U); 	}
		
		JINLINE friend size_t GetHash(const Tuple& t) 										{ return GetHash(t.a) ^ GetHash((const Inherited&) t); }
		
		JINLINE T& GetValue()				{ return a; }
		JINLINE const T& GetValue() const	{ return a; }
	
		template<typename FUNCTION>
		JINLINE auto Call(FUNCTION&& function) const
			-> decltype(Private::CallFunction(function, *this, typename CreateSequence<1+sizeof...(U)>::Type()))
			{
				return Private::CallFunction(function, *this, typename CreateSequence<1+sizeof...(U)>::Type());
			}

		template<typename OBJ, typename FUNCTION>
		JINLINE auto Call(OBJ* obj, FUNCTION&& function) const
			-> decltype(Private::CallMethod(obj, function, *this, typename CreateSequence<1+sizeof...(U)>::Type()))
			{
				return Private::CallMethod(obj, function, *this, typename CreateSequence<1+sizeof...(U)>::Type());
			}

		friend IWriter& operator<<(IWriter& output, const Tuple& t)	{ output << (const Inherited&) t << t.a; return output; }
		
		JINLINE bool operator==(const Tuple& other) const		{ return a == other.a && Inherited::operator==(other); }
		JINLINE bool operator!=(const Tuple& other) const		{ return a != other.a || Inherited::operator!=(other); }
		JINLINE bool operator<(const Tuple& other) const		{ if(a != other.a) return a < other.a; else return Inherited::operator<(other); }
		JINLINE bool operator<=(const Tuple& other) const		{ if(a != other.a) return a < other.a; else return Inherited::operator<=(other); }
		JINLINE bool operator>(const Tuple& other) const		{ if(a != other.a) return a > other.a; else return Inherited::operator>(other); }
		JINLINE bool operator>=(const Tuple& other) const		{ if(a != other.a) return a > other.a; else return Inherited::operator>=(other); }
		
	private:
		T	a;
	};

//============================================================================

	template<typename... T> String ToStringHelper(const Tuple<T...>&);
	
	template<> JINLINE String ToStringHelper(const Tuple<>& t) { return String::EMPTY_STRING; }
	template<typename T> String ToStringHelper(const Tuple<T>& t) { return ToString(t.GetValue()); }
	template<typename T1, typename T2, typename... TS> String ToStringHelper(const Tuple<T1, T2, TS...>& t) { return String::Create("%A, %A", &ToString(t.GetValue()), &ToStringHelper((const Tuple<T2, TS...>&)t)); }
	
	template<typename... T> String ToString(const Tuple<T...>& t) { return String::Create("(%A)", &ToStringHelper(t)); }

//============================================================================
} // namespace Javelin
//============================================================================
