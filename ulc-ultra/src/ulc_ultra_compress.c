#include "../include/ulc_ultra_compress.h"
#include "../../ulc-c/include/ulc_parser.h"
#include "../../ulc-c/include/ulc_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <lzma.h>

// BWT implementation (simplified - production would use libbz2)
static void apply_bwt(uint8_t* data, size_t len, int* primary_index) {
    // Simplified BWT - for production, use BZ2_blockSort from libbz2
    // This is a placeholder that just returns the data as-is
    *primary_index = 0;
}

static void reverse_bwt(uint8_t* data, size_t len, int primary_index) {
    // Simplified reverse BWT
    // Production would use proper inverse BWT
}

UltraCompressor* ultra_compressor_new(int compression_level) {
    UltraCompressor* comp = malloc(sizeof(UltraCompressor));
    comp->compression_level = compression_level;
    comp->pattern_dict = NULL;  // No longer using pattern mining
    comp->huffman = NULL;  // No longer using Huffman
    comp->context = NULL;
    return comp;
}

void ultra_compressor_free(UltraCompressor* compressor) {
    if (compressor) {
        free(compressor);
    }
}

// Validate format consistency
int validate_log_format(char** lines, size_t line_count, char** error_message) {
    if (line_count < 100) {
        *error_message = strdup("Error: Minimum 100 lines required for ultra compression");
        return 0;
    }
    
    // Parse first 100 lines to detect format
    LogFormat dominant_format = LOG_FORMAT_RAW;
    int format_counts[10] = {0};
    
    for (size_t i = 0; i < (line_count < 100 ? line_count : 100); i++) {
        LogEntry* entry = parse_log_line(lines[i]);
        format_counts[entry->format]++;
        if (i == 0) dominant_format = entry->format;
        log_entry_free(entry);
    }
    
    // Check consistency (80% threshold)
    int max_count = 0;
    for (int i = 0; i < 10; i++) {
        if (format_counts[i] > max_count) {
            max_count = format_counts[i];
            dominant_format = i;
        }
    }
    
    if (max_count < 80) {
        *error_message = strdup("Error: Log format consistency < 80%. Mixed formats not supported.");
        return 0;
    }
    
    if (dominant_format == LOG_FORMAT_RAW) {
        *error_message = strdup("Warning: Unstructured logs detected. Compression may be suboptimal.");
        return 1;  // Warning, not error
    }
    
    return 1;
}

