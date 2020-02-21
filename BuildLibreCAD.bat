call "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\BuildTools\\VC\\Auxiliary\\Build\\vcvars32.bat"
set BOOST_DIR=c:/boost/boost_1_72_0/
set BOOST_LIBDIR=c:/boost/boost_1_72_0/libs/
set FREETYPE_DIR=C:\Qt\freetype\
set PATH=%PATH%;C:\Qt\5.14.1\msvc2017\bin\
qmake libreCAD.pro -tp vc -r
msbuild librecad.sln  /p:Platform=Win32 /p:Configuration=Release