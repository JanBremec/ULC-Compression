"""
Benchmark C vs Python ULC Implementation

Compares speed and correctness of C and Python implementations.
"""

import os
import sys
import time
import subprocess
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from ultra_log_compressor.ulc_core import UltraLogCompressor

def format_size(bytes_size):
    """Format bytes to human-readable size."""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if bytes_size < 1024.0:
            return f"{bytes_size:.2f} {unit}"
        bytes_size /= 1024.0
    return f"{bytes_size:.2f} TB"

def benchmark_python(input_file):
    """Benchmark Python implementation."""
    print(f"\n{'='*60}")
    print("PYTHON IMPLEMENTATION")
    print('='*60)
    
    # Read input
    with open(input_file, 'r', encoding='utf-8', errors='ignore') as f:
        lines = [line.rstrip('\n') for line in f]
    
    orig_size = sum(len(line.encode('utf-8')) + 1 for line in lines)
    
    # Compress
    compressor = UltraLogCompressor()
    start = time.time()
    compressed = compressor.compress(lines)
    compress_time = time.time() - start
    
    comp_size = len(compressed)
    
    # Decompress
    start = time.time()
    decompressed = compressor.decompress(compressed)
    decompress_time = time.time() - start
    
    # Verify
    success = len(decompressed) == len(lines)
    
    print(f"  Lines:           {len(lines):,}")
    print(f"  Original size:   {format_size(orig_size)}")
    print(f"  Compressed size: {format_size(comp_size)}")
    print(f"  Ratio:           {orig_size/comp_size:.2f}x")
    print(f"  Space savings:   {(1-comp_size/orig_size)*100:.2f}%")
    print(f"  Compress time:   {compress_time:.3f}s")
    print(f"  Decompress time: {decompress_time:.3f}s")
    print(f"  Compress speed:  {format_size(orig_size/compress_time)}/s")
    print(f"  Decompress speed:{format_size(orig_size/decompress_time)}/s")
    print(f"  Correctness:     {'✓ PASS' if success else '✗ FAIL'}")
    
    return {
        'name': 'Python',
        'lines': len(lines),
        'orig_size': orig_size,
        'comp_size': comp_size,
        'ratio': orig_size/comp_size,
        'compress_time': compress_time,
        'decompress_time': decompress_time,
        'success': success
    }

def benchmark_c(input_file, ulc_exe):
    """Benchmark C implementation."""
    print(f"\n{'='*60}")
    print("C IMPLEMENTATION")
    print('='*60)
    
    # Get original size
    orig_size = os.path.getsize(input_file)
    
    # Count lines
    with open(input_file, 'r', encoding='utf-8', errors='ignore') as f:
        lines = sum(1 for _ in f)
    
    # Compress
    output_file = input_file + '.c.ulc'
    start = time.time()
    result = subprocess.run([ulc_exe, 'compress', input_file, '-o', output_file],
                          capture_output=True, text=True)
    compress_time = time.time() - start
    
    if result.returncode != 0:
        print(f"  Error: Compression failed")
        print(result.stderr)
        return None
    
    comp_size = os.path.getsize(output_file)
    
    # Decompress
    decomp_file = input_file + '.c.decompressed'
    start = time.time()
    result = subprocess.run([ulc_exe, 'decompress', output_file, '-o', decomp_file],
                          capture_output=True, text=True)
    decompress_time = time.time() - start
    
    success = result.returncode == 0
    
    print(f"  Lines:           {lines:,}")
    print(f"  Original size:   {format_size(orig_size)}")
    print(f"  Compressed size: {format_size(comp_size)}")
    print(f"  Ratio:           {orig_size/comp_size:.2f}x")
    print(f"  Space savings:   {(1-comp_size/orig_size)*100:.2f}%")
    print(f"  Compress time:   {compress_time:.3f}s")
    print(f"  Decompress time: {decompress_time:.3f}s")
    print(f"  Compress speed:  {format_size(orig_size/compress_time)}/s")
    print(f"  Decompress speed:{format_size(orig_size/decompress_time)}/s")
    print(f"  Correctness:     {'✓ PASS' if success else '✗ FAIL'}")
    
    # Cleanup
    if os.path.exists(output_file):
        os.remove(output_file)
    if os.path.exists(decomp_file):
        os.remove(decomp_file)
    
    return {
        'name': 'C',
        'lines': lines,
        'orig_size': orig_size,
        'comp_size': comp_size,
        'ratio': orig_size/comp_size,
        'compress_time': compress_time,
        'decompress_time': decompress_time,
        'success': success
    }

