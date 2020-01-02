//============================================================================

#pragma once
#include <vector>

//============================================================================

namespace Javelin::Assembler
{
//============================================================================
	
	class Deleteable
	{
	public:
		virtual ~Deleteable() { }
	};
	
	// DeleteWrapper adds a virtual destructor to classes that do not have one.
	template<typename T> class DeleteWrapper
	: public Deleteable, public T
	{
	public:
		DeleteWrapper() = default;
		template<typename... P> DeleteWrapper(P&&... p) : T(std::move(p)...) { }
	};
	
	class AutoDeleteVector : public std::vector<Deleteable*>
	{
	public:
		~AutoDeleteVector()
		{
			for(Deleteable *deleteable : *this)
			{
				delete deleteable;
			}
		}
	};

//============================================================================
}
//============================================================================
