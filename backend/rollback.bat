@echo off
REM ====================================
REM PaperCrawler 回滚脚本
REM 版本: 1.0.0
REM ====================================

setlocal enabledelayedexpansion
chcp 65001 >nul

echo.
echo ============================================================
echo PaperCrawler 回滚脚本
echo ============================================================
echo.

REM 配置
set "TARGET_DIR=E:\PaperCrawler\Production"
set "BACKUP_DIR=E:\PaperCrawler\Backup"

REM 停止服务
echo [步骤 1/5] 停止服务...
tasklist /FI "IMAGENAME eq PaperCrawlerServer.exe" 2>nul | find /I "PaperCrawlerServer.exe" >nul
if %ERRORLEVEL% EQU 0 (
    taskkill /F /IM PaperCrawlerServer.exe >nul 2>&1
    timeout /t 2 /nobreak >nul
    echo ✓ 服务已停止
) else (
    echo ✓ 服务未运行
)
echo.

REM 列出可用备份
echo [步骤 2/5] 查找可用备份...
dir /B /AD "%BACKUP_DIR%" 2>nul | findstr /R "^[0-9]" > "%TEMP%\backups.txt"
set /p COUNT=<"%TEMP%\backups.txt"
echo.
echo 可用备份版本:
echo.
set /a INDEX=0
for /f "tokens=*" %%i in ('dir /B /AD /O-D "%BACKUP_DIR%" 2^nul ^| findstr /R "^[0-9]"') do (
    set /a INDEX+=1
    set "BACKUP_!INDEX!=%%i"
    echo   [!INDEX!] %%i
)
echo.

if !INDEX! EQU 0 (
    echo ❌ 未找到可用备份
    pause
    exit /b 1
)

REM 选择备份
set /p CHOICE="请选择要回滚到的版本 (1-!INDEX!): "
if "!CHOICE!"=="" set CHOICE=1

for /f "usebackq delims==" %%i in (`echo %%BACKUP_%CHOICE%%%`) do set "SELECTED_BACKUP=%%i"

if not exist "%BACKUP_DIR%\!SELECTED_BACKUP!" (
    echo ❌ 选择的备份不存在
    pause
    exit /b 1
)

echo.
echo 将回滚到: !SELECTED_BACKUP!
set /p CONFIRM="确认回滚? (Y/N): "
if /I not "!CONFIRM!"=="Y" (
    echo 操作已取消
    pause
    exit /b 0
)
echo.

REM 执行回滚
echo [步骤 3/5] 恢复程序文件...
copy "%BACKUP_DIR%\!SELECTED_BACKUP!\*.exe" "%TARGET_DIR%\" /Y >nul
copy "%BACKUP_DIR%\!SELECTED_BACKUP!\*.dll" "%TARGET_DIR%\" /Y >nul
echo ✓ 程序文件已恢复

echo [步骤 4/5] 恢复模块文件...
if exist "%TARGET_DIR%\modules" rmdir /S /Q "%TARGET_DIR%\modules"
xcopy /E /I /Y "%BACKUP_DIR%\!SELECTED_BACKUP!\modules" "%TARGET_DIR%\modules" >nul
echo ✓ 模块文件已恢复
echo.

REM 启动服务
echo [步骤 5/5] 重启服务...
cd /d "%TARGET_DIR%"
start "" PaperCrawlerServer.exe

timeout /t 3 /nobreak >nul

tasklist /FI "IMAGENAME eq PaperCrawlerServer.exe" 2>nul | find /I "PaperCrawlerServer.exe" >nul
if %ERRORLEVEL% EQU 0 (
    echo ✓ 服务启动成功
) else (
    echo ⚠ 服务启动可能失败，请检查
)
echo.

echo ============================================================
echo 回滚完成！
echo ============================================================
echo.
echo 回滚到版本: !SELECTED_BACKUP!
echo.
echo 验证命令:
echo   tasklist ^| findstr PaperCrawlerServer
echo   curl http://localhost:8080/api/health
echo.
pause
