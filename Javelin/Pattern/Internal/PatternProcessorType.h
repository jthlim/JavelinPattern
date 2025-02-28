//============================================================================

#pragma once

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================
	
	enum class PatternProcessorType : uint16_t
	{
		None,
		Nfa,
		BitState,
		OnePass,
		BackTracking,
		ScanAndCapture,
		ConsistencyCheck,
		NfaOrBitStateBackTracking,
		FixedLength,
		Anchored,
		
		// Used when verifying input
		ForwardMax = NfaOrBitStateBackTracking,
		ReverseMax = Anchored,
		
		// The following are only used internally during compilation
		Default,
		NoScan,
		NotOnePass,
	};
	
//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
