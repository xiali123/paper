@echo off
echo ========================================
echo Starting PaperCrawler Backend API
echo ========================================
echo.

cd backend
if not exist build mkdir build
cd build

echo Configuring...
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:\Qt\6.10.2\mingw_64" 2>/dev/null

echo.
echo Building...
mingw32-make -j4

echo.
echo Starting server on port 8080...
echo.
echo Press Ctrl+C to stop
echo.

PaperCrawlerServer.exe
