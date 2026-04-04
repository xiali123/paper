@echo off
REM PaperCrawler Frontend Development Startup Script

echo ============================================
echo Starting PaperCrawler Development Environment
echo ============================================
echo.

REM Start CORS Proxy Server
echo [1/2] Starting CORS Proxy Server (port 3008)...
start "PaperCrawler-Proxy" cmd /k "cd /d %~dp0 && node proxy-server.cjs"
timeout /t 2 /nobreak >nul

REM Start Vite Dev Server
echo [2/2] Starting Vite Dev Server (port 3000+)...
start "PaperCrawler-Frontend" cmd /k "cd /d %~dp0 && npm run dev"

echo.
echo ============================================
echo Development Environment Started!
echo ============================================
echo.
echo Frontend: http://localhost:3007
echo Proxy:    http://localhost:3008 (to backend:8080)
echo Backend:  http://localhost:8080
echo.
echo Press any key to open browser...
pause >nul
start http://localhost:3007
echo.
echo Browser opened. Happy coding!
