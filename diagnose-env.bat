@echo off
setlocal enabledelayedexpansion

REM ========================================
REM PaperCrawler 环境诊断工具
REM ========================================

echo.
echo ========================================
echo  PaperCrawler 环境诊断
echo ========================================
echo.

REM 检查 Node.js
echo [1/6] 检查 Node.js...
where node >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ❌ Node.js 未安装
    echo.
    echo 请安装 Node.js: https://nodejs.org/
    pause
    exit /b 1
) else (
    for /f "tokens=*" %%i in ('node --version') do set NODE_VERSION=%%i
    echo ✓ Node.js 已安装 (!NODE_VERSION!)
)
echo.

REM 检查依赖
echo [2/6] 检查前端依赖...
if not exist "frontend\node_modules" (
    echo ❌ 前端依赖未安装
    echo.
    echo 正在安装依赖...
    cd frontend
    call npm install
    cd ..
    echo ✓ 依赖安装完成
) else (
    echo ✓ 前端依赖已安装
)
echo.

REM 检查 Mock API
echo [3/6] 检查 Mock API 服务器...
curl -s http://localhost:8082/api/health >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ✓ Mock API 正在运行 (端口 8082)
    set API_RUNNING=1
) else (
    echo ❌ Mock API 未运行
    set API_RUNNING=0
)
echo.

REM 检查前端服务器
echo [4/6] 检查前端开发服务器...
curl -s http://localhost:5173 >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ✓ 前端服务器正在运行 (端口 5173)
    set FRONTEND_RUNNING=1
) else (
    echo ❌ 前端服务器未运行
    set FRONTEND_RUNNING=0
)
echo.

REM 诊断结果
echo [5/6] 诊断结果...
echo.
if !API_RUNNING! EQU 1 (
    if !FRONTEND_RUNNING! EQU 1 (
        echo ✅ 所有服务正常运行！
        echo.
        echo 📱 访问地址:
        echo    - 前端:     http://localhost:5173
        echo    - 论文列表: http://localhost:5173/papers
        echo    - Mock API: http://localhost:8082
        echo.
        echo 🎉 可以开始测试了！
    ) else (
        echo ⚠️  Mock API 运行中，但前端未启动
        echo.
        echo 🔧 启动前端:
        echo    cd frontend
        echo    npm run dev
    )
) else (
    if !FRONTEND_RUNNING! EQU 1 (
        echo ⚠️  前端运行中，但 Mock API 未启动
        echo.
        echo 🔧 启动 Mock API:
        echo    node complete-mock-api.js
    ) else (
        echo ❌ 所有服务都未运行
        echo.
        echo 🔧 启动所有服务:
        echo    方式1: start-paper-test.bat
        echo    方式2: 手动启动
        echo           终端1: node complete-mock-api.js
        echo           终端2: cd frontend ^&^& npm run dev
    )
)
echo.

REM 端口检查
echo [6/6] 端口占用检查...
echo.
netstat -ano | findstr ":5173" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ✓ 端口 5173 (前端) 已被使用
) else (
    echo ℹ️  端口 5173 (前端) 空闲
)

netstat -ano | findstr ":8082" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ✓ 端口 8082 (Mock API) 已被使用
) else (
    echo ℹ️  端口 8082 (Mock API) 空闲
)
echo.

echo ========================================
echo  诊断完成
echo ========================================
echo.

pause
