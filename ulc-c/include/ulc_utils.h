#ifndef ULC_UTILS_H
#define ULC_UTILS_H

#include "ulc_types.h"

// String utilities
String* string_new(size_t initial_capacity);
void string_append(String* str, const char* data, size_t len);
void string_free(String* str);

// ByteArray utilities
ByteArray* bytearray_new(size_t initial_capacity);
void bytearray_append(ByteArray* arr, const uint8_t* data, size_t len);
void bytearray_append_byte(ByteArray* arr, uint8_t byte);
void bytearray_free(ByteArray* arr);

// Dictionary utilities
Dictionary* dict_new(size_t initial_capacity);
int dict_get_or_add(Dictionary* dict, const char* key);
void dict_free(Dictionary* dict);

// Varint encoding
void encode_varint(ByteArray* out, uint64_t value);
uint64_t decode_varint(const uint8_t* data, size_t* offset);

// Delta encoding (with ZigZag)
void encode_delta(ByteArray* out, int64_t* values, size_t count);
void decode_delta(const uint8_t* data, size_t data_len, int64_t** out_values, size_t* out_count);

// Timestamp parsing
int64_t parse_timestamp(const char* ts_str);

// IP parsing
uint32_t parse_ip(const char* ip_str);

#endif // ULC_UTILS_H
