@echo off
REM ========================================
REM PaperCrawler 论文管理模块测试启动脚本
REM ========================================

echo.
echo ========================================
echo  PaperCrawler - 论文管理模块测试
echo ========================================
echo.

REM 检查 Node.js 是否安装
where node >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [错误] 未找到 Node.js，请先安装 Node.js
    echo 下载地址: https://nodejs.org/
    pause
    exit /b 1
)

echo [1/5] 检查环境...
echo ✓ Node.js 已安装
echo.

REM 检查依赖是否安装
if not exist "frontend\node_modules" (
    echo [2/5] 首次运行，正在安装依赖...
    cd frontend
    call npm install
    cd ..
    echo ✓ 依赖安装完成
    echo.
) else (
    echo [2/5] 依赖已安装
    echo.
)

REM 设置端口
set MOCK_API_PORT=8082
set FRONTEND_PORT=5173

echo [3/5] 启动 Mock API 服务器 (端口 %MOCK_API_PORT%)...
start "Mock API Server" cmd /k "echo Mock API 服务器运行中... && echo 地址: http://localhost:%MOCK_API_PORT% && echo. && echo 按 Ctrl+C 停止服务器 && echo. && node complete-mock-api.js"
timeout /t 2 /nobreak >nul

echo [4/5] 等待 Mock API 启动...
timeout /t 3 /nobreak >nul

echo [5/5] 启动前端开发服务器 (端口 %FRONTEND_PORT%)...
cd frontend
start "Frontend Dev Server" cmd /k "echo 前端开发服务器运行中... && echo 地址: http://localhost:%FRONTEND_PORT% && echo. && echo 按 Ctrl+C 停止服务器 && echo. && npm run dev"
cd ..

echo.
echo ========================================
echo  ✓ 所有服务已启动！
echo ========================================
echo.
echo  测试地址:
echo    - 前端:     http://localhost:%FRONTEND_PORT%
echo    - 论文列表: http://localhost:%FRONTEND_PORT%/papers
echo    - Mock API: http://localhost:%MOCK_API_PORT%
echo.
echo  测试账号:
echo    - 邮箱: test@example.com
echo    - 密码: password123
echo.
echo  测试指南:
echo    请查看 TEST_PAPERS_MODULE.md
echo.
echo  按 Ctrl+C 停止所有服务
echo.
pause
