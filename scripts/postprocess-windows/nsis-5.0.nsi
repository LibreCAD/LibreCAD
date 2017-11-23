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
  OutFile "../../generated/LibreCAD-Installer.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\LibreCAD"

  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\LibreCAD" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin
  ;TargetMinimalOS 5.1

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "../../licenses/gpl-2.0.txt"
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

; get account info into $R2
  userInfo::getAccountType
  pop $0
  StrCpy $R2 $0 5

${If} ${IsWin2000}
    strCmp $R2 "Admin" lbl_checkok
    messageBox MB_OK "I am sorry, this installer needs Admin privileges, Please login as an administrator and install the software."
    Quit
${EndIf}

${If} ${IsWinXP}
    strCmp $R2 "Admin" lbl_checkok
    messageBox MB_OK "I am sorry, this installer needs Admin privileges, Please login as an administrator and install the software."
    Quit
${EndIf}

  lbl_checkok:
  Pop $R2
  Pop $R1
  Pop $R0

FunctionEnd

!define Qt_Dir "C:\Qt"
!define Qt_Version "5.0.2"

;--------------------------------
;Installer Sections

Section "Install Section" SecInstall
  SetOutPath "$INSTDIR"
  File /r "..\..\windows\*.*"
  File "${Qt_Dir}\Qt${Qt_Version}\Tools\MinGW\bin\libgcc_s_sjlj-1.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\icu*.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\Tools\MinGW\bin\libwinpthread-1.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\libstdc++-6.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\libEGL.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\libGLESv2.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\D3DCompiler_43.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\Qt5Clucene.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\Qt5Core.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\Qt5Declarative.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\Qt5Gui.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\Qt5Help.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\Qt5Network.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\Qt5OpenGL.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\Qt5PrintSupport.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\Qt5Script.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\Qt5Svg.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\Qt5Sql.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\Qt5Widgets.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\Qt5Xml.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\bin\Qt5XmlPatterns.dll"
  SetOutPath "$INSTDIR\platforms"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\plugins\platforms\qminimal.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\plugins\platforms\qwindows.dll"
  SetOutPath "$INSTDIR\sqldrivers"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\plugins\sqldrivers\qsqlite.dll"
  SetOutPath "$INSTDIR\printsupport"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\plugins\printsupport\windowsprintersupport.dll"
  SetOutPath "$INSTDIR\imageformats"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\plugins\imageformats\qgif.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\plugins\imageformats\qjpeg.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\plugins\imageformats\qtiff.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\plugins\imageformats\qmng.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\plugins\imageformats\qsvg.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\plugins\imageformats\qtga.dll"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\plugins\imageformats\qwbmp.dll"
  SetOutPath "$INSTDIR\resources\qm"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\translations\qt_??.qm"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\translations\qt_??_??.qm"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\translations\qtbase_??.qm"
  File /NONFATAL "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\translations\qtbase_??_??.qm"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\translations\qtmultimedia_??.qm"
  File /NONFATAL "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\translations\qtmultimedia_??_??.qm"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\translations\qtquick1_??.qm"
  File /NONFATAL "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\translations\qtquick1_??_??.qm"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\translations\qtscript_??.qm"
  File /NONFATAL "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\translations\qtscript_??_??.qm"
  File "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\translations\qtxmlpatterns_??.qm"
  File /NONFATAL "${Qt_Dir}\Qt${Qt_Version}\${Qt_Version}\mingw47_32\translations\qtxmlpatterns_??_??.qm"
  SetOutPath "$INSTDIR"

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
  Delete "$DESKTOP\LibreCAD.lnk"
  RMDir /r "$SMPROGRAMS\LibreCAD\"
  RMDir /r $INSTDIR

  RMDir "$INSTDIR"

  DeleteRegKey /ifempty HKCU "Software\LibreCAD"

SectionEnd


