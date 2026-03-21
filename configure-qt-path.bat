@echo off
echo ========================================
echo Qt6 路径配置工具
echo ========================================
echo.

REM 自动检测Qt安装路径
set QT_PATH=

echo 正在搜索Qt6安装...
echo.

REM 检查常见安装位置
if exist "E:\Qt\6.5.0\mingw_64" (
    set QT_PATH=E:\Qt\6.5.0\mingw_64
    echo 找到Qt安装: %QT_PATH%
    goto :found
)

if exist "C:\Qt\6.5.0\mingw_64" (
    set QT_PATH=C:\Qt\6.5.0\mingw_64
    echo 找到Qt安装: %QT_PATH%
    goto :found
)

if exist "D:\Qt\6.5.0\mingw_64" (
    set QT_PATH=D:\Qt\6.5.0\mingw_64
    echo 找到Qt安装: %QT_PATH%
    goto :found
)

REM 检查其他版本
for /d %%D in ("C:\Qt\*") do (
    if exist "%%D\mingw_64\lib\cmake\Qt6" (
        set QT_PATH=%%D\mingw_64
        echo 找到Qt安装: !QT_PATH!
        goto :found
    )
)

for /d %%D in ("E:\Qt\*") do (
    if exist "%%D\mingw_64\lib\cmake\Qt6" (
        set QT_PATH=%%D\mingw_64
        echo 找到Qt安装: !QT_PATH!
        goto :found
    }
)

echo.
echo ❌ 未找到Qt6安装！
echo.
echo 请确认:
echo 1. Qt6已安装完成
echo 2. 安装了MinGW 64-bit组件
echo.
echo 如果Qt已安装在其他位置，请手动输入路径:
echo.
set /p QT_PATH="请输入Qt安装路径 (例如 E:\Qt\6.5.0\mingw_64): "
if not exist "%QT_PATH%" (
    echo 路径不存在: %QT_PATH%
    pause
    exit /b 1
)

:found
echo.
echo ========================================
echo 配置环境变量
echo ========================================
echo.
echo Qt路径: %QT_PATH%
echo.

REM 设置用户环境变量
setx Qt6_DIR "%QT_PATH%\lib\cmake\Qt6"
echo 已设置 Qt6_DIR=%QT_PATH%\lib\cmake\Qt6

setx CMAKE_PREFIX_PATH "%QT_PATH%"
echo 已设置 CMAKE_PREFIX_PATH=%QT_PATH%

echo.
echo ========================================
echo 配置完成！
echo ========================================
echo.
echo 环境变量已设置，请:
echo.
echo 1. 重启命令提示符（使环境变量生效）
echo 2. 运行 build-desktop.bat 编译桌面应用
echo.
echo 或在当前命令提示符中运行:
echo.
echo   set Qt6_DIR=%QT_PATH%\lib\cmake\Qt6
echo   set CMAKE_PREFIX_PATH=%QT_PATH%
echo.

pause
