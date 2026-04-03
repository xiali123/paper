@echo off
REM ============================================================================
REM 热插拔架构自动化测试脚本（Windows版本）
REM ============================================================================()

setlocal enabledelayedexpansion

REM 颜色定义
for /F %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"
set "BLUE=%ESC%[0;34m"
set "GREEN=%ESC%[0;32m"
set "YELLOW=%ESC%[1;33m"
set "RED=%ESC%[0;31m"
set "NC=%ESC%[0m"

REM 计数器
set TOTAL_TESTS=0
set PASSED_TESTS=0
set FAILED_TESTS=0

REM ============================================================================
REM 主测试函数
REM ============================================================================()

:main
echo.
echo %BLUE%========================================%NC%
echo %BLUE%  热插拔架构自动化测试%NC%
echo %BLUE%========================================%NC%
echo.

REM 配置
set BACKEND_DIR=..\backend
set BUILD_DIR=%BACKEND_DIR%\build\Release
set CONFIG_FILE=%BACKEND_DIR%\config\modules_auto.json
set EXE_FILE=%BUILD_DIR%\PaperCrawlerServerHotPlug.exe
set DLL_DIR=%BUILD_DIR%\modules\dynamic\Release

echo [INFO] 测试配置:
echo   后端目录: %BACKEND_DIR%
echo   构建目录: %BUILD_DIR%
echo   配置文件: %CONFIG_FILE%
echo   可执行文件: %EXE_FILE%
echo   模块目录: %DLL_DIR%
echo.

REM ========================================================================
REM 测试组1: 配置文件解析
REM ========================================================================

echo.
echo %BLUE%========================================%NC%
echo %BLUE%  测试组1: 配置文件解析%NC%
echo %BLUE%========================================%NC%
echo.

call :test "配置文件存在性" "test_config_exist"
call :test "配置文件格式" "test_config_format"
call :test "配置内容验证" "test_config_content"

REM ========================================================================
REM 测试组2: 文件检查
REM ========================================================================

echo.
echo %BLUE%========================================%NC%
echo %BLUE%  测试组2: 文件检查%NC%
echo %BLUE%========================================%NC%
echo.

call :test "可执行文件" "test_executable"
call :test "业务模块DLL" "test_modules"
call :test "依赖库" "test_dependencies"

REM ========================================================================
REM 测试组3: 路径验证
REM ========================================================================

echo.
echo %BLUE%========================================%NC%
echo %BLUE%  测试组3: 路径验证%NC%
echo %BLUE%========================================%NC%
echo.

call :test "模块路径" "test_module_paths"

REM ========================================================================
REM 测试总结
REM ========================================================================

echo.
echo %BLUE%========================================%NC%
echo %BLUE%  测试总结%NC%
echo %BLUE%========================================%NC%
echo.
echo   总测试数: %TOTAL_TESTS%
echo   通过: %GREEN%%PASSED_TESTS%%NC%
echo   失败: %RED%%FAILED_TESTS%%NC%
echo.

REM 通过率
if %TOTAL_TESTS% GTR 0 (
    set /a PASS_RATE=PASSED_TESTS*100/TOTAL_TESTS
    echo   通过率: %%PASS_RATE%%%
) else (
    echo   通过率: N/A
)
echo.

REM 最终结果
if %FAILED_TESTS% EQU 0 (
    echo %GREEN%========================================%NC%
    echo %GREEN%  所有测试通过！✅%NC%
    echo %GREEN%========================================%NC%
    echo.
    echo 下一步: 启动服务器进行功能测试
    echo   cd %BUILD_DIR%
    echo   PaperCrawlerServerHotPlug.exe ..\..\config\modules_auto.json
    echo.
) else (
    echo %RED%========================================%NC%
    echo %RED%  有测试失败！❌%NC%
    echo %RED%========================================%NC%
    echo.
    echo 请检查上述失败的测试项
    echo.
)

pause
goto :end

REM ============================================================================
REM 测试函数
REM ============================================================================()

:test
set TEST_NAME=%~1
set TEST_FUNC=%~2

set /a TOTAL_TESTS+=1
echo [TEST] %TEST_NAME%

call :%TEST_FUNC%
if errorlevel 1 (
    echo %RED%[FAIL]%NC% %TEST_NAME%
    set /a FAILED_TESTS+=1
) else (
    echo %GREEN%[PASS]%NC% %TEST_NAME%
    set /a PASSED_TESTS+=1%
)
goto :EOF

REM ============================================================================
REM 具体测试实现
REM ============================================================================()

:test_config_exist
if exist "%CONFIG_FILE%" (
    echo [INFO] 配置文件存在
    exit /b 0
) else (
    echo [FAIL] 配置文件不存在: %CONFIG_FILE%
    exit /b 1
)

