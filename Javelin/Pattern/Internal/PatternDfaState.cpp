//============================================================================

#include "Javelin/Pattern/Internal/PatternDfaState.h"
#include "Javelin/Pattern/Internal/PatternDfaMemoryManager.h"
#include "Javelin/Stream/StandardWriter.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

const uint32_t DfaProcessorBase::State::FAIL_STATE = NfaState::Flag::HAS_EMPTY_STATES | NfaState::Flag::DFA_FAILED;
const uint32_t DfaProcessorBase::State::EMPTY_STATE = NfaState::Flag::HAS_EMPTY_STATES;
const uint32_t DfaProcessorBase::State::EMPTY_MATCH_STATE = NfaState::Flag::HAS_EMPTY_STATES | NfaState::Flag::IS_MATCH;

//============================================================================

DfaProcessorBase::State::State()
{
	memset(&localSearchData, 0, sizeof(localSearchData));
	memset((void*) nextStates, 0, sizeof(nextStates));
}

//============================================================================

void* DfaProcessorBase::State::operator new(size_t size, uint32_t numberOfStates, DfaProcessorBase& processor)
{
	JASSERT(size == sizeof(State));
	size_t numberOfBytes = sizeof(State) - sizeof(NfaState) + NfaState::GetSizeRequiredForNumberOfStates(numberOfStates);
	return DfaMemoryManager::Allocate(numberOfBytes, processor);
}

void DfaProcessorBase::State::operator delete(void* p, uint32_t numberOfStates, DfaProcessorBase& processor)
{
	size_t numberOfBytes = sizeof(State) - sizeof(NfaState) + NfaState::GetSizeRequiredForNumberOfStates(numberOfStates);
	DfaMemoryManager::Release(p, numberOfBytes, processor);
	::operator delete(p);
}

void DfaProcessorBase::State::operator delete(void* p)
{
	::operator delete(p);
}

void DfaProcessorBase::State::Dump(const char* prefix) const
{
	if(this == (void*) &FAIL_STATE)
	{
		StandardOutput.PrintF("%s: fail\n", prefix);
		return;
	}
	
	if(this == (void*) &EMPTY_STATE)
	{
		StandardOutput.PrintF("%s: empty\n", prefix);
		return;
	}
	
	if(this == (void*) &EMPTY_MATCH_STATE)
	{
		StandardOutput.PrintF("%s: empty-match\n", prefix);
		return;
	}
	
	StandardOutput.PrintF("%s: %p:%x", prefix, this, nfaState.stateFlags);
	for(uint32_t i = 0; i < nfaState.numberOfStates; ++i)
	{
		StandardOutput.PrintF(" %u", nfaState.stateList[i]);
	}

	if(stateFlags & NfaState::Flag::IS_WORD_CHARACTER) StandardOutput.PrintF(" IsWordCharacter");
	if(stateFlags & NfaState::Flag::IS_END_OF_LINE) StandardOutput.PrintF(" IsEndOfLine");
	if(stateFlags & NfaState::Flag::IS_END_OF_INPUT) StandardOutput.PrintF(" IsEndOfInput");
	if(stateFlags & NfaState::Flag::IS_START_OF_INPUT) StandardOutput.PrintF(" IsStartOfInput");
	if(stateFlags & NfaState::Flag::IS_SEARCH) StandardOutput.PrintF(" IsSearch");
	if(stateFlags & NfaState::Flag::IS_MATCH) StandardOutput.PrintF(" IsMatch");
	
	StandardOutput.PrintF("\n");
}

size_t DfaProcessorBase::State::GetNumberOfAllocatedBytes() const
{
	return sizeof(State) - sizeof(NfaState) + NfaState::GetSizeRequiredForNumberOfStates(nfaState.numberOfStates);
}

//============================================================================

void DfaProcessorBase::State::Reset()
{
	stateFlags = (stateFlags | NfaState::Flag::DFA_NEEDS_POPULATING) & ~NfaState::Flag::DFA_STATE_IS_RESETTING;
	memset((void*) nextStates, 0, sizeof(nextStates));
}

//============================================================================
