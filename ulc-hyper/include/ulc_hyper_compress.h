#ifndef ULC_HYPER_COMPRESS_H
#define ULC_HYPER_COMPRESS_H

#include <stddef.h>

// Main compression function
int hyper_compress_file(const char* input_path, const char* output_path, 
                       size_t* orig_size, size_t* comp_size, double* duration);

// Main decompression function
int hyper_decompress_file(const char* input_path, const char* output_path, 
                         double* duration);

#endif // ULC_HYPER_COMPRESS_H
