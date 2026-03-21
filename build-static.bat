@echo off
REM ============================================
REM PaperCrawler Static Build Script
REM ============================================
REM This script builds a statically linked
REM executable that includes all dependencies
REM ============================================

echo.
echo ============================================
echo   PaperCrawler Static Build
echo ============================================
echo.

REM Create build directory
if not exist build-static mkdir build-static
cd build-static

echo.
echo [1/5] Configuring CMake for static build...
echo.

REM Configure CMake with static linking options
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_SHARED_LIBS=OFF ^
    -DCMAKE_EXE_LINKER_FLAGS="-static" ^
    -DCMAKE_FIND_LIBRARY_SUFFIXES=".a" ^
    -DCMAKE_CXX_FLAGS="-static-libgcc -static-libstdc++"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed!
    cd ..
    exit /b 1
)

echo.
echo [2/5] Building static executable...
echo.

REM Build the project
cmake --build . --config Release --parallel

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed!
    cd ..
    exit /b 1
)

echo.
echo [3/5] Stripping debug symbols...
echo.

REM Strip symbols (on Linux/Unix)
if not "%OS%"=="Windows_NT" (
    strip --strip-unneeded bin/PaperCrawlerServer
)

echo.
echo [4/5] Copying runtime files...
echo.

REM Copy configuration files
if not exist bin\config mkdir bin\config
copy ..\config\config.json.example bin\config\config.json

REM Copy SQL scripts
if not exist bin\sql mkdir bin\sql
copy ..\sql\optimize.sql bin\sql\

echo.
echo [5/5] Creating distribution package...
echo.

REM Create distribution directory
set DIST_DIR=PaperCrawler-Windows-x64-%date:~10,4%-%date:~4,2%-%date:~7,2%
if exist %DIST_DIR% rmdir /s /q %DIST_DIR%
mkdir %DIST_DIR%

REM Copy executable and dependencies
mkdir %DIST_DIR%\bin
mkdir %DIST_DIR%\config
mkdir %DIST_DIR%\sql
mkdir %DIST_DIR%\docs

copy bin\PaperCrawlerServer.exe %DIST_DIR%\bin\
copy bin\config\config.json %DIST_DIR%\config\
copy ..\sql\optimize.sql %DIST_DIR%\sql\
copy ..\README.md %DIST_DIR%\docs\
copy ..\OPTIMIZATION-SUMMARY.md %DIST_DIR%\docs\

REM Create README
echo PaperCrawler Standalone Distribution > %DIST_DIR%\README.txt
echo. >> %DIST_DIR%\README.txt
echo Build Date: %date% %time% >> %DIST_DIR%\README.txt
echo Version: 1.0.0 >> %DIST_DIR%\README.txt
echo. >> %DIST_DIR%\README.txt
echo This is a statically linked build - all libraries are included. >> %DIST_DIR%\README.txt
echo No external dependencies required! >> %DIST_DIR%\README.txt
echo. >> %DIST_DIR%\README.txt
echo Quick Start: >> %DIST_DIR%\README.txt
echo 1. Edit config\config.json with your database settings >> %DIST_DIR%\README.txt
echo 2. Run bin\PaperCrawlerServer.exe >> %DIST_DIR%\README.txt
echo 3. Open http://localhost:8080 in your browser >> %DIST_DIR%\README.txt

cd ..

echo.
echo ============================================
echo   Build Complete!
echo ============================================
echo.
echo Output: %DIST_DIR%\
echo.
echo Executable: build-static/bin/PaperCrawlerServer.exe
echo.
echo This is a STATIC build - all dependencies are included!
echo You can run this on any Windows machine without installing libraries.
echo.
echo ============================================
echo.

pause
