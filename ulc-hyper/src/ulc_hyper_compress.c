#include "../include/ulc_hyper_compress.h"
#include "../include/ulc_hyper_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <lzma.h>

#define HYPER_MAGIC "ULCH"
#define HYPER_MAGIC_LEN 4

// --- Utilities ---

typedef struct {
    uint8_t* data;
    size_t length;
    size_t capacity;
} ByteArray;

ByteArray* bytearray_new(size_t cap) {
    ByteArray* arr = malloc(sizeof(ByteArray));
    arr->capacity = cap > 0 ? cap : 1024;
    arr->data = malloc(arr->capacity);
    arr->length = 0;
    return arr;
}

void bytearray_append(ByteArray* arr, const void* data, size_t len) {
    if (arr->length + len > arr->capacity) {
        while (arr->length + len > arr->capacity) arr->capacity *= 2;
        arr->data = realloc(arr->data, arr->capacity);
    }
    memcpy(arr->data + arr->length, data, len);
    arr->length += len;
}

void bytearray_free(ByteArray* arr) {
    if (arr) { free(arr->data); free(arr); }
}

void encode_varint(ByteArray* out, uint64_t value) {
    while (value >= 0x80) {
        uint8_t byte = (value & 0x7F) | 0x80;
        bytearray_append(out, &byte, 1);
        value >>= 7;
    }
    uint8_t byte = (uint8_t)value;
    bytearray_append(out, &byte, 1);
}

uint64_t decode_varint(const uint8_t* data, size_t* offset) {
    uint64_t value = 0;
    int shift = 0;
    while (1) {
        uint8_t byte = data[(*offset)++];
        value |= (uint64_t)(byte & 0x7F) << shift;
        if (!(byte & 0x80)) break;
        shift += 7;
    }
    return value;
}

// --- Dictionary ---

typedef struct {
    char* key;
    int id;
} DictEntry;

typedef struct {
    DictEntry* entries;
    size_t count;
    size_t capacity;
} Dictionary;

Dictionary* dict_new(size_t cap) {
    Dictionary* d = malloc(sizeof(Dictionary));
    d->capacity = cap > 0 ? cap : 128;
    d->entries = malloc(sizeof(DictEntry) * d->capacity);
    d->count = 0;
    return d;
}

int dict_get_or_add(Dictionary* d, const char* key) {
    for (size_t i = 0; i < d->count; i++) {
        if (strcmp(d->entries[i].key, key) == 0) return d->entries[i].id;
    }
    if (d->count >= d->capacity) {
        d->capacity *= 2;
        d->entries = realloc(d->entries, sizeof(DictEntry) * d->capacity);
    }
    d->entries[d->count].key = strdup(key);
    d->entries[d->count].id = d->count;
    return d->count++;
}

void dict_free(Dictionary* d) {
    if (d) {
        for(size_t i=0; i<d->count; i++) free(d->entries[i].key);
        free(d->entries);
        free(d);
    }
}

// --- Tokenization ---

TokenStream* tokenize_field(const char* field) {
    TokenStream* ts = malloc(sizeof(TokenStream));
    ts->capacity = 8;
    ts->tokens = malloc(sizeof(Token) * ts->capacity);
    ts->count = 0;
    
    size_t len = strlen(field);
    size_t start = 0;
    
    for (size_t i = 0; i <= len; i++) {
        char c = field[i];
        int is_delim = (c == '/' || c == ' ' || c == '?' || c == '&' || c == '=' || c == ':' || c == '[' || c == ']' || c == '"' || c == '\0');
        
        if (is_delim) {
            // Add preceding token if exists
            if (i > start) {
                if (ts->count >= ts->capacity) {
                    ts->capacity *= 2;
                    ts->tokens = realloc(ts->tokens, sizeof(Token) * ts->capacity);
                }
                size_t tok_len = i - start;
                ts->tokens[ts->count].value = malloc(tok_len + 1);
                memcpy(ts->tokens[ts->count].value, field + start, tok_len);
                ts->tokens[ts->count].value[tok_len] = '\0';
                ts->tokens[ts->count].type = TOKEN_TYPE_LITERAL; // Refine later
                ts->count++;
            }
            
            // Add delimiter as token (unless it's null terminator)
            if (c != '\0') {
                if (ts->count >= ts->capacity) {
                    ts->capacity *= 2;
                    ts->tokens = realloc(ts->tokens, sizeof(Token) * ts->capacity);
                }
                ts->tokens[ts->count].value = malloc(2);
                ts->tokens[ts->count].value[0] = c;
                ts->tokens[ts->count].value[1] = '\0';
                ts->tokens[ts->count].type = TOKEN_TYPE_DELIMITER;
                ts->count++;
            }
            start = i + 1;
        }
    }
    return ts;
}

