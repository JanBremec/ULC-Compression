@echo off
gcc -O3 src/ulc_auto.c -o ulc-auto.exe
if %errorlevel% neq 0 (
    echo Build failed!
    exit /b %errorlevel%
)
echo Build successful!
