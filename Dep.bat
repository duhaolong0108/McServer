@echo off
echo ��ȡ������ļ�·��
set /p var=�ļ�·����
echo ���벢���ɿ�ִ���ļ�
gcc -finput-charset=UTF-8 -fexec-charset=GBK "src/%var%" -o ./build/%var%.exe -lws2_32 -Ofast

:: ��� gcc �Ƿ�ɹ�
if errorlevel 1 (
    echo GCC ����
    goto :end
)
echo �л�������Ŀ¼��ִ�����ɵĿ�ִ���ļ�
cd run
cls
powershell ".\..\build\%var%.exe"
echo ����ֵ %errorlevel%
cd ..
:end