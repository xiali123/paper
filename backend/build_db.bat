@echo off
cd /d E:\PaperCrawler\backend

echo Compiling database checker...
g++ -std=c++17 -o simple_db_check.exe simple_db_check.cpp ^
  -I"C:\Program Files\MySQL\MySQL Server 8.0\include" ^
  -L"C:\Program Files\MySQL\MySQL Server 8.0\lib" ^
  -llibmysql

if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed!
    echo.
    echo Trying alternative MySQL paths...
    g++ -std=c++17 -o simple_db_check.exe simple_db_check.cpp ^
      -I"C:\xampp\mysql\include" ^
      -L"C:\xampp\mysql\lib" ^
      -llibmysql
)

if %ERRORLEVEL% NEQ 0 (
    echo Still failed. Please check MySQL installation.
    pause
    exit /b 1
)

echo Compilation successful!
echo.
echo Running database checker...
simple_db_check.exe

pause
