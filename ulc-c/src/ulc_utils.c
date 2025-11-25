#include "../include/ulc_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// String implementation
String* string_new(size_t initial_capacity) {
    String* str = malloc(sizeof(String));
    str->capacity = initial_capacity > 0 ? initial_capacity : 64;
    str->data = malloc(str->capacity);
    str->length = 0;
    str->data[0] = '\0';
    return str;
}

void string_append(String* str, const char* data, size_t len) {
    if (str->length + len + 1 > str->capacity) {
        while (str->length + len + 1 > str->capacity) {
            str->capacity *= 2;
        }
        str->data = realloc(str->data, str->capacity);
    }
    memcpy(str->data + str->length, data, len);
    str->length += len;
    str->data[str->length] = '\0';
}

void string_free(String* str) {
    if (str) {
        free(str->data);
        free(str);
    }
}

// ByteArray implementation
ByteArray* bytearray_new(size_t initial_capacity) {
    ByteArray* arr = malloc(sizeof(ByteArray));
    arr->capacity = initial_capacity > 0 ? initial_capacity : 1024;
    arr->data = malloc(arr->capacity);
    arr->length = 0;
    return arr;
}

void bytearray_append(ByteArray* arr, const uint8_t* data, size_t len) {
    if (arr->length + len > arr->capacity) {
        while (arr->length + len > arr->capacity) {
            arr->capacity *= 2;
        }
        arr->data = realloc(arr->data, arr->capacity);
    }
    memcpy(arr->data + arr->length, data, len);
    arr->length += len;
}

void bytearray_append_byte(ByteArray* arr, uint8_t byte) {
    bytearray_append(arr, &byte, 1);
}

void bytearray_free(ByteArray* arr) {
    if (arr) {
        free(arr->data);
        free(arr);
    }
}

// Dictionary implementation
Dictionary* dict_new(size_t initial_capacity) {
    Dictionary* dict = malloc(sizeof(Dictionary));
    dict->capacity = initial_capacity > 0 ? initial_capacity : 128;
    dict->entries = malloc(sizeof(DictEntry) * dict->capacity);
    dict->count = 0;
    return dict;
}

int dict_get_or_add(Dictionary* dict, const char* key) {
    // Linear search (simple but works for moderate sizes)
    for (size_t i = 0; i < dict->count; i++) {
        if (strcmp(dict->entries[i].key, key) == 0) {
            return dict->entries[i].value;
        }
    }
    
    // Not found, add new entry
    if (dict->count >= dict->capacity) {
        dict->capacity *= 2;
        dict->entries = realloc(dict->entries, sizeof(DictEntry) * dict->capacity);
    }
    
    int new_id = dict->count;
    dict->entries[dict->count].key = strdup(key);
    dict->entries[dict->count].value = new_id;
    dict->count++;
    
    return new_id;
}

void dict_free(Dictionary* dict) {
    if (dict) {
        for (size_t i = 0; i < dict->count; i++) {
            free(dict->entries[i].key);
        }
        free(dict->entries);
        free(dict);
    }
}

// Varint encoding
void encode_varint(ByteArray* out, uint64_t value) {
    while (value >= 0x80) {
        bytearray_append_byte(out, (uint8_t)((value & 0x7F) | 0x80));
        value >>= 7;
    }
    bytearray_append_byte(out, (uint8_t)value);
}

uint64_t decode_varint(const uint8_t* data, size_t* offset) {
    uint64_t value = 0;
    int shift = 0;
    
    while (1) {
        uint8_t byte = data[*offset];
        (*offset)++;
        value |= (uint64_t)(byte & 0x7F) << shift;
        if (!(byte & 0x80)) {
            break;
        }
        shift += 7;
    }
    
    return value;
}

// ZigZag encoding for signed integers
static inline uint64_t zigzag_encode(int64_t value) {
    return (uint64_t)((value << 1) ^ (value >> 63));
}

static inline int64_t zigzag_decode(uint64_t value) {
    return (int64_t)((value >> 1) ^ (-(value & 1)));
}

// Delta encoding
void encode_delta(ByteArray* out, int64_t* values, size_t count) {
    if (count == 0) return;
    
    int64_t prev = 0;
    for (size_t i = 0; i < count; i++) {
        int64_t delta = values[i] - prev;
        encode_varint(out, zigzag_encode(delta));
        prev = values[i];
    }
}

void decode_delta(const uint8_t* data, size_t data_len, int64_t** out_values, size_t* out_count) {
    // First pass: count values
    size_t offset = 0;
    size_t count = 0;
    while (offset < data_len) {
        decode_varint(data, &offset);
        count++;
    }
    
    // Allocate output
    *out_values = malloc(sizeof(int64_t) * count);
    *out_count = count;
    
    // Second pass: decode values
    offset = 0;
    int64_t prev = 0;
    for (size_t i = 0; i < count; i++) {
        uint64_t encoded = decode_varint(data, &offset);
        int64_t delta = zigzag_decode(encoded);
        int64_t value = prev + delta;
        (*out_values)[i] = value;
        prev = value;
    }
}

// Timestamp parsing (simplified - handles common formats)
int64_t parse_timestamp(const char* ts_str) {
    // Try ISO format: 2025-02-01T10:00:00.070Z
    struct tm tm_info = {0};
    int year, month, day, hour, min, sec, usec = 0;
    
    if (sscanf(ts_str, "%d-%d-%dT%d:%d:%d.%dZ", &year, &month, &day, &hour, &min, &sec, &usec) >= 6) {
        tm_info.tm_year = year - 1900;
        tm_info.tm_mon = month - 1;
        tm_info.tm_mday = day;
        tm_info.tm_hour = hour;
        tm_info.tm_min = min;
        tm_info.tm_sec = sec;
        time_t t = mktime(&tm_info);
        return (int64_t)t * 1000000LL + usec;
    }
    
    // Try simple date-time: 2025-11-24 18:55:22
    if (sscanf(ts_str, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec) == 6) {
        tm_info.tm_year = year - 1900;
        tm_info.tm_mon = month - 1;
        tm_info.tm_mday = day;
        tm_info.tm_hour = hour;
        tm_info.tm_min = min;
        tm_info.tm_sec = sec;
        time_t t = mktime(&tm_info);
        return (int64_t)t * 1000000LL;
    }
    
    // Try time only: 08:00:01
    if (sscanf(ts_str, "%d:%d:%d", &hour, &min, &sec) == 3) {
        time_t now = time(NULL);
        struct tm* now_tm = localtime(&now);
        now_tm->tm_hour = hour;
        now_tm->tm_min = min;
        now_tm->tm_sec = sec;
        time_t t = mktime(now_tm);
        return (int64_t)t * 1000000LL;
    }
    
    return 0;
}

// IP parsing
uint32_t parse_ip(const char* ip_str) {
    unsigned int a, b, c, d;
    if (sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
        return (a << 24) | (b << 16) | (c << 8) | d;
    }
    return 0;
}
