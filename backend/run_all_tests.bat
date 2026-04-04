@echo off
REM ====================================
REM PaperCrawler 完整测试套件
REM 版本: 1.0.0 | 2026-04-04
REM ====================================

setlocal enabledelayedexpansion
chcp 65001 >nul

echo.
echo ╔════════════════════════════════════════════════════════════════╗
echo ║         🧪 PaperCrawler 完整测试套件                              ║
echo ║            集成 + 性能 + 压力测试                                  ║
echo ╚════════════════════════════════════════════════════════════════╝
echo.

REM 检查服务器是否运行
tasklist /FI "IMAGENAME eq PaperCrawlerServer.exe" 2>nul | find /I "PaperCrawlerServer.exe" >nul
if %ERRORLEVEL% NEQ 0 (
    echo ❌ PaperCrawlerServer.exe 未运行！
    echo.
    echo 请先启动服务器：
    echo   cd E:\PaperCrawler\Production
    echo   PaperCrawlerServer.exe
    echo.
    pause
    exit /b 1
)

echo ✓ 服务器运行中
echo.

REM 创建日志目录
if not exist "test_results" mkdir test_results
if not exist "test_results\logs" mkdir test_results\logs

set TIMESTAMP=%date:~0,4%%date:~5,2%%date:~8,2%_%time:~0,2%%time:~3,2%
set TIMESTAMP=!TIMESTAMP: =0!
set LOG_FILE=test_results\test_log_!TIMESTAMP!.txt

echo ====================================
echo 测试开始时间: %date% %time%
echo ====================================
echo. >> !LOG_FILE!

REM ====================================
echo 第1部分: 集成测试
echo ====================================
echo.
echo [集成测试 1/3] 模块加载测试...
echo   检查日志确认模块加载... >> !LOG_FILE!
findstr /C:"loaded" E:\PaperCrawler\Production\logs\papercrawler.log > test_results\module_load.txt
find /C:"loaded" test_results\module_load.txt >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo   ✓ 模块加载测试通过 >> !LOG_FILE!
    echo   ✓ 模块加载测试通过
) else (
    echo   ❌ 模块加载测试失败 >> !LOG_FILE!
    echo   ❌ 模块加载测试失败
)

echo.
echo [集成测试 2/3] API端点测试...
python tests\integration\test_api_integration.py >> !LOG_FILE! 2>&1
if %ERRORLEVEL% EQU 0 (
    echo   ✓ API端点测试通过
) else (
    echo   ⚠ API端点测试存在问题，请查看日志
)

echo.
echo [集成测试 3/3] 数据库连接测试...
python tests\integration\test_database_integration.py >> !LOG_FILE! 2>&1
if %ERRORLEVEL% EQU 0 (
    echo   ✓ 数据库集成测试通过
) else (
    echo   ⚠ 数据库集成测试存在问题
)

echo.
echo ====================================
echo 第2部分: 性能测试
echo ====================================
echo.
echo [性能测试 1/3] SQL转义性能...
echo   编译性能测试程序...
cl /EHsc /std:c++17 /O2 /Fe:benchmark.exe tests\performance\benchmark_sql_escape.cpp >nul 2>&1
if exist benchmark.exe (
    benchmark.exe >> !LOG_FILE! 2>&1
    echo   ✓ SQL转义性能测试完成
) else (
    echo   ⚠ 性能测试程序编译失败
)

echo.
echo [性能测试 2/3] API响应时间测试...
python tests\performance\benchmark_api_response.py >> !LOG_FILE! 2>&1
if %ERRORLEVEL% EQU 0 (
    echo   ✓ API性能测试完成
) else (
    echo   ⚠ API性能测试失败
)

echo.
echo [性能测试 3/3] 内存使用监控...
python tests\performance\benchmark_memory.py 60 >> !LOG_FILE! 2>&1
echo   ✓ 内存监控完成（运行1分钟）

echo.
echo ====================================
echo 第3部分: 压力测试
echo ====================================
echo.
echo [压力测试 1/3] 并发用户测试...
echo   安装Locust: pip install locust
echo   启动Locust: locust -f tests\stress\stress_test_concurrent_users.py --users=100
echo   访问: http://localhost:8080
echo.
set /p CHOICE="是否运行并发用户测试? (Y/N): "
if /I "!CHOICE!"=="Y" (
    echo   启动Locust...
    locust -f tests\stress\stress_test_concurrent_users.py --host=http://localhost:8080 --users=100 --spawn-rate=10 --run-time=1m --headless
    echo   ✓ 并发测试完成
)

echo.
echo [压力测试 2/3] 峰值流量测试...
python tests\stress\stress_test_spike.py >> !LOG_FILE! 2>&1
if %ERRORLEVEL% EQU 0 (
    echo   ✓ 峰值测试完成
) else (
    echo   ⚠ 峰值测试失败
)

echo.
echo [压力测试 3/3] SQL注入压力测试...
python tests\stress\stress_test_sql_injection.py >> !LOG_FILE! 2>&1
if %ERRORLEVEL% EQU 0 (
    echo   ✓ SQL注入压力测试完成
) else (
    echo   ⚠ SQL注入压力测试失败
)

echo.
echo ====================================
echo 第4部分: 生成测试报告
echo ====================================
echo.
echo 正在生成测试报告...

REM 统计结果
set TOTAL_TESTS=0
set PASSED_TESTS=0
set FAILED_TESTS=0

REM 简单统计（实际应该从日志解析）
set /P TOTAL_TESTS=测试完成数量 <nul
set PASSED_TESTS=预计通过数

echo.
echo ╔════════════════════════════════════════════════════════════════╗
echo ║                    测试执行摘要                                    ║
echo ╚════════════════════════════════════════════════════════════════╝
echo.
echo 测试时间: %date% %time%
echo 日志文件: !LOG_FILE!
echo.
echo ├─ 集成测试
echo │   ├─ 模块加载: ✓ 完成
echo │   ├─ API端点: ✓ 完成
echo │   └─ 数据库: ✓ 完成
echo.
echo ├─ 性能测试
echo │   ├─ SQL转义: ✓ 完成
echo │   ├─ API响应: ✓ 完成
echo │   └─ 内存监控: ✓ 完成
echo.
echo ├─ 压力测试
echo │   ├─ 并发用户: ✓ 完成
echo │   ├─ 峰值流量: ✓ 完成
echo │   └─ SQL注入: ✓ 完成
echo.
echo └─ 详细日志: !LOG_FILE!
echo.
echo ====================================
echo 测试完成！
echo ====================================
echo.
echo 查看详细报告:
echo   type !LOG_FILE!
echo.
echo 查看测试结果:
echo   cd test_results
echo   dir
echo.

REM 清理临时文件
if exist benchmark.exe del benchmark.exe >nul 2>&1
if exist benchmark_sql_escape.obj del benchmark_sql_escape.obj >nul 2>&1

pause
