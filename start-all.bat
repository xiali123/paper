@echo off
echo Starting PaperCrawler servers...

REM Start backend
cd /d E:\PaperCrawler\backend\build\Release
if not exist libmysql.dll copy "C:\Program Files\MySQL\MySQL Server 8.0\lib\libmysql.dll" .
start "PaperCrawler Backend" cmd /k PaperCrawlerServer.exe

REM Wait for backend to start
timeout /t 3 /nobreak > /dev/null

REM Start frontend
cd /d E:\PaperCrawler\frontend
start "PaperCrawler Frontend" cmd /k npm run dev

echo.
echo ========================================
echo   Both servers started!
echo ========================================
echo   Frontend: http://localhost:5173
echo   Backend:  http://localhost:8080
echo ========================================
echo.
echo Press any key to close this window
echo (servers will continue running in separate windows)
pause > /dev/null
