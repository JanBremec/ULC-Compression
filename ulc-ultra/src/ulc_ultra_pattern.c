#include "../include/ulc_ultra_pattern.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

PatternDict* pattern_dict_new(size_t initial_capacity) {
    PatternDict* dict = malloc(sizeof(PatternDict));
    dict->capacity = initial_capacity > 0 ? initial_capacity : 256;
    dict->patterns = malloc(sizeof(Pattern) * dict->capacity);
    dict->count = 0;
    return dict;
}

void pattern_dict_free(PatternDict* dict) {
    if (dict) {
        for (size_t i = 0; i < dict->count; i++) {
            free(dict->patterns[i].pattern);
        }
        free(dict->patterns);
        free(dict);
    }
}

int find_pattern(PatternDict* dict, const char* pattern) {
    for (size_t i = 0; i < dict->count; i++) {
        if (strcmp(dict->patterns[i].pattern, pattern) == 0) {
            return dict->patterns[i].id;
        }
    }
    return -1;
}

int add_pattern(PatternDict* dict, const char* pattern, int frequency) {
    if (dict->count >= dict->capacity) {
        dict->capacity *= 2;
        dict->patterns = realloc(dict->patterns, sizeof(Pattern) * dict->capacity);
    }
    
    int id = dict->count;
    dict->patterns[dict->count].pattern = strdup(pattern);
    dict->patterns[dict->count].frequency = frequency;
    dict->patterns[dict->count].id = id;
    dict->count++;
    
    return id;
}

// Simple pattern mining: extract common substrings
void mine_patterns(PatternDict* dict, char** lines, size_t line_count, int min_support) {
    // For simplicity, extract common prefixes and suffixes
    // A full implementation would use suffix trees or other advanced algorithms
    
    if (line_count < 2) return;
    
    // Find common prefix
    size_t prefix_len = 0;
    int all_match = 1;
    
    while (all_match) {
        if (prefix_len >= strlen(lines[0])) break;
        
        char c = lines[0][prefix_len];
        for (size_t i = 1; i < line_count; i++) {
            if (prefix_len >= strlen(lines[i]) || lines[i][prefix_len] != c) {
                all_match = 0;
                break;
            }
        }
        
        if (all_match) prefix_len++;
    }
    
    if (prefix_len > 3) {  // Minimum pattern length
        char* prefix = malloc(prefix_len + 1);
        strncpy(prefix, lines[0], prefix_len);
        prefix[prefix_len] = '\0';
        add_pattern(dict, prefix, line_count);
        free(prefix);
    }
    
    // Find common suffix
    size_t suffix_len = 0;
    all_match = 1;
    size_t first_len = strlen(lines[0]);
    
    while (all_match && suffix_len < first_len) {
        char c = lines[0][first_len - suffix_len - 1];
        for (size_t i = 1; i < line_count; i++) {
            size_t len = strlen(lines[i]);
            if (suffix_len >= len || lines[i][len - suffix_len - 1] != c) {
                all_match = 0;
                break;
            }
        }
        
        if (all_match) suffix_len++;
    }
    
    if (suffix_len > 3) {
        char* suffix = malloc(suffix_len + 1);
        strncpy(suffix, lines[0] + first_len - suffix_len, suffix_len);
        suffix[suffix_len] = '\0';
        add_pattern(dict, suffix, line_count);
        free(suffix);
    }
}

char* replace_patterns(const char* text, PatternDict* dict, int** pattern_positions, int* position_count) {
    // For now, return copy of original text
    // Full implementation would replace patterns with markers
    *pattern_positions = NULL;
    *position_count = 0;
    return strdup(text);
}
