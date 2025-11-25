#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ulc_ultra_compress.h"

static void print_usage(const char* prog_name) {
    printf("ULC-Ultra: Maximum Compression for Structured Logs\n\n");
    printf("Usage:\n");
    printf("  %s compress <input> [-o <output>]\n", prog_name);
    printf("  %s decompress <input> [-o <output>]\n\n", prog_name);
    printf("WARNING: ULC-Ultra is optimized for maximum compression ratio.\n");
    printf("         It is SLOWER and uses MORE MEMORY than standard ULC.\n\n");
    printf("Supported formats:\n");
    printf("  ✓ Apache/Nginx logs\n");
    printf("  ✓ Syslog\n");
    printf("  ✓ Structured application logs\n");
    printf("  ✓ Database logs\n\n");
    printf("Requirements:\n");
    printf("  - Minimum 100 lines\n");
    printf("  - 80%% format consistency\n");
    printf("  - Repetitive patterns\n");
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
        snprintf(output_path, sizeof(output_path), "%s.ulcu", input);
        output = output_path;
    }
    
    printf("ULC-Ultra Compression\n");
    printf("====================\n");
    printf("Input:  %s\n", input);
    printf("Output: %s\n\n", output);
    
    size_t orig_size, comp_size;
    double duration;
    
    int result = ultra_compress_file(input, output, &orig_size, &comp_size, &duration);
    
    if (result != 0) {
        fprintf(stderr, "\nCompression failed!\n");
        return 1;
    }
    
    char orig_buf[64], comp_buf[64];
    printf("\n✓ Compression complete!\n");
    printf("  Original size:   %s\n", format_size(orig_size, orig_buf, sizeof(orig_buf)));
    printf("  Compressed size: %s\n", format_size(comp_size, comp_buf, sizeof(comp_buf)));
    printf("  Ratio:           %.2fx\n", (double)orig_size / comp_size);
    printf("  Space savings:   %.2f%%\n", (1.0 - (double)comp_size / orig_size) * 100.0);
    printf("  Time:            %.3fs\n", duration);
    printf("  Speed:           %.2f MB/s\n", (orig_size / duration) / (1024.0 * 1024.0));
    
    return 0;
}

static int cmd_decompress(const char* input, const char* output) {
    char output_path[512];
    if (!output) {
        if (strlen(input) > 5 && strcmp(input + strlen(input) - 5, ".ulcu") == 0) {
            strncpy(output_path, input, strlen(input) - 5);
            output_path[strlen(input) - 5] = '\0';
        } else {
            snprintf(output_path, sizeof(output_path), "%s.decompressed", input);
        }
        output = output_path;
    }
    
    printf("ULC-Ultra Decompression\n");
    printf("=======================\n");
    printf("Input:  %s\n", input);
    printf("Output: %s\n\n", output);
    
    double duration;
    int result = ultra_decompress_file(input, output, &duration);
    
    if (result != 0) {
        fprintf(stderr, "\nDecompression failed!\n");
        return 1;
    }
    
    printf("\n✓ Decompression complete!\n");
    printf("  Time: %.3fs\n", duration);
    
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
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}
