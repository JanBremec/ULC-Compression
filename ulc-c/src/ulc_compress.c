#include "../include/ulc_compress.h"
#include "../include/ulc_parser.h"
#include "../include/ulc_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <lzma.h>

// Simple serialization format (simplified compared to Python's pickle)
// Format: [field_count][field1_len][field1][type1][data1_len][data1]...

static ByteArray* serialize_compressed_data(LogEntry** entries, size_t entry_count, 
                                            Dictionary** dicts, size_t dict_count) {
    ByteArray* output = bytearray_new(1024 * 1024);
    
    // Write entry count
    encode_varint(output, entry_count);
    
    // Determine all unique fields
    Dictionary* field_dict = dict_new(64);
    for (size_t i = 0; i < entry_count; i++) {
        for (size_t j = 0; j < entries[i]->field_count; j++) {
            dict_get_or_add(field_dict, entries[i]->fields[j]);
        }
    }
    
    // Write field dictionary
    encode_varint(output, field_dict->count);
    for (size_t i = 0; i < field_dict->count; i++) {
        const char* field = field_dict->entries[i].key;
        encode_varint(output, strlen(field));
        bytearray_append(output, (uint8_t*)field, strlen(field));
    }
    
    // For each field, collect values and encode
    for (size_t field_idx = 0; field_idx < field_dict->count; field_idx++) {
        const char* field_name = field_dict->entries[field_idx].key;
        
        // Determine column type
        ColumnType col_type = COL_TYPE_STRING;
        if (strcmp(field_name, "timestamp") == 0) {
            col_type = COL_TYPE_TIMESTAMP;
        } else if (strcmp(field_name, "ip") == 0) {
            col_type = COL_TYPE_IP;
        } else if (strcmp(field_name, "status") == 0 || strcmp(field_name, "size") == 0 ||
                   strcmp(field_name, "pid") == 0) {
            col_type = COL_TYPE_INT;
        }
        
        // Write column type
        bytearray_append_byte(output, (uint8_t)col_type);
        
        // Collect values
        if (col_type == COL_TYPE_TIMESTAMP || col_type == COL_TYPE_IP || col_type == COL_TYPE_INT) {
            // Numeric column - use delta encoding
            int64_t* values = malloc(sizeof(int64_t) * entry_count);
            for (size_t i = 0; i < entry_count; i++) {
                const char* val = NULL;
                for (size_t j = 0; j < entries[i]->field_count; j++) {
                    if (strcmp(entries[i]->fields[j], field_name) == 0) {
                        val = entries[i]->values[j];
                        break;
                    }
                }
                
                if (val) {
                    if (col_type == COL_TYPE_TIMESTAMP) {
                        values[i] = parse_timestamp(val);
                    } else if (col_type == COL_TYPE_IP) {
                        values[i] = parse_ip(val);
                    } else {
                        values[i] = atoll(val);
                    }
                } else {
                    values[i] = 0;
                }
            }
            
            // Encode with delta
            ByteArray* encoded = bytearray_new(entry_count * 4);
            encode_delta(encoded, values, entry_count);
            encode_varint(output, encoded->length);
            bytearray_append(output, encoded->data, encoded->length);
            
            free(values);
            bytearray_free(encoded);
        } else {
            // String column - use dictionary encoding
            Dictionary* value_dict = dict_new(256);
            int* ids = malloc(sizeof(int) * entry_count);
            
            for (size_t i = 0; i < entry_count; i++) {
                const char* val = "";
                for (size_t j = 0; j < entries[i]->field_count; j++) {
                    if (strcmp(entries[i]->fields[j], field_name) == 0) {
                        val = entries[i]->values[j];
                        break;
                    }
                }
                ids[i] = dict_get_or_add(value_dict, val);
            }
            
            // Write dictionary
            encode_varint(output, value_dict->count);
            for (size_t i = 0; i < value_dict->count; i++) {
                const char* str = value_dict->entries[i].key;
                encode_varint(output, strlen(str));
                bytearray_append(output, (uint8_t*)str, strlen(str));
            }
            
            // Write IDs
            ByteArray* id_data = bytearray_new(entry_count * 2);
            for (size_t i = 0; i < entry_count; i++) {
                encode_varint(id_data, ids[i]);
            }
            encode_varint(output, id_data->length);
            bytearray_append(output, id_data->data, id_data->length);
            
            free(ids);
            dict_free(value_dict);
            bytearray_free(id_data);
        }
    }
    
    dict_free(field_dict);
    return output;
}

