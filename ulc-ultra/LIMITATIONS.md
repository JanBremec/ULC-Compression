# ULC-Ultra: Format Specifications & Limitations

Detailed documentation of supported log formats, requirements, and limitations for ULC-Ultra.

---

## Format Validation Rules

ULC-Ultra validates input files before compression to ensure optimal results.

### Validation Checks

1. **Line Count**: Minimum 100 lines
   - **Reason**: Pattern mining requires sufficient data
   - **Error**: "Error: Minimum 100 lines required for ultra compression"

2. **Format Consistency**: 80%+ lines must match same format
   - **Reason**: Mixed formats reduce compression efficiency
   - **Error**: "Error: Log format consistency < 80%. Mixed formats not supported."

3. **Format Recognition**: At least one recognized format
   - **Reason**: Unstructured text compresses poorly
   - **Warning**: "Warning: Unstructured logs detected. Compression may be suboptimal."

---

## Supported Formats (Detailed)

### 1. Apache/Nginx Combined Format

**Pattern:**
```
IP - - [timestamp] "METHOD path HTTP/version" status size "referer" "useragent"
```

**Example:**
```
127.0.0.1 - - [01/Jan/2024:10:00:00 +0000] "GET /index.html HTTP/1.1" 200 1234 "http://example.com" "Mozilla/5.0"
```

**Requirements:**
- ✅ All lines must follow exact format
- ✅ Timestamp in `[DD/Mon/YYYY:HH:MM:SS +ZZZZ]` format
- ✅ Status codes must be numeric
- ✅ Sizes must be numeric or `-`

**Expected Compression:**
- **Ratio**: 25-35x
- **Best Case**: Highly repetitive access patterns
- **Worst Case**: Unique URLs and user agents

**Validation:**
```bash
# Valid Apache log
✓ 192.168.1.1 - - [24/Nov/2025:10:00:00 +0000] "GET /api/users HTTP/1.1" 200 512 "-" "curl/7.68.0"

# Invalid - missing fields
✗ 192.168.1.1 "GET /api/users HTTP/1.1" 200
```

---

### 2. Syslog (RFC 3164)

**Pattern:**
```
Month Day HH:MM:SS hostname service[pid]: message
```

**Example:**
```
Nov 24 18:55:22 server-01 kernel[9255]: Out of memory: Kill process 1234
```

**Requirements:**
- ✅ Month as 3-letter abbreviation (Jan, Feb, etc.)
- ✅ Day as 1-2 digits
- ✅ Time in HH:MM:SS format
- ✅ PID is optional but must be numeric if present

**Expected Compression:**
- **Ratio**: 20-30x
- **Best Case**: Repetitive system messages
- **Worst Case**: Unique error messages

**Validation:**
```bash
# Valid Syslog
✓ Dec 25 12:00:00 web-server nginx[1234]: Connection accepted
✓ Jan 1 00:00:00 db-server postgres: Database started

# Invalid - wrong month format
✗ 12/25 12:00:00 web-server nginx[1234]: Message
```

---

### 3. Generic Structured Format

**Pattern:**
```
[timestamp] service level: message
```

**Example:**
```
[08:00:01] AUTH INFO: User 1024 successfully logged in
```

**Requirements:**
- ✅ Timestamp in brackets
- ✅ Service/component name
- ✅ Log level (INFO, ERROR, DEBUG, etc.)
- ✅ Colon separator before message

**Expected Compression:**
- **Ratio**: 15-25x
- **Best Case**: Consistent service names and levels
- **Worst Case**: Highly variable messages

**Validation:**
```bash
# Valid Generic
✓ [10:30:45] DATABASE INFO: Query executed in 0.5ms
✓ [14:22:33] API ERROR: Connection timeout

# Invalid - missing brackets
✗ 10:30:45 DATABASE INFO: Query executed
```

---

### 4. Security/Audit Logs

**Pattern:**
```
YYYY-MM-DD HH:MM:SS service[pid]: message
```

**Example:**
```
2025-11-24 18:55:22 sshd[6760]: Accepted password for user from 192.168.1.100
```

**Requirements:**
- ✅ Date in YYYY-MM-DD format
- ✅ Time in HH:MM:SS format
- ✅ Service name required
- ✅ PID optional

**Expected Compression:**
- **Ratio**: 15-25x
- **Best Case**: Repetitive security events
- **Worst Case**: Unique IP addresses and usernames

---

### 5. Database Logs

**Pattern:** (Varies by database)

**PostgreSQL Example:**
```
2025-11-24 10:00:00 UTC [1234]: [1-1] user=admin,db=production LOG: SELECT * FROM users WHERE id=1
```

**MySQL Example:**
```
2025-11-24T10:00:00.123456Z 1234 Query SELECT * FROM users WHERE id=1
```

**Requirements:**
- ✅ Consistent timestamp format
- ✅ Query patterns with similar structure
- ✅ Repetitive table/column names

**Expected Compression:**
- **Ratio**: 30-50x (BEST)
- **Reason**: Highly repetitive SQL patterns
- **Best Case**: Similar queries with different parameters

---

### 6. JSON Logs

