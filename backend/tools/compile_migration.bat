@echo off
cd /d E:\PaperCrawler\backend

echo Compiling migration tool...
g++ -std=c++17 -o tools/run_auth_migration.exe tools/run_auth_migration.cpp -I"E:\Program Files\MySQL\MySQL Server 8.0\include" -L"E:\Program Files\MySQL\MySQL Server 8.0\lib" -llibmysql

if %ERRORLEVEL% EQU 0 (
    echo Compilation successful!
    echo Running migration tool...
    tools\run_auth_migration.exe
) else (
    echo Compilation failed!
    echo.
    echo Please ensure:
    echo 1. MySQL is installed
    echo 2. MySQL Connector/C is in: E:\Program Files\MySQL\MySQL Server 8.0
    echo 3. Or modify the paths in this batch file
)

pause
