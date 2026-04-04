@echo off
REM ============================================================================
REM 测试新的构建系统
REM ============================================================================()

setlocal enabledelayedexpansion

echo.
echo ================================================
echo   测试新的构建系统
echo ================================================
echo.

REM 检查文件是否存在
echo [1/5] 检查配置文件...
if exist "backend\cmake\OutputDirs.cmake" (
    echo [OK] cmake/OutputDirs.cmake
) else (
    echo [FAIL] cmake/OutputDirs.cmake not found
    goto :error
)

if exist "backend\cmake\Dependencies.cmake" (
    echo [OK] cmake/Dependencies.cmake
) else (
    echo [FAIL] cmake/Dependencies.cmake not found
    goto :error
)

if exist "backend\cmake\CompilerOptions.cmake" (
    echo [OK] cmake/CompilerOptions.cmake
) else (
    echo [FAIL] cmake/CompilerOptions.cmake not found
    goto :error
)

if exist "backend\modules-cmake\CMakeLists.txt" (
    echo [OK] modules-cmake/CMakeLists.txt
) else (
    echo [FAIL] modules-cmake/CMakeLists.txt not found
    goto :error
)

if exist "backend\scripts\build.bat" (
    echo [OK] scripts/build.bat
) else (
    echo [FAIL] scripts/build.bat not found
    goto :error
)

echo.
echo [2/5] 检查CMakeLists.txt结构...
findstr /C:"include(cmake" backend\CMakeLists.txt >nul
if errorlevel 1 (
    echo [FAIL] CMakeLists.txt does not include cmake modules
    goto :error
)
echo [OK] CMakeLists.txt includes cmake modules

echo.
echo [3/5] 验证输出目录配置...
findstr /C:"CORE_LIB_DIR" backend\cmake\OutputDirs.cmake >nul
if errorlevel 1 (
    echo [FAIL] OutputDirs.cmake missing CORE_LIB_DIR
    goto :error
)
findstr /C:"MODULE_LIB_DIR" backend\cmake\OutputDirs.cmake >nul
if errorlevel 1 (
    echo [FAIL] OutputDirs.cmake missing MODULE_LIB_DIR
    goto :error
)
findstr /C:"EXECUTABLE_OUTPUT_DIR" backend\cmake\OutputDirs.cmake >nul
if errorlevel 1 (
    echo [FAIL] OutputDirs.cmake missing EXECUTABLE_OUTPUT_DIR
    goto :error
)
echo [OK] Output directories configured

echo.
echo [4/5] 验证模块构建配置...
findstr /C:"build_business_module" backend\modules-cmake\CMakeLists.txt >nul
if errorlevel 1 (
    echo [FAIL] modules-cmake missing build_business_module function
    goto :error
)
echo [OK] Module build function defined

echo.
echo [5/5] 显示预期构建结构...
echo.
echo Expected build structure:
echo.
echo build\Release\
echo ├── bin\               # 可执行文件
echo │   └── PaperCrawlerServerHotPlug.exe
echo │
echo ├── lib\               # 库文件
echo │   ├── core\          # 核心库
echo │   ├── modules\       # 业务模块
echo │   │   ├── libAuthApiModule.dll
echo │   │   ├── libUserApiModule.dll
echo │   │   └── ...
echo │   ├── data\          # 数据层
echo │   └── network\       # 网络层
echo │
echo └── modules\           # 模块配置
echo     └── config\
echo         └── modules_auto.json
echo.

echo ================================================
echo   所有检查通过！✅
echo ================================================
echo.
echo 下一步:
echo 1. 运行构建脚本: cd backend ^&^& scripts\build.bat
echo 2. 或手动构建: mkdir build ^&^& cd build ^&^& cmake .. ^&^& cmake --build . --config Release
echo.

goto :end

:error
echo.
echo ================================================
echo   检查失败！❌
echo ================================================
echo.
echo 请检查上述错误信息。
echo.
exit /b 1

:end
pause
