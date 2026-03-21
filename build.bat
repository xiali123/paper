@echo off
REM PaperCrawler Build Script for Windows

echo ========================================
echo PaperCrawler Build Script
echo ========================================

REM Check if vcpkg is installed
if not exist "vcpkg" (
    echo vcpkg not found. Please install vcpkg first:
    echo git clone https://github.com/Microsoft/vcpkg.git
    echo cd vcpkg
    echo .\bootstrap-vcpkg.bat
    pause
    exit /b 1
)

REM Set vcpkg toolchain
set VCPKG_ROOT=%CD%\vcpkg
set CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

REM Create build directory
if not exist "build" mkdir build
cd build

REM Configure with CMake
echo Configuring project...
cmake -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=%CMAKE_TOOLCHAIN_FILE% ^
    -DCMAKE_BUILD_TYPE=Release ..
if errorlevel 1 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build project
echo Building project...
cmake --build . --config Release
if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo ========================================
echo Build completed successfully!
echo Executable: build\bin\Release\PaperCrawler.exe
echo ========================================
pause
