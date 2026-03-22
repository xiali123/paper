@echo off
REM ========================================
REM PaperCrawler Cache Test Script
REM ========================================

echo ========================================
echo   PaperCrawler 缓存功能测试
echo ========================================
echo.

REM 检查可执行文件
if not exist "e:\PaperCrawler\desktop\build\PaperCrawlerDesktop.exe" (
    echo ❌ 桌面客户端未找到
    echo.
    echo 请先构建:
    echo   e:\PaperCrawler\desktop\build_with_cache.bat
    echo.
    pause
    exit /b 1
)

echo ✅ 找到桌面客户端
echo.

REM 检查后端服务器
echo [检查] 后端服务器...
tasklist /FI "IMAGENAME eq PaperCrawlerServer.exe" 2>NUL | find /I /N "PaperCrawlerServer.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo ✅ 后端服务器正在运行
) else (
    echo ⚠️  后端服务器未运行，尝试启动...
    start "PaperCrawler Backend" e:\PaperCrawler\backend\PaperCrawlerServer.exe
    timeout /t 3 /nobreak >nul
)

echo.
echo ========================================
echo   缓存功能测试指南
echo ========================================
echo.
echo 测试步骤:
echo.
echo 1️⃣  搜索测试
echo    - 输入关键词 "machine learning"
echo    - 点击搜索
echo    - 观察状态栏: "搜索完成！找到 XX 篇相关论文"
echo.
echo 2️⃣  下页测试
echo    - 点击 "▶ 下一页" 按钮
echo    - 观察: 应该看到不同的论文
echo    - 状态栏: "正在加载第 2 页..."
echo.
echo 3️⃣  上页测试（缓存命中）✨
echo    - 点击 "◀ 上一页" 按钮
echo    - 观察状态栏: 应该显示 "第 1 页（来自缓存）"
echo    - ⚡ 加载速度应该非常快（~10ms）
echo.
echo 4️⃣  首页测试（缓存命中）✨
echo    - 浏览到第3页
echo    - 点击 "⏮ 首页" 按钮
echo    - 观察状态栏: 应该显示 "第 1 页（来自缓存）"
echo.
echo 5️⃣  尾页测试
echo    - 点击 "⏭ 尾页" 按钮
echo    - 观察: 跳转到最后一页
echo    - 再次点击首页: 应该从缓存加载 ✨
echo.
echo 6️⃣  缓存持久性测试
echo    - 改变每页数量（10/20/50/100）
echo    - 返回之前的页
echo    - 观察: 缓存键不同，需要重新加载
echo.
echo ========================================
echo   预期结果
echo ========================================
echo.
echo ✅ 缓存命中特征:
echo    - 状态栏显示 "（来自缓存）"
echo    - 页面瞬间显示（无加载延迟）
echo    - 调试输出显示 "Cache: HIT"
echo.
echo ❌ 缓存未命中:
echo    - 状态栏显示 "正在加载..."
echo    - 有明显加载延迟（~500ms）
echo    - 调试输出显示 "Cache: MISS"
echo.
echo ========================================
echo   性能对比
echo ========================================
echo.
echo 操作              无缓存     有缓存     提升
echo ─────────────────────────────────────────
echo 上页              ~500ms     ~10ms      50x
echo 首页              ~500ms     ~10ms      50x
echo 已访问页          ~500ms     ~10ms      50x
echo ─────────────────────────────────────────
echo.

echo ========================================
echo   启动桌面客户端...
echo ========================================
echo.

start e:\PaperCrawler\desktop\build\PaperCrawlerDesktop.exe

echo ✅ 桌面客户端已启动
echo.
echo 💡 提示:
echo    - 查看控制台输出可以看到缓存日志
echo    - 第一次访问某页会从后端加载
echo    - 再次访问相同页会从缓存加载（极快！）
echo.
echo 要停止测试:
echo    - 关闭桌面客户端窗口
echo.
echo 查看完整优化文档:
echo    e:\PaperCrawler\desktop\CACHE_OPTIMIZATION.md
echo.

pause
