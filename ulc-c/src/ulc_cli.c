#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ulc_compress.h"

static void print_usage(const char* prog_name) {
    printf("Ultra Log Compressor (ULC) - C Implementation\n\n");
    printf("Usage:\n");
    printf("  %s compress <input> [-o <output>]\n", prog_name);
    printf("  %s decompress <input> [-o <output>]\n", prog_name);
    printf("  %s info <input>\n\n", prog_name);
    printf("Commands:\n");
    printf("  compress    Compress a log file\n");
    printf("  decompress  Decompress a .ulc file\n");
    printf("  info        Show file information\n");
}

static const char* format_size(size_t bytes, char* buffer, size_t buffer_size) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    double size = (double)bytes;
    int unit_idx = 0;
    
    while (size >= 1024.0 && unit_idx < 3) {
        size /= 1024.0;
        unit_idx++;
    }
    
    snprintf(buffer, buffer_size, "%.2f %s", size, units[unit_idx]);
    return buffer;
}

static int cmd_compress(const char* input, const char* output) {
    char output_path[512];
    if (!output) {
        snprintf(output_path, sizeof(output_path), "%s.ulc", input);
        output = output_path;
    }
    
    printf("Compressing log file: %s\n", input);
    printf("Output: %s\n", output);
    
    size_t orig_size, comp_size;
    double duration;
    
    int result = ulc_compress_file(input, output, &orig_size, &comp_size, &duration);
    
    if (result != 0) {
        fprintf(stderr, "Compression failed\n");
        return 1;
    }
    
    char orig_buf[64], comp_buf[64];
    printf("\nCompression complete!\n");
    printf("  Original size:   %s\n", format_size(orig_size, orig_buf, sizeof(orig_buf)));
    printf("  Compressed size: %s\n", format_size(comp_size, comp_buf, sizeof(comp_buf)));
    printf("  Ratio:           %.2f\n", (double)orig_size / comp_size);
    printf("  Space savings:   %.2f%%\n", (1.0 - (double)comp_size / orig_size) * 100.0);
    printf("  Time:            %.3fs\n", duration);
    printf("  Speed:           %.2f MB/s\n", (orig_size / duration) / (1024.0 * 1024.0));
    
    return 0;
}

static int cmd_decompress(const char* input, const char* output) {
    char output_path[512];
    if (!output) {
        if (strlen(input) > 4 && strcmp(input + strlen(input) - 4, ".ulc") == 0) {
            strncpy(output_path, input, strlen(input) - 4);
            output_path[strlen(input) - 4] = '\0';
        } else {
            snprintf(output_path, sizeof(output_path), "%s.decompressed", input);
        }
        output = output_path;
    }
    
    printf("Decompressing: %s\n", input);
    printf("Output: %s\n", output);
    
    double duration;
    int result = ulc_decompress_file(input, output, &duration);
    
    if (result != 0) {
        fprintf(stderr, "Decompression failed\n");
        return 1;
    }
    
    printf("\nDecompression complete!\n");
    printf("  Time: %.3fs\n", duration);
    
    return 0;
}

static int cmd_info(const char* input) {
    printf("File: %s\n", input);
    printf("(Info command not fully implemented in this version)\n");
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* command = argv[1];
    
    if (strcmp(command, "compress") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing input file\n");
            print_usage(argv[0]);
            return 1;
        }
        
        const char* input = argv[2];
        const char* output = NULL;
        
        if (argc >= 5 && strcmp(argv[3], "-o") == 0) {
            output = argv[4];
        }
        
        return cmd_compress(input, output);
    } else if (strcmp(command, "decompress") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing input file\n");
            print_usage(argv[0]);
            return 1;
        }
        
        const char* input = argv[2];
        const char* output = NULL;
        
        if (argc >= 5 && strcmp(argv[3], "-o") == 0) {
            output = argv[4];
        }
        
        return cmd_decompress(input, output);
    } else if (strcmp(command, "info") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing input file\n");
            print_usage(argv[0]);
            return 1;
        }
        
        return cmd_info(argv[2]);
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}