int ulc_compress_file(const char* input_path, const char* output_path,
                      size_t* orig_size, size_t* comp_size, double* duration) {
    clock_t start = clock();
    
    // Read input file
    FILE* fp = fopen(input_path, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open input file: %s\n", input_path);
        return -1;
    }
    
    // Read all lines
    LogEntry** entries = NULL;
    size_t entry_count = 0;
    size_t entry_capacity = 1024;
    entries = malloc(sizeof(LogEntry*) * entry_capacity);
    
    char line[16384];
    size_t total_bytes = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
        if (len > 1 && line[len-2] == '\r') line[len-2] = '\0';
        
        total_bytes += strlen(line) + 1;
        
        LogEntry* entry = parse_log_line(line);
        
        if (entry_count >= entry_capacity) {
            entry_capacity *= 2;
            entries = realloc(entries, sizeof(LogEntry*) * entry_capacity);
        }
        entries[entry_count++] = entry;
    }
    fclose(fp);
    
    *orig_size = total_bytes;
    
    // Serialize
    ByteArray* serialized = serialize_compressed_data(entries, entry_count, NULL, 0);
    
    // Compress with LZMA
    size_t compressed_capacity = serialized->length + 1024;
    uint8_t* compressed = malloc(compressed_capacity);
    
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_easy_encoder(&strm, 9 | LZMA_PRESET_EXTREME, LZMA_CHECK_CRC64);
    
    if (ret != LZMA_OK) {
        fprintf(stderr, "Error: LZMA encoder init failed\n");
        free(compressed);
        bytearray_free(serialized);
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
        return -1;
    }
    
    size_t compressed_size = strm.total_out;
    lzma_end(&strm);
    
    // Write output file
    FILE* out_fp = fopen(output_path, "wb");
    if (!out_fp) {
        fprintf(stderr, "Error: Cannot open output file: %s\n", output_path);
        free(compressed);
        bytearray_free(serialized);
        return -1;
    }
    
    // Write magic header
    fwrite(ULC_MAGIC, 1, ULC_MAGIC_LEN, out_fp);
    fwrite(compressed, 1, compressed_size, out_fp);
    fclose(out_fp);
    
    *comp_size = compressed_size + ULC_MAGIC_LEN;
    
    // Cleanup
    free(compressed);
    bytearray_free(serialized);
    for (size_t i = 0; i < entry_count; i++) {
        log_entry_free(entries[i]);
    }
    free(entries);
    
    clock_t end = clock();
    *duration = (double)(end - start) / CLOCKS_PER_SEC;
    
    return 0;
}

int ulc_decompress_file(const char* input_path, const char* output_path, double* duration) {
    clock_t start = clock();
    
    // Read compressed file
    FILE* fp = fopen(input_path, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open input file: %s\n", input_path);
        return -1;
    }
    
    // Check magic
    char magic[ULC_MAGIC_LEN];
    if (fread(magic, 1, ULC_MAGIC_LEN, fp) != ULC_MAGIC_LEN ||
        memcmp(magic, ULC_MAGIC, ULC_MAGIC_LEN) != 0) {
        fprintf(stderr, "Error: Invalid ULC file (bad magic)\n");
        fclose(fp);
        return -1;
    }
    
    // Read compressed data
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, ULC_MAGIC_LEN, SEEK_SET);
    
    size_t compressed_size = file_size - ULC_MAGIC_LEN;
    uint8_t* compressed = malloc(compressed_size);
    fread(compressed, 1, compressed_size, fp);
    fclose(fp);
    
    // Decompress with LZMA
    size_t decompressed_capacity = compressed_size * 20; // Estimate
    uint8_t* decompressed = malloc(decompressed_capacity);
    
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_stream_decoder(&strm, UINT64_MAX, 0);
    
    if (ret != LZMA_OK) {
        fprintf(stderr, "Error: LZMA decoder init failed\n");
        free(compressed);
        free(decompressed);
        return -1;
    }
    
    strm.next_in = compressed;
    strm.avail_in = compressed_size;
    strm.next_out = decompressed;
    strm.avail_out = decompressed_capacity;
    
    ret = lzma_code(&strm, LZMA_FINISH);
    
    if (ret != LZMA_STREAM_END) {
        fprintf(stderr, "Error: LZMA decompression failed\n");
        lzma_end(&strm);
        free(compressed);
        free(decompressed);
        return -1;
    }
    
    size_t decompressed_size = strm.total_out;
    lzma_end(&strm);
    free(compressed);
    
    // For now, just write a placeholder message
    // Full deserialization would reconstruct the log lines
    FILE* out_fp = fopen(output_path, "w");
    if (!out_fp) {
        fprintf(stderr, "Error: Cannot open output file: %s\n", output_path);
        free(decompressed);
        return -1;
    }
    
    fprintf(out_fp, "# Decompressed data (simplified implementation)\n");
    fprintf(out_fp, "# Decompressed %zu bytes\n", decompressed_size);
    fclose(out_fp);
    
    free(decompressed);
    
    clock_t end = clock();
    *duration = (double)(end - start) / CLOCKS_PER_SEC;
    
    return 0;
}
