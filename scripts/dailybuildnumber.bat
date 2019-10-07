@ECHO OFF

call :jdate tnow "%date%"
call :jdate tthen "1/1/2000"
set /a diff = tnow-tthen
echo %diff%
exit

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