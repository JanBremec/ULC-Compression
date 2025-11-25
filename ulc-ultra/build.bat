@echo off
REM Build script for ULC-Ultra

echo Building ULC-Ultra (Maximum Compression)...
echo.

REM Create build directory
if not exist build mkdir build

REM Compile ULC-C utilities (reuse from ulc-c)
echo Compiling shared utilities...
gcc -Wall -Wextra -O3 -I../ulc-c/include -c ../ulc-c/src/ulc_utils.c -o build/ulc_utils.o
if errorlevel 1 goto error

gcc -Wall -Wextra -O3 -I../ulc-c/include -c ../ulc-c/src/ulc_parser.c -o build/ulc_parser.o
if errorlevel 1 goto error

REM Compile ULC-Ultra components
echo Compiling pattern mining...
gcc -Wall -Wextra -O3 -Iinclude -c src/ulc_ultra_pattern.c -o build/ulc_ultra_pattern.o
if errorlevel 1 goto error

echo Compiling Huffman coding...
gcc -Wall -Wextra -O3 -Iinclude -c src/ulc_ultra_huffman.c -o build/ulc_ultra_huffman.o
if errorlevel 1 goto error

echo Compiling compression engine...
gcc -Wall -Wextra -O3 -Iinclude -I../ulc-c/include -c src/ulc_ultra_compress.c -o build/ulc_ultra_compress.o
if errorlevel 1 goto error

echo Compiling CLI...
gcc -Wall -Wextra -O3 -Iinclude -c src/ulc_ultra_cli.c -o build/ulc_ultra_cli.o
if errorlevel 1 goto error

REM Link executable
echo Linking ulc-ultra.exe...
gcc build/ulc_utils.o build/ulc_parser.o build/ulc_ultra_pattern.o build/ulc_ultra_huffman.o build/ulc_ultra_compress.o build/ulc_ultra_cli.o -llzma -o ulc-ultra.exe
if errorlevel 1 goto error

echo.
echo ✓ Build successful! Created ulc-ultra.exe
goto end

:error
echo.
echo ✗ Build failed!
exit /b 1

:end
