;NSIS Modern User Interface
;Basic Example Script

;--------------------------------
;Include custom settings if exists
  !include /NONFATAL "custom.nsh"

;--------------------------------
;Include version information
  !include /NONFATAL "generated_scmrev.nsh"
!ifndef SCMREVISION
    !define SCMREVISION "2.0.x"
!endif

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"
  !include "WinVer.nsh"

  !define APPNAME "LibreCAD"
  !define MUI_ICON "..\..\librecad\res\main\librecad.ico"
  !define MUI_UNICON "..\..\librecad\res\main\uninstall.ico"

  !define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

;--------------------------------
;General

  ;Name and file
  Name "${APPNAME}"
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

;--- define Qt folders if not already defined in custom-5.1.nsh
!ifndef Qt_Dir
    !define Qt_Dir 	"C:\Qt\Qt5.2.0"
!endif
!ifndef Qt_Version
    !define Qt_Version 	"5.2.0"
!endif
!ifndef Mingw_Ver
    !define Mingw_Ver 	"mingw48_32"
!endif
;--- folder contains mingw32-make.exe
!define MINGW_DIR 	"${Qt_Dir}\Tools\${Mingw_Ver}\bin"
!define QTCREATOR_DIR 	"${Qt_Dir}\Tools\QtCreator\bin"
!define QTMINGW_DIR 	"${Qt_Dir}\${Qt_Version}\${Mingw_Ver}"
;--- folder contains qmake.exe
!define QMAKE_DIR 	"${QTMINGW_DIR}\bin"
!define PLUGINS_DIR 	"${QTMINGW_DIR}\plugins"
!define TRANSLATIONS_DIR	"${QTMINGW_DIR}\translations"

;--------------------------------
;Installer Sections

Section "Install Section" SecInstall
  SetOutPath "$INSTDIR"
  File /r "..\..\windows\*.*"
  File "${MINGW_DIR}\libgcc_s_dw2-1.dll"
  File "${QMAKE_DIR}\icu*.dll"
  File "${MINGW_DIR}\libwinpthread-1.dll"
  File "${QMAKE_DIR}\libstdc++-6.dll"
  File "${QTCREATOR_DIR}\libEGL.dll"
  File "${QTCREATOR_DIR}\libGLESv2.dll"
  File "${QTCREATOR_DIR}\D3DCompiler_43.dll"
  File "${QMAKE_DIR}\Qt5Clucene.dll"
  File "${QMAKE_DIR}\Qt5Core.dll"
  File "${QMAKE_DIR}\Qt5Declarative.dll"
  File "${QMAKE_DIR}\Qt5Gui.dll"
  File "${QMAKE_DIR}\Qt5Help.dll"
  File "${QMAKE_DIR}\Qt5Network.dll"
  File "${QMAKE_DIR}\Qt5OpenGL.dll"
  File "${QMAKE_DIR}\Qt5PrintSupport.dll"
  File "${QMAKE_DIR}\Qt5Script.dll"
  File "${QMAKE_DIR}\Qt5Svg.dll"
  File "${QMAKE_DIR}\Qt5Sql.dll"
  File "${QMAKE_DIR}\Qt5Widgets.dll"
  File "${QMAKE_DIR}\Qt5Xml.dll"
  File "${QMAKE_DIR}\Qt5XmlPatterns.dll"
  SetOutPath "$INSTDIR\platforms"
  File "${PLUGINS_DIR}\platforms\qminimal.dll"
  File "${PLUGINS_DIR}\platforms\qwindows.dll"
  SetOutPath "$INSTDIR\sqldrivers"
  File "${PLUGINS_DIR}\sqldrivers\qsqlite.dll"
  SetOutPath "$INSTDIR\printsupport"
  File "${PLUGINS_DIR}\printsupport\windowsprintersupport.dll"
  SetOutPath "$INSTDIR\imageformats"
  File "${PLUGINS_DIR}\imageformats\qgif.dll"
  File "${PLUGINS_DIR}\imageformats\qjpeg.dll"
  File "${PLUGINS_DIR}\imageformats\qtiff.dll"
  File "${PLUGINS_DIR}\imageformats\qmng.dll"
  File "${PLUGINS_DIR}\imageformats\qsvg.dll"
  File "${PLUGINS_DIR}\imageformats\qtga.dll"
  File "${PLUGINS_DIR}\imageformats\qwbmp.dll"
  SetOutPath "$INSTDIR\resources\qm"
  File "${TRANSLATIONS_DIR}\qt_??.qm"
  File "${TRANSLATIONS_DIR}\qt_??_??.qm"
  File "${TRANSLATIONS_DIR}\qtbase_??.qm"
  File /NONFATAL "${TRANSLATIONS_DIR}\qtbase_??_??.qm"
  File "${TRANSLATIONS_DIR}\qtmultimedia_??.qm"
  File /NONFATAL "${TRANSLATIONS_DIR}\qtmultimedia_??_??.qm"
  File "${TRANSLATIONS_DIR}\qtquick1_??.qm"
  File /NONFATAL "${TRANSLATIONS_DIR}\qtquick1_??_??.qm"
  File "${TRANSLATIONS_DIR}\qtscript_??.qm"
  File /NONFATAL "${TRANSLATIONS_DIR}\qtscript_??_??.qm"
  File "${TRANSLATIONS_DIR}\qtxmlpatterns_??.qm"
  File /NONFATAL "${TRANSLATIONS_DIR}\qtxmlpatterns_??_??.qm"
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

  ; create add/remove software entries
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayName" "${APPNAME}"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayIcon" "$INSTDIR\LibreCAD.exe"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayVersion" "${SCMREVISION}"
  WriteRegStr HKLM "${UNINSTKEY}" "Publisher" "LibreCAD Team"
  WriteRegStr HKLM "${UNINSTKEY}" "Version" "2.0"
  WriteRegStr HKLM "${UNINSTKEY}" "HelpLink" "https://librecad.org/"
  WriteRegStr HKLM "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${UNINSTKEY}" "URLInfoAbout" "http://librecad.org/"
  WriteRegStr HKLM "${UNINSTKEY}" "Comments" "LibreCAD - Open Source 2D-CAD"
  WriteRegStr HKLM "${UNINSTKEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKLM "${UNINSTKEY}" "VersionMinor" "0"
  WriteRegDWORD HKLM "${UNINSTKEY}" "VersionMajor" "2"
  WriteRegDWORD HKLM "${UNINSTKEY}" "NoModify" "1"
  WriteRegDWORD HKLM "${UNINSTKEY}" "NoRepair" "1"

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
  DeleteRegKey HKLM "${UNINSTKEY}"

SectionEnd


