@echo off
REM ============================================================================
REM PaperCrawler Backend Server Startup Script
REM ============================================================================

echo.
echo ============================================================
echo PaperCrawler Backend Server
echo ============================================================
echo.

REM Change to backend directory
cd /d %~dp0backend

REM Check if database exists
if not exist "papercrawler_test.db" (
    echo [INFO] Test database not found. Creating...
    python create_test_db.py
    if errorlevel 1 (
        echo [ERROR] Failed to create test database
        pause
        exit /b 1
    )
)

REM Verify database schema
echo [INFO] Verifying database schema...
python verify_db_schema.py
if errorlevel 1 (
    echo [WARNING] Database schema verification had issues
    echo.
)

REM Check if build directory exists
if not exist "build\PaperCrawlerServer.exe" (
    echo [ERROR] Backend not compiled. Please run:
    echo   cd build
    echo   cmake .. -G "MinGW Makefiles"
    echo   make
    echo.
    pause
    exit /b 1
)

echo [INFO] Starting backend server on http://127.0.0.1:8080
echo [INFO] Press Ctrl+C to stop the server
echo.

REM Start the server
build\PaperCrawlerServer.exe

pause
