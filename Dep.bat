:: encoding GBK

@echo off
echo ���벢���ɿ�ִ���ļ�
gcc -finput-charset=UTF-8 -fexec-charset=GBK "src/Main.c" -o ./build/build.exe -lws2_32 -Ofast

:: ��� gcc �Ƿ�ɹ�
if errorlevel 1 (
    echo GCC ����
    goto :end
)
echo �л�������Ŀ¼��ִ�����ɵĿ�ִ���ļ�
cd run

powershell ".\..\build\build.exe"
echo ����ֵ %errorlevel%
cd ..
:end