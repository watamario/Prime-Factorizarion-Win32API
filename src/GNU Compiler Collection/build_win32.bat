@echo off

set PATH=C:\MinGW\bin
cd /d %~dp0

if exist "resource.rc" goto withres

echo *** C++�\�[�X(1.cpp)���r���h��...
g++ -Wall -o PF_IA-32.exe -static-libstdc++ -static-libgcc -mwindows -DUNICODE 1.cpp
if not exist "PF_IA-32.exe" goto errors
goto success

:withres
echo *** C++�\�[�X(1.cpp)���R���p�C��+�A�Z���u����...
if exist "obj.o" del "obj.o"
g++ -Wall -c 1.cpp -o obj.o -DUNICODE
if not exist "obj.o" goto errors

echo *** ���\�[�X(resource.rc)��O����+�A�Z���u����...
if exist "resource.rc.o" del "resource.rc.o"
windres resource.rc resource.rc.o -DUNICODE
if not exist "resource.rc.o" goto errors

echo *** obj.o��resource.rc.o�������N��...
g++ -Wall obj.o resource.rc.o -o PF_IA-32.exe -static-libstdc++ -static-libgcc -mwindows -DUNICODE
if not exist "PF_IA-32.exe" goto errors
goto success


:success
echo �r���h����
PF_IA-32.exe
exit /B

:errors
echo �r���h���s
pause
exit /B