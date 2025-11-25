#ifndef ULC_COMPRESS_H
#define ULC_COMPRESS_H

#include "ulc_types.h"

// Compress log lines to file
int ulc_compress_file(const char* input_path, const char* output_path, 
                      size_t* orig_size, size_t* comp_size, double* duration);

// Decompress file to log lines
int ulc_decompress_file(const char* input_path, const char* output_path, double* duration);

#endif // ULC_COMPRESS_H
