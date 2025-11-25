@echo off
REM Build script for ULC C implementation

echo Building ULC C Implementation...

REM Create build directory
if not exist build mkdir build

REM Compile source files
echo Compiling ulc_utils.c...
gcc -Wall -Wextra -O3 -Iinclude -c src/ulc_utils.c -o build/ulc_utils.o
if errorlevel 1 goto error

echo Compiling ulc_parser.c...
gcc -Wall -Wextra -O3 -Iinclude -c src/ulc_parser.c -o build/ulc_parser.o
if errorlevel 1 goto error

echo Compiling ulc_compress.c...
gcc -Wall -Wextra -O3 -Iinclude -c src/ulc_compress.c -o build/ulc_compress.o
if errorlevel 1 goto error

echo Compiling ulc_cli.c...
gcc -Wall -Wextra -O3 -Iinclude -c src/ulc_cli.c -o build/ulc_cli.o
if errorlevel 1 goto error

REM Link executable
echo Linking ulc.exe...
gcc build/ulc_utils.o build/ulc_parser.o build/ulc_compress.o build/ulc_cli.o -llzma -o ulc.exe
if errorlevel 1 goto error

echo.
echo Build successful! Created ulc.exe
goto end

:error
echo.
echo Build failed!
exit /b 1

:end
