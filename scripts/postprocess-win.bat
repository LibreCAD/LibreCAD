@ECHO OFF

cd > PWD
set /p PWD= < PWD

set RESOURCEDIR=%PWD%\release\resources
set TSDIR=%PWD%\ts
set DOCDIR=%PWD%\support\doc

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
copy "support\patterns\*.dxf" "%RESOURCEDIR%\patterns"
copy "support\fonts\*.cxf" "%RESOURCEDIR%\fonts"
copy "support\fonts\*.lff" "%RESOURCEDIR%\fonts"
copy "support\doc\*.qhc" "%RESOURCEDIR%\doc"
copy "support\doc\*.qch" "%RESOURCEDIR%\doc"
copy "support\library\misc\*.dxf" "%RESOURCEDIR%\library\misc"
copy "support\library\templates\*.dxf" "%RESOURCEDIR%\library\templates"


REM Generate translations
echo "Generating Translations"
lrelease librecad.pro
mkdir "%RESOURCEDIR%\qm"

cd "%TSDIR%"
for /f %%F in ('dir /b *.qm') do (
        copy "%%F" "%RESOURCEDIR%\qm\%%F"
)

cd "%PWD%"
