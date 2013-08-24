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
if not exist "%RESOURCEDIR%\fonts\" (mkdir "%RESOURCEDIR%\fonts")
if not exist "%RESOURCEDIR%\patterns\" (mkdir "%RESOURCEDIR%\patterns")
if not exist "%RESOURCEDIR%\library\" (mkdir "%RESOURCEDIR%\library")
if not exist "%RESOURCEDIR%\doc\" (mkdir "%RESOURCEDIR%\doc")
if not exist "%RESOURCEDIR%\library\misc\" (mkdir "%RESOURCEDIR%\library\misc")
if not exist "%RESOURCEDIR%\library\templates\" (mkdir "%RESOURCEDIR%\library\templates")

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
if not exist "%RESOURCEDIR%\qm\" (mkdir "%RESOURCEDIR%\qm")

cd "%TSDIRLC%"
for /f %%F in ('dir /b *.qm') do (
        copy "%%F" "%RESOURCEDIR%\qm\%%F"
)

cd "%TSDIRPI%"
for /f %%F in ('dir /b *.qm') do (
        copy "%%F" "%RESOURCEDIR%\qm\%%F"
)

cd "%PWD%"
