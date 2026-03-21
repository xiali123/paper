@echo off
echo ========================================
echo PaperCrawler Platform Launcher
echo ========================================
echo.
echo Please choose an option:
echo.
echo 1. Start Backend API Server (Port 8080)
echo 2. Start Frontend Dev Server (Port 5173)
echo 3. Start Desktop Application
echo 4. Start All Services (Docker)
echo 5. Run Tests
echo 6. Exit
echo.

set /p choice="Enter your choice (1-6): "

if "%choice%"=="1" goto backend
if "%choice%"=="2" goto frontend
if "%choice%"=="3" goto desktop
if "%choice%"=="4" goto docker
if "%choice%"=="5" goto test
if "%choice%"=="6" goto end

:backend
echo.
echo Starting Backend API Server...
cd backend
if not exist build mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo CMake configuration failed!
    pause
    goto main
)
cmake --build . --config Release
if errorlevel 1 (
    echo Build failed!
    pause
    goto main
)
echo.
echo Backend server starting on port 8080...
echo Press Ctrl+C to stop
Release\PaperCrawlerServer.exe
goto end

:frontend
echo.
echo Starting Frontend Dev Server...
cd frontend
if not exist node_modules (
    echo Installing dependencies...
    call npm install
)
echo.
echo Frontend starting on http://localhost:5173
call npm run dev
goto end

:desktop
echo.
echo Starting Desktop Application...
cd desktop
if not exist build mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo CMake configuration failed!
    pause
    goto main
)
cmake --build . --config Release
if errorlevel 1 (
    echo Build failed!
    pause
    goto main
)
echo.
echo Starting Desktop Application...
Release\PaperCrawlerDesktop.exe
goto end

:docker
echo.
echo Starting All Services with Docker...
docker-compose up -d
echo.
echo Services started!
echo Web frontend: http://localhost
echo API backend: http://localhost:8080
echo.
echo To view logs: docker-compose logs -f
echo To stop: docker-compose down
goto end

:test
echo.
echo Running Tests...
call quick-test.bat
goto main

:main
goto :start

:end
echo.
echo Goodbye!
pause
