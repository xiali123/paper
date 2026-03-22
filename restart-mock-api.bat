@echo off
setlocal enabledelayedexpansion

REM ========================================
REM PaperCrawler Mock API 重启脚本
REM ========================================

echo.
echo ========================================
echo  重启 Mock API 服务器
echo ========================================
echo.

REM 检查是否有 node 进程在运行 8082 端口
echo [1/4] 检查端口 8082...
netstat -ano | findstr ":8082" | findstr "LISTENING" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ✓ 端口 8082 被占用，尝试释放...
    for /f "tokens=5" %%a in ('netstat -ano ^| findstr ":8082" ^| findstr "LISTENING"') do (
        echo   终止进程 PID: %%a
        taskkill /F /PID %%a >nul 2>&1
    )
    timeout /t 2 /nobreak >nul
    echo ✓ 端口已释放
) else (
    echo ✓ 端口 8082 空闲
)
echo.

REM 启动 Mock API 服务器
echo [2/4] 启动 Mock API 服务器...
echo.
echo ✅ Mock API 服务器正在启动...
echo 📡 监听地址: http://localhost:8082
echo.
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo.
echo 按 Ctrl+C 可以停止服务器
echo.
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo.

REM 启动服务器并保持窗口打开
start "PaperCrawler Mock API" cmd /k "echo PaperCrawler Mock API Server && echo ======================== && echo. && echo Starting server on http://localhost:8082 && echo. && node complete-mock-api.js"

REM 等待服务器启动
echo [3/4] 等待服务器启动...
timeout /t 3 /nobreak >nul

REM 测试连接
echo [4/4] 测试 API 连接...
curl -s http://localhost:8082/api/health >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ✓ API 服务器运行正常
    echo.
    echo 🎉 Mock API 已成功启动！
    echo.
    echo 📋 可用的 API 端点:
    echo    - GET  /api/health
    echo    - GET  /api/papers
    echo    - POST /api/papers
    echo    - GET  /api/papers/:id
    echo    - PUT  /api/papers/:id
    echo    - DELETE /api/papers/:id
    echo    - GET  /api/papers/stats
    echo    - GET  /api/papers/search
    echo.
    echo 💡 测试 API 端点:
    echo    node test-paper-api.js
    echo.
    echo 💡 访问前端应用:
    echo    http://localhost:5173/papers
    echo.
) else (
    echo ❌ API 服务器启动失败
    echo.
    echo 请检查:
    echo 1. Node.js 是否已安装
    echo 2. complete-mock-api.js 文件是否存在
    echo 3. 端口 8082 是否被其他程序占用
    echo.
)

echo ========================================
echo.
pause
