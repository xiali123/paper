call C:\PROGRA~1\MICROS~2\2022\COMMUN~1\VC\AUXIL~1\Build\vcvars64.bat
cd E:\PaperCrawler\backend\build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release --parallel 4
pause
