@echo off
setlocal EnableDelayedExpansion

echo ========================================
echo PaperCrawler Qt Desktop Application
echo ========================================
echo.

REM Add Qt DLLs to PATH
set PATH=C:\Qt\6.10.2\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;%PATH%

echo Starting PaperCrawler Desktop...
echo.

cd /d "%~dp0"

if exist "build\PaperCrawlerDesktop.exe" (
    start "" "build\PaperCrawlerDesktop.exe"
    echo Application launched!
) else if exist "PaperCrawlerDesktop.exe" (
    start "" "PaperCrawlerDesktop.exe"
    echo Application launched!
) else (
    echo Error: PaperCrawlerDesktop.exe not found!
    echo.
    echo Please build the application first:
    echo   cd desktop
    echo   mkdir build && cd build
    echo   cmake .. -G "MinGW Makefiles"
    echo   mingw32-make
    pause
    exit /b 1
)

echo.
echo ========================================
echo Tips:
echo - Use the search box to find papers
echo - Click on results to view details
echo - Try the dark theme in View menu
echo - For full functionality, use web version:
echo   Run START-WEB.bat
echo ========================================
echo.

endlocal
