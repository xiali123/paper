@echo off
setlocal EnableDelayedExpansion

echo ========================================
echo PaperCrawler - 复制到纯英文路径并编译
echo ========================================
echo.

set SOURCE_DIR=E:\研究生\资料\PaperCrawler
set TARGET_DIR=E:\PaperCrawler

echo 源目录: %SOURCE_DIR%
echo 目标目录: %TARGET_DIR%
echo.

if exist "%TARGET_DIR%" (
    echo 目标目录已存在，是否删除并重新复制?
    set /p CONFIRM="输入 Y 确认，其他键取消: "

    if /i "!CONFIRM!"=="Y" (
        echo 正在删除旧文件...
        rmdir /s /q "%TARGET_DIR%"
    ) else (
        echo 使用现有目录...
        goto :build
    )
)

echo.
echo ========================================
echo 正在复制项目文件...
echo ========================================
echo.

REM 复制项目文件
xcopy "%SOURCE_DIR%" "%TARGET_DIR%" /E /I /H /Y /EXCLUDE:%TEMP%\exclude.txt

echo.
echo 复制完成！
echo.

:build
echo.
echo ========================================
echo 准备编译Qt桌面应用
echo ========================================
echo.
echo 项目已复制到: %TARGET_DIR%
echo.
echo 接下来的步骤:
echo.
echo 1. 打开新的命令行窗口
echo 2. 运行以下命令:
echo.
echo    cd /d %TARGET_DIR%\desktop
echo    build-qt-desktop.bat
echo.

pause
