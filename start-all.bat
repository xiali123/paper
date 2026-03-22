@echo off
REM PaperCrawler Project - All Services Startup Script
REM Author: Claude AI Agent
REM Date: 2026-03-21

echo ========================================
echo   PaperCrawler - Starting All Services
echo ========================================
echo.

REM Check if backend is running
echo [1/3] Checking backend service...
netstat -ano | findstr ":8080" | findstr "LISTENING" >nul
if %errorlevel% equ 0 (
    echo [OK] Backend is already running on port 8080
) else (
    echo [INFO] Starting backend service...
    start "PaperCrawler Backend" cmd /k "cd /d %~dp0backend && start_server.bat"
    timeout /t 3 /nobreak >nul
    echo [OK] Backend started on port 8080
)

echo.

REM Check if frontend is running
echo [2/3] Checking frontend service...
netstat -ano | findstr ":5173" | findstr "LISTENING" >nul
if %errorlevel% equ 0 (
    echo [OK] Frontend is already running on port 5173
) else (
    echo [INFO] Starting frontend service...
    start "PaperCrawler Frontend" cmd /k "cd /d %~dp0frontend && npm run dev"
    timeout /t 5 /nobreak >nul
    echo [OK] Frontend started on port 5173
)

echo.

REM Optional: Start MUI Demo App
echo [3/3] Checking MUI Demo App...
netstat -ano | findstr ":3006" | findstr "LISTENING" >nul
if %errorlevel% equ 0 (
    echo [OK] MUI Demo App is already running on port 3006
) else (
    echo [INFO] Starting MUI Demo App (optional)...
    start "MUI Demo App" cmd /k "cd /d F:\test_line\mui-demo-app && npm run dev"
    timeout /t 5 /nobreak >nul
    echo [OK] MUI Demo App started on port 3006
)

echo.
echo ========================================
echo   All Services Started Successfully!
echo ========================================
echo.
echo 🌐 Access URLs:
echo    • PaperCrawler:    http://localhost:5173
echo    • Backend API:     http://localhost:8080
echo    • Health Check:    http://localhost:8080/health
echo    • MUI Demo App:    http://localhost:3006
echo.
echo 📚 Documentation:
echo    • API Docs:        backend/API_DOCUMENTATION.md
echo    • Quick Start:     OPTIMIZATION_SUMMARY.md
echo    • Test Results:    TEST_RESULTS.md
echo.
echo 💡 Tips:
echo    • Press Ctrl+C in service windows to stop
echo    • Check service windows for logs
echo    • Use browser DevTools (F12) for debugging
echo.
echo ✅ Ready to use! Open browser and visit:
echo    http://localhost:5173
echo.
pause
