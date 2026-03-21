@echo off
echo ========================================
echo Testing PaperCrawler API
echo ========================================
echo.

echo Testing 1: Health Check
echo ----------------------------------------
curl -s http://localhost:8080/health
echo.
echo.

echo Testing 2: Search Papers
echo ----------------------------------------
curl -s "http://localhost:8080/api/search?q=deep"
echo.
echo.

echo Testing 3: Statistics
echo ----------------------------------------
curl -s http://localhost:8080/api/stats/overview
echo.
echo.

echo Testing 4: Export CSV
echo ----------------------------------------
curl -s http://localhost:8080/api/export/csv
echo.
echo.

echo ========================================
echo All tests completed!
echo ========================================
echo.
pause
