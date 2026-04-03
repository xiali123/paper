@echo off
REM ====================================
REM PaperCrawler 快速部署脚本
REM 版本: 1.0.0
REM 日期: 2026-04-04
REM ====================================

setlocal enabledelayedexpansion
chcp 65001 >nul

echo.
echo ============================================================
echo PaperCrawler 快速部署脚本
echo ============================================================
echo.

REM 配置变量
set "SOURCE_DIR=E:\PaperCrawler\backend\build\Release"
set "TARGET_DIR=E:\PaperCrawler\Production"
set "BACKUP_DIR=E:\PaperCrawler\Backup"

REM 步骤1: 创建目录
echo [步骤 1/7] 创建部署目录...
if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"
if not exist "%TARGET_DIR%\logs" mkdir "%TARGET_DIR%\logs"
if not exist "%TARGET_DIR%\data" mkdir "%TARGET_DIR%\data"
if not exist "%TARGET_DIR%\backup" mkdir "%TARGET_DIR%\backup"
echo ✓ 目录创建完成
echo.

REM 步骤2: 备份旧版本
echo [步骤 2/7] 备份现有版本...
if exist "%TARGET_DIR%\PaperCrawlerServer.exe" (
    set "TIMESTAMP=%date:~0,4%%date:~5,2%%date:~8,2%_%time:~0,2%%time:~3,2%%time:~6,2%"
    set "TIMESTAMP=!TIMESTAMP: =0!"

    if not exist "%BACKUP_DIR%\!TIMESTAMP!" mkdir "%BACKUP_DIR%\!TIMESTAMP!"

    copy "%TARGET_DIR%\*.exe" "%BACKUP_DIR%\!TIMESTAMP!\" >nul 2>&1
    copy "%TARGET_DIR%\*.dll" "%BACKUP_DIR%\!TIMESTAMP!\" >nul 2>&1
    xcopy /E /I /Y "%TARGET_DIR%\modules" "%BACKUP_DIR%\!TIMESTAMP!\modules" >nul 2>&1

    echo ✓ 已备份到: %BACKUP_DIR%\!TIMESTAMP!
) else (
    echo ✓ 无需备份（首次部署）
)
echo.

REM 步骤3: 停止服务
echo [步骤 3/7] 停止现有服务...
tasklist /FI "IMAGENAME eq PaperCrawlerServer.exe" 2>nul | find /I "PaperCrawlerServer.exe" >nul
if %ERRORLEVEL% EQU 0 (
    echo 正在停止服务...
    taskkill /F /IM PaperCrawlerServer.exe >nul 2>&1
    timeout /t 2 /nobreak >nul
    echo ✓ 服务已停止
) else (
    echo ✓ 服务未运行
)
echo.

REM 步骤4: 复制新文件
echo [步骤 4/7] 部署新版本...
copy "%SOURCE_DIR%\PaperCrawlerServer.exe" "%TARGET_DIR%\" /Y >nul
if %ERRORLEVEL% EQU 0 (
    echo ✓ 主程序已部署
) else (
    echo ❌ 主程序部署失败
    goto :error
)

xcopy /E /I /Y "%SOURCE_DIR%\modules" "%TARGET_DIR%\modules" >nul
if %ERRORLEVEL% EQU 0 (
    echo ✓ 动态模块已部署
) else (
    echo ❌ 模块部署失败
    goto :error
)

copy "%SOURCE_DIR%\*.dll" "%TARGET_DIR%\" /Y >nul 2>&1
echo ✓ 依赖库已部署
echo.

REM 步骤5: 验证部署
echo [步骤 5/7] 验证部署...
if exist "%TARGET_DIR%\PaperCrawlerServer.exe" (
    if exist "%TARGET_DIR%\modules\dynamic\UserApiModule.dll" (
        if exist "%TARGET_DIR%\modules\dynamic\SearchApiModule.dll" (
            echo ✓ 所有关键文件验证通过
        ) else (
            echo ❌ 模块文件缺失
            goto :error
        )
    ) else (
        echo ❌ 模块文件缺失
        goto :error
    )
) else (
    echo ❌ 主程序文件缺失
    goto :error
)
echo.

REM 步骤6: 启动服务
echo [步骤 6/7] 启动服务...
cd /d "%TARGET_DIR%"
start "" PaperCrawlerServer.exe

REM 等待服务启动
timeout /t 3 /nobreak >nul

REM 验证服务运行
tasklist /FI "IMAGENAME eq PaperCrawlerServer.exe" 2>nul | find /I "PaperCrawlerServer.exe" >nul
if %ERRORLEVEL% EQU 0 (
    echo ✓ 服务启动成功
) else (
    echo ⚠ 服务可能未正常启动，请检查日志
)
echo.

REM 步骤7: 健康检查
echo [步骤 7/7] 执行健康检查...
timeout /t 2 /nobreak >nul
powershell -Command "try { $response = Invoke-WebRequest -Uri 'http://localhost:8080/api/health' -TimeoutSec 5; Write-Host '✓ 健康检查通过 (HTTP ' $response.StatusCode)'; } catch { Write-Host '⚠ 健康检查失败，服务可能仍在初始化'; }"
echo.

REM 完成
echo ============================================================
echo 部署完成！
echo ============================================================
echo.
echo 📊 部署摘要:
echo   目标目录: %TARGET_DIR%
echo   备份目录: %BACKUP_DIR%
echo.
echo 🔍 验证命令:
echo   1. 检查服务状态: tasklist ^| findstr PaperCrawlerServer
echo   2. 查看日志:     type %TARGET_DIR%\logs\papercrawler.log
echo   3. 健康检查:     curl http://localhost:8080/api/health
echo.
echo 📚 相关文档:
echo   - 部署指南:     docs\DEPLOYMENT_GUIDE.md
echo   - Bug修复报告:  BUG_FIX_REPORT.md
echo   - 验证报告:     VERIFICATION_REPORT.md
echo.
goto :end

:error
echo.
echo ============================================================
echo ❌ 部署失败！
echo ============================================================
echo.
echo 请检查错误信息并参考故障排查文档。
echo.
pause
exit /b 1

:end
echo 如需回滚，请运行: rollback.bat
echo.
pause
exit /b 0
