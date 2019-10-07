@ECHO OFF

REM from librecad/src goto LibreCAD root folder
REM CAUTION! pushd isn't tolerant for /, use \
pushd ..
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
if not exist "%LC_RESOURCEDIR%\library\algoritm\" (mkdir "%LC_RESOURCEDIR%\library\algoritm")
if not exist "%LC_RESOURCEDIR%\library\block\" (mkdir "%LC_RESOURCEDIR%\library\block")
if not exist "%LC_RESOURCEDIR%\library\kinetics\" (mkdir "%LC_RESOURCEDIR%\library\kinetics")
if not exist "%LC_RESOURCEDIR%\library\power_station\" (mkdir "%LC_RESOURCEDIR%\library\power_station")
if not exist "%LC_RESOURCEDIR%\library\sheets\" (mkdir "%LC_RESOURCEDIR%\library\sheets")

copy "librecad\support\patterns\*.dxf" "%LC_RESOURCEDIR%\patterns"
copy "librecad\support\fonts\*.lff" "%LC_RESOURCEDIR%\fonts"
copy "librecad\support\library\misc\*.dxf" "%LC_RESOURCEDIR%\library\misc"
copy "librecad\support\library\templates\*.dxf" "%LC_RESOURCEDIR%\library\templates"
copy "librecad\support\library\algoritm\*.dxf" "%LC_RESOURCEDIR%\library\algorithm"
copy "librecad\support\library\block\*.dxf" "%LC_RESOURCEDIR%\library\block"
copy "librecad\support\library\kinetics\*.dxf" "%LC_RESOURCEDIR%\library\kinetics"
copy "librecad\support\library\power_station\*.dxf" "%LC_RESOURCEDIR%\library\power_station"
copy "librecad\support\library\sheets\*.dxf" "%LC_RESOURCEDIR%\library\sheets"


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

call :jdate tnow "%date%"
call :jdate tthen "1/1/2000"
set /a diff = tnow-tthen
REM Create NSIS-Include file
set LC_SCMREV_NSH=%LC_NSISDIR%\generated_scmrev.nsh
echo Create %LC_SCMREV_NSH% for NSIS Installer
echo ;CAUTION! >%LC_SCMREV_NSH%
echo ;this file is created by postprocess-win.bat during build process >>%LC_SCMREV_NSH%
echo ;changes will be overwritten, use custom.nsh for local settings >>%LC_SCMREV_NSH%
echo. >>%LC_SCMREV_NSH%
echo !define SCMREVISION "2.2.0.%diff%" >>%LC_SCMREV_NSH%
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

:jdate JD DateStr -- converts a date string to julian day number with respect to regional date format
::                -- JD      [out,opt] - julian days
::                -- DateStr [in,opt]  - date string, e.g. "03/31/2006" or "Fri 03/31/2006" or "31.3.2006"
:$reference https://groups.google.com/group/alt.msdos.batch.nt/browse_frm/thread/a0c34d593e782e94/50ed3430b6446af8#50ed3430b6446af8
:$created 20060101 :$changed 20080219
:$source https://www.dostips.com
SETLOCAL
set DateStr=%~2&if "%~2"=="" set DateStr=%date%
for /f "skip=1 tokens=2-4 delims=(-)" %%a in ('"echo.|date"') do (
    for /f "tokens=1-3 delims=/.- " %%A in ("%DateStr:* =%") do (
        set %%a=%%A&set %%b=%%B&set %%c=%%C))
set /a "yy=10000%yy% %%10000,mm=100%mm% %% 100,dd=100%dd% %% 100"
set /a JD=dd-32075+1461*(yy+4800+(mm-14)/12)/4+367*(mm-2-(mm-14)/12*12)/12-3*((yy+4900+(mm-14)/12)/100)/4
ENDLOCAL & IF "%~1" NEQ "" (SET %~1=%JD%) ELSE (echo.%JD%)
EXIT /b