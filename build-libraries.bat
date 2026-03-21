@echo off
REM ============================================
REM PaperCrawler Library Build Script (Windows)
REM ============================================
REM Builds both static and shared libraries
REM and creates a complete distribution
REM ============================================

setlocal enabledelayedexpansion

set PROJECT_ROOT=%~dp0
set BUILD_DIR=%PROJECT_ROOT%build
set DIST_DIR=%PROJECT_ROOT%dist

echo.
echo ============================================
echo   PaperCrawler Library Build Script
echo ============================================
echo.

REM Parse command line arguments
set BUILD_TYPE=Release
set BUILD_STATIC=ON
set BUILD_SHARED=ON
set BUILD_DESKTOP=ON

:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if /i "%~1"=="--static-only" (
    set BUILD_SHARED=OFF
    shift
    goto parse_args
)
if /i "%~1"=="--shared-only" (
    set BUILD_STATIC=OFF
    shift
    goto parse_args
)
if /i "%~1"=="--no-desktop" (
    set BUILD_DESKTOP=OFF
    shift
    goto parse_args
)
if /i "%~1"=="--help" (
    echo Usage: %0 [OPTIONS]
    echo.
    echo Options:
    echo   --debug         Build debug version (default: release)
    echo   --static-only   Build only static libraries
    echo   --shared-only   Build only shared libraries
    echo   --no-desktop    Don't build desktop application
    echo   --help          Show this help message
    exit /b 0
)
echo Unknown option: %~1
echo Use --help for usage information
exit /b 1

:end_parse

echo Build Configuration:
echo   Build Type: %BUILD_TYPE%
echo   Static Libs: %BUILD_STATIC%
echo   Shared Libs: %BUILD_SHARED%
echo   Desktop App: %BUILD_DESKTOP%
echo.

REM Clean build directory
if exist "%BUILD_DIR%" (
    echo Cleaning build directory...
    rmdir /s /q "%BUILD_DIR%"
)

REM Create build directory
mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

echo.
echo [1/4] Configuring CMake...
echo.

REM Configure CMake
cmake .. ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_STATIC=%BUILD_STATIC% ^
    -DBUILD_SHARED=%BUILD_SHARED% ^
    -DBUILD_DESKTOP=%BUILD_DESKTOP%

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed!
    cd "%PROJECT_ROOT%"
    exit /b 1
)

echo.
echo [2/4] Building project...
echo.

REM Build the project
cmake --build . --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    cd "%PROJECT_ROOT%"
    exit /b 1
)

echo.
echo [3/4] Running tests (if enabled)...
echo.

REM Run tests if they exist
if exist "bin\test_paper_api.exe" (
    ctest --output-on-failure || true
)

echo.
echo [4/4] Creating distribution packages...
echo.

REM Create distribution directory
set TIMESTAMP=%date:~10,4%%date:~4,2%%date:~7,2%_%time:~0,2%%time:~3,2%%time:~6,2%
set TIMESTAMP=%TIMESTAMP: =0%
set DIST_BASE=%DIST_DIR%\%TIMESTAMP%
mkdir "%DIST_BASE%"

REM Package static libraries
if "%BUILD_STATIC%"=="ON" (
    echo Packaging static libraries...
    set STATIC_DIR=%DIST_BASE%\static
    mkdir "%STATIC_DIR%\lib"
    mkdir "%STATIC_DIR%\include"
    mkdir "%STATIC_DIR%\bin"
    mkdir "%STATIC_DIR%\config"
    mkdir "%STATIC_DIR%\sql"
    mkdir "%STATIC_DIR%\docs"

    REM Copy static libraries
    for %%f in (lib\*.a) do copy "%%f" "%STATIC_DIR%\lib\" >nul

    REM Copy headers
    xcopy /E /I /Y "%PROJECT_ROOT%\include" "%STATIC_DIR%\include\" >nul

    REM Copy executables
    for %%f in (bin\*.exe) do copy "%%f" "%STATIC_DIR%\bin\" >nul

    REM Copy config and docs
    copy "%PROJECT_ROOT%\config\*.json" "%STATIC_DIR%\config\" >nul 2>&1
    copy "%PROJECT_ROOT%\sql\*.sql" "%STATIC_DIR%\sql\" >nul 2>&1
    copy "%PROJECT_ROOT%\*.md" "%STATIC_DIR%\docs\" >nul 2>&1

    REM Create README
    echo PaperCrawler Static Libraries Distribution > "%STATIC_DIR%\README.txt"
    echo Build Date: %date% %time% >> "%STATIC_DIR%\README.txt"
    echo Build Type: %BUILD_TYPE% >> "%STATIC_DIR%\README.txt"
    echo. >> "%STATIC_DIR%\README.txt"
    echo This package contains: >> "%STATIC_DIR%\README.txt"
    echo - Static libraries (.lib files) >> "%STATIC_DIR%\README.txt"
    echo - Header files >> "%STATIC_DIR%\README.txt"
    echo - Executables (statically linked) >> "%STATIC_DIR%\README.txt"
    echo - Configuration files >> "%STATIC_DIR%\README.txt"
    echo - SQL scripts >> "%STATIC_DIR%\README.txt"
    echo. >> "%STATIC_DIR%\README.txt"
    echo To use these libraries: >> "%STATIC_DIR%\README.txt"
    echo 1. Include headers in your project >> "%STATIC_DIR%\README.txt"
    echo 2. Link against the .lib files >> "%STATIC_DIR%\README.txt"
    echo 3. No runtime DLL dependencies required! >> "%STATIC_DIR%\README.txt"
)

