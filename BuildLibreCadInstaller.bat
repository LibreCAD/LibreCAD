cd scripts
set PATH=%PATH%;C:\Qt\5.14.1\msvc2017\bin\
IF NOT EXIST %WORKSPACE%\\generated\\ ( mkdir %WORKSPACE%\\generated\\ )
call %WORKSPACE%\\scripts\\build-windows.bat