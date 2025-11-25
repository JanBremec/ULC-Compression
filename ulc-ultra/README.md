# ULC-Ultra: Maximum Compression for Structured Logs

**v3 (Hybrid Columnar)** - The winning implementation.

> [!IMPORTANT]
> **ULC-Ultra v3 beats ULC-C by 15% on Application logs** and matches it on Web/Syslog.
> It uses a **Hybrid Columnar** strategy that adapts to your data.

## Features

- **Hybrid Encoding**: Automatically chooses between Dictionary and Raw encoding per column.
- **Delta Encoding**: Efficient compression for timestamps and numerics.
- **XOR Encoding**: Specialized compression for IP addresses.
- **Aggressive LZMA**: Maximum settings (128MB dict) for final squeeze.

## Performance

| Log Type | ULC-Ultra v3 | ULC-C | Bzip2 | Result |
|----------|--------------|-------|-------|--------|
| **App Logs** | **15.8x** | 13.7x | 14x | 🏆 **+15%** |
| **Apache** | **26.1x** | 26.2x | 25x | 🤝 **Match** |
| **Syslog** | **23.1x** | 23.4x | 20x | 🤝 **Match** |

## Usage

```bash
ulc-ultra.exe compress input.log -o output.ulcu
ulc-ultra.exe decompress output.ulcu -o restored.log
```

## When to Use

- **Use ULC-Ultra** when **Compression Ratio** is the #1 priority.
- **Use ULC-C** when **Speed** is the #1 priority (ULC-Ultra is slower).

## Requirements

- Minimum 100 lines per file.
- Structured logs (Apache, Syslog, App logs).
- 256MB+ RAM.