**Pattern:**
```json
{"timestamp":"2025-11-24T10:00:00Z","level":"INFO","service":"api","message":"Request processed"}
```

**Requirements:**
- ✅ Valid JSON on each line
- ✅ Consistent schema (same fields in same order)
- ✅ Field names must be identical across lines

**Expected Compression:**
- **Ratio**: 10-20x
- **Best Case**: Consistent schema, repetitive values
- **Worst Case**: Variable schemas, unique values

**Validation:**
```bash
# Valid JSON logs (consistent schema)
✓ {"ts":"2025-11-24T10:00:00Z","level":"INFO","msg":"User login"}
✓ {"ts":"2025-11-24T10:00:01Z","level":"INFO","msg":"User logout"}

# Invalid - inconsistent schema
✗ {"timestamp":"2025-11-24T10:00:00Z","level":"INFO"}
✗ {"ts":"2025-11-24T10:00:01Z","severity":"INFO","message":"Text"}
```

---

## Unsupported Formats

### Binary Logs
**Reason**: Not text-based, no patterns to exploit
**Alternative**: Use specialized binary compression

**Example:**
```
✗ \x00\x01\x02\x03\x04...
```

### Encrypted Logs
**Reason**: Encryption removes all patterns
**Alternative**: Compress before encryption

**Example:**
```
✗ U2FsdGVkX1+vupppZksvRf5pq5g5XjFRlipRkwB0K1Y=
```

### Random/Unstructured Text
**Reason**: No repetition or structure
**Alternative**: Use Bzip2 or LZMA

**Example:**
```
✗ Lorem ipsum dolor sit amet, consectetur adipiscing elit...
✗ Random error occurred at some point maybe
```

### Mixed Format Logs
**Reason**: Violates 80% consistency rule
**Alternative**: Split by format first

**Example:**
```
✗ Nov 24 10:00:00 server sshd: Login
✗ 127.0.0.1 - - [24/Nov/2025:10:00:01] "GET /"
✗ {"timestamp":"2025-11-24T10:00:02Z","msg":"Event"}
```

---

## Performance by Format

| Format | Lines Tested | Ratio | Comp Speed | Decomp Speed | Memory |
|--------|-------------|-------|------------|--------------|--------|
| Apache | 2,000 | 26.2x | 1.5 MB/s | 8.1 MB/s | 280 MB |
| Syslog | 2,000 | 23.4x | 1.4 MB/s | 8.3 MB/s | 270 MB |
| Generic | 3,052 | 13.7x | 1.8 MB/s | 16.2 MB/s | 290 MB |
| Database | 5,000 | 45.0x* | 1.2 MB/s | 7.5 MB/s | 320 MB |
| JSON | 1,000 | 12.5x | 1.6 MB/s | 9.0 MB/s | 260 MB |

*Estimated based on pattern analysis

---

## Best Practices

### ✅ DO:
1. **Validate format** before compressing large files
2. **Split mixed formats** into separate files
3. **Use for archival** where compression ratio matters most
4. **Ensure sufficient memory** (256MB+ available)
5. **Test on sample** before compressing production logs

### ❌ DON'T:
1. **Don't use for real-time** compression
2. **Don't compress encrypted** logs
3. **Don't mix formats** in same file
4. **Don't use on tiny files** (< 100 lines)
5. **Don't expect speed** - this is for maximum compression

---

## Troubleshooting

### Low Compression Ratio (< 10x)

**Possible Causes:**
1. Unstructured or highly variable logs
2. Mixed formats in same file
3. Encrypted or binary data
4. Unique values in every line

**Solutions:**
- Check format consistency with validation
- Split by format
- Use standard ULC or Bzip2 instead

### Out of Memory Error

**Possible Causes:**
1. File too large (> 100MB)
2. Insufficient RAM (< 256MB available)
3. 128MB LZMA dictionary too large

**Solutions:**
- Split file into smaller chunks
- Close other applications
- Use standard ULC-C instead

### Slow Compression (< 0.5 MB/s)

**Expected Behavior:**
- ULC-Ultra is intentionally slow for maximum compression
- 1-3 MB/s is normal

**If slower:**
- Check CPU usage
- Ensure SSD (not HDD) for temp files
- Consider using ULC-C for speed

---

## Format Detection Examples

### Automatic Detection

ULC-Ultra automatically detects format by trying patterns in order:

1. JSON (starts with `{`)
2. Apache (matches IP pattern)
3. Syslog (matches month pattern)
4. Security (matches YYYY-MM-DD pattern)
5. Generic (matches `[timestamp]` pattern)
6. Raw (fallback)

### Detection Confidence

```
High Confidence (80%+ match):
✓ Compression will be optimal
✓ Proceed with compression

Medium Confidence (60-79% match):
⚠️ Warning issued
⚠️ Compression may be suboptimal
✓ Compression proceeds

Low Confidence (< 60% match):
✗ Error issued
✗ Compression rejected
```

---

## Version History

**v1.0** - Initial release
- Pattern mining (prefix/suffix)
- Huffman coding
- 128MB LZMA dictionary
- Format validation
- 7 supported formats

---

## See Also

- [README.md](README.md) - Main documentation
- [Benchmark Results](../unified_benchmark_report.html) - Performance comparison
