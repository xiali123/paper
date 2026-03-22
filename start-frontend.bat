@echo off
REM ============================================================================
REM PaperCrawler Frontend Startup Script
REM ============================================================================

echo.
echo ============================================================
echo PaperCrawler Frontend Dev Server
echo ============================================================
echo.

REM Change to frontend directory
cd /d %~dp0frontend

REM Check if node_modules exists
if not exist "node_modules\" (
    echo [INFO] Installing dependencies...
    call npm install
    if errorlevel 1 (
        echo [ERROR] Failed to install dependencies
        pause
        exit /b 1
    )
)

echo [INFO] Starting frontend dev server on http://localhost:5173
echo [INFO] Press Ctrl+C to stop the server
echo.

REM Start the dev server
call npm run dev

pause