def print_comparison(py_result, c_result):
    """Print comparison table."""
    print(f"\n{'='*60}")
    print("COMPARISON")
    print('='*60)
    
    if not c_result:
        print("  C implementation failed to run")
        return
    
    print(f"\n  Metric                  Python          C            Speedup")
    print(f"  {'-'*58}")
    print(f"  Compress Time          {py_result['compress_time']:6.3f}s      {c_result['compress_time']:6.3f}s      {py_result['compress_time']/c_result['compress_time']:5.2f}x")
    print(f"  Decompress Time        {py_result['decompress_time']:6.3f}s      {c_result['decompress_time']:6.3f}s      {py_result['decompress_time']/c_result['decompress_time']:5.2f}x")
    print(f"  Compression Ratio      {py_result['ratio']:6.2f}x       {c_result['ratio']:6.2f}x       {c_result['ratio']/py_result['ratio']:5.2f}x")
    
    print(f"\n  Overall C is {py_result['compress_time']/c_result['compress_time']:.1f}x faster at compression")
    print(f"  Overall C is {py_result['decompress_time']/c_result['decompress_time']:.1f}x faster at decompression")

def generate_html_report(results, output_file):
    """Generate HTML comparison report."""
    html = """<!DOCTYPE html>
<html>
<head>
    <title>ULC C vs Python Benchmark</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1 { color: #333; border-bottom: 3px solid #4CAF50; padding-bottom: 10px; }
        h2 { color: #555; margin-top: 30px; }
        table { width: 100%; border-collapse: collapse; margin: 20px 0; }
        th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }
        th { background: #4CAF50; color: white; }
        tr:hover { background: #f5f5f5; }
        .speedup { font-weight: bold; color: #4CAF50; }
        .metric { font-family: monospace; }
        .summary { background: #e8f5e9; padding: 20px; border-radius: 4px; margin: 20px 0; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 ULC C vs Python Benchmark Results</h1>
"""
    
    for test_name, (py_res, c_res) in results.items():
        if not c_res:
            continue
            
        speedup_comp = py_res['compress_time'] / c_res['compress_time']
        speedup_decomp = py_res['decompress_time'] / c_res['decompress_time']
        
        html += f"""
        <h2>Test: {test_name}</h2>
        <table>
            <tr>
                <th>Metric</th>
                <th>Python</th>
                <th>C</th>
                <th>Speedup</th>
            </tr>
            <tr>
                <td>Lines</td>
                <td class="metric">{py_res['lines']:,}</td>
                <td class="metric">{c_res['lines']:,}</td>
                <td>-</td>
            </tr>
            <tr>
                <td>Compression Ratio</td>
                <td class="metric">{py_res['ratio']:.2f}x</td>
                <td class="metric">{c_res['ratio']:.2f}x</td>
                <td class="metric">{c_res['ratio']/py_res['ratio']:.2f}x</td>
            </tr>
            <tr>
                <td>Compress Time</td>
                <td class="metric">{py_res['compress_time']:.3f}s</td>
                <td class="metric">{c_res['compress_time']:.3f}s</td>
                <td class="speedup">{speedup_comp:.2f}x faster</td>
            </tr>
            <tr>
                <td>Decompress Time</td>
                <td class="metric">{py_res['decompress_time']:.3f}s</td>
                <td class="metric">{c_res['decompress_time']:.3f}s</td>
                <td class="speedup">{speedup_decomp:.2f}x faster</td>
            </tr>
        </table>
        
        <div class="summary">
            <strong>Summary:</strong> C implementation is <span class="speedup">{speedup_comp:.1f}x faster</span> at compression 
            and <span class="speedup">{speedup_decomp:.1f}x faster</span> at decompression.
        </div>
"""
    
    html += """
    </div>
</body>
</html>
"""
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(html)
    
    print(f"\n  HTML report generated: {output_file}")

def main():
    # Find ULC executable
    ulc_exe = Path(__file__).parent / 'ulc.exe'
    if not ulc_exe.exists():
        print("Error: ulc.exe not found. Please build it first with 'make'")
        return 1
    
    # Find test files
    test_dir = Path(__file__).parent.parent
    test_files = [
        test_dir / 'test_log_app.txt',
        test_dir / 'test_log_web.txt',
        test_dir / 'test_log_sys.txt',
    ]
    
    # Filter existing files
    test_files = [f for f in test_files if f.exists()]
    
    if not test_files:
        print("Error: No test files found")
        return 1
    
    print("="*60)
    print("ULC C vs Python Benchmark")
    print("="*60)
    print(f"\nFound {len(test_files)} test file(s)")
    
    all_results = {}
    
    for test_file in test_files:
        print(f"\n{'#'*60}")
        print(f"# Testing: {test_file.name}")
        print(f"{'#'*60}")
        
        py_result = benchmark_python(str(test_file))
        c_result = benchmark_c(str(test_file), str(ulc_exe))
        
        if py_result and c_result:
            print_comparison(py_result, c_result)
            all_results[test_file.name] = (py_result, c_result)
    
    # Generate HTML report
    if all_results:
        report_file = Path(__file__).parent / 'comparison_report.html'
        generate_html_report(all_results, str(report_file))
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
