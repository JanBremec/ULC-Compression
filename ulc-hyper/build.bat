@echo off
gcc -O3 -I./include src/ulc_hyper_compress.c src/ulc_hyper_cli.c -o ulc-hyper.exe -llzma
if %errorlevel% neq 0 (
    echo Build failed!
    exit /b %errorlevel%
)
echo Build successful!
