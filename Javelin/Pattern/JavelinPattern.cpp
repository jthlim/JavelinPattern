//============================================================================

#include "Javelin/JavelinPattern.h"
#include "Javelin/Pattern/Pattern.h"
#include "Javelin/Type/DataBlock.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

class CStackGrowthHandler : public Pattern::StackGrowthHandler
{
public:
	constexpr CStackGrowthHandler() { }

	virtual void* Allocate(size_t size) const	{ return custom_allocate(size); }
	virtual void Free(void* p) const			{ custom_free(p);				}
	
	static void* (*custom_allocate)(size_t);
	static void (*custom_free)(void*);
	
	static const CStackGrowthHandler instance;
};

constexpr CStackGrowthHandler CStackGrowthHandler::instance;
void* (*CStackGrowthHandler::custom_allocate)(size_t);
void (*CStackGrowthHandler::custom_free)(void*);

//============================================================================

int jp_bytecode_compile(jp_bytecode_t* result, const char* pattern, int options)
{
	try
	{
		*result = new DataBlock(Pattern::CreateByteCode(String{pattern}, options));
		return JP_RESULT_OK;
	}
	catch(const PatternException& exception)
	{
		*result = nullptr;
		return (int) exception.GetType();
	}
	catch(...)
	{
		*result = nullptr;
		return JP_RESULT_INTERNAL_ERROR;
	}
}

const void* jp_bytecode_get_data(jp_bytecode_t bytecode)
{
	return static_cast<DataBlock*>(bytecode)->GetData();
}

size_t jp_bytecode_get_length(jp_bytecode_t bytecode)
{
	return static_cast<DataBlock*>(bytecode)->GetNumberOfBytes();
}

void jp_bytecode_free(jp_bytecode_t bytecode)
{
	delete static_cast<DataBlock*>(bytecode);
}

//============================================================================

int jp_pattern_compile(jp_pattern_t* result, const char* pattern, int options)
{
	try
	{
		*result = new Pattern(String{pattern}, options);
		return JP_RESULT_OK;
	}
	catch(const PatternException& exception)
	{
		*result = nullptr;
		return (int) exception.GetType();
	}
	catch(...)
	{
		*result = nullptr;
		return JP_RESULT_INTERNAL_ERROR;
	}
}

int jp_pattern_create(jp_pattern_t* result, const void* byte_code, size_t byte_code_length, bool make_copy)
{
	try
	{
		*result = new Pattern(byte_code, byte_code_length, make_copy);
		return JP_RESULT_OK;
	}
	catch(const PatternException& exception)
	{
		*result = nullptr;
		return (int) exception.GetType();
	}
	catch(...)
	{
		*result = nullptr;
		return JP_RESULT_INTERNAL_ERROR;
	}
}

void jp_pattern_free(jp_pattern_t pattern)
{
	delete static_cast<Pattern*>(pattern);
}

//============================================================================

int jp_get_number_of_captures(jp_pattern_t pattern)
{
	return static_cast<Pattern*>(pattern)->GetNumberOfCaptures();
}

bool jp_has_full_match(jp_pattern_t pattern, const void* data, size_t length)
{
	return static_cast<Pattern*>(pattern)->HasFullMatch(data, length);
}

bool jp_has_partial_match(jp_pattern_t pattern, const void* data, size_t length, size_t offset)
{
	return static_cast<Pattern*>(pattern)->HasPartialMatch(data, length);
}

bool jp_full_match(jp_pattern_t pattern, const void* data, size_t length, const void** captures)
{
	return static_cast<Pattern*>(pattern)->FullMatch(data, length, captures);
}

bool jp_partial_match(jp_pattern_t pattern, const void* data, size_t length, const void** captures, size_t offset)
{
	return static_cast<Pattern*>(pattern)->PartialMatch(data, length, captures, offset);
}

size_t jp_count_partial_matches(jp_pattern_t pattern, const void* data, size_t length, size_t offset)
{
	return static_cast<Pattern*>(pattern)->CountPartialMatches(data, length, offset);
}

//============================================================================

void jp_dfa_memory_manager_mode_set_unlimited()
{
	Pattern::SetDfaMemoryModeConfiguration(DfaMemoryManagerMode::Unlimited);
}

void jp_dfa_memory_manager_mode_set_global_limit(size_t limit)
{
	Pattern::SetDfaMemoryModeConfiguration(DfaMemoryManagerMode::GlobalLimit, limit);
}

void jp_dfa_memory_manager_mode_set_per_pattern_limit(size_t limit)
{
	Pattern::SetDfaMemoryModeConfiguration(DfaMemoryManagerMode::PerPatternLimit, limit);
}

//============================================================================

void jp_stack_growth_handler_disable()
{
	Pattern::SetNoStackGrowth();
}

void jp_stack_growth_handler_set(void* (*allocate)(size_t), void (*free)(void*))
{
	CStackGrowthHandler::custom_allocate = allocate;
	CStackGrowthHandler::custom_free = free;
	Pattern::SetStackGrowthHandler(&CStackGrowthHandler::instance);
}

//============================================================================
