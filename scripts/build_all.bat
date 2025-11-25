@echo off
echo ========================================
echo Building ULC Suite
echo ========================================

echo.
echo [1/4] Building ULC-C...
cd ulc-c
call build.bat
if %errorlevel% neq 0 (
    echo ERROR: ULC-C build failed
    exit /b 1
)
cd ..

echo.
echo [2/4] Building ULC-Ultra...
cd ulc-ultra
call build.bat
if %errorlevel% neq 0 (
    echo ERROR: ULC-Ultra build failed
    exit /b 1
)
cd ..

echo.
echo [3/4] Building ULC-Hyper...
cd ulc-hyper
call build.bat
if %errorlevel% neq 0 (
    echo ERROR: ULC-Hyper build failed
    exit /b 1
)
cd ..

echo.
echo [4/4] Building ULC-Unified...
cd ulc-unified
call build.bat
if %errorlevel% neq 0 (
    echo ERROR: ULC-Unified build failed
    exit /b 1
)
cd ..

echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Executables:
echo   ulc-c\ulc.exe
echo   ulc-ultra\ulc-ultra.exe
echo   ulc-hyper\ulc-hyper.exe
echo   ulc-unified\ulc-auto.exe
echo.
