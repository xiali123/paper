@echo off
REM ============================================================================
REM PaperCrawler 后端构建脚本（Windows版本）
REM ============================================================================()

setlocal enabledelayedexpansion

REM 配置
set BUILD_TYPE=Release
set BUILD_DIR=build
set CLEAN_BUILD=false
set PARALLEL_JOBS=%NUMBER_OF_PROCESSORS%

REM 颜色定义（Windows 10+）
for /F %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"
set "BLUE=%ESC%[0;34m"
set "GREEN=%ESC%[0;32m"
set "YELLOW=%ESC%[1;33m"
set "RED=%ESC%[0;31m"
set "NC=%ESC%[0m"

echo.
echo %BLUE%========================================%NC%
echo %BLUE%  PaperCrawler Backend Build Script%NC%
echo %BLUE%========================================%NC%
echo.

REM 显示配置
echo [INFO] Configuration:
echo   Build Type: %BUILD_TYPE%
echo   Build Directory: %BUILD_DIR%
echo   Parallel Jobs: %PARALLEL_JOBS%
echo.

REM 清理（如果需要）
if "%CLEAN_BUILD%"=="true" (
    echo [INFO] Cleaning build directory...
    if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
    echo [SUCCESS] Build directory cleaned
)

REM 创建构建目录
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM 配置CMake
echo [INFO] Configuring CMake...
cd %BUILD_DIR%
cmake .. -G "Visual Studio 17 2022" -A x64
cd ..
if errorlevel 1 (
    echo %RED%[ERROR]%NC% CMake configuration failed
    exit /b 1
)
echo [SUCCESS] CMake configured

REM 编译
echo [INFO] Building project...
cd %BUILD_DIR%
cmake --build . --config %BUILD_TYPE% --parallel %PARALLEL_JOBS%
cd ..
if errorlevel 1 (
    echo %RED%[ERROR]%NC% Build failed
    exit /b 1
)
echo [SUCCESS] Build completed

REM 复制配置文件
echo [INFO] Copying configuration files...
if not exist "%BUILD_DIR%\%BUILD_TYPE%\modules\config" mkdir "%BUILD_DIR%\%BUILD_TYPE%\modules\config"
if exist "config\modules_auto.json" (
    copy "config\modules_auto.json" "%BUILD_DIR%\%BUILD_TYPE%\modules\config\" >nul
    echo [SUCCESS] Configuration files copied
) else (
    echo %YELLOW%[WARNING]%NC% config\modules_auto.json not found
)

REM 显示结果
echo.
echo %GREEN%========================================%NC%
echo %GREEN%  Build Output Structure%NC%
echo %GREEN%========================================%NC%
echo.
echo %BUILD_DIR%\%BUILD_TYPE%\
echo ├── bin\
echo │   └── PaperCrawlerServerHotPlug.exe
echo ├── lib\
echo │   ├── core\
echo │   │   └── PaperCrawlerCore.lib
echo │   └── modules\
echo │       ├── libAuthApiModule.dll
echo │       ├── libUserApiModule.dll
echo │       └── ...
echo └── modules\
echo     └── config\
echo         └── modules.json
echo.
echo %GREEN%========================================%NC%
echo.
echo %GREEN%[SUCCESS] All done! 🚀%NC%
echo.

pause
