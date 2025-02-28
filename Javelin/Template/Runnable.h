//============================================================================

#pragma once
#include "Javelin/Type/SmartPointer.h"

//============================================================================

namespace Javelin
{
//============================================================================

	template<typename...> class Tuple;
	template<class> class Runnable;
	
    template<typename R, typename... P> class JABSTRACT Runnable<R(P...)>
    {
    public:
		typedef R ReturnType;
		typedef R (Type)(P...);
		typedef Tuple<P...> ParameterList;

		virtual ~Runnable() { }
		JINLINE R operator()(P... p) { return Run(p...); }
		virtual R Run(P...) = 0;
    };

//============================================================================

	template<typename C>
	class ReferenceCountedRunnable
		: public Runnable<C>,
		  public SingleAssignSmartObject
	{
	};

//============================================================================
	
	template<typename FTYPE, FTYPE* F, typename... P> class FunctionRunnable
	: public Runnable< FTYPE >
	{
	public:
		constexpr FunctionRunnable() { }
		
		virtual typename Runnable<FTYPE>::ReturnType Run(P... p) final { return (*F)(p...); }
		
		static const FunctionRunnable instance;
	};
	
	template<typename FTYPE, FTYPE* F, typename... P> constexpr FunctionRunnable<FTYPE, F, P...> FunctionRunnable<FTYPE, F, P...>::instance;
	
//============================================================================

	template<typename R, typename... P> class FunctionPointerRunnable : public Runnable<R(P...)>
	{
	private:
		typedef R (*FunctionPointerType)(P...);
		
	public:
		FunctionPointerRunnable() = delete;
		constexpr FunctionPointerRunnable(FunctionPointerType aFunctionPointer) : functionPointer(aFunctionPointer) { }
		
		virtual R Run(P... p) final { return (*functionPointer)(p...); }
		
	private:
		FunctionPointerType	functionPointer;
	};

//============================================================================
	
	template<typename L, typename R, typename... P>
	class LambdaRunnable : public Runnable< R(P...) >, private L
	{
	public:
		LambdaRunnable() = delete;
		constexpr LambdaRunnable(const L& lambda) : L(lambda) { }
		
		virtual R Run(P... p) final { return L::operator()(p...); }
	};

//============================================================================

	template<typename L, typename R, typename... P>
	class ReferenceCountedLambdaRunnable : public ReferenceCountedRunnable<R(P...)>, private L
	{
	public:
		ReferenceCountedLambdaRunnable() = delete;
		constexpr ReferenceCountedLambdaRunnable(const L& lambda) : L(lambda) { }
		
		virtual R Run(P... p) final { return L::operator()(p...); }
	};

//============================================================================
} // namespace Javelin
//============================================================================
