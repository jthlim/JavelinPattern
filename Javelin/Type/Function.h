//============================================================================

#pragma once
#include "Javelin/System/Assert.h"
#include "Javelin/Template/Runnable.h"
#include "Javelin/Type/TypeData.h"

//============================================================================

namespace Javelin
{
//============================================================================

	template<typename...> class Tuple;
	
	namespace Private
	{
		template<int n, typename T, typename... U> struct FunctionParameterHelper		{ typedef typename FunctionParameterHelper<n-1, U...>::Type Type; };
		template<typename T, typename... U> struct FunctionParameterHelper<0, T, U...>	{ typedef T Type; };
	}

//============================================================================

	class FunctionBase
	{
	public:
		JINLINE bool IsUsingInlineStorage() const						{ return runnable == (Runnable<void()>*) &storage; }
		
	protected:
		FunctionBase() 													{ }
		FunctionBase(Runnable<void()>* aRunnable) : runnable(aRunnable) { }
		~FunctionBase() 												{ Release(); }

		void Copy(const FunctionBase& a);
		void Move(FunctionBase&& a);
		void Release();
		
		struct Storage { void* data[2]; };
		
		Storage 			storage;
		Runnable<void()>*	runnable;
	};
	
//============================================================================

	template<typename T> class Function;

	template<typename R, typename... P>
	class Function<R(P...)> : public FunctionBase
	{
	private:
		typedef FunctionBase Inherited;
		
	public:
		typedef R ReturnType;
		typedef R (Type)(P...);
		typedef R (*PointerType)(P...);
		static const size_t PARAMETER_COUNT = sizeof...(P);
		template<int n> struct Parameter { typedef typename Private::FunctionParameterHelper<n, P...>::Type Type; };
		typedef Tuple<P...> ParameterList;
		typedef Tuple<typename TypeData<P>::StorageType...> ParameterStoreList;
	
		Function() : Inherited(nullptr)							{ }
		Function(PointerType fp)								{ runnable = (Runnable<void()>*) new(Placement(&storage)) FunctionPointerRunnable<R, P...>(fp); }
		Function(const Function& a)								{ Copy(a); 				}
		Function(Function&& a)									{ Move((Function&&) a); }
		template<typename L> Function(const L& lambda) 			{ Assign(lambda); 		}

		void operator=(decltype(nullptr))						{ Release(); runnable = nullptr; 				}
		void operator=(PointerType af) 							{ Release(); runnable = (Runnable<void()>*) new(Placement(&storage)) FunctionPointerRunnable<R, P...>(af); }
		void operator=(const Function& a) 						{ Release(); Copy(a); 							}
		void operator=(Function&& a) 							{ Release(); Move((Function&&) a); 				}
		template<typename L> void operator=(const L& lambda) 	{ Release(); Assign(lambda); 					}
		ReturnType operator()(P... p) const 					{ return (*(Runnable<R(P...)>*)runnable)(p...); }
		
		template<typename... U>
		ReturnType operator()(const Tuple<U...>& parameters) const { return parameters.Call(*(Runnable<R(P...)>*) runnable); }
		
	private:
		template<typename L> void Assign(const L& lambda) 		{
																	// The only way sizeof(LambdaRunnable) == sizeof(void*)
																	// is if it is only made up of its vtbl pointer. In this
																	// case, it is safe to place it in &storage
																	if(sizeof(LambdaRunnable<L,R,P...>) == sizeof(void*))
																	{
																		runnable = (Runnable<void()>*) new(Placement(&storage)) LambdaRunnable<L,R,P...>(lambda);
																	}
																	else
																	{
																		runnable = (Runnable<void()>*) new ReferenceCountedLambdaRunnable<L,R,P...>(lambda);
																	}
																}
	};

	template<typename R, typename C, typename... P>
	class Function<R (C::*)(P...) const>
	{
	public:
		typedef R ReturnType;
		typedef R (C::*Type)(P...);
		typedef R (C::*PointerType)(P...);
		static const size_t PARAMETER_COUNT = sizeof...(P);
		template<int n> struct Parameter { typedef typename Private::FunctionParameterHelper<n, P...>::Type Type; };
		typedef Tuple<P...> ParameterList;
		typedef Tuple<typename TypeData<P>::StorageType...> ParameterStoreList;
	};
	
	template<typename T> class Function : public Function<decltype(&T::operator())>
	{
	};
	
//============================================================================
} // namespace Javelin
//============================================================================
