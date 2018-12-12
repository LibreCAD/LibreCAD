@ECHO OFF

REM from librecad/src goto LibreCAD root folder
REM CAUTION! pushd isn't tolerant for /, use \
pushd ..\..
cd > PWD
set /p LC_PWD= < PWD
del PWD

set LC_RESOURCEDIR=%LC_PWD%\windows\resources
set LC_TSDIRLC=%LC_PWD%\librecad\ts
set LC_TSDIRPI=%LC_PWD%\plugins\ts
set LC_NSISDIR=%LC_PWD%\scripts\postprocess-windows

REM Postprocess for windows
echo " Copying fonts and patterns"
if not exist "%LC_RESOURCEDIR%\fonts\" (mkdir "%LC_RESOURCEDIR%\fonts")
if not exist "%LC_RESOURCEDIR%\patterns\" (mkdir "%LC_RESOURCEDIR%\patterns")
if not exist "%LC_RESOURCEDIR%\library\" (mkdir "%LC_RESOURCEDIR%\library")
if not exist "%LC_RESOURCEDIR%\library\misc\" (mkdir "%LC_RESOURCEDIR%\library\misc")
if not exist "%LC_RESOURCEDIR%\library\templates\" (mkdir "%LC_RESOURCEDIR%\library\templates")

copy "librecad\support\patterns\*.dxf" "%LC_RESOURCEDIR%\patterns"
copy "librecad\support\fonts\*.lff" "%LC_RESOURCEDIR%\fonts"
copy "librecad\support\library\misc\*.dxf" "%LC_RESOURCEDIR%\library\misc"
copy "librecad\support\library\templates\*.dxf" "%LC_RESOURCEDIR%\library\templates"


REM Generate translations
echo "Generating Translations"
lrelease librecad\src\src.pro
lrelease plugins\plugins.pro
if not exist "%LC_RESOURCEDIR%\qm\" (mkdir "%LC_RESOURCEDIR%\qm")

REM translations for LibreCAD
cd "%LC_TSDIRLC%"
for /f %%F in ('dir /b *.qm') do (
        copy "%%F" "%LC_RESOURCEDIR%\qm\%%F"
)

REM translations for PlugIns
cd "%LC_TSDIRPI%"
for /f %%F in ('dir /b *.qm') do (
        copy "%%F" "%LC_RESOURCEDIR%\qm\%%F"
)

REM Create NSIS-Include file
set LC_SCMREV_NSH=%LC_NSISDIR%\generated_scmrev.nsh
echo Create %LC_SCMREV_NSH% for NSIS Installer
echo ;CAUTION! >%LC_SCMREV_NSH%
echo ;this file is created by postprocess-win.bat during build process >>%LC_SCMREV_NSH%
echo ;changes will be overwritten, use custom.nsh for local settings >>%LC_SCMREV_NSH%
echo. >>%LC_SCMREV_NSH%
echo !define SCMREVISION "%1" >>%LC_SCMREV_NSH%
echo. >>%LC_SCMREV_NSH%

if exist %LC_NSISDIR%\custom-*.ns? (
	echo.
	echo Warning!
	echo An old NSIS custom include file was found!
	echo Please, rename it to custom.nsh.
	echo.
)

REM return to librecad/src
popd
