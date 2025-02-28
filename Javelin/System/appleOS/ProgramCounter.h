//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	// These are all static to try and avoid instruction size coding differences
	class ProgramCounter final
	{
	public:
		JEXPORT static size_t JCALL	GetOffset();
		
		JEXPORT static void	JCALL	Begin();
		JEXPORT static void	JCALL	End();
		
	private:
		static union Data
		{
			void* 	address;
			size_t	offset;
		} data;
		
		static size_t CalculateOffset();
	};
	
//============================================================================
} // namespace Javelin
//============================================================================
