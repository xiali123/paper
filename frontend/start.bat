@echo off
echo ========================================
echo  PaperCrawler Frontend - Quick Start
echo ========================================
echo.

echo [1/3] Installing dependencies...
call npm install
if %errorlevel% neq 0 (
    echo ERROR: Failed to install dependencies
    pause
    exit /b 1
)

echo.
echo [2/3] Starting development server...
echo.
echo Frontend will be available at: http://localhost:5173
echo Backend API expected at: http://localhost:8080
echo.
echo Press Ctrl+C to stop the server
echo.

call npm run dev

pause
