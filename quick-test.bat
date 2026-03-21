@echo off
setlocal enabledelayedexpansion

echo ========================================
echo PaperCrawler Quick Test Suite
echo ========================================
echo.

set PASSED=0
set FAILED=0

:print_result
if %1==0 (
    echo [PASS] %2
    set /a PASSED+=1
) else (
    echo [FAIL] %2
    set /a FAILED+=1
)
goto :eof

echo Test 1: Checking project structure...
if exist "CMakeLists.txt" (call :print_result 0 "Root CMakeLists.txt") else (call :print_result 1 "Root CMakeLists.txt")
if exist "core" (call :print_result 0 "Core directory") else (call :print_result 1 "Core directory")
if exist "desktop" (call :print_result 0 "Desktop directory") else (call :print_result 1 "Desktop directory")
if exist "backend" (call :print_result 0 "Backend directory") else (call :print_result 1 "Backend directory")
if exist "frontend" (call :print_result 0 "Frontend directory") else (call :print_result 1 "Frontend directory")
if exist "docker-compose.yml" (call :print_result 0 "Docker compose file") else (call :print_result 1 "Docker compose file")
echo.

echo Test 2: Checking core library...
if exist "core\include\core\PaperCrawlerAPI.hpp" (call :print_result 0 "PaperCrawlerAPI.hpp") else (call :print_result 1 "PaperCrawlerAPI.hpp")
if exist "core\src\core\PaperCrawlerAPI.cpp" (call :print_result 0 "PaperCrawlerAPI.cpp") else (call :print_result 1 "PaperCrawlerAPI.cpp")
if exist "core\CMakeLists.txt" (call :print_result 0 "Core CMakeLists.txt") else (call :print_result 1 "Core CMakeLists.txt")
echo.

echo Test 3: Checking desktop client...
if exist "desktop\include\MainWindow.hpp" (call :print_result 0 "MainWindow.hpp") else (call :print_result 1 "MainWindow.hpp")
if exist "desktop\src\MainWindow.cpp" (call :print_result 0 "MainWindow.cpp") else (call :print_result 1 "MainWindow.cpp")
if exist "desktop\CMakeLists.txt" (call :print_result 0 "Desktop CMakeLists.txt") else (call :print_result 1 "Desktop CMakeLists.txt")
echo.

echo Test 4: Checking backend API...
if exist "backend\src\main.cpp" (call :print_result 0 "Backend main.cpp") else (call :print_result 1 "Backend main.cpp")
if exist "backend\CMakeLists.txt" (call :print_result 0 "Backend CMakeLists.txt") else (call :print_result 1 "Backend CMakeLists.txt")
echo.

echo Test 5: Checking frontend...
if exist "frontend\package.json" (call :print_result 0 "Frontend package.json") else (call :print_result 1 "Frontend package.json")
if exist "frontend\vite.config.ts" (call :print_result 0 "Vite config") else (call :print_result 1 "Vite config")
if exist "frontend\src\main.ts" (call :print_result 0 "Frontend main.ts") else (call :print_result 1 "Frontend main.ts")
echo.

echo Test 6: Checking configuration...
if exist "config\config.json" (call :print_result 0 "Config file") else (call :print_result 1 "Config file")
if exist "docker-compose.yml" (call :print_result 0 "Docker compose file") else (call :print_result 1 "Docker compose file")
if exist "sql\init.sql" (call :print_result 0 "Database init script") else (call :print_result 1 "Database init script")
echo.

echo Test 7: Checking documentation...
if exist "README.md" (call :print_result 0 "README.md") else (call :print_result 1 "README.md")
if exist "README.EXPANDED.md" (call :print_result 0 "README.EXPANDED.md") else (call :print_result 1 "README.EXPANDED.md")
if exist "DEPLOYMENT.md" (call :print_result 0 "DEPLOYMENT.md") else (call :print_result 1 "DEPLOYMENT.md")
echo.

echo ========================================
echo Test Summary
echo ========================================
echo Passed: %PASSED%
echo Failed: %FAILED%
echo.

if %FAILED%==0 (
    echo All tests passed!
    echo.
    echo Next steps:
    echo 1. Review config\config.json and update database settings
    echo 2. Initialize MySQL database
    echo 3. Choose deployment method:
    echo    - Docker: docker-compose up -d
    echo    - Manual: Follow TEST-GUIDE.md
) else (
    echo Some tests failed. Please review the output above.
)

pause
