#ifndef ULC_PARSER_H
#define ULC_PARSER_H

#include "ulc_types.h"

// Parse a single log line
LogEntry* parse_log_line(const char* line);

// Free log entry
void log_entry_free(LogEntry* entry);

// Add field to log entry
void log_entry_add_field(LogEntry* entry, const char* field, const char* value);

#endif // ULC_PARSER_H
