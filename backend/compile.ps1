# PaperCrawler 编译脚本
$VSPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
$BuildDir = "E:\PaperCrawler\backend\build"

# 创建build目录
if (!(Test-Path $BuildDir))) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# 切换到build目录
Set-Location $BuildDir

# 运行CMake配置
Write-Host "配置CMake..."
cmd /c "`"$VSPath`" && cmake .. -G `"Visual Studio 17 2022`" -A x64"

# 编译项目
Write-Host "编译项目..."
cmd /c "`"$VSPath`" && cmake --build . --config Release --parallel 4"

Write-Host "编译完成！"
