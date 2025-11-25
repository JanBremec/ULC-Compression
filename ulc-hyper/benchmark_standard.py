import os
import gzip
import bz2
import lzma
import time

LOG_FILES = [
    "../../test_log_web.txt",
    "../../test_log_sys.txt",
    "../../test_log_app.txt"
]

ULC_HYPER_RESULTS = {
    "../../test_log_web.txt": 6504,
    "../../test_log_sys.txt": 9104,
    "../../test_log_app.txt": 11116
}

def get_size(path):
    return os.path.getsize(path)

def benchmark():
    print(f"{'File':<25} {'Algo':<10} {'Size (B)':<10} {'Ratio':<10} {'Time (s)':<10}")
    print("-" * 70)

    for log_file in LOG_FILES:
        if not os.path.exists(log_file):
            print(f"Skipping {log_file} (not found)")
            continue

        orig_size = get_size(log_file)
        filename = os.path.basename(log_file)

        # Gzip
        start = time.time()
        with open(log_file, 'rb') as f_in:
            with gzip.open(log_file + '.gz', 'wb', compresslevel=9) as f_out:
                f_out.writelines(f_in)
        gzip_size = get_size(log_file + '.gz')
        gzip_time = time.time() - start
        print(f"{filename:<25} {'Gzip':<10} {gzip_size:<10} {orig_size/gzip_size:<10.2f} {gzip_time:<10.4f}")
        os.remove(log_file + '.gz')

        # Bzip2
        start = time.time()
        with open(log_file, 'rb') as f_in:
            with bz2.open(log_file + '.bz2', 'wb', compresslevel=9) as f_out:
                f_out.writelines(f_in)
        bz2_size = get_size(log_file + '.bz2')
        bz2_time = time.time() - start
        print(f"{filename:<25} {'Bzip2':<10} {bz2_size:<10} {orig_size/bz2_size:<10.2f} {bz2_time:<10.4f}")
        os.remove(log_file + '.bz2')

        # LZMA
        start = time.time()
        with open(log_file, 'rb') as f_in:
            with lzma.open(log_file + '.xz', 'wb', preset=9 | lzma.PRESET_EXTREME) as f_out:
                f_out.writelines(f_in)
        lzma_size = get_size(log_file + '.xz')
        lzma_time = time.time() - start
        print(f"{filename:<25} {'LZMA':<10} {lzma_size:<10} {orig_size/lzma_size:<10.2f} {lzma_time:<10.4f}")
        os.remove(log_file + '.xz')

        # ULC-Hyper (Static from previous run)
        hyper_size = ULC_HYPER_RESULTS.get(log_file, 0)
        if hyper_size > 0:
            print(f"{filename:<25} {'ULC-Hyper':<10} {hyper_size:<10} {orig_size/hyper_size:<10.2f} {'-':<10}")

        print("-" * 70)

if __name__ == "__main__":
    benchmark()
