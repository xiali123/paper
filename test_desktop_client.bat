@echo off
REM ========================================
REM PaperCrawler Desktop Client - UI Test
REM ========================================

echo ========================================
echo   PaperCrawler Desktop UI Test
echo ========================================
echo.

echo [1/4] 启动后端服务器（旧版本，仅用于UI测试）
echo ----------------------------------------
start "PaperCrawler Backend" e:\PaperCrawler\backend\PaperCrawlerServer.exe
timeout /t 3 /nobreak >nul

echo.
echo [2/4] 等待后端启动...
timeout /t 5 /nobreak >nul

echo.
echo [3/4] 启动桌面客户端...
echo ----------------------------------------
echo 启动中...
start e:\PaperCrawler\desktop\build\PaperCrawlerDesktop.exe

echo.
echo ========================================
echo   测试说明
echo ========================================
echo.
echo 桌面客户端已启动！请测试以下功能：
echo.
echo ✅ UI组件测试:
echo    - 主窗口显示（1400x900）
echo    - Hero section 顶部展示
echo    - FeatureCards 功能卡片
echo    - 搜索框（可输入关键词）
echo    - 主题切换按钮（右上角）
echo.
echo ✅ 主题切换测试:
echo    - 点击右上角 🌙/☀️ 按钮
echo    - 观察背景渐变变化
echo    - 状态栏显示当前主题
echo.
echo ✅ 搜索功能测试:
echo    - 在搜索框输入: "machine learning"
echo    - 点击 🔍 搜索按钮
echo    - 观察论文列表显示
echo.
echo ⚠️  已知限制（后端使用旧代码）:
echo    - 分页按钮可能显示相同结果
echo    - 标题可能包含URL参数（如 "test&offset=0&limit=3"）
echo    - 这是预期的，因为后端尚未重建
echo.
echo ✅ 菜单功能测试:
echo    - File -> Export Results
echo    - View -> Toggle Theme
echo    - Tools -> Statistics
echo    - Help -> About
echo.
echo ========================================
echo   测试完成后
echo ========================================
echo.
echo 要停止程序:
echo   1. 关闭桌面客户端窗口
echo   2. 在后端服务器窗口按 Ctrl+C
echo.
echo 要应用分页修复并重新测试:
echo   1. 查看 FINAL_SUMMARY_REPORT.md
echo   2. 选择构建环境（Docker/WSL）
echo   3. 重建后端服务
echo   4. 重新测试分页功能
echo.

pause
