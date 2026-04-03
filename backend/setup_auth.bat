@echo off
setlocal enabledelayedexpansion

echo ==========================================================
echo PaperCrawler Authentication System - Setup
echo ==========================================================
echo.

REM MySQL configuration
set MYSQL_HOST=127.0.0.1
set MYSQL_PORT=3306
set MYSQL_USER=root
set MYSQL_PASS=123456
set MYSQL_DB=papercrawler_db

REM Find MySQL executable
set "MYSQL_BIN="
for %%p in (
    "C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe"
    "C:\xampp\mysql\bin\mysql.exe"
    "C:\wamp64\bin\mysql\mysql8.0.31\bin\mysql.exe"
    "C:\laragon\bin\mysql\mysql-8.0.30\bin\mysql.exe"
) do (
    if exist %%p (
        set "MYSQL_BIN=%%p"
        goto :found_mysql
    )
)

echo ERROR: MySQL executable not found!
echo Please ensure MySQL is installed.
echo.
pause
exit /b 1

:found_mysql
echo [OK] Found MySQL at: !MYSQL_BIN!
echo.

REM Test connection
echo [1/3] Testing MySQL connection...
"!MYSQL_BIN!" -h !MYSQL_HOST! -P !MYSQL_PORT! -u !MYSQL_USER! -p!MYSQL_PASS! -e "SELECT VERSION();" >nul 2>&1
if !errorlevel! equ 0 (
    echo [OK] Connected to MySQL successfully
) else (
    echo [ERROR] Failed to connect to MySQL
    echo Please check MySQL is running and password is correct
    pause
    exit /b 1
)
echo.

REM Execute SQL file
echo [2/3] Creating users table...
"!MYSQL_BIN!" -h !MYSQL_HOST! -P !MYSQL_PORT! -u !MYSQL_USER! -p!MYSQL_PASS! !MYSQL_DB! < create_users_table.sql
if !errorlevel! equ 0 (
    echo [OK] SQL script executed successfully
) else (
    echo [ERROR] Failed to execute SQL script
    pause
    exit /b 1
)
echo.

REM Verify tables
echo [3/3] Verifying tables...
"!MYSQL_BIN!" -h !MYSQL_HOST! -P !MYSQL_PORT! -u !MYSQL_USER! -p!MYSQL_PASS! !MYSQL_DB! -e "SHOW TABLES LIKE 'user%%';" >nul 2>&1
echo.
echo ==========================================================
echo Setup completed successfully!
echo ==========================================================
echo.
echo You can now test the registration API:
echo   curl -X POST http://localhost:8080/api/auth/register ^
echo     -H "Content-Type: application/json" ^
echo     -d "{\"email\":\"test@example.com\",\"password\":\"test123\",\"name\":\"Test User\"}"
echo.
pause
