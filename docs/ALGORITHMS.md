# Algorithms

Technical details of the ULC compression algorithms.

## Overview

The ULC family uses a **columnar compression** approach optimized for structured log data. Each variant applies different strategies to maximize compression on specific log types.

## ULC-C: Fast Columnar Baseline

### Strategy
1. **Columnar Parsing**: Split each log line into space-separated fields
2. **Dictionary Encoding**: Build a dictionary for each column
3. **LZMA Compression**: Apply LZMA to the serialized columnar data

### Algorithm

```
For each log file:
  1. Parse lines into columns (space-delimited)
  2. For each column:
     - Build dictionary of unique values
     - Replace values with dictionary IDs
  3. Serialize:
     - Write column count
     - For each column:
       - Write dictionary
       - Write ID sequence
  4. Compress with LZMA (preset 9)
```

### Best For
- Syslog (short, structured lines)
- Fast compression needed
- Consistent field structure

### Performance
- **Speed**: ⚡⚡⚡ Fastest
- **Ratio**: 23-26x on structured logs

---

## ULC-Ultra: Hybrid Columnar

### Strategy
1. **Adaptive Column Analysis**: Detect column type (numeric, IP, string)
2. **Specialized Encoding**:
   - **Numeric**: Delta encoding
   - **IP Addresses**: XOR encoding
   - **Low-cardinality strings**: Dictionary
   - **High-cardinality strings**: Raw
3. **LZMA Compression**: Extreme preset for maximum compression

### Algorithm

```
For each column:
  1. Analyze first 100 values:
     - Check if numeric
     - Check if IP address
     - Calculate unique ratio
  
  2. Select encoding:
     IF numeric:
       → Delta encoding (store differences)
     ELSE IF IP address:
       → XOR encoding (store XOR with previous)
     ELSE IF unique_ratio < 0.5:
       → Dictionary encoding
     ELSE:
       → Raw encoding
  
  3. Serialize with type marker
  4. Compress with LZMA (preset 9 extreme)
```

### Encoding Types

| Type | ID | Use Case | Example |
|------|----|---------| --------|
| Dictionary | 1 | Repeated strings | Log levels, hostnames |
| Delta | 2 | Sequential numbers | Timestamps, counters |
| IP XOR | 3 | IP addresses | Source/dest IPs |
| Raw | 4 | Unique strings | UUIDs, hashes |

### Best For
- Syslog with IPs and timestamps
- Mixed data types
- Balanced speed/compression

### Performance
- **Speed**: ⚡⚡ Fast
- **Ratio**: 23-26x on structured logs

---

## ULC-Hyper: Semantic Decomposition

### Strategy
1. **Recursive Tokenization**: Break complex fields into semantic tokens
2. **Sub-Column Encoding**: Treat tokens as new columns
3. **Adaptive Sub-Encoding**: Dictionary or Raw for each token position
4. **LZMA Compression**: Final compression layer

### Algorithm

```
For each column:
  1. Analyze characteristics:
     - Average length
     - Presence of URLs, special chars
  
  2. IF complex (long, URLs, etc.):
     a. Tokenize each value:
        - Split by delimiters: / ? & = : [ ] " space
        - Create token stream for each row
     
     b. Check token redundancy:
        - IF >50% tokens unique OR avg_length < 15:
          → Fallback to Raw encoding
        - ELSE:
          → Proceed with decomposition
     
     c. For each token position (sub-column):
        - Analyze unique ratio
        - IF ratio < 0.5:
          → Dictionary encode
        - ELSE:
          → Raw encode
  
  3. ELSE (simple column):
     → Use ULC-Ultra style encoding
  
  4. Compress with LZMA (preset 9 extreme)
```

### Example: URL Decomposition

```
Input URLs:
  /api/v1/users/123
  /api/v1/users/456
  /api/v2/posts/789

Tokenization:
  Row 1: ["api", "v1", "users", "123"]
  Row 2: ["api", "v1", "users", "456"]
  Row 3: ["api", "v2", "posts", "789"]

Sub-Column Encoding:
  Token[0]: "api" (100% same) → Dictionary: {"api": 0}
  Token[1]: "v1", "v1", "v2" (67% same) → Dictionary: {"v1": 0, "v2": 1}
  Token[2]: "users", "users", "posts" → Dictionary: {"users": 0, "posts": 1}
  Token[3]: "123", "456", "789" (100% unique) → Raw

Result: Massive compression on repeated URL patterns
```

