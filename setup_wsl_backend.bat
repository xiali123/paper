@echo off
REM ========================================
REM PaperCrawler Backend - WSL Build Setup
REM ========================================

echo ========================================
echo   PaperCrawler WSL Backend Build
echo ========================================
echo.

echo [1/6] 安装 Ubuntu 24.04 LTS...
echo ----------------------------------------
wsl --install -d Ubuntu-24.04
echo.
echo 请等待Ubuntu安装完成，然后按任意键继续...
pause >nul

echo.
echo [2/6] 更新APT包管理器...
echo ----------------------------------------
wsl -d Ubuntu-24.04 -- bash -c "sudo apt update"
echo.

echo [3/6] 安装构建依赖...
echo ----------------------------------------
wsl -d Ubuntu-24.04 -- bash -c "sudo apt install -y build-essential cmake git libmysqlclient-dev libcurl4-openssl-dev libgumbo-dev"
echo.

echo [4/6] 进入项目目录...
echo ----------------------------------------
wsl -d Ubuntu-24.04 -- bash -c "cd /mnt/e/PaperCrawler && pwd && ls -la"
echo.

echo [5/6] 创建构建目录并配置...
echo ----------------------------------------
wsl -d Ubuntu-24.04 -- bash -c "cd /mnt/e/PaperCrawler && mkdir -p build && cd build && cmake .."
echo.

echo [6/6] 编译后端服务器...
echo ----------------------------------------
wsl -d Ubuntu-24.04 -- bash -c "cd /mnt/e/PaperCrawler/build && make -j4"
echo.

echo ========================================
echo   构建完成
echo ========================================
echo.
echo 后端可执行文件位置:
echo   /mnt/e/PaperCrawler/build/backend/PaperCrawlerServer
echo.
echo 要启动后端服务器:
echo   wsl -d Ubuntu-24.04 -- bash -c "cd /mnt/e/PaperCrawler/build && ./backend/PaperCrawlerServer"
echo.
echo 或者复制到Windows:
echo   wsl -d Ubuntu-24.04 -- bash -c "cp /mnt/e/PaperCrawler/build/backend/PaperCrawlerServer /mnt/e/PaperCrawler/backend/"
echo.

pause
