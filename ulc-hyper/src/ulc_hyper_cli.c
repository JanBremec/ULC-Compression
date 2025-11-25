#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/ulc_hyper_compress.h"

int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: ulc-hyper <compress|decompress> <input> -o <output>\n");
        return 1;
    }
    
    const char* mode = argv[1];
    const char* input = argv[2];
    const char* output = argv[4];
    
    if (strcmp(mode, "compress") == 0) {
        size_t orig, comp;
        double duration;
        if (hyper_compress_file(input, output, &orig, &comp, &duration) == 0) {
            printf("Compressed: %zu -> %zu bytes (%.2fx) in %.3fs\n", 
                   orig, comp, (double)orig/comp, duration);
        } else {
            printf("Compression failed.\n");
            return 1;
        }
    } else if (strcmp(mode, "decompress") == 0) {
        double duration;
        if (hyper_decompress_file(input, output, &duration) == 0) {
            printf("Decompressed in %.3fs\n", duration);
        } else {
            printf("Decompression failed.\n");
            return 1;
        }
    }
    
    return 0;
}