### Optimizations

1. **Constant Token Count**: If all rows have same token count, store once
2. **Length Heuristic**: Skip decomposition for short fields (< 15 chars)
3. **Redundancy Check**: Skip if tokens are mostly unique (> 50%)

### Best For
- Web server logs (Apache, Nginx)
- Application logs with URLs
- Variable message templates

### Performance
- **Speed**: ⚡ Moderate (tokenization overhead)
- **Ratio**: 24-32x on complex logs

---

## ULC-Unified: Intelligent Dispatcher

### Strategy
Analyze log characteristics and automatically select the best ULC variant.

### Decision Algorithm

```
Analyze sample (first 1000 lines):
  - Calculate average line length
  - Detect URLs (http://, /api/, GET, POST)
  - Detect IPs (xxx.xxx.xxx.xxx pattern)
  - Detect timestamps ([, 2024-, 2025-)
  - Calculate unique ratio

Decision Tree:
  IF has_urls AND avg_line_len > 150:
    → ULC-Hyper (web/app logs)
  
  ELSE IF avg_line_len > 200 AND unique_ratio > 0.7:
    → ULC-Hyper (complex data)
  
  ELSE IF avg_line_len < 100 AND has_timestamps AND has_ips:
    → ULC-C (syslog)
  
  ELSE IF 100 <= avg_line_len <= 200:
    → ULC-Ultra (balanced)
  
  ELSE:
    → ULC-C (default)
```

### Best For
- Mixed log environments
- Unknown log types
- Automated pipelines

### Performance
- **Speed**: Varies (depends on selected algorithm)
- **Ratio**: Matches or approaches best variant

---

## Comparison Matrix

| Feature | ULC-C | ULC-Ultra | ULC-Hyper | ULC-Unified |
|---------|-------|-----------|-----------|-------------|
| **Columnar** | ✅ | ✅ | ✅ | ✅ |
| **Dictionary** | ✅ | ✅ | ✅ | ✅ |
| **Delta Encoding** | ❌ | ✅ | ✅ | Varies |
| **IP XOR** | ❌ | ✅ | ✅ | Varies |
| **Tokenization** | ❌ | ❌ | ✅ | Varies |
| **Auto-Select** | ❌ | ❌ | ❌ | ✅ |
| **Speed** | ⚡⚡⚡ | ⚡⚡ | ⚡ | Varies |
| **Apache Logs** | 26x | 26x | **32x** | 26x |
| **Syslog** | **23x** | **23x** | 15x | **23x** |
| **App Logs** | 14x | 16x | **24x** | 14x |

## Implementation Notes

### LZMA Settings

All variants use aggressive LZMA settings for final compression:

```c
lzma_options_lzma opt;
lzma_lzma_preset(&opt, 9 | LZMA_PRESET_EXTREME);
opt.dict_size = 128 * 1024 * 1024;  // 128MB dictionary
```

### Memory Usage

| Variant | Memory (Compression) | Memory (Decompression) |
|---------|---------------------|------------------------|
| ULC-C | ~150 MB | ~150 MB |
| ULC-Ultra | ~150 MB | ~150 MB |
| ULC-Hyper | ~200 MB | ~200 MB |
| ULC-Unified | Varies | Varies |

### File Format

Each variant uses a different magic number for format detection:

- ULC-C: `ULC\x01`
- ULC-Ultra: `ULCU`
- ULC-Hyper: `ULCH`

ULC-Unified detects the format during decompression.

## Future Improvements

1. **Machine Learning**: Train model to predict optimal algorithm
2. **Parallel Processing**: Multi-threaded compression
3. **Streaming**: Support for large files that don't fit in memory
4. **JSON/CSV Support**: Specialized parsers for structured formats
5. **Adaptive LZMA**: Tune LZMA settings based on data characteristics

## References

- LZMA SDK: https://www.7-zip.org/sdk.html
- Columnar Compression: Parquet, ORC formats
- Delta Encoding: Used in time-series databases
- Dictionary Encoding: Common in columnar databases
