@echo off
REM ========================================================================
REM PaperCrawler 编译脚本 - 使用Visual Studio编译器
REM ========================================================================
REM
REM 使用方法:
REM 1. 打开 "x64 Native Tools Command Prompt for VS 2022"
REM    (开始菜单 → Visual Studio 2022 → x64 Native Tools Command Prompt)
REM
REM 2. 切换到项目目录
REM    cd E:\PaperCrawler\backend
REM
REM 3. 运行此脚本
REM    build_with_vs.bat
REM
REM ========================================================================

echo ======================================
echo PaperCrawler 编译脚本
echo ======================================
echo.

REM 检查VS编译器
where cl.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [错误] 未找到VS编译器
    echo.
    echo 请按以下步骤操作:
    echo 1. 打开 "x64 Native Tools Command Prompt for VS 2022"
    echo    (开始菜单 → Visual Studio 2022)
    echo 2. 切换到项目目录: cd E:\PaperCrawler\backend
    echo 3. 运行此脚本: build_with_vs.bat
    echo.
    pause
    exit /b 1
)

echo [步骤1] 检查编译环境
echo --------------------------------------
echo.
cl.exe 2>&1 | findstr /C:"Microsoft"
echo.
echo [OK] Visual Studio编译器已找到
echo.

REM 设置环境变量
set CMAKE_BUILD_TYPE=Release
set BACKEND_DIR=%~dp0
cd /d %BACKEND_DIR%

echo [步骤2] 清理旧的构建文件
echo --------------------------------------
echo.
if exist build (
    echo 删除旧的build目录...
    rmdir /s /q build
)
echo 创建新的build目录...
mkdir build
cd build
echo [OK] build目录已清理
echo.

echo [步骤3] 运行CMake配置
echo --------------------------------------
echo.
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo [错误] CMake配置失败
    pause
    exit /b 1
)
echo [OK] CMake配置成功
echo.

echo [步骤4] 编译项目
echo --------------------------------------
echo.
cmake --build . --config Release --parallel 4
if %ERRORLEVEL% NEQ 0 (
    echo [错误] 编译失败
    pause
    exit /b 1
)
echo [OK] 编译成功
echo.

echo [步骤5] 检查输出文件
echo --------------------------------------
echo.
if exist Release\PaperCrawlerServer.exe (
    echo [成功] 可执行文件已生成:
    dir Release\PaperCrawlerServer.exe
    echo.

    echo [步骤6] 复制依赖文件
    echo --------------------------------------
    echo.
    if not exist Release\modules mkdir Release\modules

    REM 复制DLL
    if exist ..\core\external\curl-8.19.0_4-win64-mingw\bin\libcurl-x64.dll (
        copy ..\core\external\curl-8.19.0_4-win64-mingw\bin\libcurl-x64.dll Release\ >nul
        echo [OK] libcurl-x64.dll 已复制
    )

    if exist "C:\Program Files\MySQL\MySQL Server 8.0\lib\libmysql.dll" (
        copy "C:\Program Files\MySQL\MySQL Server 8.0\lib\libmysql.dll" Release\ >nul
        echo [OK] libmysql.dll 已复制
    )

    REM 复制配置文件
    if exist ..\config.json (
        copy ..\config.json Release\ >nul
        echo [OK] config.json 已复制
    )

    echo.
    echo ======================================
    echo 编译完成！
    echo ======================================
    echo.
    echo 可执行文件位置:
    echo   %CD%\Release\PaperCrawlerServer.exe
    echo.
    echo 下一步:
    echo   1. cd Release
    echo   2. PaperCrawlerServer.exe config.json
    echo.
) else (
    echo [错误] 未找到可执行文件
    dir /s /b *.exe
)

echo.
pause
