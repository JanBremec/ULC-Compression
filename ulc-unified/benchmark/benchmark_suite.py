import os
import gzip
import bz2
import lzma
import time
import subprocess

# Use absolute paths from script location
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BASE_DIR = os.path.dirname(SCRIPT_DIR)  # final/ulc-unified
FINAL_DIR = os.path.dirname(BASE_DIR)   # final/

LOG_FILES = {
    "test_log_web.txt": os.path.join(FINAL_DIR, "test_log_web.txt"),
    "test_log_sys.txt": os.path.join(FINAL_DIR, "test_log_sys.txt"),
    "test_log_app.txt": os.path.join(FINAL_DIR, "test_log_app.txt")
}

RESULTS = {}

def get_size(path):
    if os.path.exists(path):
        return os.path.getsize(path)
    return 0

def benchmark():
    print("Starting comprehensive benchmark...")
    print(f"Base directory: {BASE_DIR}")
    
    for filename, log_path in LOG_FILES.items():
        if not os.path.exists(log_path):
            print(f"SKIP: {filename} not found at {log_path}")
            continue
            
        orig_size = get_size(log_path)
        RESULTS[filename] = {"Original": orig_size}
        print(f"\n{'='*60}")
        print(f"Benchmarking: {filename} ({orig_size:,} bytes)")
        print(f"{'='*60}")
        
        # Gzip
        print("  Testing Gzip...", end=" ")
        start = time.time()
        gz_path = log_path + '.gz'
        try:
            with open(log_path, 'rb') as f_in, gzip.open(gz_path, 'wb', compresslevel=9) as f_out:
                f_out.writelines(f_in)
            size = get_size(gz_path)
            duration = time.time() - start
            RESULTS[filename]["Gzip"] = (size, duration)
            print(f"{size:,} bytes ({orig_size/size:.2f}x) in {duration:.3f}s")
            os.remove(gz_path)
        except Exception as e:
            print(f"FAILED: {e}")
            RESULTS[filename]["Gzip"] = (0, 0)
        
        # Bzip2
        print("  Testing Bzip2...", end=" ")
        start = time.time()
        bz2_path = log_path + '.bz2'
        try:
            with open(log_path, 'rb') as f_in, bz2.open(bz2_path, 'wb', compresslevel=9) as f_out:
                f_out.writelines(f_in)
            size = get_size(bz2_path)
            duration = time.time() - start
            RESULTS[filename]["Bzip2"] = (size, duration)
            print(f"{size:,} bytes ({orig_size/size:.2f}x) in {duration:.3f}s")
            os.remove(bz2_path)
        except Exception as e:
            print(f"FAILED: {e}")
            RESULTS[filename]["Bzip2"] = (0, 0)

        # LZMA
        print("  Testing LZMA...", end=" ")
        start = time.time()
        xz_path = log_path + '.xz'
        try:
            with open(log_path, 'rb') as f_in, lzma.open(xz_path, 'wb', preset=9 | lzma.PRESET_EXTREME) as f_out:
                f_out.writelines(f_in)
            size = get_size(xz_path)
            duration = time.time() - start
            RESULTS[filename]["LZMA"] = (size, duration)
            print(f"{size:,} bytes ({orig_size/size:.2f}x) in {duration:.3f}s")
            os.remove(xz_path)
        except Exception as e:
            print(f"FAILED: {e}")
            RESULTS[filename]["LZMA"] = (0, 0)

        # ULC-C
        ulc_c_exe = os.path.join(FINAL_DIR, "ulc-c", "ulc.exe")
        if os.path.exists(ulc_c_exe):
            print("  Testing ULC-C...", end=" ")
            out_file = os.path.join(BASE_DIR, "temp_ulc_c.ulc")
            try:
                start = time.time()
                result = subprocess.run([ulc_c_exe, "compress", log_path, "-o", out_file], 
                                      capture_output=True, text=True, timeout=60)
                duration = time.time() - start
                size = get_size(out_file)
                if size > 0:
                    RESULTS[filename]["ULC-C"] = (size, duration)
                    print(f"{size:,} bytes ({orig_size/size:.2f}x) in {duration:.3f}s")
                    os.remove(out_file)
                else:
                    print(f"FAILED: No output file")
                    RESULTS[filename]["ULC-C"] = (0, 0)
            except Exception as e:
                print(f"FAILED: {e}")
                RESULTS[filename]["ULC-C"] = (0, 0)

        # ULC-Ultra
        ulc_ultra_exe = os.path.join(BASE_DIR, "bin", "ulc-ultra.exe")
        if os.path.exists(ulc_ultra_exe):
            print("  Testing ULC-Ultra...", end=" ")
            out_file = os.path.join(BASE_DIR, "temp_ultra.ulcu")
            try:
                start = time.time()
                result = subprocess.run([ulc_ultra_exe, "compress", log_path, "-o", out_file], 
                                      capture_output=True, text=True, timeout=60)
                duration = time.time() - start
                size = get_size(out_file)
                if size > 0:
                    RESULTS[filename]["ULC-Ultra"] = (size, duration)
                    print(f"{size:,} bytes ({orig_size/size:.2f}x) in {duration:.3f}s")
                    os.remove(out_file)
                else:
                    print(f"FAILED: No output file")
                    RESULTS[filename]["ULC-Ultra"] = (0, 0)
            except Exception as e:
                print(f"FAILED: {e}")
                RESULTS[filename]["ULC-Ultra"] = (0, 0)

        # ULC-Hyper
        ulc_hyper_exe = os.path.join(BASE_DIR, "bin", "ulc-hyper.exe")
        if os.path.exists(ulc_hyper_exe):
            print("  Testing ULC-Hyper...", end=" ")
            out_file = os.path.join(BASE_DIR, "temp_hyper.ulch")
            try:
                start = time.time()
                result = subprocess.run([ulc_hyper_exe, "compress", log_path, "-o", out_file], 
                                      capture_output=True, text=True, timeout=60)
                duration = time.time() - start
                size = get_size(out_file)
                if size > 0:
                    RESULTS[filename]["ULC-Hyper"] = (size, duration)
                    print(f"{size:,} bytes ({orig_size/size:.2f}x) in {duration:.3f}s")
                    os.remove(out_file)
                else:
                    print(f"FAILED: No output file")
                    RESULTS[filename]["ULC-Hyper"] = (0, 0)
            except Exception as e:
                print(f"FAILED: {e}")
                RESULTS[filename]["ULC-Hyper"] = (0, 0)

        # ULC-Unified
        ulc_auto_exe = os.path.join(BASE_DIR, "ulc-auto.exe")
        if os.path.exists(ulc_auto_exe):
            print("  Testing ULC-Unified...", end=" ")
            out_file = os.path.join(BASE_DIR, "temp_auto.ulc")
            try:
                start = time.time()
                result = subprocess.run([ulc_auto_exe, "compress", log_path, "-o", out_file], 
                                      capture_output=True, text=True, timeout=60)
                duration = time.time() - start
                size = get_size(out_file)
                if size > 0:
                    RESULTS[filename]["ULC-Unified"] = (size, duration)
                    print(f"{size:,} bytes ({orig_size/size:.2f}x) in {duration:.3f}s")
                    os.remove(out_file)
                else:
                    print(f"FAILED: No output file")
                    RESULTS[filename]["ULC-Unified"] = (0, 0)
            except Exception as e:
                print(f"FAILED: {e}")
                RESULTS[filename]["ULC-Unified"] = (0, 0)

