@echo off
echo ========================================
echo PaperCrawler 桌面应用编译脚本
echo ========================================
echo.

REM 检查Qt6路径
if not defined Qt6_DIR (
    if not defined CMAKE_PREFIX_PATH (
        echo.
        echo ❌ 未找到Qt6环境变量！
        echo.
        echo 请先运行: configure-qt-path.bat
        echo 或手动设置:
        echo   set Qt6_DIR=您的Qt路径\lib\cmake\Qt6
        echo.
        pause
        exit /b 1
    )
)

echo Qt6路径配置:
if defined Qt6_DIR (
    echo   Qt6_DIR=%Qt6_DIR%
)
if defined CMAKE_PREFIX_PATH (
    echo   CMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH%
)
echo.

cd desktop
if not exist build mkdir build
cd build

echo.
echo ========================================
echo 步骤 1: 配置项目
echo ========================================
echo.

if exist "CMakeCache.txt" (
    echo 清理旧的配置...
    del /Q CMakeCache.txt
)

REM 根据环境变量选择生成器
if defined CMAKE_PREFIX_PATH (
    cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%"
) else (
    cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="%Qt6_DIR:\..\..\.."
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
    echo 请检查:
    echo - Qt6_DIR=%Qt6_DIR%
    echo - CMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH%
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo 步骤 2: 编译项目
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
echo 可执行文件位置:
if exist "Release\PaperCrawlerDesktop.exe" (
    echo   build\desktop\build\Release\PaperCrawlerDesktop.exe
) else if exist "PaperCrawlerDesktop.exe" (
    echo   build\desktop\build\PaperCrawlerDesktop.exe
)

echo.
echo 按任意键运行应用程序...
pause > nul

echo.
echo 启动应用程序...
if exist "Release\PaperCrawlerDesktop.exe" (
    start "" Release\PaperCrawlerDesktop.exe
) else if exist "PaperCrawlerDesktop.exe" (
    start "" PaperCrawlerDesktop.exe
) else (
    echo 未找到可执行文件！
    pause
)

echo.
echo ========================================
echo 提示
echo ========================================
echo.
echo 如果应用程序启动失败:
echo.
echo 1. 检查是否安装了Qt运行时
echo 2. 检查MySQL数据库是否运行
echo 3. 查看 config\config.json 配置
echo.

goto end

:error
echo.
echo 编译或配置失败！
echo.
echo 请参考:
echo - QT-INSTALL-GUIDE.md
echo - README-QUICK.md
echo.
pause

:end
