#include "../include/ulc_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Helper to check if line is JSON
static int is_json(const char* line) {
    while (isspace(*line)) line++;
    return *line == '{';
}

// Simple regex-like matching for common patterns
static int parse_apache(const char* line, LogEntry* entry) {
    // Format: IP - - [timestamp] "METHOD path HTTP/1.1" status size "referer" "useragent"
    char ip[64], timestamp[128], method[16], path[512], status[16], size[32];
    char referer[512], useragent[512];
    
    int n = sscanf(line, "%63s - - [%127[^]]] \"%15s %511s HTTP/%*[^\"]\" %15s %31s \"%511[^\"]\" \"%511[^\"]\"",
                   ip, timestamp, method, path, status, size, referer, useragent);
    
    if (n >= 6) {
        entry->format = LOG_FORMAT_APACHE;
        log_entry_add_field(entry, "ip", ip);
        log_entry_add_field(entry, "timestamp", timestamp);
        log_entry_add_field(entry, "method", method);
        log_entry_add_field(entry, "path", path);
        log_entry_add_field(entry, "status", status);
        log_entry_add_field(entry, "size", size);
        if (n >= 7) log_entry_add_field(entry, "referer", referer);
        if (n >= 8) log_entry_add_field(entry, "useragent", useragent);
        return 1;
    }
    return 0;
}

static int parse_generic(const char* line, LogEntry* entry) {
    // Format: [timestamp] service level: message
    char timestamp[128], service[64], level[32], message[1024];
    
    if (sscanf(line, "[%127[^]]] %63s %31[^:]: %1023[^\n]", timestamp, service, level, message) == 4) {
        entry->format = LOG_FORMAT_GENERIC;
        log_entry_add_field(entry, "timestamp", timestamp);
        log_entry_add_field(entry, "service", service);
        log_entry_add_field(entry, "level", level);
        log_entry_add_field(entry, "message", message);
        return 1;
    }
    return 0;
}

static int parse_syslog(const char* line, LogEntry* entry) {
    // Format: Nov 24 18:55:22 hostname service[pid]: message
    char month[16], day[8], time[16], host[128], service[64], message[1024];
    int pid;
    
    // Try with PID
    if (sscanf(line, "%15s %7s %15s %127s %63[^[][%d]: %1023[^\n]", 
               month, day, time, host, service, &pid, message) == 7) {
        char timestamp[64], pid_str[16];
        snprintf(timestamp, sizeof(timestamp), "%s %s %s", month, day, time);
        snprintf(pid_str, sizeof(pid_str), "%d", pid);
        
        entry->format = LOG_FORMAT_SYSLOG;
        log_entry_add_field(entry, "timestamp", timestamp);
        log_entry_add_field(entry, "host", host);
        log_entry_add_field(entry, "service", service);
        log_entry_add_field(entry, "pid", pid_str);
        log_entry_add_field(entry, "message", message);
        return 1;
    }
    
    // Try without PID
    if (sscanf(line, "%15s %7s %15s %127s %63[^:]: %1023[^\n]",
               month, day, time, host, service, message) == 6) {
        char timestamp[64];
        snprintf(timestamp, sizeof(timestamp), "%s %s %s", month, day, time);
        
        entry->format = LOG_FORMAT_SYSLOG;
        log_entry_add_field(entry, "timestamp", timestamp);
        log_entry_add_field(entry, "host", host);
        log_entry_add_field(entry, "service", service);
        log_entry_add_field(entry, "message", message);
        return 1;
    }
    return 0;
}

static int parse_security(const char* line, LogEntry* entry) {
    // Format: 2025-11-24 18:55:22 service[pid]: message
    char timestamp[64], service[64], message[1024];
    int pid;
    
    if (sscanf(line, "%63s %*s %63[^[][%d]: %1023[^\n]", timestamp, service, &pid, message) == 3) {
        char pid_str[16];
        snprintf(pid_str, sizeof(pid_str), "%d", pid);
        
        // Combine date and time
        char full_timestamp[128];
        sscanf(line, "%127s", full_timestamp);
        
        entry->format = LOG_FORMAT_SEC;
        log_entry_add_field(entry, "timestamp", full_timestamp);
        log_entry_add_field(entry, "service", service);
        log_entry_add_field(entry, "pid", pid_str);
        log_entry_add_field(entry, "message", message);
        return 1;
    }
    return 0;
}

LogEntry* parse_log_line(const char* line) {
    LogEntry* entry = malloc(sizeof(LogEntry));
    entry->fields = NULL;
    entry->values = NULL;
    entry->field_count = 0;
    entry->format = LOG_FORMAT_RAW;
    
    // Try different parsers
    if (is_json(line)) {
        // For simplicity, treat JSON as raw for now
        // A full implementation would use a JSON parser
        entry->format = LOG_FORMAT_RAW;
        log_entry_add_field(entry, "raw_message", line);
    } else if (parse_apache(line, entry)) {
        // Parsed as Apache
    } else if (parse_generic(line, entry)) {
        // Parsed as Generic
    } else if (parse_syslog(line, entry)) {
        // Parsed as Syslog
    } else if (parse_security(line, entry)) {
        // Parsed as Security
    } else {
        // Fallback to raw
        entry->format = LOG_FORMAT_RAW;
        log_entry_add_field(entry, "raw_message", line);
    }
    
    return entry;
}

void log_entry_add_field(LogEntry* entry, const char* field, const char* value) {
    entry->field_count++;
    entry->fields = realloc(entry->fields, sizeof(char*) * entry->field_count);
    entry->values = realloc(entry->values, sizeof(char*) * entry->field_count);
    
    entry->fields[entry->field_count - 1] = strdup(field);
    entry->values[entry->field_count - 1] = strdup(value);
}

void log_entry_free(LogEntry* entry) {
    if (entry) {
        for (size_t i = 0; i < entry->field_count; i++) {
            free(entry->fields[i]);
            free(entry->values[i]);
        }
        free(entry->fields);
        free(entry->values);
        free(entry);
    }
}
