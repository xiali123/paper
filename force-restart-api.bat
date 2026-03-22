@echo off
setlocal enabledelayedexpansion

echo.
echo ========================================
echo  强制重启 Mock API 服务器
echo ========================================
echo.

REM 强制终止所有 node 进程
echo [1/3] 终止所有 Node.js 进程...
taskkill /F /IM node.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ✓ 已终止所有 Node.js 进程
) else (
    echo ℹ️  没有运行中的 Node.js 进程
)
echo.

REM 等待端口释放
echo [2/3] 等待端口释放...
timeout /t 2 /nobreak >nul
echo.

REM 启动 Mock API
echo [3/3] 启动 Mock API 服务器...
echo.
echo ========================================
echo  PaperCrawler Mock API Server
echo  Running on http://localhost:8082
echo ========================================
echo.
echo 按 Ctrl+C 停止服务器
echo.

node complete-mock-api.js
