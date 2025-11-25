#ifndef ULC_ULTRA_TYPES_H
#define ULC_ULTRA_TYPES_H

#include <stdint.h>
#include <stddef.h>

// Magic header for ULC-Ultra files
#define ULCU_MAGIC "ULCU"
#define ULCU_MAGIC_LEN 4

// Pattern structure for frequent pattern mining
typedef struct {
    char* pattern;
    int frequency;
    int id;
} Pattern;

// Pattern dictionary
typedef struct {
    Pattern* patterns;
    size_t count;
    size_t capacity;
} PatternDict;

// Huffman tree node
typedef struct HuffmanNode {
    int symbol;
    int frequency;
    struct HuffmanNode* left;
    struct HuffmanNode* right;
} HuffmanNode;

// Huffman code
typedef struct {
    uint32_t code;
    int length;
} HuffmanCode;

// Huffman encoder
typedef struct {
    HuffmanCode* codes;
    size_t code_count;
    HuffmanNode* root;
} HuffmanEncoder;

// Context model for field prediction
typedef struct {
    int** correlation_matrix;  // Field correlation scores
    int* field_counts;
    size_t field_count;
} ContextModel;

// Bit stream for Huffman encoding
typedef struct {
    uint8_t* data;
    size_t byte_pos;
    int bit_pos;
    size_t capacity;
} BitStream;

// Ultra compressor state
typedef struct {
    PatternDict* pattern_dict;
    HuffmanEncoder* huffman;
    ContextModel* context;
    int compression_level;  // 1-10, higher = more aggressive
} UltraCompressor;

#endif // ULC_ULTRA_TYPES_H
