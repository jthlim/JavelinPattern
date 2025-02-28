//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class String;
	
//============================================================================

	class JABSTRACT ICharacterReader
	{
	public:
		virtual ~ICharacterReader() 	{ }

		virtual unsigned char JCALL ReadByte() = 0;
		JEXPORT virtual String JCALL ReadLine();
		JEXPORT virtual String JCALL ReadAllAsString();
	};

//============================================================================
} // namespace Javelin
//============================================================================
