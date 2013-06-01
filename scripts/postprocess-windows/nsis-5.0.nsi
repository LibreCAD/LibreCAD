;NSIS Modern User Interface
;Basic Example Script

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"
  !include "WinVer.nsh"

  !define MUI_ICON "..\..\librecad\res\main\librecad.ico"

;--------------------------------
;General

  ;Name and file
  Name "LibreCAD"
  OutFile "LibreCAD-Installer.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\LibreCAD"

  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\LibreCAD" ""

  ;Request application pivileges for Windows Vista
  RequestExecutionLevel admin
  ;TargetMinimalOS 5.1

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "../../gpl-2.0.txt"
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"



Function .onInit

  Push $R0
  Push $R1
  Push $R2

; get acount info into $R2
  userInfo::getAccountType
  pop $0
  StrCpy $R2 $0 5

${If} ${IsWin2000}
    strCmp $R2 "Admin" lbl_checkok
    messageBox MB_OK "I am sorry, this installer needs Admin priviledges, Please login as an administrator and install the software."
    Quit
${EndIf}

${If} ${IsWinXP}
    strCmp $R2 "Admin" lbl_checkok
    messageBox MB_OK "I am sorry, this installer needs Admin priviledges, Please login as an administrator and install the software."
    Quit
${EndIf}

  lbl_checkok:
  Pop $R2
  Pop $R1
  Pop $R0

FunctionEnd

;--------------------------------
;Installer Sections

Section "Install Section" SecInstall
;imageformats/qgif4.dll
;imageformats/qjpeg4.dll
;sceneformats/qsceneai4.dll
;QtWebKit/qmldir
;QtWebKit/qmlwebkitplugin.dll
;application.exe
;Qt3D.dll
;Qt3DQuick.dll
;QtCore4.dll
;QtDeclarative.dll
;QtGui4.dll
;QtNetwork4.dll
;QtOpenGL4.dll
;QtScript4.dll
;QtScriptTools4.dll
;QtWebKit4.dll
;QtXml4.dll
;QtXmlPatterns4.dll
  SetOutPath "$INSTDIR"
  File /r "..\..\..\build-librecad-Desktop_Qt_5_0_2_MinGW_32bit-Release\windows\*.*"
;C:\Users\dli.TAMAGGO\cygwin\home\dli\github\build-librecad-Desktop_Qt_5_0_2_MinGW_32bit-Release
  File "C:\Qt\Qt5.0.2\Tools\MinGW\bin\libgcc_s_sjlj-1.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\icu*.dll"
  File "C:\Qt\Qt5.0.2\Tools\MinGW\bin\libwinpthread-1.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\libstdc++-6.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\libEGL.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\libGLESv2.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\D3DCompiler_43.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5Clucene.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5Core.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5Core.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5Declarative.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5Gui.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5Help.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5Network.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5OpenGL.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5PrintSupport.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5Script.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5Svg.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5Sql.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5Widgets.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5Xml.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\Qt5XmlPatterns.dll"
  SetOutPath "$INSTDIR\platforms"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\plugins\platforms\qminimal.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\plugins\platforms\qwindows.dll"
  SetOutPath "$INSTDIR\sqldrivers"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\plugins\sqldrivers\qsqlite.dll"
  SetOutPath "$INSTDIR\imageformats"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\plugins\imageformats\qgif.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\plugins\imageformats\qjpeg.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\plugins\imageformats\qtiff.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\plugins\imageformats\qmng.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\plugins\imageformats\qsvg.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\plugins\imageformats\qtga.dll"
  File "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\plugins\imageformats\qwbmp.dll"

  ;Store installation folder
  WriteRegStr HKCU "Software\LibreCAD" "" $INSTDIR

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; create shortcuts
  createShortCut "$DESKTOP\LibreCAD.lnk" "$INSTDIR\LibreCAD.exe"

  ; Startmenu shortcuts
  createDirectory "$SMPROGRAMS\LibreCAD\"
  createShortCut "$SMPROGRAMS\LibreCAD\LibreCAD.lnk" "$INSTDIR\LibreCAD.exe"
  createShortCut "$SMPROGRAMS\LibreCAD\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

  ; Open Donate URL
  Exec "rundll32 url.dll,FileProtocolHandler http://librecad.org/donate.html"

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecInstall ${LANG_ENGLISH} "A test section."

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...

  Delete "$INSTDIR\Uninstall.exe"
  RMDir /r "$SMPROGRAMS\LibreCAD\"
  RMDir /r $INSTDIR

  RMDir "$INSTDIR"

  DeleteRegKey /ifempty HKCU "Software\LibreCAD"

SectionEnd