REM Package shared libraries
if "%BUILD_SHARED%"=="ON" (
    echo Packaging shared libraries...
    set SHARED_DIR=%DIST_BASE%\shared
    mkdir "%SHARED_DIR%\lib"
    mkdir "%SHARED_DIR%\include"
    mkdir "%SHARED_DIR%\bin"
    mkdir "%SHARED_DIR%\config"
    mkdir "%SHARED_DIR%\sql"
    mkdir "%SHARED_DIR%\docs"

    REM Copy shared libraries
    for %%f in (lib\*.dll) do copy "%%f" "%SHARED_DIR%\lib\" >nul
    for %%f in (lib\*.lib) do copy "%%f" "%SHARED_DIR%\lib\" >nul

    REM Copy headers and executables
    xcopy /E /I /Y "%PROJECT_ROOT%\include" "%SHARED_DIR%\include\" >nul
    for %%f in (bin\*.exe) do copy "%%f" "%SHARED_DIR%\bin\" >nul

    REM Copy config and docs
    copy "%PROJECT_ROOT%\config\*.json" "%SHARED_DIR%\config\" >nul 2>&1
    copy "%PROJECT_ROOT%\sql\*.sql" "%SHARED_DIR%\sql\" >nul 2>&1
    copy "%PROJECT_ROOT%\*.md" "%SHARED_DIR%\docs\" >nul 2>&1

    REM Create README
    echo PaperCrawler Shared Libraries Distribution > "%SHARED_DIR%\README.txt"
    echo Build Date: %date% %time% >> "%SHARED_DIR%\README.txt"
    echo Build Type: %BUILD_TYPE% >> "%SHARED_DIR%\README.txt"
    echo. >> "%SHARED_DIR%\README.txt"
    echo This package contains: >> "%SHARED_DIR%\README.txt"
    echo - Shared libraries (.dll + .lib files) >> "%SHARED_DIR%\README.txt"
    echo - Header files >> "%SHARED_DIR%\README.txt"
    echo - Executables >> "%SHARED_DIR%\README.txt"
    echo - Configuration files >> "%SHARED_DIR%\README.txt"
    echo - SQL scripts >> "%SHARED_DIR%\README.txt"
    echo. >> "%SHARED_DIR%\README.txt"
    echo To use these libraries: >> "%SHARED_DIR%\README.txt"
    echo 1. Include headers in your project >> "%SHARED_DIR%\README.txt"
    echo 2. Link against the .lib files >> "%SHARED_DIR%\README.txt"
    echo 3. Distribute .dll files with your executable >> "%SHARED_DIR%\README.txt"
)

REM Create latest symlink (Windows junction)
if exist "%DIST_DIR%\latest" rmdir "%DIST_DIR%\latest"
mklink /J "%DIST_DIR%\latest" "%DIST_BASE%" >nul 2>&1

cd "%PROJECT_ROOT%"

echo.
echo ============================================
echo   Build Complete!
echo ============================================
echo.
echo Build artifacts:
dir /b "%BUILD_DIR%\bin\" 2>nul
echo.
echo Library files:
dir /b "%BUILD_DIR%\lib\" 2>nul
echo.
echo Distribution packages created in:
echo   %DIST_BASE%
echo   %DIST_DIR%\latest -> %TIMESTAMP%
echo.
echo Static build: %BUILD_STATIC%
echo Shared build: %BUILD_SHARED%
echo.
echo ============================================
echo.

pause
