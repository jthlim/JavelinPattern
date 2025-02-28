//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Thread/Task.h"
#include "Javelin/Type/Function.h"

//============================================================================

#if defined(JPLATFORM_WIN32) || defined(JPLATFORM_WIN64)
	#include "Javelin/Thread/Windows/Thread.h"
#else
	#include "Javelin/Thread/Unix/Thread.h"
#endif

//============================================================================

namespace Javelin
{
//============================================================================
	
	namespace Private
	{
		template<typename T> JINLINE void* ThreadReturnSetHelper(T value)		{ return (void*) value; }
		template<typename T> JINLINE T ThreadReturnGetHelper(void* joinValue) 	{ return (T) joinValue; }
		
		template<typename R, typename T, typename... S> struct ThreadHelper		{
																					T							function;
																					typename T::ParameterList	parameters;

																					static void* ThreadEntry(void* pv)
																					{
																						ThreadHelper* p = (ThreadHelper*) pv;
																						void* returnValue = ThreadReturnSetHelper<R>(p->function(p->parameters));
																						delete p;
																						return returnValue;
																					}
																				};
		
		template<typename T, typename... S> struct ThreadHelper<void, T, S...>	{
																					T							function;
																					typename T::ParameterList	parameters;
																					
																					static void* ThreadEntry(void* pv)
																					{
																						ThreadHelper* p = (ThreadHelper*) pv;
																						p->function(p->parameters);
																						delete p;
																						return nullptr;
																					}
																				};

		template<typename R, typename T> struct ThreadHelperNoParameters		{
																					static void* ThreadEntry(void* pv)
																					{
																						return ThreadReturnSetHelper<R>((*(typename T::PointerType) pv)());
																					}
																				};
		
		template<typename T> struct ThreadHelperNoParameters<void, T>			{
																					static void* ThreadEntry(void* pv)
																					{
																						(*(typename T::PointerType) pv)();
																						return nullptr;
																					}
																				};
	}
	
//============================================================================

	class Thread : private ThreadHandle
	{
	private:
		typedef ThreadHandle Inherited;
		
	public:
		template<typename P>
			void Start(const Function<void*(P*)>& f, P* threadArgument)		{ Inherited::Start(*(Function<void*(void*)>*) &f, threadArgument); }
		template<typename R>
			void Start(R (*function)())										{ Inherited::Start(&Private::ThreadHelperNoParameters<R, Function<R()>>::ThreadEntry, (void*) function); }
		template<typename R, typename... P, typename... S>
			void Start(const Function<R(P...)>& f, S&& ...param)			{ Private::ThreadHelper<R, Function<R(P...)>, S...>* threadData = new Private::ThreadHelper<R, Function<R(P...)>, S...>{f, {(S&&) param...}}; Inherited::Start(threadData->ThreadEntry, threadData);   }
		template<typename R, typename... P, typename... S>
			void Start(R (*function)(P...), S&& ...param) 					{ Start(Function<R(P...)>(function), param...); }
		
		using Inherited::Run;
		static void Run(Runnable<void(void)>* task)							{ Inherited::Run(&ExecuteRunnable, task); }
		template<typename P>
			static void Run(const Function<void(P*)>& f, P* threadArgument)	{ Inherited::Run(*(Function<void*(void*)>*) &f, threadArgument); }
		template<typename R, typename... P, typename... S>
			static void Run(const Function<void(P...)>& f, S&&... param) 	{ Run(new Private::ExecuteFunctionTask<void(P...), S...>(f, {(S&&) param...})); }
		template<typename R, typename... P, typename... S>
			static Future<R> Run(const Function<R(P...)>& f, S&&... param)	{
																				Private::ExecuteFutureTask<R(P...), S...>* task = new Private::ExecuteFutureTask<R(P...), S...>(f, {(S&&) param...});
																				Future<R> result(task);
																				Run(task);
																				return result;
																			}
		template<typename R, typename... P, typename... S>
			static Future<R> Run(R (*function)(P...), S&& ...param) 		{ return Run(Function<R(P...)>(function), param...); }

		using Inherited::Sleep;
		using Inherited::IsActive;
		
		JINLINE void Join() 					{ Inherited::Join(); }
		template<typename R> JINLINE R Join()	{ return Private::ThreadReturnGetHelper<R>(Inherited::Join()); }
		
		static void Yield();
		
	private:
		static void ExecuteRunnable(void* parameter);
	};
	
//============================================================================
	
	class IThread : private Thread, public Runnable<void(void)>
	{
	private:
		typedef Thread Inherited;
		
	public:
		JINLINE void Start()			{ Inherited::Start(&IThread::EntryPoint, this); }
		JINLINE void Join()				{ Inherited::Join(); 							}
		
		using Inherited::IsActive;
		using Inherited::Sleep;
		using Inherited::Yield;
		
	private:
		virtual void Run() = 0;
		
		static void* EntryPoint(IThread* p);
	};

//============================================================================
} // namespace Javelin
//============================================================================