void tokenstream_free(TokenStream* ts) {
    if (ts) {
        for(size_t i=0; i<ts->count; i++) free(ts->tokens[i].value);
        free(ts->tokens);
        free(ts);
    }
}

// --- Compression Engine ---

int hyper_compress_file(const char* input_path, const char* output_path, 
                       size_t* orig_size, size_t* comp_size, double* duration) {
    clock_t start = clock();
    
    FILE* fp = fopen(input_path, "r");
    if (!fp) return -1;
    
    // Read lines
    char** lines = NULL;
    size_t line_count = 0;
    size_t line_cap = 1024;
    lines = malloc(sizeof(char*) * line_cap);
    char buf[16384];
    size_t total_bytes = 0;
    
    while (fgets(buf, sizeof(buf), fp)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
        if (len > 1 && buf[len-2] == '\r') buf[len-2] = '\0';
        total_bytes += strlen(buf) + 1;
        
        if (line_count >= line_cap) {
            line_cap *= 2;
            lines = realloc(lines, sizeof(char*) * line_cap);
        }
        lines[line_count++] = strdup(buf);
    }
    fclose(fp);
    *orig_size = total_bytes;
    
    printf("ULC-Hyper: Semantic Decomposition\n");
    printf("Parsing %zu lines...\n", line_count);
    
    // 1. Initial Parse (Space separated for now, ULC-C style)
    // We will treat each space-separated part as a "Major Column"
    // Then decompose each Major Column into "Minor Columns" (Tokens)
    
    size_t max_cols = 0;
    char*** grid = malloc(sizeof(char**) * line_count);
    size_t* col_counts = malloc(sizeof(size_t) * line_count);
    
    for (size_t i = 0; i < line_count; i++) {
        // Simple split by space (respecting quotes would be better, but keeping it simple for now)
        // Actually, let's use a smarter split that respects quotes/brackets like ULC-C
        // For this demo, we'll just use the tokenizer we wrote but only split on spaces
        // Wait, let's reuse the tokenizer but flatten it? No, let's just use the tokenizer on the WHOLE LINE first?
        // Better: Split by space to get fields. Then tokenize fields.
        
        // Simplified field parsing
        size_t cap = 16;
        grid[i] = malloc(sizeof(char*) * cap);
        size_t cols = 0;
        char* ptr = lines[i];
        while (*ptr) {
            while (*ptr == ' ') ptr++;
            if (!*ptr) break;
            char* start = ptr;
            if (*ptr == '[') {
                while (*ptr && *ptr != ']') ptr++;
                if (*ptr) ptr++;
            } else if (*ptr == '"') {
                ptr++;
                while (*ptr && *ptr != '"') ptr++;
                if (*ptr) ptr++;
            } else {
                while (*ptr && *ptr != ' ') ptr++;
            }
            
            size_t len = ptr - start;
            if (cols >= cap) {
                cap *= 2;
                grid[i] = realloc(grid[i], sizeof(char*) * cap);
            }
            grid[i][cols] = malloc(len + 1);
            memcpy(grid[i][cols], start, len);
            grid[i][cols][len] = '\0';
            cols++;
        }
        col_counts[i] = cols;
        if (cols > max_cols) max_cols = cols;
    }
    
    printf("Detected %zu max columns. Decomposing...\n", max_cols);
    
    // 2. Semantic Decomposition & Serialization
    ByteArray* serialized = bytearray_new(total_bytes);
    encode_varint(serialized, line_count);
    encode_varint(serialized, max_cols);
    
    // We process column by column (Major Columns)
    for (size_t c = 0; c < max_cols; c++) {
        // Analyze Column First
        Dictionary* col_dict = dict_new(256);
        int is_numeric = 1;
        int is_ip = 1;
        size_t non_empty_count = 0;
        
        for (size_t i = 0; i < line_count; i++) {
            if (c < col_counts[i]) {
                const char* val = grid[i][c];
                if (strlen(val) > 0) {
                    non_empty_count++;
                    dict_get_or_add(col_dict, val);
                    
                    // Check type (heuristic on first 100 non-empty)
                    if (non_empty_count < 100) {
                        char* endptr;
                        strtoll(val, &endptr, 10);
                        if (*endptr != '\0') is_numeric = 0;
                        
                        int dots = 0, digits = 0;
                        for(size_t k=0; k<strlen(val); k++) {
                            if(val[k] == '.') dots++;
                            else if(isdigit(val[k])) digits++;
                            else { is_ip = 0; break; }
                        }
                        if(dots < 3 || digits < 4) is_ip = 0;
                    }
                }
            }
        }
        
        double unique_ratio = (double)col_dict->count / line_count;
        
        // Decision Logic
        // 0=Raw (Hyper Decomp), 1=Dict, 2=Delta, 3=IP_XOR
        int encoding_type = 0;
        
        if (is_numeric && non_empty_count > 10) encoding_type = 2;
        else if (is_ip && non_empty_count > 10) encoding_type = 3;
        else if (unique_ratio < 0.5 || col_dict->count < 256) encoding_type = 1;
        else encoding_type = 0; // High cardinality string -> Hyper Decomp
        
        // Write Encoding Type
        bytearray_append(serialized, (uint8_t*)&encoding_type, 1);
        
        if (encoding_type == 1) {
            // DICTIONARY (v3 style)
            encode_varint(serialized, col_dict->count);
            for (size_t k = 0; k < col_dict->count; k++) {
                encode_varint(serialized, strlen(col_dict->entries[k].key));
                bytearray_append(serialized, col_dict->entries[k].key, strlen(col_dict->entries[k].key));
            }
            for (size_t i = 0; i < line_count; i++) {
                if (c < col_counts[i]) {
                    int id = dict_get_or_add(col_dict, grid[i][c]);
                    encode_varint(serialized, id);
                } else {
                    encode_varint(serialized, 0); // Should be handled better, but sticking to v3 logic
                }
            }
        } else if (encoding_type == 2) {
            // DELTA (v3 style)
            long long prev = 0;
            for (size_t i = 0; i < line_count; i++) {
                if (c < col_counts[i] && strlen(grid[i][c]) > 0) {
                    long long val = strtoll(grid[i][c], NULL, 10);
                    long long delta = val - prev;
                    uint64_t zigzag = (delta << 1) ^ (delta >> 63);
                    encode_varint(serialized, zigzag);
                    prev = val;
                } else {
                    encode_varint(serialized, 0);
                }
            }
        } else if (encoding_type == 3) {
            // IP XOR (v3 style)
            uint32_t prev_ip = 0;
            for (size_t i = 0; i < line_count; i++) {
                unsigned int a, b, c, d;
                if (c < col_counts[i] && sscanf(grid[i][c], "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
                    uint32_t ip = (a << 24) | (b << 16) | (c << 8) | d;
                    uint32_t xor_val = ip ^ prev_ip;
                    encode_varint(serialized, xor_val);
                    prev_ip = ip;
                } else {
                    encode_varint(serialized, 0);
                }
            }
        } else {
            // HYPER DECOMPOSITION vs RAW
            // Analyze if decomposition is actually beneficial
            TokenStream** streams = malloc(sizeof(TokenStream*) * line_count);
            size_t max_tokens = 0;
            size_t total_token_count = 0;
            
            // Tokenize first
            for (size_t i = 0; i < line_count; i++) {
                if (c < col_counts[i]) {
                    streams[i] = tokenize_field(grid[i][c]);
                    if (streams[i]->count > max_tokens) max_tokens = streams[i]->count;
                    total_token_count += streams[i]->count;
                } else {
                    streams[i] = tokenize_field("");
                }
            }
            
            // Check Token Redundancy AND Length
            Dictionary* token_dict = dict_new(1024);
            size_t total_len = 0;
            for (size_t i = 0; i < line_count; i++) {
                if (c < col_counts[i]) total_len += strlen(grid[i][c]);
                for (size_t k = 0; k < streams[i]->count; k++) {
                    dict_get_or_add(token_dict, streams[i]->tokens[k].value);
                }
            }
            
            double token_unique_ratio = (total_token_count > 0) ? (double)token_dict->count / total_token_count : 1.0;
            double avg_len = (double)total_len / line_count;
            dict_free(token_dict);
            
            // Heuristic:
            // 1. If tokens are mostly unique (> 50%), use Raw.
            // 2. If string is short (< 15 chars), decomposition overhead outweighs benefits. Use Raw.
            if (token_unique_ratio > 0.5 || avg_len < 15.0) {
                // FALLBACK TO RAW (v3 style)
                serialized->length--; 
                uint8_t raw_type = 4;
                bytearray_append(serialized, &raw_type, 1);
                
                for (size_t i = 0; i < line_count; i++) {
                    if (c < col_counts[i]) {
                        const char* val = grid[i][c];
                        encode_varint(serialized, strlen(val));
                        bytearray_append(serialized, val, strlen(val));
                    } else {
                        encode_varint(serialized, 0);
                    }
                }
            } else {
            // Check if token count is constant
            int is_constant_count = 1;
            if (line_count > 0) {
                size_t first_count = streams[0]->count;
                for (size_t i = 1; i < line_count; i++) {
                    if (streams[i]->count != first_count) {
                        is_constant_count = 0;
                        break;
                    }
                }
            }
            
            // Write Constant Count Flag
            // We need to signal this. We can use a bit in max_tokens or a separate byte.
            // Let's use a separate byte before max_tokens? No, max_tokens is read first.
            // Let's write it AFTER max_tokens.
            
            encode_varint(serialized, max_tokens);
            bytearray_append(serialized, (uint8_t*)&is_constant_count, 1);
            
            if (is_constant_count) {
                // Write count once (if line_count > 0)
                if (line_count > 0) encode_varint(serialized, streams[0]->count);
            } else {
                // Store Token Counts per row
                for (size_t i = 0; i < line_count; i++) {
                    encode_varint(serialized, streams[i]->count);
                }
            }
            
            for (size_t sc = 0; sc < max_tokens; sc++) {
                    Dictionary* sub_dict = dict_new(256);
                    for (size_t i = 0; i < line_count; i++) {
                        if (sc < streams[i]->count) dict_get_or_add(sub_dict, streams[i]->tokens[sc].value);
                    }
                    
                    double ratio = (double)sub_dict->count / line_count;
                    int use_dict = (ratio < 0.5 || sub_dict->count < 256);
                    
                    bytearray_append(serialized, (uint8_t*)&use_dict, 1);
                    
                    if (use_dict) {
                        encode_varint(serialized, sub_dict->count);
                        for (size_t k = 0; k < sub_dict->count; k++) {
                            encode_varint(serialized, strlen(sub_dict->entries[k].key));
                            bytearray_append(serialized, sub_dict->entries[k].key, strlen(sub_dict->entries[k].key));
                        }
                        for (size_t i = 0; i < line_count; i++) {
                            if (sc < streams[i]->count) {
                                int id = dict_get_or_add(sub_dict, streams[i]->tokens[sc].value);
                                encode_varint(serialized, id);
                            }
                        }
                    } else {
                        for (size_t i = 0; i < line_count; i++) {
                            if (sc < streams[i]->count) {
                                const char* val = streams[i]->tokens[sc].value;
                                encode_varint(serialized, strlen(val));
                                bytearray_append(serialized, val, strlen(val));
                            }
                        }
                    }
                    dict_free(sub_dict);
                }
            }
            
            for(size_t i=0; i<line_count; i++) tokenstream_free(streams[i]);
            free(streams);
        }
        dict_free(col_dict);
    }
    
    printf("Serialized size: %zu bytes\n", serialized->length);
    
    // 3. LZMA Compression
    lzma_options_lzma opt;
    lzma_lzma_preset(&opt, 9 | LZMA_PRESET_EXTREME);
    opt.dict_size = 128 * 1024 * 1024;
    opt.lc = 4; opt.lp = 0; opt.pb = 2;
    opt.mf = LZMA_MF_BT4;
    opt.depth = 512;
    
    lzma_filter filters[] = { { .id = LZMA_FILTER_LZMA2, .options = &opt }, { .id = LZMA_VLI_UNKNOWN, .options = NULL } };
    
    size_t comp_cap = serialized->length + 1024;
    uint8_t* compressed = malloc(comp_cap);
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_stream_encoder(&strm, filters, LZMA_CHECK_CRC64);
    strm.next_in = serialized->data;
    strm.avail_in = serialized->length;
    strm.next_out = compressed;
    strm.avail_out = comp_cap;
    lzma_code(&strm, LZMA_FINISH);
    size_t comp_len = strm.total_out;
    lzma_end(&strm);
    
    // Write output
    FILE* out_fp = fopen(output_path, "wb");
    fwrite(HYPER_MAGIC, 1, HYPER_MAGIC_LEN, out_fp);
    fwrite(compressed, 1, comp_len, out_fp);
    fclose(out_fp);
    
    *comp_size = comp_len + HYPER_MAGIC_LEN;
    
    // Cleanup
    free(compressed);
    bytearray_free(serialized);
    for(size_t i=0; i<line_count; i++) {
        for(size_t j=0; j<col_counts[i]; j++) free(grid[i][j]);
        free(grid[i]);
        free(lines[i]);
    }
    free(grid);
    free(col_counts);
    free(lines);
    
    clock_t end = clock();
    *duration = (double)(end - start) / CLOCKS_PER_SEC;
    return 0;
}

int hyper_decompress_file(const char* input_path, const char* output_path, double* duration) {
    clock_t start = clock();
    
    FILE* fp = fopen(input_path, "rb");
    if (!fp) return -1;
    
    char magic[HYPER_MAGIC_LEN];
    fread(magic, 1, HYPER_MAGIC_LEN, fp);
    if (memcmp(magic, HYPER_MAGIC, HYPER_MAGIC_LEN) != 0) { fclose(fp); return -1; }
    
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, HYPER_MAGIC_LEN, SEEK_SET);
    size_t comp_len = fsize - HYPER_MAGIC_LEN;
    uint8_t* compressed = malloc(comp_len);
    fread(compressed, 1, comp_len, fp);
    fclose(fp);
    
    // Decompress
    size_t decomp_cap = comp_len * 30;
    uint8_t* decompressed = malloc(decomp_cap);
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_stream_decoder(&strm, UINT64_MAX, 0);
    strm.next_in = compressed;
    strm.avail_in = comp_len;
    strm.next_out = decompressed;
    strm.avail_out = decomp_cap;
    lzma_code(&strm, LZMA_FINISH);
    lzma_end(&strm);
    free(compressed);
    
    // Parse
    size_t offset = 0;
    uint64_t line_count = decode_varint(decompressed, &offset);
    uint64_t max_cols = decode_varint(decompressed, &offset);
    
    // Reconstruct grid
    // grid[row][col] -> string (we'll build this from tokens)
    char*** grid = malloc(sizeof(char**) * line_count);
    for(size_t i=0; i<line_count; i++) grid[i] = calloc(max_cols, sizeof(char*));
    
    for (size_t c = 0; c < max_cols; c++) {
        uint8_t encoding_type = decompressed[offset++];
        
        if (encoding_type == 1) {
            // DICTIONARY
            uint64_t dict_count = decode_varint(decompressed, &offset);
            char** dict = malloc(sizeof(char*) * dict_count);
            for(size_t k=0; k<dict_count; k++) {
                uint64_t len = decode_varint(decompressed, &offset);
                dict[k] = malloc(len+1);
                memcpy(dict[k], decompressed+offset, len);
                dict[k][len] = '\0';
                offset += len;
            }
            for(size_t i=0; i<line_count; i++) {
                uint64_t id = decode_varint(decompressed, &offset);
                if (id < dict_count) grid[i][c] = strdup(dict[id]);
                else grid[i][c] = strdup("");
            }
            for(size_t k=0; k<dict_count; k++) free(dict[k]);
            free(dict);
        } else if (encoding_type == 2) {
            // DELTA
            long long prev = 0;
            for(size_t i=0; i<line_count; i++) {
                uint64_t zigzag = decode_varint(decompressed, &offset);
                long long delta = (zigzag >> 1) ^ -(zigzag & 1);
                long long val = prev + delta;
                char buf[64];
                snprintf(buf, sizeof(buf), "%lld", val);
                grid[i][c] = strdup(buf);
                prev = val;
            }
        } else if (encoding_type == 3) {
            // IP XOR
            uint32_t prev_ip = 0;
            for(size_t i=0; i<line_count; i++) {
                uint32_t xor_val = decode_varint(decompressed, &offset);
                uint32_t ip = prev_ip ^ xor_val;
                char buf[64];
                snprintf(buf, sizeof(buf), "%u.%u.%u.%u", 
                        (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
                grid[i][c] = strdup(buf);
                prev_ip = ip;
            }
        } else {
            // HYPER DECOMPOSITION
            uint64_t max_tokens = decode_varint(decompressed, &offset);
            uint8_t is_constant_count = decompressed[offset++];
            
            uint64_t* token_counts = malloc(sizeof(uint64_t) * line_count);
            
            if (is_constant_count) {
                uint64_t count = 0;
                if (line_count > 0) count = decode_varint(decompressed, &offset);
                for(size_t i=0; i<line_count; i++) token_counts[i] = count;
            } else {
                for(size_t i=0; i<line_count; i++) token_counts[i] = decode_varint(decompressed, &offset);
            }
            
            char*** sub_cols = malloc(sizeof(char**) * max_tokens);
            
            for (size_t sc = 0; sc < max_tokens; sc++) {
                sub_cols[sc] = malloc(sizeof(char*) * line_count);
                uint8_t use_dict = decompressed[offset++];
                
                if (use_dict) {
                    uint64_t dict_count = decode_varint(decompressed, &offset);
                    char** dict = malloc(sizeof(char*) * dict_count);
                    for(size_t k=0; k<dict_count; k++) {
                        uint64_t len = decode_varint(decompressed, &offset);
                        dict[k] = malloc(len+1);
                        memcpy(dict[k], decompressed+offset, len);
                        dict[k][len] = '\0';
                        offset += len;
                    }
                    for(size_t i=0; i<line_count; i++) {
                        if (sc < token_counts[i]) {
                            uint64_t id = decode_varint(decompressed, &offset);
                            if (id < dict_count) sub_cols[sc][i] = strdup(dict[id]);
                            else sub_cols[sc][i] = strdup("");
                        } else {
                            sub_cols[sc][i] = NULL;
                        }
                    }
                    for(size_t k=0; k<dict_count; k++) free(dict[k]);
                    free(dict);
                } else {
                    for(size_t i=0; i<line_count; i++) {
                        if (sc < token_counts[i]) {
                            uint64_t len = decode_varint(decompressed, &offset);
                            sub_cols[sc][i] = malloc(len+1);
                            memcpy(sub_cols[sc][i], decompressed+offset, len);
                            sub_cols[sc][i][len] = '\0';
                            offset += len;
                        } else {
                            sub_cols[sc][i] = NULL;
                        }
                    }
                }
            }
            
            for (size_t i = 0; i < line_count; i++) {
                size_t total_len = 0;
                for (size_t sc = 0; sc < token_counts[i]; sc++) {
                    if (sub_cols[sc][i]) total_len += strlen(sub_cols[sc][i]);
                }
                grid[i][c] = malloc(total_len + 1);
                grid[i][c][0] = '\0';
                for (size_t sc = 0; sc < token_counts[i]; sc++) {
                    if (sub_cols[sc][i]) strcat(grid[i][c], sub_cols[sc][i]);
                }
            }
            
            for(size_t sc=0; sc<max_tokens; sc++) {
                for(size_t i=0; i<line_count; i++) free(sub_cols[sc][i]);
                free(sub_cols[sc]);
            }
            free(sub_cols);
            free(token_counts);
        }
    }
    
    free(decompressed);
    
    // Write output
    FILE* out_fp = fopen(output_path, "w");
    for (size_t i = 0; i < line_count; i++) {
        for (size_t c = 0; c < max_cols; c++) {
            if (grid[i][c]) {
                fprintf(out_fp, "%s", grid[i][c]);
                if (c < max_cols - 1 && grid[i][c+1]) fprintf(out_fp, " "); // Assuming space separator
            }
        }
        fprintf(out_fp, "\n");
    }
    fclose(out_fp);
    
    // Cleanup
    for(size_t i=0; i<line_count; i++) {
        for(size_t c=0; c<max_cols; c++) free(grid[i][c]);
        free(grid[i]);
    }
    free(grid);
    
    clock_t end = clock();
    *duration = (double)(end - start) / CLOCKS_PER_SEC;
    return 0;
}
