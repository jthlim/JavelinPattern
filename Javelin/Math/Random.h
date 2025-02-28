//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class Random
	{
	public:
		Random();
		Random(long long a) : s1((unsigned) a), s2((unsigned) (a >> 32)) { }
		Random(unsigned a1, unsigned a2) : s1(a1), s2(a2) { }

		template<typename T> JEXPORT T JCALL Generate();
		
		// max is exclusive
		template<typename T> JEXPORT T JCALL Generate(T max);
		template<typename T> JINLINE T JCALL Generate(T min, T max)			{ return min + Generate<T>(max); }
		
		static Random instance;
		
	private:
		unsigned	s1;
		unsigned	s2;
	};

//============================================================================

	constexpr Random* theRandom = &Random::instance;
	
//============================================================================
} // namespace Javelin
//============================================================================
