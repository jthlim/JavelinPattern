//============================================================================

#pragma once
#include <stdbool.h>
#include <stddef.h>

//============================================================================

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================

typedef void* jp_pattern_t;
typedef void* jp_bytecode_t;

//============================================================================

// Options to pass into compile
#define JP_OPTION_IGNORE_CASE               1       // "i" option
#define JP_OPTION_MULTILINE                 2       // "m" option. This allows ^ and $ to match \n lines mid-string
#define JP_OPTION_DOTALL                    4       // "s" option. This allows . to match newlines
#define JP_OPTION_UNICODE_CASE              8       // "u" option. When IGNORE_CASE is used, this allows unicode case folding
#define JP_OPTION_UNGREEDY                  0x10    // "U" option

#define JP_OPTION_UTF8                      0x100   // Without this flag, text is treated as ASCII

#define JP_OPTION_GLOB_SYNTAX               0x400   // With this flag, ? is treated as [^/], * as [^/]* and ** as .*  
                                                    // You will probably combine this with JP_OPTION_ANCHORED

#define JP_OPTION_ANCHORED                  0x800

#define JP_OPTION_NO_OPTIMIZE               0x1000

#define JP_OPTION_PREFER_NFA                0x2000
#define JP_OPTION_PREFER_BACK_TRACKING      0x4000
#define JP_OPTION_PREFER_SCAN_AND_CAPTURE   0x6000
#define JP_OPTION_PREFER_NO_SCAN            0x8000	// Use this when you're primarily using jp_full_match to extract sub-matches from strings that are expected to match

//============================================================================

// Result codes from jp_pattern_compile & jp_bytecode_compile
#define JP_RESULT_OK                                     0
#define JP_RESULT_INTERNAL_ERROR                         1

#define JP_RESULT_EXPECTED_CLOSE_GROUP                   2
#define JP_RESULT_INVALID_BACK_REFERENCE                 3
#define JP_RESULT_INVALID_OPTIONS                        4
#define JP_RESULT_LOOK_BEHIND_NOT_CONSTANT_BYTE_LENGTH   5
#define JP_RESULT_MALFORMED_CONDITIONAL                  6
#define JP_RESULT_MAXIMUM_REPETITION_COUNT_EXCEEDED      7
#define JP_RESULT_MINIMUM_COUNT_EXCEEDS_MAXIMUM_COUNT    8
#define JP_RESULT_TOO_MANY_BYTECODE_INSTRUCTIONS         9
#define JP_RESULT_TOO_MANY_CAPTURES                      10
#define JP_RESULT_TOO_MANY_PROGRESS_CHECK_INSTRUCTIONS   11
#define JP_RESULT_UNABLE_TO_PARSE_GROUP_TYPE             12
#define JP_RESULT_UNABLE_TO_PARSE_REPETITION             13
#define JP_RESULT_UNABLE_TO_RESOLVE_RECURSE_TARGET       14
#define JP_RESULT_UNEXPECTED_CONTROL_CHARACTER           15
#define JP_RESULT_UNEXPECTED_END_OF_PATTERN              16
#define JP_RESULT_UNEXPECTED_GROUP_OPTIONS               17
#define JP_RESULT_UNEXPECTED_HEX_CHARACTER               18
#define JP_RESULT_UNEXPECTED_LOOK_BEHIND_TYPE            19
#define JP_RESULT_UNEXPECTED_TOKEN                       20
#define JP_RESULT_UNKNOWN_ESCAPE                         21
#define JP_RESULT_UNKNOWN_POSIX_CHARACTER_CLASS          22

//============================================================================

// Functions to create a precompiled pattern. precompiled patterns can be used with jp_create
int         jp_bytecode_compile(jp_bytecode_t* out_result, const char* pattern, int options);
const void* jp_bytecode_get_data(jp_bytecode_t bytecode);
size_t      jp_bytecode_get_length(jp_bytecode_t bytecode);
void        jp_bytecode_free(jp_bytecode_t bytecode);

// Functions to create a pattern object.
int  jp_pattern_compile(jp_pattern_t* out_result, const char* pattern, int options);
int  jp_pattern_create(jp_pattern_t* out_result, const void* byte_code, size_t byte_code_length, bool make_copy_of_byte_code);
void jp_pattern_free(jp_pattern_t pattern);

// Functions using pattern objects
int    jp_get_number_of_captures(jp_pattern_t pattern);
bool   jp_has_full_match(jp_pattern_t pattern, const void* data, size_t data_length);
bool   jp_has_partial_match(jp_pattern_t pattern, const void* data, size_t data_length, size_t data_offset);
bool   jp_full_match(jp_pattern_t pattern, const void* data, size_t data_length, const void** captures);
bool   jp_partial_match(jp_pattern_t pattern, const void* data, size_t data_length, const void** captures, size_t data_offset);
size_t jp_count_partial_matches(jp_pattern_t pattern, const void* data, size_t data_length, size_t data_offset);

// Functions to control behavior of matching.
void jp_dfa_memory_manager_mode_set_unlimited();
void jp_dfa_memory_manager_mode_set_global_limit(size_t number_of_bytes_limit);
void jp_dfa_memory_manager_mode_set_per_pattern_limit(size_t number_of_bytes_limit);

void jp_stack_growth_handler_disable();
void jp_stack_growth_handler_set(void* (*allocate_function)(size_t), void (*free_function)(void*));

//============================================================================

#ifdef __cplusplus
}
#endif 

//============================================================================
