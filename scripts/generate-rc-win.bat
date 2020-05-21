@echo off
cd ../../scripts
set cmd="call dailybuildnumber"
FOR /F "tokens=*" %%i IN (' %cmd% ') DO SET DAILY_BUILD=%%i
FOR /F "tokens=2" %%i IN (' "date /t" ') DO SET DATE_STR=%%i

>../librecad/res/main/librecad.rc (
echo #ifndef RCFILE_H
echo #define RCFILE_H
echo #include ^<windows.h^>
echo LANGUAGE 0x409, 1252
echo IDI_ICON1               ICON    DISCARDABLE     "librecad.ico"
echo VS_VERSION_INFO VERSIONINFO
echo FILEVERSION 2,2,0,%DAILY_BUILD%
echo PRODUCTVERSION 2,2,0
echo BEGIN
echo     BLOCK "StringFileInfo"
echo     BEGIN
echo         BLOCK "040904E4"
echo         BEGIN
echo             VALUE "CompanyName",        "Hypertherm"
echo             VALUE "FileDescription",    "LibreCAD for ProNest"
echo             VALUE "FileVersion",        "2.2.0.%DAILY_BUILD%"
echo             VALUE "InternalName",       "LibreCAD for ProNest"
echo             VALUE "LegalCopyright",     "GPL v2.0"
echo             VALUE "OriginalFilename",   "LibreCAD.exe"
echo             VALUE "ProductName",        "LibreCAD"
echo             VALUE "ProductVersion",     "2.2.0"
echo             VALUE "Rev",     %DAILY_BUILD%
echo             VALUE "Date",     %DATE_STR%
echo         END
echo     END
echo     BLOCK "VarFileInfo"
echo     BEGIN
echo         VALUE "Translation", 0x409, 1252
echo     END
echo END
echo #endif // RCFILE_H
)
