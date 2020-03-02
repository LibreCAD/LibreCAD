@ECHO OFF
CD /D %0\..

ECHO.
ECHO Telemetry.dll
%WinDir%\Microsoft.NET\Framework\v4.0.30319\RegAsm /nologo /unregister Telemetry.dll
%WinDir%\Microsoft.NET\Framework\v4.0.30319\RegAsm /nologo Telemetry.dll
