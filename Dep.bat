:: encoding GBK

@echo off
echo 编译并生成可执行文件
gcc -finput-charset=UTF-8 -fexec-charset=GBK "src/Main.c" -o ./build/build.exe -lws2_32 -Ofast

:: 检查 gcc 是否成功
if errorlevel 1 (
    echo GCC 报错！
    goto :end
)
echo 切换到运行目录并执行生成的可执行文件
cd run

powershell ".\..\build\build.exe"
echo 返回值 %errorlevel%
cd ..
:end