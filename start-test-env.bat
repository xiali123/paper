@echo off
REM ============================================================================
REM PaperCrawler Test Environment Startup Script
REM Starts both backend and frontend servers in separate windows
REM ============================================================================

echo.
echo ============================================================
echo PaperCrawler Test Environment Launcher
echo ============================================================
echo.
echo This will launch:
echo   1. Backend Server  (http://127.0.0.1:8080)
echo   2. Frontend Server (http://localhost:5173)
echo.
echo Press any key to continue...
pause > /dev/null

REM Start backend in new window
echo [1/2] Starting backend server...
start "PaperCrawler Backend" cmd /k "cd /d %~dp0 && start-backend.bat"

REM Wait a bit for backend to start
timeout /t 3 /nobreak > /dev/null

REM Start frontend in new window
echo [2/2] Starting frontend server...
start "PaperCrawler Frontend" cmd /k "cd /d %~dp0 && start-frontend.bat"

echo.
echo ============================================================
echo Both servers are starting in separate windows...
echo ============================================================
echo.
echo Backend:  http://127.0.0.1:8080
echo Frontend: http://localhost:5173
echo.
echo Open your browser and navigate to:
echo   http://localhost:5173
echo.
echo Test User Credentials (after registration):
echo   Email: test@example.com
echo   Password: TestPass123!
echo.
echo To stop both servers, close their windows or press Ctrl+C in each.
echo.
pause
