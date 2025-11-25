# Usage Guide

Complete guide to using the ULC compression suite.

## Installation

### Prerequisites

**Windows**:
- MinGW-w64 (GCC compiler)
- LZMA library (included with MinGW)

**Linux/Mac**:
```bash
# Ubuntu/Debian
sudo apt-get install build-essential liblzma-dev

# macOS
brew install xz
```

### Building from Source

```bash
# Clone repository
git clone https://github.com/[username]/ulc.git
cd ulc

# Build all variants
cd scripts
./build_all.bat  # Windows
# or
./build_all.sh   # Linux/Mac
```

Executables will be created in each variant's directory:
- `ulc-c/ulc.exe`
- `ulc-ultra/ulc-ultra.exe`
- `ulc-hyper/ulc-hyper.exe`
- `ulc-unified/ulc-auto.exe`

---

## Basic Usage

### ULC-Unified (Recommended)

**Automatic algorithm selection** - best for most use cases.

```bash
# Compress
ulc-unified/ulc-auto.exe compress input.log -o output.ulc

# Decompress
ulc-unified/ulc-auto.exe decompress output.ulc -o restored.log
```

### ULC-Hyper

**Maximum compression** for web/app logs.

```bash
# Compress
ulc-hyper/ulc-hyper.exe compress apache.log -o apache.ulch

# Decompress
ulc-hyper/ulc-hyper.exe decompress apache.ulch -o apache_restored.log
```

### ULC-Ultra

**Optimized** for syslog and structured logs.

```bash
# Compress
ulc-ultra/ulc-ultra.exe compress syslog.log -o syslog.ulcu

# Decompress
ulc-ultra/ulc-ultra.exe decompress syslog.ulcu -o syslog_restored.log
```

### ULC-C

**Fast** baseline compression.

```bash
# Compress
ulc-c/ulc.exe compress input.log -o output.ulc

# Decompress
ulc-c/ulc.exe decompress output.ulc -o restored.log
```

---

## Advanced Usage

### Batch Processing

```bash
# Compress all .log files in a directory
for file in *.log; do
    ulc-unified/ulc-auto.exe compress "$file" -o "${file}.ulc"
done

# Decompress all .ulc files
for file in *.ulc; do
    ulc-unified/ulc-auto.exe decompress "$file" -o "${file%.ulc}.restored"
done
```

### Pipeline Integration

```bash
# Compress logs as they rotate
tail -f /var/log/apache2/access.log | \
    split -b 100M - log_chunk_ && \
    for chunk in log_chunk_*; do
        ulc-hyper/ulc-hyper.exe compress "$chunk" -o "$chunk.ulch"
        rm "$chunk"
    done
```

### Verification

```bash
# Compress and verify
ulc-unified/ulc-auto.exe compress input.log -o output.ulc
ulc-unified/ulc-auto.exe decompress output.ulc -o restored.log

# Compare (should be identical)
diff input.log restored.log
# or on Windows:
fc /b input.log restored.log
```

---

## Choosing the Right Variant

### Decision Flowchart

```
Is it a web server log (Apache/Nginx)?
├─ YES → Use ULC-Hyper (31x compression)
└─ NO
    │
    Is it syslog or system logs?
    ├─ YES → Use ULC-C or ULC-Ultra (23x compression)
    └─ NO
        │
        Is it application logs with stack traces?
        ├─ YES → Use ULC-Hyper (24x compression)
        └─ NO → Use ULC-Unified (auto-select)
```

### Use Case Matrix

| Log Type | Best Variant | Expected Ratio | Why |
|----------|--------------|----------------|-----|
| Apache/Nginx | ULC-Hyper | 30-32x | URLs and user agents compress well with tokenization |
| Syslog | ULC-C | 23-24x | Short structured lines work best with dictionary |
| Application | ULC-Hyper | 23-25x | Variable messages benefit from semantic decomposition |
| Database | ULC-Ultra | 20-25x | Mixed numeric and string data |
| Security | ULC-Ultra | 18-22x | IPs and timestamps use specialized encoding |
| Mixed | ULC-Unified | Varies | Automatic selection ensures optimal compression |

---

## Performance Tips

### 1. File Size

- **Small files (< 1MB)**: Any variant works, minimal difference
- **Medium files (1-100MB)**: Use recommended variant for log type
- **Large files (> 100MB)**: ULC-Hyper or ULC-Ultra for best compression

### 2. Speed vs Compression

