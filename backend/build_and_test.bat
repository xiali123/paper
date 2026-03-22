@echo off
REM ========================================
REM PaperCrawler Backend - 编译和测试脚本
REM ========================================

echo ========================================
echo   编译和测试修复的WebSocket服务器
echo ========================================
echo.

cd /d e:\PaperCrawler\backend

if not exist build mkdir build
cd build

echo [1/4] 配置CMake...
echo ----------------------------------------
cmake .. -G "MinGW Makefiles" ^
  -DCMAKE_PREFIX_PATH="C:/Qt/Tools/mingw1310_64" ^
  -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo.
    echo ❌ CMake配置失败！
    echo.
    pause
    exit /b 1
)

echo.
echo [2/4] 编译项目...
echo ----------------------------------------
mingw32-make -j4 2>&1 | tee compile.log

if errorlevel 1 (
    echo.
    echo ❌ 编译失败！查看 compile.log 获取详情
    echo.
    pause
    exit /b 1
)

echo.
echo [3/4] 检查可执行文件...
echo ----------------------------------------
if exist "PaperCrawlerServer.exe" (
    echo ✅ 编译成功！
    dir PaperCrawlerServer.exe
) else (
    echo ❌ 可执行文件未生成
    pause
    exit /b 1
)

echo.
echo [4/4] 快速功能测试...
echo ----------------------------------------

echo 启动服务器进行测试...
start "PaperCrawler Server" PaperCrawlerServer.exe

echo 等待服务器启动...
timeout /t 3 /nobreak >nul

echo.
echo ========================================
echo   测试API
echo ========================================

echo 测试1: 健康检查
curl -s http://localhost:8080/health
echo.

echo 测试2: 搜索API
curl -s "http://localhost:8080/api/search?q=test&offset=0&limit=5"
echo.

echo 测试3: 统计API
curl -s http://localhost:8080/api/stats
echo.

echo ========================================
echo   WebSocket连接测试
echo ========================================

echo 注意: WebSocket测试需要专门的客户端
echo 可以使用websocat或其他WebSocket工具
echo.
echo 测试命令示例:
echo   websocat ws://localhost:8088/ws
echo.

echo ========================================
echo   测试完成
echo ========================================
echo.
echo 如果所有测试通过，WebSocket修复成功！
echo.
echo 要停止服务器，在后端窗口按 Ctrl+C
echo.

pause
