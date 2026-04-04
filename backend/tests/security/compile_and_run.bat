@echo off
setlocal enabledelayedexpansion

echo ============================================================
echo SQL Security Test - Auto Build and Run
echo ============================================================
echo.

REM 设置Visual Studio环境
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

echo [1/3] Compiling test_security_standalone.cpp...
cl /EHsc /std:c++17 /O2 /Fe:test_security.exe test_security_standalone.cpp 2>&1 | findstr /C:"error" /C:"test_security"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ❌ Compilation failed!
    echo.
    goto :cleanup
)

echo.
echo [2/3] Compilation successful!
echo.

if not exist test_security.exe (
    echo ❌ test_security.exe not found!
    goto :cleanup
)

echo [3/3] Running SQL Injection Security Tests...
echo.
test_security.exe

echo.
echo ============================================================
echo Test Execution Completed
echo ============================================================

:cleanup
if exist test_security_standalone.obj del test_security_standalone.obj
endlocal
