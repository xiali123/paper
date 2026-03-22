@echo off
REM ========================================
REM PaperCrawler 分页功能测试
REM ========================================

setlocal enabledelayedexpansion

echo ========================================
echo   PaperCrawler 分页功能完整测试
echo ========================================
echo.

REM 检查后端服务器
echo [检查] 后端服务器文件...
if not exist "e:\PaperCrawler\backend\PaperCrawlerServer.exe" (
    echo.
    echo ❌ 错误: 后端服务器未找到
    echo.
    echo 请先构建后端:
    echo   方式1: 运行 e:\PaperCrawler\setup_wsl_backend.bat
    echo   方式2: 查看 e:\PaperCrawler\WSL_BUILD_GUIDE.md
    echo.
    pause
    exit /b 1
)
echo ✅ 后端服务器文件存在

echo.
echo [启动] 后端服务器...
start "PaperCrawler Backend" e:\PaperCrawler\backend\PaperCrawlerServer.exe

echo 等待服务器启动...
timeout /t 5 /nobreak >nul

echo.
echo ========================================
echo   测试分页API
echo ========================================
echo.

REM 测试1: 基本搜索
echo [测试1] 搜索'test'，第1页（3条）
echo ----------------------------------------
curl -s "http://localhost:8080/api/search?q=test&offset=0&limit=3" | head -c 500
echo.
echo.

REM 测试2: 第2页
echo [测试2] 搜索'test'，第2页（3条）
echo ----------------------------------------
curl -s "http://localhost:8080/api/search?q=test&offset=3&limit=3" | head -c 500
echo.
echo.

REM 测试3: 第3页
echo [测试3] 搜索'test'，第3页（3条）
echo ----------------------------------------
curl -s "http://localhost:8080/api/search?q=test&offset=6&limit=3" | head -c 500
echo.
echo.

REM 测试4: 大页面
echo [测试4] 搜索'AI'，第1页（20条）
echo ----------------------------------------
curl -s "http://localhost:8080/api/search?q=AI&offset=0&limit=20" | head -c 500
echo.
echo.

REM 测试5: 空搜索
echo [测试5] 空搜索（返回所有论文）
echo ----------------------------------------
curl -s "http://localhost:8080/api/search?q=&offset=0&limit=5" | head -c 500
echo.
echo.

echo ========================================
echo   验证结果
echo ========================================
echo.
echo 请检查上面的输出:
echo.
echo ✅ 预期结果:
echo    - 每次请求返回不同的论文（ID不同）
echo    - 标题字段是正常论文标题
echo    - 标题不包含URL参数（如 "&offset=0&limit=3"）
echo    - 返回数量正确（3条、20条、5条）
echo.
echo ❌ 如果看到错误:
echo    - 标题包含 "&offset=0&limit=3" 等URL参数
echo    - 所有页面返回相同论文
echo    - 返回数量不正确
echo.
echo    则说明后端未使用修复后的代码，需要重新构建！
echo.

echo ========================================
echo   测试桌面客户端集成
echo ========================================
echo.
echo 是否启动桌面客户端测试? (Y/N)
set /p choice=
if /i "!choice!"=="Y" (
    echo.
    echo 启动桌面客户端...
    start e:\PaperCrawler\desktop\build\PaperCrawlerDesktop.exe
    echo.
    echo 请测试:
    echo    1. 搜索功能
    echo    2. 分页按钮（下一页应该显示不同论文）
    echo    3. 每页显示数量（10/20/50/100）
    echo.
)

echo.
echo ========================================
echo   测试完成
echo ========================================
echo.
echo 要停止后端服务器，请在后端窗口按 Ctrl+C
echo.

pause
