//============================================================================

#pragma once

//============================================================================

namespace Javelin
{
//============================================================================

	template<bool flag, typename TrueType, typename FalseType>
	struct SelectType
	{
		typedef TrueType Result;
	};

	template<typename TrueType, typename FalseType>
	struct SelectType<false, TrueType, FalseType>
	{
		typedef FalseType Result;
	};

//============================================================================
} // namespace Javelin
//============================================================================
