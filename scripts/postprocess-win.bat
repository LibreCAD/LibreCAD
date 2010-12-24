@ECHO OFF

cd > PWD
set /p PWD= < PWD

set RESOURCEDIR=%PWD%\release\resources
set TSDIR=%PWD%\ts

REM Postprocess for windows
echo " Copying fonts and pattersn"
mkdir %RESOURCEDIR%\fonts
mkdir %RESOURCEDIR%\patterns
mkdir %RESOURCEDIR%\library
mkdir %RESOURCEDIR%\library\misc
mkdir %RESOURCEDIR%\library\templates
copy support\patterns\*.dxf %RESOURCEDIR%\patterns
copy support\fonts\*.cxf %RESOURCEDIR%\fonts
copy support\library\misc\*.dxf %RESOURCEDIR%\library\misc
copy support\library\templates\*.dxf %RESOURCEDIR%\library\templates


REM Generate translations
echo "Generating Translations"
lrelease librecad.pro
mkdir %RESOURCEDIR%\qm 

for %%D in (actions,cmd,lib,main,ui) do (
	cd %TSDIR%
	cd "%%D"
	for /f %%F in ('dir /b *.qm') do (
		copy %%F %RESOURCEDIR%\qm\%%F
	)
)

cd %PWD%
