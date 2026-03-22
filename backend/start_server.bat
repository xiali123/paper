@echo off
REM PaperCrawler API Server Startup Script for Windows

echo ==========================================
echo   PaperCrawler API Server
echo ==========================================
echo.

REM Check if build directory exists
if not exist "build" (
    echo [WARNING] Build directory not found. Creating...
    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release
    cd ..
)

REM Check if executable exists
if not exist "build\PaperCrawlerServer.exe" (
    echo [WARNING] Executable not found. Building...
    cd build
    cmake --build . --config Release
    cd ..
)

REM Check if config exists
if not exist "config\config.json" (
    echo [ERROR] Configuration file not found!
    echo Please copy config.example.json to config\config.json
    echo and update it with your database credentials.
    pause
    exit /b 1
)

echo [INFO] Starting PaperCrawler API Server...
echo.
echo Server will be available at: http://localhost:8080
echo Press Ctrl+C to stop the server
echo.

REM Start the server
build\PaperCrawlerServer.exe
