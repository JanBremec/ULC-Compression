#ifndef ULC_HYPER_TYPES_H
#define ULC_HYPER_TYPES_H

#include <stdint.h>
#include <stddef.h>

// Token types for semantic decomposition
typedef enum {
    TOKEN_TYPE_LITERAL,  // Static string (e.g., "api", "Mozilla")
    TOKEN_TYPE_DELIMITER,// Delimiter (e.g., "/", " ")
    TOKEN_TYPE_VARIABLE, // Dynamic value (e.g., "123", "admin")
    TOKEN_TYPE_NUMERIC,  // Numeric value
    TOKEN_TYPE_IP        // IP Address
} TokenType;

// A decomposed token
typedef struct {
    char* value;
    TokenType type;
    int dict_id;         // ID in the local or global dictionary
} Token;

// A stream of tokens representing a field (e.g., a URL)
typedef struct {
    Token* tokens;
    size_t count;
    size_t capacity;
} TokenStream;

// A column in the hyper-table
typedef struct {
    char* name;
    TokenStream** rows;  // Array of TokenStreams, one per line
    size_t row_count;
    
    // Sub-columns for recursive storage
    // Instead of storing TokenStreams directly, we pivot them:
    // URL -> [Segment1_Col, Delim1_Col, Segment2_Col...]
    struct HyperColumn** sub_columns; 
    size_t sub_col_count;
    
    // Statistics
    int is_constant;     // If all rows have same value
    int is_dict_encoded; // If we use dictionary for this column
} HyperColumn;

#endif // ULC_HYPER_TYPES_H
