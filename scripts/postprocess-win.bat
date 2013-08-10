@ECHO OFF

cd > PWD
set /p PWD= < PWD


set PWD=%PWD%\..\..


set RESOURCEDIR=%PWD%\windows\resources
set TSDIRLC=%PWD%\librecad\ts
set TSDIRPI=%PWD%\plugins\ts
set DOCDIR=%PWD%\librecad\support\doc

REM Generate Help Files
cd "%DOCDIR%"
qcollectiongenerator LibreCADdoc.qhcp

cd "%PWD%"

REM Postprocess for windows
echo " Copying fonts and patterns"
mkdir "%RESOURCEDIR%\fonts"
mkdir "%RESOURCEDIR%\patterns"
mkdir "%RESOURCEDIR%\library"
mkdir "%RESOURCEDIR%\doc"
mkdir "%RESOURCEDIR%\library\misc"
mkdir "%RESOURCEDIR%\library\templates"

copy "librecad\support\patterns\*.dxf" "%RESOURCEDIR%\patterns"
copy "librecad\support\fonts\*.lff" "%RESOURCEDIR%\fonts"
copy "librecad\support\doc\*.qhc" "%RESOURCEDIR%\doc"
copy "librecad\support\doc\*.qch" "%RESOURCEDIR%\doc"
copy "librecad\support\library\misc\*.dxf" "%RESOURCEDIR%\library\misc"
copy "librecad\support\library\templates\*.dxf" "%RESOURCEDIR%\library\templates"


REM Generate translations
echo "Generating Translations"
lrelease librecad\src\src.pro
lrelease plugins\plugins.pro
mkdir "%RESOURCEDIR%\qm"

cd "%TSDIRLC%"
for /f %%F in ('dir /b *.qm') do (
        copy "%%F" "%RESOURCEDIR%\qm\%%F"
)

cd "%TSDIRPI%"
for /f %%F in ('dir /b *.qm') do (
        copy "%%F" "%RESOURCEDIR%\qm\%%F"
)

cd "%PWD%"
