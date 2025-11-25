#ifndef ULC_ULTRA_PATTERN_H
#define ULC_ULTRA_PATTERN_H

#include "ulc_ultra_types.h"

// Create pattern dictionary
PatternDict* pattern_dict_new(size_t initial_capacity);

// Mine frequent patterns from log lines
void mine_patterns(PatternDict* dict, char** lines, size_t line_count, int min_support);

// Find pattern in dictionary
int find_pattern(PatternDict* dict, const char* pattern);

// Add pattern to dictionary
int add_pattern(PatternDict* dict, const char* pattern, int frequency);

// Free pattern dictionary
void pattern_dict_free(PatternDict* dict);

// Replace patterns in text with IDs
char* replace_patterns(const char* text, PatternDict* dict, int** pattern_positions, int* position_count);

#endif // ULC_ULTRA_PATTERN_H
