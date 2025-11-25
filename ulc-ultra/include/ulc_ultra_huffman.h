#ifndef ULC_ULTRA_HUFFMAN_H
#define ULC_ULTRA_HUFFMAN_H

#include "ulc_ultra_types.h"

// Create Huffman encoder from frequency table
HuffmanEncoder* huffman_create(int* frequencies, size_t symbol_count);

// Encode data with Huffman codes
BitStream* huffman_encode(HuffmanEncoder* encoder, uint8_t* data, size_t data_len);

// Decode Huffman-encoded data
uint8_t* huffman_decode(HuffmanEncoder* encoder, BitStream* stream, size_t* out_len);

// Free Huffman encoder
void huffman_free(HuffmanEncoder* encoder);

// Bit stream operations
BitStream* bitstream_new(size_t initial_capacity);
void bitstream_write_bit(BitStream* stream, int bit);
void bitstream_write_bits(BitStream* stream, uint32_t value, int num_bits);
int bitstream_read_bit(BitStream* stream);
uint32_t bitstream_read_bits(BitStream* stream, int num_bits);
void bitstream_free(BitStream* stream);

#endif // ULC_ULTRA_HUFFMAN_H
