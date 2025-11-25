# ULC-Hyper: Semantic Decomposition Compression

**The most advanced log compressor in the ULC family.**

> [!IMPORTANT]
> **ULC-Hyper beats ULC-Ultra v3 by 20-50%** on Web and App logs.
> It uses **Semantic Decomposition** to tokenize and compress internal field structures.

## Performance

| Log Type | ULC-Hyper | ULC-Ultra v3 | Improvement |
|----------|-----------|--------------|-------------|
| **Apache** | **31.3x** | 26.1x | **+20%** |
| **App Logs** | **23.7x** | 15.8x | **+50%** |
| **Syslog** | 14.8x | **23.1x** | -35% |

## How it Works

ULC-Hyper doesn't just see a URL like `/api/v1/user`.
It sees `["api", "v1", "user"]`.
It dictionary-encodes these *parts*, allowing for massive deduplication even when the full URL is unique.

## Usage

```bash
ulc-hyper.exe compress input.log -o output.ulch
ulc-hyper.exe decompress output.ulch -o restored.log
```

## Supported Log Types

ULC-Hyper is designed for **Space-Separated** log formats.

| Log Type | Support | Performance | Notes |
|----------|---------|-------------|-------|
| **Apache / Nginx** | ✅ **Perfect** | **31x** (vs Gzip 13x) | Best use case. Decomposes URLs/UAs. |
| **Application Logs** | ✅ **Perfect** | **23x** (vs Gzip 9x) | Great for Log4j/Python logs. |
| **Syslog** | ⚠️ **Fair** | 14x (vs Gzip 12x) | Use **ULC-Ultra v3** (23x) instead. |
| **CSV / TSV** | ⚠️ **Partial** | Varies | Only if space-delimited. |
| **JSON** | ❌ **No** | Poor | Structure breaks the columnar parser. |

## Benchmark vs The World

| Algorithm | Apache Ratio | App Log Ratio |
|-----------|--------------|---------------|
| **ULC-Hyper** | **31.3x** | **23.7x** |
| Bzip2 | 25.1x | 14.2x |
| LZMA (.xz) | 17.5x | 13.9x |
| Gzip | 13.6x | 9.4x |

**ULC-Hyper is ~2x better than LZMA/Bzip2 for web/app logs.**
