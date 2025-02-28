//============================================================================

#include "Javelin/Pattern/Internal/PatternReverseProcessor.h"
#include "Javelin/Thread/Mutex.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class NfaOrDfaReverseProcessor final : public ReverseProcessor
{
public:
	NfaOrDfaReverseProcessor(const void* data, size_t length);
	~NfaOrDfaReverseProcessor();
	
	virtual const void* Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char **captures, bool matchIsAnchored) const;

private:
	mutable ReverseProcessor*	nfaOrDfaProcessor;		// Starts out pointing to dfa. If the dfa fails, then it gets set to thompson
	ReverseProcessor*			dfaProcessor;
	mutable ReverseProcessor*	nfaProcessor;
	const void*					data;
	size_t						length;
	Mutex 						nfaCreationLock;
	
	void CreateNfaProcessor() const;
};

//============================================================================

NfaOrDfaReverseProcessor::NfaOrDfaReverseProcessor(const void* aData, size_t aLength)
: data(aData), length(aLength)
{
	nfaProcessor = nullptr;
	dfaProcessor = ReverseProcessor::CreateDfaReverseProcessor(data, length);
	nfaOrDfaProcessor = dfaProcessor;
}

NfaOrDfaReverseProcessor::~NfaOrDfaReverseProcessor()
{
	delete nfaProcessor;
	delete dfaProcessor;
}

//============================================================================

void NfaOrDfaReverseProcessor::CreateNfaProcessor() const
{
	nfaCreationLock.Synchronize([&] {
		if(nfaProcessor == nullptr)
		{
			nfaProcessor = ReverseProcessor::CreateNfaReverseProcessor(data, length);
			nfaOrDfaProcessor = nfaProcessor;
		}
	});
}

//============================================================================

const void* NfaOrDfaReverseProcessor::Match(const void* data, size_t length, size_t startOffset, const void* matchEnd, const char **captures, bool matchIsAnchored) const
{
	const void* result = nfaOrDfaProcessor->Match(data, length, startOffset, matchEnd, captures, matchIsAnchored);
	if(result != (void*) DFA_FAILED) return result;
	CreateNfaProcessor();
	return nfaProcessor->Match(data, length, startOffset, matchEnd, captures, matchIsAnchored);
}

//============================================================================

ReverseProcessor* ReverseProcessor::CreateNfaOrDfaReverseProcessor(const void* data, size_t length)
{
	return new NfaOrDfaReverseProcessor(data, length);
}

//============================================================================
