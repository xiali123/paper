@echo off
REM ============================================================================
REM PaperCrawler 完整系统启动脚本
REM 包含 Mock API + 前端
REM ============================================================================

echo.
echo ============================================================
echo PaperCrawler 系统启动
echo ============================================================
echo.

REM 检查 Node.js
where node >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [错误] Node.js 未安装
    echo 请先安装 Node.js: https://nodejs.org/
    pause
    exit /b 1
)

echo [1/3] 检查依赖...
cd /d %~dp0

if not exist "node_modules\express" (
    echo 安装 Express 和 CORS...
    call npm install express cors
)

echo.
echo [2/3] 启动 Mock API 服务器 (端口 8080)...
start "PaperCrawler Mock API" cmd /k "node mock-auth-api.js"

REM 等待 API 启动
timeout /t 3 /nobreak > nul

echo.
echo [3/3] 启动前端开发服务器 (端口 5173)...
cd frontend
start "PaperCrawler Frontend" cmd /k "npm run dev"

echo.
echo ============================================================
echo 系统启动完成！
echo ============================================================
echo.
echo 服务地址:
echo   - 前端:     http://localhost:5173
echo   - Mock API: http://127.0.0.1:8080
echo.
echo 测试账号:
echo   - 邮箱: test@example.com
echo   - 密码: TestPass123!
echo.
echo 按任意键关闭此窗口 (服务将继续运行)...
pause > nul
