#ifndef ULC_ULTRA_COMPRESS_H
#define ULC_ULTRA_COMPRESS_H

#include "ulc_ultra_types.h"

// Create ultra compressor with compression level (1-10)
UltraCompressor* ultra_compressor_new(int compression_level);

// Compress log file with ultra compression
int ultra_compress_file(const char* input_path, const char* output_path,
                        size_t* orig_size, size_t* comp_size, double* duration);

// Decompress ultra-compressed file
int ultra_decompress_file(const char* input_path, const char* output_path, double* duration);

// Validate log format consistency
int validate_log_format(char** lines, size_t line_count, char** error_message);

// Free ultra compressor
void ultra_compressor_free(UltraCompressor* compressor);

#endif // ULC_ULTRA_COMPRESS_H
