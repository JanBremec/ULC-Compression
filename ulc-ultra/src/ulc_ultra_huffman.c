#include "../include/ulc_ultra_huffman.h"
#include <stdlib.h>
#include <string.h>

// Bit stream implementation
BitStream* bitstream_new(size_t initial_capacity) {
    BitStream* stream = malloc(sizeof(BitStream));
    stream->capacity = initial_capacity > 0 ? initial_capacity : 1024;
    stream->data = calloc(stream->capacity, 1);
    stream->byte_pos = 0;
    stream->bit_pos = 0;
    return stream;
}

void bitstream_write_bit(BitStream* stream, int bit) {
    if (stream->byte_pos >= stream->capacity) {
        stream->capacity *= 2;
        stream->data = realloc(stream->data, stream->capacity);
    }
    
    if (bit) {
        stream->data[stream->byte_pos] |= (1 << (7 - stream->bit_pos));
    }
    
    stream->bit_pos++;
    if (stream->bit_pos == 8) {
        stream->bit_pos = 0;
        stream->byte_pos++;
    }
}

void bitstream_write_bits(BitStream* stream, uint32_t value, int num_bits) {
    for (int i = num_bits - 1; i >= 0; i--) {
        bitstream_write_bit(stream, (value >> i) & 1);
    }
}

int bitstream_read_bit(BitStream* stream) {
    if (stream->byte_pos >= stream->capacity) return 0;
    
    int bit = (stream->data[stream->byte_pos] >> (7 - stream->bit_pos)) & 1;
    
    stream->bit_pos++;
    if (stream->bit_pos == 8) {
        stream->bit_pos = 0;
        stream->byte_pos++;
    }
    
    return bit;
}

uint32_t bitstream_read_bits(BitStream* stream, int num_bits) {
    uint32_t value = 0;
    for (int i = 0; i < num_bits; i++) {
        value = (value << 1) | bitstream_read_bit(stream);
    }
    return value;
}

void bitstream_free(BitStream* stream) {
    if (stream) {
        free(stream->data);
        free(stream);
    }
}

// Huffman tree node comparison for priority queue
static int compare_nodes(const void* a, const void* b) {
    HuffmanNode* na = *(HuffmanNode**)a;
    HuffmanNode* nb = *(HuffmanNode**)b;
    return na->frequency - nb->frequency;
}

// Build Huffman tree from frequencies
static HuffmanNode* build_huffman_tree(int* frequencies, size_t symbol_count) {
    // Create leaf nodes
    HuffmanNode** nodes = malloc(sizeof(HuffmanNode*) * symbol_count * 2);
    size_t node_count = 0;
    
    for (size_t i = 0; i < symbol_count; i++) {
        if (frequencies[i] > 0) {
            HuffmanNode* node = malloc(sizeof(HuffmanNode));
            node->symbol = i;
            node->frequency = frequencies[i];
            node->left = NULL;
            node->right = NULL;
            nodes[node_count++] = node;
        }
    }
    
    if (node_count == 0) return NULL;
    if (node_count == 1) return nodes[0];
    
    // Build tree
    while (node_count > 1) {
        qsort(nodes, node_count, sizeof(HuffmanNode*), compare_nodes);
        
        HuffmanNode* left = nodes[0];
        HuffmanNode* right = nodes[1];
        
        HuffmanNode* parent = malloc(sizeof(HuffmanNode));
        parent->symbol = -1;
        parent->frequency = left->frequency + right->frequency;
        parent->left = left;
        parent->right = right;
        
        nodes[0] = parent;
        for (size_t i = 1; i < node_count - 1; i++) {
            nodes[i] = nodes[i + 1];
        }
        node_count--;
    }
    
    HuffmanNode* root = nodes[0];
    free(nodes);
    return root;
}

// Generate codes from tree
static void generate_codes(HuffmanNode* node, HuffmanCode* codes, uint32_t code, int length) {
    if (!node) return;
    
    if (node->symbol != -1) {
        codes[node->symbol].code = code;
        codes[node->symbol].length = length;
        return;
    }
    
    generate_codes(node->left, codes, (code << 1) | 0, length + 1);
    generate_codes(node->right, codes, (code << 1) | 1, length + 1);
}

HuffmanEncoder* huffman_create(int* frequencies, size_t symbol_count) {
    HuffmanEncoder* encoder = malloc(sizeof(HuffmanEncoder));
    encoder->code_count = symbol_count;
    encoder->codes = calloc(symbol_count, sizeof(HuffmanCode));
    
    encoder->root = build_huffman_tree(frequencies, symbol_count);
    if (encoder->root) {
        generate_codes(encoder->root, encoder->codes, 0, 0);
    }
    
    return encoder;
}

BitStream* huffman_encode(HuffmanEncoder* encoder, uint8_t* data, size_t data_len) {
    BitStream* stream = bitstream_new(data_len * 2);
    
    for (size_t i = 0; i < data_len; i++) {
        uint8_t symbol = data[i];
        if (symbol < encoder->code_count) {
            HuffmanCode code = encoder->codes[symbol];
            bitstream_write_bits(stream, code.code, code.length);
        }
    }
    
    return stream;
}

static void free_huffman_tree(HuffmanNode* node) {
    if (!node) return;
    free_huffman_tree(node->left);
    free_huffman_tree(node->right);
    free(node);
}

void huffman_free(HuffmanEncoder* encoder) {
    if (encoder) {
        free(encoder->codes);
        free_huffman_tree(encoder->root);
        free(encoder);
    }
}

uint8_t* huffman_decode(HuffmanEncoder* encoder, BitStream* stream, size_t* out_len) {
    // Simplified decoder - full implementation would decode using tree
    *out_len = 0;
    return NULL;
}
