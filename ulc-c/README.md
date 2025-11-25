# ULC C Implementation

High-performance C implementation of the Ultra Log Compressor algorithm.

## Features

- **Full feature parity** with Python implementation
- **Multiple log format support**: Apache, Syslog, Generic, Security, Raw
- **Columnar transformation** with typed columns
- **Advanced encoding**: Delta encoding, Varint encoding, ZigZag encoding
- **LZMA compression** for maximum compression ratios
- **Fast performance**: Expected 5-50x speedup over Python

## Building

### Prerequisites

- GCC compiler (MinGW on Windows)
- liblzma library

### Compile

```bash
make
```

This will create `ulc.exe` executable.

## Usage

### Compress a log file

```bash
ulc.exe compress input.log -o output.ulc
```

### Decompress a file

```bash
ulc.exe decompress output.ulc -o restored.log
```

### Show file info

```bash
ulc.exe info output.ulc
```

## Benchmarking

Compare C and Python implementations:

```bash
python benchmark_c_vs_python.py
```

This will:
- Run both implementations on test files
- Compare compression speed
- Compare decompression speed
- Verify correctness
- Generate HTML comparison report

## Project Structure

```
ulc-c/
├── include/          # Header files
│   ├── ulc_types.h   # Data structures
│   ├── ulc_utils.h   # Utility functions
│   ├── ulc_parser.h  # Log parsing
│   └── ulc_compress.h # Compression engine
├── src/              # Source files
│   ├── ulc_utils.c
│   ├── ulc_parser.c
│   ├── ulc_compress.c
│   └── ulc_cli.c     # Main CLI
├── Makefile          # Build system
└── benchmark_c_vs_python.py  # Benchmark script
```

## Implementation Notes

This C implementation focuses on:
- **Speed**: Optimized algorithms and minimal overhead
- **Correctness**: Same compression format as Python version
- **Simplicity**: Clean, readable code structure
- **Portability**: Standard C with minimal dependencies

## Performance

Expected performance improvements over Python:
- **Compression**: 5-20x faster
- **Decompression**: 10-50x faster
- **Memory usage**: 2-5x lower

Actual results depend on log format and data characteristics.
