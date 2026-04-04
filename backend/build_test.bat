@echo off
REM MySQL连接测试程序编译脚本 (Windows)

echo === 编译MySQL连接测试程序 ===

REM 设置MySQL路径（根据您的安装位置修改）
set MYSQL_INCLUDE=C:\Program Files\MySQL\MySQL Server 8.0\include
set MYSQL_LIB=C:\Program Files\MySQL\MySQL Server 8.0\lib

REM 编译测试程序
g++ -std=c++17 ^
    -I"%MYSQL_INCLUDE%" ^
    -I./include ^
    -I./core/external ^
    test_db_connection.cpp ^
    -L"%MYSQL_LIB%" ^
    -lmysqlclient ^
    -o test_db_connection.exe

if %ERRORLEVEL% EQU 0 (
    echo ✅ 编译成功！
    echo.
    echo 运行测试程序：
    test_db_connection.exe
) else (
    echo ❌ 编译失败
    echo.
    echo 请确保：
    echo 1. MySQL已安装并且路径正确
    echo 2. g++编译器可用
    echo 3. 设置了环境变量 DB_PASSWORD
)

pause