def generate_reports():
    if not RESULTS:
        print("No results to report!")
        return

    # Markdown Report
    md_path = os.path.join(os.path.dirname(SCRIPT_DIR), "comparison.md")
    with open(md_path, "w") as f:
        f.write("# Extensive Log Compression Benchmark\n\n")
        f.write(f"**Generated:** {time.strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        
        for filename, data in RESULTS.items():
            orig = data["Original"]
            f.write(f"## {filename}\n")
            f.write(f"**Original Size:** {orig:,} bytes\n\n")
            f.write("| Algorithm | Compressed Size | Ratio | Time (s) | vs Gzip | vs LZMA |\n")
            f.write("|-----------|----------------|-------|----------|---------|----------|\n")
            
            gzip_data = data.get("Gzip", (orig, 1))
            lzma_data = data.get("LZMA", (orig, 1))
            gzip_size = gzip_data[0] if gzip_data[0] > 0 else orig
            lzma_size = lzma_data[0] if lzma_data[0] > 0 else orig
            
            algos = [(k, v) for k, v in data.items() if k != "Original"]
            algos.sort(key=lambda x: x[1][0] if x[1][0] > 0 else float('inf'))
            
            for algo, (size, duration) in algos:
                if size == 0:
                    f.write(f"| {algo} | FAILED | - | - | - | - |\n")
                    continue
                    
                ratio = orig / size
                vs_gzip = gzip_size / size
                vs_lzma = lzma_size / size
                
                winner = "**" if algo == algos[0][0] and size > 0 else ""
                f.write(f"| {winner}{algo}{winner} | {size:,} | **{ratio:.2f}x** | {duration:.3f} | {vs_gzip:.2f}x | {vs_lzma:.2f}x |\n")
            f.write("\n")
    
    print(f"\nMarkdown report saved to: {md_path}")

    # HTML Report
    html_path = os.path.join(os.path.dirname(SCRIPT_DIR), "comparison.html")
    with open(html_path, "w") as f:
        f.write("""<!DOCTYPE html>
<html><head><title>ULC Benchmark</title>
<style>
body { font-family: 'Segoe UI', sans-serif; padding: 40px; background: #f5f5f5; }
.container { max-width: 1200px; margin: 0 auto; }
.card { background: white; padding: 30px; margin-bottom: 30px; border-radius: 8px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }
h1 { text-align: center; color: #333; }
h2 { color: #555; border-bottom: 2px solid #eee; padding-bottom: 10px; }
table { width: 100%; border-collapse: collapse; margin-top: 20px; }
th, td { padding: 12px; text-align: left; border-bottom: 1px solid #eee; }
th { background: #f8f9fa; font-weight: 600; }
.winner { color: #27ae60; font-weight: bold; }
.bar-container { width: 100%; background: #e9ecef; border-radius: 4px; height: 8px; margin-top: 5px; }
.bar { height: 100%; border-radius: 4px; }
.ulc { background: linear-gradient(90deg, #3498db, #2980b9); }
.std { background: linear-gradient(90deg, #95a5a6, #7f8c8d); }
</style></head><body><div class="container"><h1>Log Compression Benchmark</h1>
""")
        
        for filename, data in RESULTS.items():
            orig = data["Original"]
            f.write(f"<div class='card'><h2>{filename}</h2>")
            f.write(f"<p>Original Size: <strong>{orig:,} bytes</strong></p>")
            f.write("<table><thead><tr><th>Algorithm</th><th>Size</th><th>Ratio</th><th>Time</th><th>Visual</th></tr></thead><tbody>")
            
            algos = [(k, v) for k, v in data.items() if k != "Original"]
            algos.sort(key=lambda x: x[1][0] if x[1][0] > 0 else float('inf'))
            
            valid_algos = [x for x in algos if x[1][0] > 0]
            if valid_algos:
                max_size = max(x[1][0] for x in valid_algos)
                
                for algo, (size, duration) in algos:
                    if size == 0: continue
                    
                    ratio = orig / size
                    width = (size / max_size) * 100
                    color = "ulc" if "ULC" in algo else "std"
                    winner_class = "winner" if algo == valid_algos[0][0] else ""
                    
                    f.write(f"<tr><td class='{winner_class}'>{algo}</td>")
                    f.write(f"<td>{size:,} B</td><td>{ratio:.2f}x</td><td>{duration:.3f}s</td>")
                    f.write(f"<td><div class='bar-container'><div class='bar {color}' style='width:{width}%'></div></div></td></tr>")
            
            f.write("</tbody></table></div>")
        
        f.write("</div></body></html>")
    
    print(f"HTML report saved to: {html_path}")

if __name__ == "__main__":
    benchmark()
    generate_reports()
    print("\n" + "="*60)
    print("BENCHMARK COMPLETE")
    print("="*60)