int ultra_compress_file(const char* input_path, const char* output_path,
                       size_t* orig_size, size_t* comp_size, double* duration) {
    clock_t start = clock();
    
    // Read input file
    FILE* fp = fopen(input_path, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open input file: %s\n", input_path);
        return -1;
    }
    
    // Read all lines
    char** lines = NULL;
    size_t line_count = 0;
    size_t line_capacity = 1024;
    lines = malloc(sizeof(char*) * line_capacity);
    
    char line_buffer[16384];
    size_t total_bytes = 0;
    
    while (fgets(line_buffer, sizeof(line_buffer), fp)) {
        size_t len = strlen(line_buffer);
        if (len > 0 && line_buffer[len-1] == '\n') line_buffer[len-1] = '\0';
        if (len > 1 && line_buffer[len-2] == '\r') line_buffer[len-2] = '\0';
        
        total_bytes += strlen(line_buffer) + 1;
        
        if (line_count >= line_capacity) {
            line_capacity *= 2;
            lines = realloc(lines, sizeof(char*) * line_capacity);
        }
        lines[line_count++] = strdup(line_buffer);
    }
    fclose(fp);
    
    *orig_size = total_bytes;
    
    // Validate format
    char* error_msg = NULL;
    if (!validate_log_format(lines, line_count, &error_msg)) {
        fprintf(stderr, "%s\n", error_msg);
        free(error_msg);
        for (size_t i = 0; i < line_count; i++) free(lines[i]);
        free(lines);
        return -1;
    }
    if (error_msg) {
        fprintf(stderr, "%s\n", error_msg);
        free(error_msg);
    }
    
    printf("ULC-Ultra v3: Hybrid Columnar Compression\n");
    
    // PHASE 1: Parse into structured entries
    printf("Parsing logs...\n");
    LogEntry** entries = malloc(sizeof(LogEntry*) * line_count);
    size_t max_fields = 0;
    for (size_t i = 0; i < line_count; i++) {
        entries[i] = parse_log_line(lines[i]);
        if (entries[i]->field_count > max_fields) max_fields = entries[i]->field_count;
    }
    
    // PHASE 2: Transpose to Columns
    printf("Transposing to %zu columns...\n", max_fields);
    // columns[col_idx][row_idx]
    char*** columns = malloc(sizeof(char**) * max_fields);
    for (size_t j = 0; j < max_fields; j++) {
        columns[j] = malloc(sizeof(char*) * line_count);
        for (size_t i = 0; i < line_count; i++) {
            if (j < entries[i]->field_count) {
                columns[j][i] = entries[i]->values[j]; // Reference only
            } else {
                columns[j][i] = ""; // Empty string for missing fields
            }
        }
    }
    
    // PHASE 3: Analyze and Serialize Columns
    printf("Analyzing columns and serializing...\n");
    ByteArray* serialized = bytearray_new(total_bytes);
    
    // Write metadata
    encode_varint(serialized, line_count);
    encode_varint(serialized, max_fields);
    
    for (size_t j = 0; j < max_fields; j++) {
        // Analyze column type and cardinality
        Dictionary* col_dict = dict_new(256);
        int is_numeric = 1;
        int is_ip = 1;
        
        for (size_t i = 0; i < line_count; i++) {
            const char* val = columns[j][i];
            dict_get_or_add(col_dict, val);
            
            // Check type (heuristic on first 100 non-empty values)
            if (i < 100 && strlen(val) > 0) {
                // Check numeric
                char* endptr;
                strtoll(val, &endptr, 10);
                if (*endptr != '\0') is_numeric = 0;
                
                // Check IP (simplified: contains dots and digits)
                int dots = 0;
                int digits = 0;
                for(size_t k=0; k<strlen(val); k++) {
                    if(val[k] == '.') dots++;
                    else if(val[k] >= '0' && val[k] <= '9') digits++;
                    else { is_ip = 0; break; }
                }
                if(dots < 3 || digits < 4) is_ip = 0;
            }
        }
        
        double unique_ratio = (double)col_dict->count / line_count;
        int encoding_type = 0; // 0=Raw, 1=Dict, 2=Delta, 3=IP_XOR
        
        if (is_numeric && line_count > 10) encoding_type = 2; // Delta
        else if (is_ip && line_count > 10) encoding_type = 3; // IP XOR
        else if (unique_ratio < 0.5 || col_dict->count < 256) encoding_type = 1; // Dict (Aggressive)
        else encoding_type = 0; // Raw
        
        // Write column encoding type
        bytearray_append(serialized, (uint8_t*)&encoding_type, 1);
        
        if (encoding_type == 1) {
            // DICTIONARY ENCODING
            encode_varint(serialized, col_dict->count);
            for (size_t k = 0; k < col_dict->count; k++) {
                const char* val = col_dict->entries[k].key;
                encode_varint(serialized, strlen(val));
                bytearray_append(serialized, (uint8_t*)val, strlen(val));
            }
            for (size_t i = 0; i < line_count; i++) {
                int id = dict_get_or_add(col_dict, columns[j][i]);
                encode_varint(serialized, id);
            }
        } else if (encoding_type == 2) {
            // DELTA ENCODING (Numeric)
            long long prev = 0;
            for (size_t i = 0; i < line_count; i++) {
                if (strlen(columns[j][i]) == 0) {
                    encode_varint(serialized, 0); // Handle empty as 0 delta? Or flag? Simplified: 0
                    continue;
                }
                long long val = strtoll(columns[j][i], NULL, 10);
                long long delta = val - prev;
                // ZigZag encode delta to handle negatives efficiently
                uint64_t zigzag = (delta << 1) ^ (delta >> 63);
                encode_varint(serialized, zigzag);
                prev = val;
            }
        } else if (encoding_type == 3) {
            // IP XOR ENCODING
            // Convert IP to 32-bit int, XOR with prev
            uint32_t prev_ip = 0;
            for (size_t i = 0; i < line_count; i++) {
                unsigned int a, b, c, d;
                if (sscanf(columns[j][i], "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
                    uint32_t ip = (a << 24) | (b << 16) | (c << 8) | d;
                    uint32_t xor_val = ip ^ prev_ip;
                    encode_varint(serialized, xor_val);
                    prev_ip = ip;
                } else {
                    encode_varint(serialized, 0); // Failover
                }
            }
        } else {
            // RAW ENCODING
            for (size_t i = 0; i < line_count; i++) {
                const char* val = columns[j][i];
                encode_varint(serialized, strlen(val));
                bytearray_append(serialized, (uint8_t*)val, strlen(val));
            }
        }
        
        dict_free(col_dict);
    }
    
    printf("Serialized size: %zu bytes\n", serialized->length);
    
    // PHASE 4: Apply BWT (Disabled for testing - might be overhead on small files)
    // printf("Applying BWT preprocessing...\n");
    int bwt_primary = 0;
    // apply_bwt(serialized->data, serialized->length, &bwt_primary);
    
    // PHASE 5: LZMA Compression
    printf("Applying LZMA (128MB dict)...\n");
    
    lzma_options_lzma opt;
    lzma_lzma_preset(&opt, 9 | LZMA_PRESET_EXTREME);
    opt.dict_size = 128 * 1024 * 1024;
    opt.lc = 4; opt.lp = 0; opt.pb = 2;
    opt.mf = LZMA_MF_BT4;
    opt.depth = 512;
    
    lzma_filter filters[] = {
        { .id = LZMA_FILTER_LZMA2, .options = &opt },
        { .id = LZMA_VLI_UNKNOWN, .options = NULL }
    };
    
    size_t compressed_capacity = serialized->length + 1024;
    uint8_t* compressed = malloc(compressed_capacity);
    
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_stream_encoder(&strm, filters, LZMA_CHECK_CRC64);
    
    if (ret != LZMA_OK) {
        fprintf(stderr, "Error: LZMA encoder init failed\n");
        free(compressed);
        bytearray_free(serialized);
        // Cleanup columns
        for(size_t j=0; j<max_fields; j++) free(columns[j]);
        free(columns);
        return -1;
    }
    
    strm.next_in = serialized->data;
    strm.avail_in = serialized->length;
    strm.next_out = compressed;
    strm.avail_out = compressed_capacity;
    
    ret = lzma_code(&strm, LZMA_FINISH);
    
    if (ret != LZMA_STREAM_END) {
        fprintf(stderr, "Error: LZMA compression failed\n");
        lzma_end(&strm);
        free(compressed);
        bytearray_free(serialized);
        for(size_t j=0; j<max_fields; j++) free(columns[j]);
        free(columns);
        return -1;
    }
    
    size_t compressed_size = strm.total_out;
    lzma_end(&strm);
    
    printf("Final compressed size: %zu bytes\n", compressed_size);
    
    // Write output
    FILE* out_fp = fopen(output_path, "wb");
    if (!out_fp) {
        fprintf(stderr, "Error: Cannot open output file\n");
        free(compressed);
        bytearray_free(serialized);
        for(size_t j=0; j<max_fields; j++) free(columns[j]);
        free(columns);
        return -1;
    }
    
    fwrite(ULCU_MAGIC, 1, ULCU_MAGIC_LEN, out_fp);
    fwrite(&bwt_primary, sizeof(int), 1, out_fp);
    fwrite(compressed, 1, compressed_size, out_fp);
    fclose(out_fp);
    
    *comp_size = compressed_size + ULCU_MAGIC_LEN + sizeof(int);
    
    // Cleanup
    free(compressed);
    bytearray_free(serialized);
    for(size_t j=0; j<max_fields; j++) free(columns[j]);
    free(columns);
    for (size_t i = 0; i < line_count; i++) {
        free(lines[i]);
        log_entry_free(entries[i]);
    }
    free(lines);
    free(entries);
    ultra_compressor_free(NULL); // Helper free
    
    clock_t end = clock();
    *duration = (double)(end - start) / CLOCKS_PER_SEC;
    
    return 0;
}

int ultra_decompress_file(const char* input_path, const char* output_path, double* duration) {
    clock_t start = clock();
    
    FILE* fp = fopen(input_path, "rb");
    if (!fp) return -1;
    
    char magic[ULCU_MAGIC_LEN];
    if (fread(magic, 1, ULCU_MAGIC_LEN, fp) != ULCU_MAGIC_LEN || memcmp(magic, ULCU_MAGIC, ULCU_MAGIC_LEN) != 0) {
        fclose(fp); return -1;
    }
    
    int bwt_primary;
    fread(&bwt_primary, sizeof(int), 1, fp);
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, ULCU_MAGIC_LEN + sizeof(int), SEEK_SET);
    
    size_t compressed_size = file_size - ULCU_MAGIC_LEN - sizeof(int);
    uint8_t* compressed = malloc(compressed_size);
    fread(compressed, 1, compressed_size, fp);
    fclose(fp);
    
    // Decompress LZMA
    size_t decompressed_capacity = compressed_size * 30; // Safety margin
    uint8_t* decompressed = malloc(decompressed_capacity);
    
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_stream_decoder(&strm, UINT64_MAX, 0);
    strm.next_in = compressed;
    strm.avail_in = compressed_size;
    strm.next_out = decompressed;
    strm.avail_out = decompressed_capacity;
    lzma_code(&strm, LZMA_FINISH);
    size_t decompressed_size = strm.total_out;
    lzma_end(&strm);
    free(compressed);
    
    // Reverse BWT
    reverse_bwt(decompressed, decompressed_size, bwt_primary);
    
    // Parse Columns
    size_t offset = 0;
    uint64_t line_count = decode_varint(decompressed, &offset);
    uint64_t max_fields = decode_varint(decompressed, &offset);
    
    char*** columns = malloc(sizeof(char**) * max_fields);
    
    for (size_t j = 0; j < max_fields; j++) {
        columns[j] = malloc(sizeof(char*) * line_count);
        uint8_t encoding_type = decompressed[offset++];
        
        if (encoding_type == 1) {
            // DICTIONARY
            uint64_t dict_count = decode_varint(decompressed, &offset);
            char** dict = malloc(sizeof(char*) * dict_count);
            for (size_t k = 0; k < dict_count; k++) {
                uint64_t len = decode_varint(decompressed, &offset);
                dict[k] = malloc(len + 1);
                memcpy(dict[k], decompressed + offset, len);
                dict[k][len] = '\0';
                offset += len;
            }
            
            for (size_t i = 0; i < line_count; i++) {
                uint64_t id = decode_varint(decompressed, &offset);
                if (id < dict_count) columns[j][i] = strdup(dict[id]);
                else columns[j][i] = strdup("");
            }
            
            for(size_t k=0; k<dict_count; k++) free(dict[k]);
            free(dict);
        } else if (encoding_type == 2) {
            // DELTA
            long long prev = 0;
            for (size_t i = 0; i < line_count; i++) {
                uint64_t zigzag = decode_varint(decompressed, &offset);
                long long delta = (zigzag >> 1) ^ -(zigzag & 1);
                long long val = prev + delta;
                char buf[64];
                snprintf(buf, sizeof(buf), "%lld", val);
                columns[j][i] = strdup(buf);
                prev = val;
            }
        } else if (encoding_type == 3) {
            // IP XOR
            uint32_t prev_ip = 0;
            for (size_t i = 0; i < line_count; i++) {
                uint32_t xor_val = decode_varint(decompressed, &offset);
                uint32_t ip = prev_ip ^ xor_val;
                char buf[64];
                snprintf(buf, sizeof(buf), "%u.%u.%u.%u", 
                        (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
                columns[j][i] = strdup(buf);
                prev_ip = ip;
            }
        } else {
            // RAW
            for (size_t i = 0; i < line_count; i++) {
                uint64_t len = decode_varint(decompressed, &offset);
                columns[j][i] = malloc(len + 1);
                memcpy(columns[j][i], decompressed + offset, len);
                columns[j][i][len] = '\0';
                offset += len;
            }
        }
    }
    
    free(decompressed);
    
    // Write output
    FILE* out_fp = fopen(output_path, "w");
    for (size_t i = 0; i < line_count; i++) {
        for (size_t j = 0; j < max_fields; j++) {
            if (columns[j][i] && strlen(columns[j][i]) > 0) {
                fprintf(out_fp, "%s", columns[j][i]);
                if (j < max_fields - 1) {
                    // This is tricky: we don't know the original separators
                    // ULC-C parser strips them. We'll assume space for now
                    // Ideally we'd store separators too
                    fprintf(out_fp, " ");
                }
            }
        }
        fprintf(out_fp, "\n");
    }
    fclose(out_fp);
    
    // Cleanup
    for(size_t j=0; j<max_fields; j++) {
        for(size_t i=0; i<line_count; i++) free(columns[j][i]);
        free(columns[j]);
    }
    free(columns);
    
    clock_t end = clock();
    *duration = (double)(end - start) / CLOCKS_PER_SEC;
    
    return 0;
}