| Priority | Recommended | Ratio | Speed |
|----------|-------------|-------|-------|
| **Maximum Compression** | ULC-Hyper | 30x | Moderate |
| **Balanced** | ULC-Ultra | 23x | Fast |
| **Maximum Speed** | ULC-C | 23x | Fastest |
| **Automatic** | ULC-Unified | Varies | Varies |

### 3. Memory Usage

All variants use ~150-200MB RAM during compression. For very large files:

```bash
# Process in chunks
split -b 500M large.log chunk_
for chunk in chunk_*; do
    ulc-unified/ulc-auto.exe compress "$chunk" -o "$chunk.ulc"
done
```

---

## Integration Examples

### Log Rotation Script

```bash
#!/bin/bash
# /etc/logrotate.d/ulc-compress

/var/log/apache2/*.log {
    daily
    rotate 7
    compress
    delaycompress
    postrotate
        for log in /var/log/apache2/*.log.1; do
            ulc-hyper/ulc-hyper.exe compress "$log" -o "$log.ulch"
            rm "$log"
        done
    endscript
}
```

### Python Integration

```python
import subprocess

def compress_log(input_file, output_file):
    """Compress a log file using ULC-Unified"""
    result = subprocess.run([
        'ulc-unified/ulc-auto.exe',
        'compress',
        input_file,
        '-o',
        output_file
    ], capture_output=True)
    
    if result.returncode == 0:
        print(f"Compressed {input_file} → {output_file}")
    else:
        print(f"Error: {result.stderr.decode()}")

# Usage
compress_log('app.log', 'app.log.ulc')
```

### Systemd Service

```ini
[Unit]
Description=ULC Log Compression Service
After=network.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/ulc-compress-logs.sh

[Install]
WantedBy=multi-user.target
```

---

## Troubleshooting

### Common Issues

**1. "Cannot open input file"**
```bash
# Check file exists and has read permissions
ls -l input.log
chmod +r input.log
```

**2. "Decompression failed"**
```bash
# Verify file is not corrupted
file output.ulc

# Try with correct variant
ulc-hyper/ulc-hyper.exe decompress output.ulch -o restored.log
```

**3. "Out of memory"**
```bash
# Process in smaller chunks
split -b 100M large.log chunk_
```

**4. "Compression ratio lower than expected"**
- Check if log format matches expected type
- Try different variant (ULC-Hyper for complex logs)
- Verify LZMA library is installed correctly

### Debug Mode

```bash
# Enable verbose output (if implemented)
ULC_DEBUG=1 ulc-unified/ulc-auto.exe compress input.log -o output.ulc
```

---

## Benchmarking

### Run Benchmarks

```bash
cd benchmarks/scripts
python benchmark_suite.py
```

Results will be in `benchmarks/results/`:
- `comparison.html` - Interactive visualization
- `comparison.md` - Markdown tables

### Custom Benchmark

```python
import time
import os

def benchmark_compression(input_file, compressor):
    start = time.time()
    os.system(f'{compressor} compress {input_file} -o test.ulc')
    duration = time.time() - start
    
    orig_size = os.path.getsize(input_file)
    comp_size = os.path.getsize('test.ulc')
    ratio = orig_size / comp_size
    
    print(f"Ratio: {ratio:.2f}x, Time: {duration:.3f}s")
    os.remove('test.ulc')

benchmark_compression('my.log', 'ulc-unified/ulc-auto.exe')
```

---

## FAQ

**Q: Which variant should I use?**
A: Use ULC-Unified for automatic selection, or ULC-Hyper for web/app logs and ULC-C for syslog.

**Q: Is compression lossless?**
A: Yes, 100% lossless. Decompressed files are byte-for-byte identical to the original.

**Q: Can I compress already compressed files?**
A: Not recommended. ULC works best on uncompressed text logs.

**Q: How does it compare to gzip?**
A: ULC achieves 1.7-2.2x better compression than gzip on structured logs.

**Q: Can I use this in production?**
A: Yes, but test thoroughly first. ULC is optimized for log archival, not real-time streaming.

**Q: What about JSON logs?**
A: Current version works best on space-delimited logs. JSON support is planned for future releases.

---

## Support

For issues and questions:
- GitHub Issues: https://github.com/[username]/ulc/issues
- Documentation: https://github.com/[username]/ulc/docs

## Contributing

Contributions welcome! See CONTRIBUTING.md for guidelines.
