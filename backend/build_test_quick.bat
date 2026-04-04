@echo off
REM 快速MySQL连接测试编译脚本

echo === 编译MySQL连接测试程序 ===

REM 设置MySQL路径（根据您的安装位置修改）
set MYSQL_INCLUDE=C:\Program Files\MySQL\MySQL Server 8.0\include
set MYSQL_LIB=C:\Program Files\MySQL\MySQL Server 8.0\lib

echo MySQL Include Path: %MYSQL_INCLUDE%
echo MySQL Lib Path: %MYSQL_LIB%
echo.

REM 编译测试程序
g++ -std=c++17 ^
    -I"%MYSQL_INCLUDE%" ^
    -I"./include" ^
    -I"./core/external" ^
    test_mysql_quick.cpp ^
    -L"%MYSQL_LIB%" ^
    -lmysqlclient ^
    -o test_mysql_quick.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ✅ 编译成功！
    echo.
    echo 运行测试程序：
    echo.
    test_mysql_quick.exe
) else (
    echo.
    echo ❌ 编译失败
    echo.
    echo 可能的原因：
    echo 1. MySQL未安装或路径不正确
    echo 2. g++编译器不可用
    echo 3. 缺少MySQL开发库
    echo.
    echo 请修改脚本中的MYSQL_INCLUDE和MYSQL_LIB路径
)

echo.
pause
