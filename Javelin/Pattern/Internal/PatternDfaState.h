//============================================================================

#include "Javelin/Pattern/Internal/DfaProcessorBase.h"
#include "Javelin/Pattern/Internal/PatternNfaState.h"
#include "Javelin/Type/Atomic.h"

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

		struct DfaProcessorBase::State
		{
			uint32_t				stateFlags;
			SearchHandler			searchHandler;
			const void*				searchData;
			union
			{
				ByteCodeSearchByteData	localSearchData;
				uint8_t					workingArea[36];
			};
			State *volatile			nextStates[258];		// The last slot is for reverse processor if IS_START_OF_SEARCH is set
			Atomic<uint32_t>		activeCounter	= 0;
			NfaState				nfaState;

			State();
			
			static const int START_OF_SEARCH_INDEX = 257;
			
			static void* operator new(size_t size, uint32_t numberOfStates, DfaProcessorBase& processor);
			static void operator delete(void* p, uint32_t numberOfStates, DfaProcessorBase& processor);
			static void operator delete(void* p);
			
			void Dump(const char* prefix) const;
			
			void Reset();
			void MarkAsResetting()				{ stateFlags |= NfaState::Flag::DFA_STATE_IS_RESETTING;					}
			void TagForRelease()				{ stateFlags |= NfaState::Flag::DFA_STATE_WILL_RELEASE; 				}
			void RemoveTagForRelease()			{ stateFlags &= ~NfaState::Flag::DFA_STATE_WILL_RELEASE; 				}
			void TagAsPopulating()				{ stateFlags |= NfaState::Flag::DFA_STATE_IS_POPULATING; 				}
			bool IsPopulating() const			{ return (stateFlags & NfaState::Flag::DFA_STATE_IS_POPULATING) != 0;	}
			bool ShouldRelease() const			{ return (stateFlags & NfaState::Flag::DFA_STATE_WILL_RELEASE) != 0;	}
			
			size_t GetNumberOfAllocatedBytes() const;
			
			static const uint32_t FAIL_STATE;
			static const uint32_t EMPTY_STATE;
			static const uint32_t EMPTY_MATCH_STATE;
		};

//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
