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

  ; GPL is not an EULA, no need to agree to it.
  !define MUI_LICENSEPAGE_BUTTON $(^NextBtn)
  !define MUI_LICENSEPAGE_TEXT_BOTTOM "You are now aware of your rights. Click Next to continue."

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

;--- define Qt folders if not already defined in custom-5.3.nsi
!ifndef Qt_Dir
    !define Qt_Dir 	"C:\Qt\Qt5.4.0"
!endif
!ifndef Qt_Version
    !define Qt_Version 	"5.4"
!endif
!ifndef Mingw_Ver
    !define Mingw_Ver 	"mingw491_32"
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
  SetOutPath "$INSTDIR\resources\qm"
  File /NONFATAL "${TRANSLATIONS_DIR}\qt*.qm"
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


