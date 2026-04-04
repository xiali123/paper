@echo off
REM PaperCrawler::Core Build Script for Windows
REM Automates the build process with various configurations

setlocal enabledelayedexpansion

REM Configuration
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release
if "%BUILD_DIR%"=="" set BUILD_DIR=build
if "%ENABLE_TESTS%"=="" set ENABLE_TESTS=ON
if "%ENABLE_EXAMPLES%"=="" set ENABLE_EXAMPLES=ON
if "%ENABLE_BENCHMARKS%"=="" set ENABLE_BENCHMARKS=OFF
if "%BUILD_DOCS%"=="" set BUILD_DOCS=OFF

REM Parse arguments
:parse_args
if "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if "%~1"=="--release" (
    set BUILD_TYPE=Release
    shift
    goto parse_args
)
if "%~1"=="--relwithdebinfo" (
    set BUILD_TYPE=RelWithDebInfo
    shift
    goto parse_args
)
if "%~1"=="--tests" (
    set ENABLE_TESTS=ON
    shift
    goto parse_args
)
if "%~1"=="--no-tests" (
    set ENABLE_TESTS=OFF
    shift
    goto parse_args
)
if "%~1"=="--examples" (
    set ENABLE_EXAMPLES=ON
    shift
    goto parse_args
)
if "%~1"=="--no-examples" (
    set ENABLE_EXAMPLES=OFF
    shift
    goto parse_args
)
if "%~1"=="--benchmarks" (
    set ENABLE_BENCHMARKS=ON
    shift
    goto parse_args
)
if "%~1"=="--docs" (
    set BUILD_DOCS=ON
    shift
    goto parse_args
)
if "%~1"=="--clean" (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    echo Build directory cleaned
    exit /b 0
)
if "%~1"=="--help" (
    echo Usage: %~nx0 [OPTIONS]
    echo.
    echo Options:
    echo   --debug           Build in Debug mode (default: Release)
    echo   --release         Build in Release mode
    echo   --relwithdebinfo  Build in RelWithDebInfo mode
    echo   --tests           Enable tests (default: ON)
    echo   --no-tests        Disable tests
    echo   --examples        Enable examples (default: ON)
    echo   --no-examples     Disable examples
    echo   --benchmarks      Enable benchmarks
    echo   --docs            Generate documentation
    echo   --clean           Clean build directory
    echo   --help            Show this help message
    echo.
    echo Environment variables:
    echo   BUILD_TYPE        Build type (Debug/Release/RelWithDebInfo)
    echo   BUILD_DIR         Build directory (default: build)
    exit /b 0
)

REM Print configuration
echo ========================================
echo PaperCrawler::Core Build Configuration
echo ========================================
echo Build Type:       %BUILD_TYPE%
echo Build Directory:  %BUILD_DIR%
echo Tests:            %ENABLE_TESTS%
echo Examples:         %ENABLE_EXAMPLES%
echo Benchmarks:       %ENABLE_BENCHMARKS%
echo Documentation:    %BUILD_DOCS%
echo.

REM Check dependencies
echo ========================================
echo Checking Dependencies
echo ========================================

where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found. Please install CMake.
    exit /b 1
)
echo [OK] CMake found

where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARNING] MSVC not found in PATH. Please run from Visual Studio Developer Command Prompt or Developer PowerShell for VS.
)

REM Configure
echo ========================================
echo Configuring CMake
echo ========================================

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

cmake -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_TESTS=%ENABLE_TESTS% -DBUILD_EXAMPLES=%ENABLE_EXAMPLES% -DENABLE_BENCHMARKS=%ENABLE_BENCHMARKS% -DBUILD_DOCS=%BUILD_DOCS% ..
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed
    exit /b 1
)

REM Build
echo ========================================
echo Building
echo ========================================

cmake --build . --config %BUILD_TYPE%
if %errorlevel% neq 0 (
    echo [ERROR] Build failed
    exit /b 1
)

REM Run tests if enabled
if "%ENABLE_TESTS%"=="ON" (
    echo ========================================
    echo Running Tests
    echo ========================================

    if exist "%BUILD_TYPE%\PaperCrawlerCoreTests.exe" (
        echo Running test suite...
        "%BUILD_TYPE%\PaperCrawlerCoreTests.exe" --gtest_color=yes
        if %errorlevel% neq 0 (
            echo [ERROR] Tests failed
            exit /b 1
        )
    ) else (
        echo [WARNING] Test executable not found
    )
)

REM Summary
echo ========================================
echo Build Complete!
echo ========================================

if "%ENABLE_TESTS%"=="ON" (
    echo [OK] All tests passed
)

echo.
echo PaperCrawler::Core built successfully!
echo.
echo Build artifacts:
echo   - Libraries: %BUILD_DIR%\%BUILD_TYPE%\
echo   - Examples:  %BUILD_DIR%\%BUILD_TYPE%\

if "%ENABLE_EXAMPLES%"=="ON" (
    echo.
    echo Available examples:
    if exist "%BUILD_TYPE%\MinimalModuleExample.exe" echo   - %BUILD_TYPE%\MinimalModuleExample.exe
    if exist "%BUILD_TYPE%\DependencyInjectionExample.exe" echo   - %BUILD_TYPE%\DependencyInjectionExample.exe
    if exist "%BUILD_TYPE%\EventDrivenExample.exe" echo   - %BUILD_TYPE%\EventDrivenExample.exe
    if exist "%BUILD_TYPE%\CompleteApplicationExample.exe" echo   - %BUILD_TYPE%\CompleteApplicationExample.exe
)

echo.
echo To install the library, run:
echo   - cmake --install .
echo.

endlocal
