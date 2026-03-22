@echo off
REM Statistics Page Debug Script for Windows
REM This script helps diagnose and fix the statistics page loading issue

echo === PaperCrawler Statistics Page Debug Script ===
echo.

REM Check if backend is running
echo 1. Checking if backend server is running on port 8080...
curl -s http://localhost:8080/api/stats/overview >nul 2>&1
if %errorlevel% equ 0 (
    echo ✅ Backend server is running
    echo Response:
    curl -s http://localhost:8080/api/stats/overview
    echo.
) else (
    echo ❌ Backend server is NOT running on port 8080
    echo Please start the backend server first
    pause
    exit /b 1
)

echo.

REM Check if frontend dev server is running
echo 2. Checking if frontend dev server is running on port 5173...
curl -s http://localhost:5173 >nul 2>&1
if %errorlevel% equ 0 (
    echo ✅ Frontend dev server is running
) else (
    echo ❌ Frontend dev server is NOT running on port 5173
    echo Please start the frontend dev server: cd frontend && npm run dev
    pause
    exit /b 1
)

echo.

REM Test proxy connection
echo 3. Testing proxy connection through frontend...
curl -s http://localhost:5173/api/stats/overview
if %errorlevel% equ 0 (
    echo ✅ Proxy connection successful
) else (
    echo ❌ Proxy connection failed
)

echo.

REM Open diagnostic tool
echo 4. Opening diagnostic tool in browser...
start http://localhost:5173/diagnostic.html

echo.
echo === Debug Steps ===
echo 1. The diagnostic tool should open in your browser
echo 2. Run all tests to identify any issues
echo 3. Open the Stats page in your app: http://localhost:5173/#/stats
echo 4. Open browser DevTools (F12) and check the Console tab
echo 5. Look for log messages with emoji prefixes:
echo    🔄 = Fetching data
echo    ✅ = Success
echo    ❌ = Error
echo    📊 = Data state
echo.
echo 6. If you see Vue DevTools, inspect the Stats component state:
echo    - stats.loading should be: false
echo    - stats.hasData should be: true
echo    - stats.overview should contain data
echo    - stats.error should be: null
echo.
echo For detailed instructions, see: frontend\STATS_DEBUG_GUIDE.md
echo.
pause
