@ECHO OFF

cd > PWD
set /p PWD= < PWD

set RESOURCEDIR=%PWD%\release\resources
set TSDIR=%PWD%\ts

REM Postprocess for windows
echo " Copying fonts and pattersn"
mkdir %RESOURCEDIR%\fonts
mkdir %RESOURCEDIR%\patterns
copy support\patterns\*.dxf %RESOURCEDIR%\patterns
copy support\fonts\*.cxf %RESOURCEDIR%\fonts


REM Generate translations
echo "Generating Translations"
lrelease caduntu.pro
mkdir %RESOURCEDIR%\qm 

for %%D in (actions,cmd,lib,main,ui) do (
	cd %TSDIR%
	cd "%%D"
	for /f %%F in ('dir /b *.qm') do (
		copy %%F %RESOURCEDIR%\qm\%%F
	)
)

cd %PWD%
