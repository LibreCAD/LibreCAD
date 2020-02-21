call %comspec% /k "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\BuildTools\\VC\\Auxiliary\\Build\\vcvars32.bat"
call "set"
call "qmake libreCAD.pro -tp vc -r"
call "msbuild librecad.sln  /p:Platform=Win32 /p:Configuration=Release"