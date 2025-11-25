#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void print_usage() {
    printf("ULC-Unified: Intelligent Auto-Dispatcher\n");
    printf("Usage: ulc-auto <compress|decompress> <input> -o <output>\n");
}

typedef struct {
    double avg_line_len;
    double unique_ratio;
    int has_urls;
    int has_ips;
    int has_timestamps;
} LogProfile;

LogProfile analyze_log(const char* input_path) {
    LogProfile profile = {0};
    
    FILE* fp = fopen(input_path, "r");
    if (!fp) return profile;
    
    char buf[4096];
    size_t line_count = 0;
    size_t total_len = 0;
    size_t max_sample = 1000;
    
    // Simple hash set for unique line detection
    char** unique_lines = malloc(sizeof(char*) * max_sample);
    size_t unique_count = 0;
    
    while (line_count < max_sample && fgets(buf, sizeof(buf), fp)) {
        size_t len = strlen(buf);
        total_len += len;
        
        // Check for URLs
        if (strstr(buf, "http://") || strstr(buf, "https://") || strstr(buf, "/api/") || strstr(buf, "GET ") || strstr(buf, "POST ")) {
            profile.has_urls = 1;
        }
        
        // Check for IPs (simple heuristic: look for pattern like "xxx.xxx.xxx.xxx")
        for (size_t i = 0; i < len - 6; i++) {
            if (isdigit(buf[i]) && buf[i+1] == '.' && isdigit(buf[i+2])) {
                profile.has_ips = 1;
                break;
            }
        }
        
        // Check for timestamps (look for patterns like "2024-" or "[" at start)
        if (buf[0] == '[' || strstr(buf, "2024-") || strstr(buf, "2025-")) {
            profile.has_timestamps = 1;
        }
        
        // Track uniqueness (simplified)
        int is_unique = 1;
        for (size_t i = 0; i < unique_count; i++) {
            if (strcmp(unique_lines[i], buf) == 0) {
                is_unique = 0;
                break;
            }
        }
        if (is_unique && unique_count < max_sample) {
            unique_lines[unique_count++] = strdup(buf);
        }
        
        line_count++;
    }
    fclose(fp);
    
    profile.avg_line_len = (line_count > 0) ? (double)total_len / line_count : 0;
    profile.unique_ratio = (line_count > 0) ? (double)unique_count / line_count : 1.0;
    
    // Cleanup
    for (size_t i = 0; i < unique_count; i++) free(unique_lines[i]);
    free(unique_lines);
    
    return profile;
}

const char* select_best_engine(LogProfile profile) {
    // Decision tree based on log characteristics
    
    // If has URLs and long lines -> ULC-Hyper (best for web/app logs)
    if (profile.has_urls && profile.avg_line_len > 150) {
        printf("  Detected: Web/App logs (URLs, long lines)\n");
        return "bin\\ulc-hyper.exe";
    }
    
    // If very long lines and high uniqueness -> ULC-Hyper (complex data)
    if (profile.avg_line_len > 200 && profile.unique_ratio > 0.7) {
        printf("  Detected: Complex logs (long, unique)\n");
        return "bin\\ulc-hyper.exe";
    }
    
    // If short lines and structured -> ULC-C (fast, good for syslog)
    if (profile.avg_line_len < 100 && profile.has_timestamps && profile.has_ips) {
        printf("  Detected: Syslog (short, structured)\n");
        return "..\\ulc-c\\ulc.exe";
    }
    
    // Medium complexity -> ULC-Ultra (balanced)
    if (profile.avg_line_len >= 100 && profile.avg_line_len <= 200) {
        printf("  Detected: Medium complexity logs\n");
        return "bin\\ulc-ultra.exe";
    }
    
    // Default: ULC-C (safest, fastest)
    printf("  Detected: Standard logs (default)\n");
    return "..\\ulc-c\\ulc.exe";
}

int main(int argc, char** argv) {
    if (argc < 5) {
        print_usage();
        return 1;
    }

    const char* mode = argv[1];
    const char* input = argv[2];
    const char* output = argv[4];

    // Decompression
    if (strcmp(mode, "decompress") == 0) {
        FILE* fp = fopen(input, "rb");
        if (!fp) {
            printf("Error: Cannot open input file.\n");
            return 1;
        }
        char magic[4];
        fread(magic, 1, 4, fp);
        fclose(fp);

        char cmd[1024];
        if (memcmp(magic, "ULCH", 4) == 0) {
            sprintf(cmd, "bin\\ulc-hyper.exe decompress \"%s\" -o \"%s\"", input, output);
        } else if (memcmp(magic, "ULCU", 4) == 0) {
            sprintf(cmd, "bin\\ulc-ultra.exe decompress \"%s\" -o \"%s\"", input, output);
        } else if (memcmp(magic, "ULC", 3) == 0) {
            sprintf(cmd, "..\\ulc-c\\ulc.exe decompress \"%s\" -o \"%s\"", input, output);
        } else {
            printf("Error: Unknown format.\n");
            return 1;
        }
        return system(cmd);
    }

    // Compression
    if (strcmp(mode, "compress") == 0) {
        printf("[ULC-Unified] Analyzing log file...\n");
        
        LogProfile profile = analyze_log(input);
        printf("  Avg line length: %.1f chars\n", profile.avg_line_len);
        printf("  Unique ratio: %.2f\n", profile.unique_ratio);
        printf("  Has URLs: %s\n", profile.has_urls ? "Yes" : "No");
        
        const char* engine = select_best_engine(profile);
        
        // Extract engine name for display
        const char* engine_name = "Unknown";
        if (strstr(engine, "hyper")) engine_name = "ULC-Hyper";
        else if (strstr(engine, "ultra")) engine_name = "ULC-Ultra";
        else if (strstr(engine, "ulc.exe")) engine_name = "ULC-C";
        
        printf("[ULC-Unified] Selected: %s\n", engine_name);
        
        char cmd[1024];
        sprintf(cmd, "%s compress \"%s\" -o \"%s\"", engine, input, output);
        return system(cmd);
    }

    print_usage();
    return 1;
}
