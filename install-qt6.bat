@echo off
echo ========================================
echo Qt6 自动安装助手
echo ========================================
echo.
echo 这个脚本将帮助您下载和安装Qt6
echo.

set QT_VERSION=6.5.0
set QT_INSTALLER=qt-unified-windows-x64-online.exe
set QT_DOWNLOAD_URL=https://download.qt.io/official_releases/online_installers/qt-unified-windows-x64-online.exe
set QT_INSTALLER_PATH=%TEMP%\%QT_INSTALLER%

echo 正在检查是否已有Qt安装程序...
if exist "%QT_INSTALLER_PATH%" (
    echo 找到已下载的安装程序: %QT_INSTALLER_PATH%
) else (
    echo.
    echo 正在下载Qt安装程序...
    echo 下载地址: %QT_DOWNLOAD_URL%
    echo.
    echo 请稍候，文件约100MB...

    curl -L -o "%QT_INSTALLER_PATH%" "%QT_DOWNLOAD_URL%"

    if errorlevel 1 (
        echo.
        echo 自动下载失败！
        echo.
        echo 请手动下载:
        echo 1. 访问: https://www.qt.io/download-qt-installer
        echo 2. 下载 Windows 在线安装程序
        echo 3. 保存到: %QT_INSTALLER_PATH%
        echo.
        pause
        exit /b 1
    )

    echo 下载完成！
)

echo.
echo ========================================
echo 准备启动Qt6安装程序
echo ========================================
echo.
echo Qt6安装向导将打开，请按以下步骤操作:
echo.
echo ┌─────────────────────────────────────┐
echo │  Qt6 安装步骤                        │
echo └─────────────────────────────────────┘
echo.
echo 1. 欢迎界面
echo    → 点击 "Next"
echo.
echo 2. 登录Qt账号
echo    → 选择 "Skip" （跳过登录，无需注册）
echo.
echo 3. 选择安装目录
echo    → 默认: C:\Qt
echo    → 建议改为: E:\Qt （节省C盘空间）
echo.
echo 4. 选择安装组件 ⭐ 重要！
echo    → 必须勾选:
echo      ☑ Qt %QT_VERSION%
echo      ☑ MinGW 11.2.0 64-bit
echo      ☑ Qt Charts
echo      ☑ CMake
echo.
echo 5. 同意许可协议
echo    → 选择 "开源使用者"
echo    → 勾选 "我已阅读并同意"
echo.
echo 6. 准备安装
echo    → 确认组件选择
echo    → 点击 "Install"
echo.
echo 7. 开始下载和安装
echo    → 大约需要5-10分钟（取决于网速）
echo.
echo ┌─────────────────────────────────────┐
echo │  组件勾选详情                        │
echo ├─────────────────────────────────────┤
echo │ ☑ Qt 6.5.0 (必须)                   │
echo │   ├─ MinGW 11.2.0 64-bit (必须)     │
echo │   └─ MSVC 2019 64-bit (可选)        │
echo │ ☑ Qt Charts (推荐)                  │
echo │ ☑ Qt Creator (可选，IDE)           │
echo └─────────────────────────────────────┘
echo.

pause

echo.
echo 正在启动Qt安装程序...
start "" "%QT_INSTALLER_PATH%"

echo.
echo ========================================
echo 安装完成后
echo ========================================
echo.
echo Qt6安装完成后（约5-10分钟），请:
echo.
echo 1. 记录Qt安装路径，通常是:
echo    E:\Qt\6.5.0\mingw_64
echo.
echo 2. 运行环境设置脚本:
echo    configure-qt-path.bat
echo.
echo 3. 开始编译PaperCrawler桌面应用!
echo.

echo.
echo 安装程序已启动，请按屏幕提示操作...
echo.
echo 祝您安装顺利！
echo.

REM 等待安装程序
timeout /t 5

echo.
echo 💡 提示:
echo - 安装过程中可以选择只安装必需组件以节省空间
echo - 建议安装在E盘或其他非系统盘
echo - MinGW版本优先级高于MSVC版本
echo.

pause
