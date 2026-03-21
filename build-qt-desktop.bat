@echo off
setlocal EnableDelayedExpansion

echo ========================================
echo PaperCrawler Qt6 桌面应用编译
echo ========================================
echo.

REM 设置Qt6路径
set QT_PATH=C:\Qt

REM 查找Qt6版本
echo 正在查找Qt6安装...
for /d %%D in ("%QT_PATH%\6.*") do (
    if exist "%%D\mingw_64\lib\cmake\Qt6" (
        set QT_VERSION=%%~nxD
        set QT_MINGW=%%D\mingw_64
        echo 找到Qt版本: !QT_VERSION!
        echo 路径: !QT_MINGW!
        goto :found_qt
    )
)

echo ❌ 未找到Qt6 MinGW版本！
echo.
echo 请确认:
echo 1. Qt6安装在: %QT_PATH%
echo 2. 已安装MinGW 64-bit组件
echo.
pause
exit /b 1

:found_qt
echo.
echo ========================================
echo 配置环境
echo ========================================
echo.

set Qt6_DIR=!QT_MINGW!\lib\cmake\Qt6
set CMAKE_PREFIX_PATH=!QT_MINGW!
set PATH=!QT_MINGW!\bin;%PATH%

echo Qt6_DIR=!Qt6_DIR!
echo CMAKE_PREFIX_PATH=!CMAKE_PREFIX_PATH!
echo.

REM 进入desktop目录
cd desktop
if not exist build mkdir build
cd build

echo.
echo ========================================
echo 步骤 1: 清理旧的配置
echo ========================================
echo.

if exist CMakeCache.txt (
    echo 删除旧的CMake配置...
    del /Q CMakeCache.txt
)

echo.
echo ========================================
echo 步骤 2: 配置CMake项目
echo ========================================
echo.

REM 检查是否使用MinGW
if exist "!QT_MINGW!\..\..\Tools\mingw*\bin\g++.exe" (
    echo 使用MinGW生成器
    cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="!QT_MINGW!" -DCMAKE_BUILD_TYPE=Release
) else (
    echo 使用Unix生成器
    cmake .. -DCMAKE_PREFIX_PATH="!QT_MINGW!" -DCMAKE_BUILD_TYPE=Release
)

if errorlevel 1 (
    echo.
    echo ❌ CMake配置失败！
    echo.
    echo 可能的原因:
    echo 1. Qt6路径不正确
    echo 2. MinGW未安装
    echo 3. CMake未安装
    echo.
    echo 当前配置:
    echo   Qt6_DIR=!Qt6_DIR!
    echo   CMAKE_PREFIX_PATH=!CMAKE_PREFIX_PATH!
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo 步骤 3: 编译项目
echo ========================================
echo.

cmake --build . -j4

if errorlevel 1 (
    echo.
    echo ❌ 编译失败！
    echo.
    echo 请检查上方的错误信息
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo 编译成功！
echo ========================================
echo.

REM 查找可执行文件
if exist "Release\PaperCrawlerDesktop.exe" (
    echo 可执行文件: desktop\build\Release\PaperCrawlerDesktop.exe
    set EXE_PATH=Release\PaperCrawlerDesktop.exe
) else if exist "PaperCrawlerDesktop.exe" (
    echo 可执行文件: desktop\build\PaperCrawlerDesktop.exe
    set EXE_PATH=PaperCrawlerDesktop.exe
) else if exist "Debug\PaperCrawlerDesktop.exe" (
    echo 可执行文件: desktop\build\Debug\PaperCrawlerDesktop.exe
    set EXE_PATH=Debug\PaperCrawlerDesktop.exe
) else (
    echo 警告: 未找到可执行文件
    pause
    exit /b 0
)

echo.
echo 按任意键运行应用程序...
pause > nul

echo.
echo 启动应用程序...
start "" "!EXE_PATH!"

echo.
echo ========================================
echo 提示
echo ========================================
echo.
echo 如果应用程序启动失败，请检查:
echo.
echo 1. config\config.json 配置文件
echo 2. MySQL数据库是否运行
echo 3. 数据库连接信息是否正确
echo.
echo ========================================
echo.

endlocal
