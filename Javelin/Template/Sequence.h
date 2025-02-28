//============================================================================
//
// This is a helper class to generate a parameter pack for Sequence of integers.
// Check Tuple.h to see usage
//
// CreateSequence<10> creates
//  CreateSequence<9, 9> creates
//  CreateSequence<8, 8, 9>
//  CreateSequence<7, 7, 8, 9>
//  ...
//  CreateSequence<0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9>
//
// And ::Type() excludes the first 0.
//
// ie. returns Sequence<0, 1, 2, 3, 4, 5, 6, 7, 8, 9>
//
//============================================================================

#pragma once

//============================================================================

namespace Javelin
{
//============================================================================

	template<int ...> struct Sequence { };
	template<int N, int ...S> struct CreateSequence : CreateSequence<N-1, N-1, S...> { };
	template<int ...S> struct CreateSequence<0, S...> { typedef Sequence<S...> Type; };

//============================================================================
} // namespace Javelin
//============================================================================
