@echo off
REM PaperCrawler Desktop Client Launcher
REM Author: Claude AI Agent
REM Date: 2026-03-22

echo ========================================
echo   PaperCrawler Desktop Client
echo ========================================
echo.

REM Check if build directory exists
if not exist "build" (
    echo [INFO] Build directory not found. Creating...
    mkdir build
)

cd build

REM Check if CMake cache exists
if not exist "CMakeCache.txt" (
    echo [INFO] Configuring project with CMake...
    cmake .. -G "MinGW Makefiles"
    if %errorlevel% neq 0 (
        echo [ERROR] CMake configuration failed!
        pause
        exit /b 1
    )
)

REM Build the project
echo [INFO] Building desktop client...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo [OK] Build completed successfully!
echo.
echo ========================================
echo   Starting Desktop Client...
echo ========================================
echo.

REM Run the executable
Release\PaperCrawlerDesktop.exe

pause
