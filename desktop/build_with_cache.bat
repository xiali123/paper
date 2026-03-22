@echo off
REM ========================================
REM PaperCrawler Desktop - Build with Cache
REM ========================================

echo ========================================
echo   Building PaperCrawler Desktop with Cache
echo ========================================
echo.

cd /d e:\PaperCrawler\desktop

if not exist build mkdir build
cd build

echo [1/3] Configuring CMake...
echo ----------------------------------------
cmake .. -G "MinGW Makefiles" ^
  -DCMAKE_PREFIX_PATH="C:/Qt/6.10.2/mingw_131" ^
  -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo.
    echo ❌ CMake configuration failed!
    echo.
    pause
    exit /b 1
)

echo.
echo [2/3] Building...
echo ----------------------------------------
mingw32-make -j4

if errorlevel 1 (
    echo.
    echo ❌ Build failed!
    echo.
    pause
    exit /b 1
)

echo.
echo [3/3] Build completed successfully!
echo ----------------------------------------
echo.
echo ✅ Desktop client with cache support built successfully!
echo.
echo Location: e:\PaperCrawler\desktop\build\PaperCrawlerDesktop.exe
echo.
echo New features:
echo   ✅ Local page cache for instant back/forward navigation
echo   ✅ LRU cache eviction (max 20 pages per search)
echo   ✅ Cache statistics in debug output
echo   ✅ Cached data is preserved across sessions
echo.
echo To test the cache:
echo   1. Search for a keyword
echo   2. Navigate through several pages
echo   3. Click "Previous" - should load instantly from cache
echo   4. Click "Next" again - also instant if cached
echo.

pause