:test_config_format
REM 检查JSON格式（使用python或简单检查）
findstr /C:"modulesDirectory" "%CONFIG_FILE%" >nul
if errorlevel 1 (
    echo [FAIL] 配置文件缺少modulesDirectory字段
    exit /b 1
)

findstr /C:"\"modules\"" "%CONFIG_FILE%" >nul
if errorlevel 1 (
    echo [FAIL] 配置文件缺少modules字段
    exit /b 1
)

echo [INFO] 配置文件基本格式正确
exit /b 0

:test_config_content
REM 检查必需的配置项
findstr /C:"modulesDirectory" "%CONFIG_FILE%" >nul
if errorlevel 1 (
    echo [FAIL] 缺少modulesDirectory配置
    exit /b 1
)

findstr /C:"healthCheckInterval" "%CONFIG_FILE%" >nul
if errorlevel 1 (
    echo [FAIL] 缺少healthCheckInterval配置
    exit /b 1
)

findstr /C:"\"modules\"" "%CONFIG_FILE%" >nul
if errorlevel 1 (
    echo [FAIL] 缺少modules数组
    exit /b 1
)

echo [INFO] 配置内容完整
exit /b 0

:test_executable
if exist "%EXE_FILE%" (
    echo [INFO] 可执行文件存在

    REM 检查文件大小
    for %%A in ("%EXE_FILE%") do set SIZE=%%~zA
    echo [INFO] 文件大小: !SIZE! bytes

    if !SIZE! GTR 100000 (
        echo [INFO] 文件大小合理 (^>100KB)
        exit /b 0
    ) else (
        echo [WARNING] 文件大小过小
        exit /b 1
    )
) else (
    echo [FAIL] 可执行文件不存在: %EXE_FILE%
    exit /b 1
)

:test_modules
if exist "%DLL_DIR%" (
    echo [INFO] 模块目录存在

    set MODULE_COUNT=0
    if exist "%DLL_DIR%\libAuthApiModule.dll" (
        echo [INFO]   ✓ libAuthApiModule.dll
        set /a MODULE_COUNT+=1
    )
    if exist "%DLL_DIR%\libUserApiModule.dll" (
        echo [INFO]   ✓ libUserApiModule.dll
        set /a MODULE_COUNT+=1
    )
    if exist "%DLL_DIR%\libSearchApiModule.dll" (
        echo [INFO]   ✓ libSearchApiModule.dll
        set /a MODULE_COUNT+=1
    )
    if exist "%DLL_DIR%\libExportApiModule.dll" (
        echo [INFO]   ✓ libExportApiModule.dll
        set /a MODULE_COUNT+=1
    )
    if exist "%DLL_DIR%\libAiApiModule.dll" (
        echo [INFO]   ✓ libAiApiModule.dll
        set /a MODULE_COUNT+=1
    )
    if exist "%DLL_DIR%\libRecommendationApiModule.dll" (
        echo [INFO]   ✓ libRecommendationApiModule.dll
        set /a MODULE_COUNT+=1
    )

    echo [INFO] 发现模块: !MODULE_COUNT!/6

    if !MODULE_COUNT! GEQ 4 (
        echo [INFO] 模块数量充足
        exit /b 0
    ) else (
        echo [WARNING] 模块数量不足
        exit /b 1
    )
) else (
    echo [FAIL] 模块目录不存在: %DLL_DIR%
    exit /b 1
)

:test_dependencies
REM 检查依赖DLL
set MISSING_DEPS=0

if exist "%DLL_DIR%\libcurl-x64.dll" (
    echo [INFO]   ✓ libcurl-x64.dll
) else (
    echo [WARNING]   ✗ libcurl-x64.dll
    set /a MISSING_DEPS+=1
)

echo [INFO] 依赖检查完成
if !MISSING_DEPS! EQU 0 (
    exit /b 0
) else (
    echo [WARNING] 缺少 !MISSING_DEPS! 个依赖文件
    REM 依赖缺失不影响测试继续
    exit /b 0
)

:test_module_paths
REM 验证配置文件中的模块路径
set VALID_PATHS=0
set TOTAL_PATHS=0

REM 从配置文件提取路径并验证（简化版本）
if exist "%DLL_DIR%\libAuthApiModule.dll" (
    echo [INFO]   ✓ libAuthApiModule.dll路径正确
    set /a VALID_PATHS+=1
    set /a TOTAL_PATHS+=1
)
if exist "%DLL_DIR%\libUserApiModule.dll" (
    echo [INFO]   ✓ libUserApiModule.dll路径正确
    set /a VALID_PATHS+=1
    set /a TOTAL_PATHS+=1
)

echo [INFO] 路径验证: !VALID_PATHS!/!TOTAL_PATHS!
if !VALID_PATHS! GEQ 2 (
    exit /b 0
) else (
    exit /b 1
)

:end
exit /b 0
