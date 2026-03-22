@echo off
REM ========================================
REM PaperCrawler Desktop - Deploy Runtime DLLs
REM ========================================

echo ========================================
echo   Deploying PaperCrawler Desktop
echo ========================================
echo.

cd /d e:\PaperCrawler\desktop\build

if not exist "PaperCrawlerDesktop.exe" (
    echo ❌ PaperCrawlerDesktop.exe not found!
    echo    Please build first: build_with_cache.bat
    echo.
    pause
    exit /b 1
)

echo [1/3] Killing running instances...
taskkill /F /IM PaperCrawlerDesktop.exe >nul 2>&1
timeout /t 2 /nobreak >nul

echo.
echo [2/3] Copying Qt6 DLLs...
copy /Y "C:\Qt\6.10.2\mingw_64\bin\Qt6Core.dll" . >nul
copy /Y "C:\Qt\6.10.2\mingw_64\bin\Qt6Gui.dll" . >nul
copy /Y "C:\Qt\6.10.2\mingw_64\bin\Qt6Widgets.dll" . >nul
copy /Y "C:\Qt\6.10.2\mingw_64\bin\Qt6Network.dll" . >nul
copy /Y "C:\Qt\6.10.2\mingw_64\bin\Qt6Sql.dll" . >nul
copy /Y "C:\Qt\6.10.2\mingw_64\bin\Qt6Charts.dll" . >nul

echo.
echo [3/3] Copying MinGW runtime DLLs...
copy /Y "C:\Qt\Tools\mingw1310_64\bin\libgcc_s_seh-1.dll" . >nul
copy /Y "C:\Qt\Tools\mingw1310_64\bin\libstdc++-6.dll" . >nul
copy /Y "C:\Qt\Tools\mingw1310_64\bin\libwinpthread-1.dll" . >nul

echo.
echo ========================================
echo   Deployment Complete
echo ========================================
echo.
echo ✅ All runtime DLLs deployed!
echo.
echo Location: e:\PaperCrawler\desktop\build\
echo.
echo You can now run: PaperCrawlerDesktop.exe
echo.

dir /B *.dll | find /C ".dll"
echo DLL files deployed.

pause
