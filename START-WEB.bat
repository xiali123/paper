@echo off
echo ========================================
echo PaperCrawler - Web Version Launcher
echo ========================================
echo.
echo This will start the backend API and frontend web interface
echo (Qt Desktop application requires Qt6 installation - see QT-INSTALL-GUIDE.md)
echo.
echo Press any key to continue...
pause > nul

echo.
echo ========================================
echo Step 1: Starting Backend API
echo ========================================
echo.

cd backend
if not exist build mkdir build
cd build

echo Configuring backend...
cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo.
    echo Backend configuration failed. Please check:
    echo 1. Visual Studio 2019 or later is installed
    echo 2. vcpkg dependencies are available
    echo.
    pause
    goto error
)

echo Building backend...
cmake --build . --config Release
if errorlevel 1 (
    echo.
    echo Backend build failed. Please check the error messages above.
    echo.
    pause
    goto error
)

echo.
echo Backend will start on port 8080...
echo Starting backend in new window...
start "PaperCrawler Backend" cmd /k "Release\PaperCrawlerServer.exe"

echo.
echo Waiting for backend to start...
timeout /t 3 /nobreak > nul

echo.
echo ========================================
echo Step 2: Starting Frontend
echo ========================================
echo.

cd ..\..\frontend

if not exist node_modules (
    echo Installing frontend dependencies...
    call npm install
    if errorlevel 1 (
        echo.
        echo Failed to install frontend dependencies.
        echo Please ensure Node.js 20+ is installed.
        echo.
        pause
        goto error
    )
)

echo.
echo Starting frontend development server...
echo Frontend will be available at: http://localhost:5173
echo.
echo Press Ctrl+C to stop the frontend.
echo.

call npm run dev

goto end

:error
echo.
echo ========================================
echo Startup failed!
echo ========================================
echo.
echo Troubleshooting:
echo.
echo 1. Backend issues:
echo    - Ensure Visual Studio 2019+ is installed
echo    - Install vcpkg dependencies:
echo      vcpkg install curl openssl mysql
echo.
echo 2. Frontend issues:
echo    - Ensure Node.js 20+ is installed
echo    - Try: npm install
echo.
echo 3. For Qt Desktop application:
echo    - See QT-INSTALL-GUIDE.md
echo.
pause

:end
