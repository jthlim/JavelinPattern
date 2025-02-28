//============================================================================

#pragma once
#include "Javelin/Container/Optional.h"
#include "Javelin/Container/Tuple.h"
#include "Javelin/Template/Runnable.h"
#include "Javelin/Thread/Lock.h"
#include "Javelin/Thread/Barrier.h"
#include "Javelin/Type/Function.h"
#include "Javelin/Type/SmartPointer.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	class Task : public Runnable<void(void)>
	{
	public:
		virtual ~Task() { }

		// From Runnable
		virtual void Run();	// Default calls RunTask, then OnComplete();
		virtual void RunTask() { }

		virtual void Cancel()		{ OnComplete(); 	}

	protected:
		virtual void OnComplete()	{ delete this; 		}
	};
	
//============================================================================

	namespace Private
	{
		template<typename T> class FutureResultTask : public Task, public SmartObject
		{
		protected:
			typedef T ResultType;
			
		public:
			// Start with a reference count of one so that the thread it will execute
			// on is ensured a valid object. RemoveReference in OnComplete of the task
			FutureResultTask() : SmartObject(1)	{ }
			~FutureResultTask()					{ }
			
			// From Task
			virtual void RunTask() override = 0;
			
			virtual void OnComplete() final					{ if(RemoveReference()) delete this; else resultReady.Signal(); }
		
			void Wait()										{ Sentry<Barrier> sentry(resultReady); }
			const T& GetResult() const 						{ Sentry<Barrier> sentry(resultReady); return result; }
			
		private:
			mutable Barrier resultReady;
		
		protected:
			Optional<T>	result;
		};
		
		template<typename F, typename... T> class ExecuteFutureTask : public FutureResultTask<typename Function<F>::ReturnType>
		{
		private:
			typedef FutureResultTask<typename Function<F>::ReturnType> Inherited;
			typedef typename Function<F>::ParameterList ParameterListType;
			
		public:
			ExecuteFutureTask(const Function<F>& aFunction, const ParameterListType& aParameters) : function(aFunction), parameters(aParameters) { }
			
			virtual void RunTask() override
			{
				this->result = function(parameters);
			}
		
		private:
			Function<F> function;
			ParameterListType parameters;
		};


		template<typename F, typename... T> class ExecuteFunctionTask : public Task
		{
		private:
			typedef typename Function<F>::ParameterList ParameterListType;
			
		public:
			ExecuteFunctionTask(const Function<F>& aFunction, const ParameterListType aParameters) : function(aFunction), parameters(aParameters) { }
			
			virtual void RunTask() override
			{
				function(parameters);
			}

		private:
			Function<F> function;
			ParameterListType parameters;
		};
	}

//============================================================================

	template<typename T> class Future : public SmartPointer< Private::FutureResultTask<T> >
	{
	private:
		typedef SmartPointer< Private::FutureResultTask<T> > Inherited;
		
	public:
		Future(Private::FutureResultTask<T>* p) : Inherited(p) { }
		
		const T& GetResult() const { return (*this)->GetResult(); }
	};
	
//============================================================================
} // namespace Javelin
//============================================================================
