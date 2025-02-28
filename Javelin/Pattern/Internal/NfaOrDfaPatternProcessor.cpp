//============================================================================

#include "Javelin/Pattern/Internal/PatternProcessor.h"
#include "Javelin/Thread/Mutex.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class NfaOrDfaPatternProcessor final : public PatternProcessor
{
public:
	NfaOrDfaPatternProcessor(const void* data, size_t length);
	~NfaOrDfaPatternProcessor();
	
	virtual const void* FullMatch(const void* data, size_t length) const;
	virtual const void* FullMatch(const void* data, size_t length, const char **captures) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const;
	virtual Interval<const void*> LocatePartialMatch(const void* data, size_t length, size_t offset) const;
	virtual const void* PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const;

private:
	mutable PatternProcessor*	nfaOrDfaProcessor;		// Starts out pointing to dfa. If the dfa fails, then it gets set to thompson
	PatternProcessor*			dfaProcessor;
	mutable PatternProcessor*	nfaProcessor;
	const void*					data;
	size_t						length;
	Mutex 						nfaCreationLock;
	
	void CreateNfaProcessor() const;
};

//============================================================================

NfaOrDfaPatternProcessor::NfaOrDfaPatternProcessor(const void* aData, size_t aLength)
: data(aData), length(aLength)
{
	nfaProcessor = nullptr;
	dfaProcessor = PatternProcessor::CreateDfaProcessor(data, length);
	nfaOrDfaProcessor = dfaProcessor;
}

NfaOrDfaPatternProcessor::~NfaOrDfaPatternProcessor()
{
	delete nfaProcessor;
	delete dfaProcessor;
}

//============================================================================

void NfaOrDfaPatternProcessor::CreateNfaProcessor() const
{
	nfaCreationLock.Synchronize([&] {
		if(nfaProcessor == nullptr)
		{
			nfaProcessor = PatternProcessor::CreateThompsonNfaProcessor(data, length);
			nfaOrDfaProcessor = nfaProcessor;
		}
	});
}

//============================================================================

const void* NfaOrDfaPatternProcessor::FullMatch(const void* data, size_t length) const
{
	const void* result = nfaOrDfaProcessor->FullMatch(data, length);
	if(result != (void*) DFA_FAILED) return result;
	CreateNfaProcessor();
	return nfaProcessor->FullMatch(data, length);
}

const void* NfaOrDfaPatternProcessor::FullMatch(const void* data, size_t length, const char **captures) const
{
	JERROR("Should never be called");
	return nullptr;
}

const void* NfaOrDfaPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset) const
{
	const void* result = nfaOrDfaProcessor->PartialMatch(data, length, offset);
	if(result != (void*) DFA_FAILED) return result;
	CreateNfaProcessor();
	return nfaProcessor->PartialMatch(data, length, offset);
}

const void* NfaOrDfaPatternProcessor::PartialMatch(const void* data, size_t length, size_t offset, const char **captures) const
{
	JERROR("Should never be called");
	return nullptr;
}

Interval<const void*> NfaOrDfaPatternProcessor::LocatePartialMatch(const void* data, size_t length, size_t offset) const
{
	JERROR("Should never be called");
	return {nullptr, nullptr};
}

const void* NfaOrDfaPatternProcessor::PopulateCaptures(const void* data, size_t length, size_t offset, const char **captures) const
{
	JERROR("Should never be called");
	return nullptr;
}

//============================================================================

PatternProcessor* PatternProcessor::CreateNfaOrDfaProcessor(const void* data, size_t length)
{
	return new NfaOrDfaPatternProcessor(data, length);
}

//============================================================================
