@echo off
REM ========================================
REM PaperCrawler Backend - Apply Pagination Fix
REM ========================================
REM
REM This script will rebuild the backend with the pagination fix
REM
REM Prerequisites:
REM 1. Visual Studio 2022 with C++ development tools
REM 2. CMake 3.15+
REM 3. Git (for dependency fetching)
REM 4. Internet connection (for first-time dependency download)
REM
REM ========================================

echo ========================================
echo PaperCrawler Backend - Pagination Fix
echo ========================================
echo.

REM Check if we're in the right directory
if not exist "core\src\core\PaperCrawlerAPI.cpp" (
    echo ERROR: Please run this script from the PaperCrawler root directory
    echo Current directory: %CD%
    pause
    exit /b 1
)

echo [1/6] Stopping running backend server...
taskkill /F /IM PaperCrawlerServer.exe 2>nul
timeout /t 2 /nobreak >nul

echo [2/6] Cleaning previous build...
if exist "core\build" (
    rmdir /s /q core\build
)
if exist "backend\build" (
    rmdir /s /q backend\build
)

echo [3/6] Configuring core library (this may take a while on first run)...
cd core
mkdir build
cd build

REM Configure with Visual Studio
cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed!
    echo.
    echo Common issues:
    echo 1. Visual Studio not installed
    echo 2. Network issues downloading dependencies
    echo 3. Missing C++ tools
    echo.
    echo Try running: "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    pause
    exit /b 1
)

echo [4/6] Building core library with pagination fix...
cmake --build . --config Release
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    echo.
    echo Check the error messages above for details.
    pause
    exit /b 1
)

cd ..\..\..

echo [5/6] Building backend server...
cd backend
if not exist build mkdir build
cd build

cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo ERROR: Backend configuration failed!
    pause
    exit /b 1
)

cmake --build . --config Release
if errorlevel 1 (
    echo ERROR: Backend build failed!
    pause
    exit /b 1
)

cd ..\..

echo [6/6] Testing the fix...
echo Starting backend server...
start /B backend\build\Release\PaperCrawlerServer.exe
timeout /t 5 /nobreak >nul

echo Testing pagination...
curl -s "http://localhost:8080/api/search?q=test&offset=0&limit=3" | findstr /C:"title"
echo.
echo Should return papers with title containing "test" (not the full URL)
echo.

echo ========================================
echo Fix applied successfully!
echo ========================================
echo.
echo The backend server is now running with the pagination fix.
echo You can test it with:
echo   curl "http://localhost:8080/api/search?q=your_keyword&offset=0&limit=10"
echo.
echo To stop the server, press Ctrl+C in the server window.
echo.
pause
