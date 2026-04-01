@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d E:\PaperCrawler\backend\build
echo 开始编译...
cmake .. -G "Visual Studio 17 2022" -A x64
echo 开始构建...
cmake --build . --config Release --parallel 4
echo 编译完成！
pause
