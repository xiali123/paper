@echo off
chcp 65001 >nul
cd /d "%~dp0"
echo ============================================================
echo SQL Security Test Suite
echo ============================================================
echo.
echo Calling Visual Studio compiler...
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl /EHsc /std:c++17 /O2 /Fe:test_security.exe test_security_standalone.cpp > compile.log 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Compilation successful!
    echo.
    echo Running tests...
    echo.
    test_security.exe
    del test_security_standalone.obj >nul 2>&1
) else (
    echo Compilation failed! See compile.log for details.
    type compile.log
)
echo.
pause
