#ifndef ULC_TYPES_H
#define ULC_TYPES_H

#include <stdint.h>
#include <stddef.h>

// Magic header for ULC files
#define ULC_MAGIC "ULC1"
#define ULC_MAGIC_LEN 4

// Log format types
typedef enum {
    LOG_FORMAT_JSON = 0,
    LOG_FORMAT_APACHE,
    LOG_FORMAT_SYSLOG,
    LOG_FORMAT_SEC,
    LOG_FORMAT_JAVA,
    LOG_FORMAT_GENERIC,
    LOG_FORMAT_LOGFMT,
    LOG_FORMAT_RAW
} LogFormat;

// Column data types
typedef enum {
    COL_TYPE_STRING = 0,
    COL_TYPE_INT,
    COL_TYPE_TIMESTAMP,
    COL_TYPE_IP
} ColumnType;

// Dynamic string
typedef struct {
    char* data;
    size_t length;
    size_t capacity;
} String;

// Dynamic byte array
typedef struct {
    uint8_t* data;
    size_t length;
    size_t capacity;
} ByteArray;

// Dictionary entry
typedef struct {
    char* key;
    int value;
} DictEntry;

// Dictionary (string -> int mapping)
typedef struct {
    DictEntry* entries;
    size_t count;
    size_t capacity;
} Dictionary;

// Column data
typedef struct {
    char* name;
    ColumnType type;
    ByteArray data;
} Column;

// Parsed log entry
typedef struct {
    char** fields;      // Field names
    char** values;      // Field values
    size_t field_count;
    LogFormat format;
} LogEntry;

// Compressor state
typedef struct {
    Dictionary* dictionaries;  // Array of dictionaries, one per field
    size_t dict_count;
    Column* columns;
    size_t column_count;
} CompressorState;

#endif // ULC_TYPES_H
