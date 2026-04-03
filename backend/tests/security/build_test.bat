@echo off
REM SQL注入安全测试编译脚本
REM 使用Visual Studio 2022编译器

echo ====================================
echo SQL Injection Security Test Builder
echo ====================================
echo.

REM 设置Visual Studio环境
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

REM 编译测试程序
echo.
echo [1/2] Compiling SQL Security Test...
cl /EHsc /std:c++17 /O2 /Fe:test_SQLSecurity.exe test_SQLInjectionSecurity_simple.cpp

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [2/2] Running SQL Security Tests...
    echo.
    test_SQLSecurity.exe

    echo.
    echo ====================================
    echo Test Execution Completed
    echo ====================================
) else (
    echo.
    echo ❌ Compilation Failed!
    echo.
)

pause
