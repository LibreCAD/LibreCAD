;NSIS Modern User Interface
;Updated for MSVC support in GitHub Actions CI (works with both MinGW and MSVC builds)

;--------------------------------
;Include custom settings if exists (e.g., custom-win.nsh or custom-win-x64.nsh copied by CI)
  !include /NONFATAL "custom.nsh"

;--------------------------------
;Include version information
  !include /NONFATAL "generated_scmrev.nsh"
!ifndef SCMREVISION
    !define SCMREVISION "2.2.x"
!endif

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"
  !include "WinVer.nsh"

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
  OutFile "../../generated/${InstallerName}.exe"

  ;Default installation folder
  InstallDir "${ProgramsFolder}\LibreCAD"

  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\${AppKeyName}" ""

  ;Request application privileges for Windows Vista and later
  RequestExecutionLevel admin

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

  ; Get account type
  userInfo::getAccountType
  Pop $0
  StrCpy $R2 $0 5

  ${If} ${IsWin2000}
    StrCmp $R2 "Admin" lbl_checkok
    MessageBox MB_OK "Sorry, this installer requires Administrator privileges. Please log in as an administrator."
    Quit
  ${EndIf}

  ${If} ${IsWinXP}
    StrCmp $R2 "Admin" lbl_checkok
    MessageBox MB_OK "Sorry, this installer requires Administrator privileges. Please log in as an administrator."
    Quit
  ${EndIf}

  lbl_checkok:
  Pop $R2
  Pop $R1
  Pop $R0

FunctionEnd

;--------------------------------
; Dynamic Qt path detection for GitHub Actions CI (MSVC builds)
; In local builds, these can be overridden in custom.nsh if desired

!ifndef Qt_Dir
    !define Qt_Dir "..\\Qt"
!endif
!ifndef Qt_Version
    !define Qt_Version "5.15.2"
!endif

; Detect architecture and MSVC variant (set by CI workflow)
!ifndef Arch_Suffix
    !ifdef WIN64
        !define Arch_Suffix "_64"
    !else
        !define Arch_Suffix ""
    !endif
!endif
!ifndef MSVC_Ver
    !define MSVC_Ver "msvc2019${Arch_Suffix}"
!endif

; Qt binary, plugin, and translation directories (same structure for MSVC and MinGW Qt packages)
!define QT_BIN_DIR     "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\bin"
!define PLUGINS_DIR    "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\plugins"
!define TRANSLATIONS_DIR "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\translations"

;--------------------------------
;Installer Sections

Section "Install Section" SecInstall

  SetOutPath "$INSTDIR"
  ; Copy all built files from the windows/ directory (populated by windeployqt)
  File /r "..\..\windows\*.*"

  ; Copy Qt translation files if available
  SetOutPath "$INSTDIR\resources\qm"
  File /NONFATAL "${TRANSLATIONS_DIR}\qt_*.qm"
  File /NONFATAL "${TRANSLATIONS_DIR}\qtbase_*.qm"

  SetOutPath "$INSTDIR"

  ; Store installation folder
  WriteRegStr HKCU "Software\LibreCAD" "" $INSTDIR

  ; Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Desktop shortcut
  CreateShortCut "$DESKTOP\LibreCAD.lnk" "$INSTDIR\LibreCAD.exe"

  ; Start Menu shortcuts
  CreateDirectory "$SMPROGRAMS\LibreCAD"
  CreateShortCut "$SMPROGRAMS\LibreCAD\LibreCAD.lnk" "$INSTDIR\LibreCAD.exe"
  CreateShortCut "$SMPROGRAMS\LibreCAD\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

  ; Add/Remove Programs entry
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayName" "${APPNAME}"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayIcon" "$INSTDIR\LibreCAD.exe"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayVersion" "${SCMREVISION}"
  WriteRegStr HKLM "${UNINSTKEY}" "Publisher" "LibreCAD Team"
  WriteRegStr HKLM "${UNINSTKEY}" "Version" "2.2.1"
  WriteRegStr HKLM "${UNINSTKEY}" "HelpLink" "https://librecad.org/"
  WriteRegStr HKLM "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${UNINSTKEY}" "URLInfoAbout" "https://librecad.org/"
  WriteRegStr HKLM "${UNINSTKEY}" "Comments" "LibreCAD - Open Source 2D-CAD"
  WriteRegStr HKLM "${UNINSTKEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKLM "${UNINSTKEY}" "VersionMajor" 2
  WriteRegDWORD HKLM "${UNINSTKEY}" "VersionMinor" 2
  WriteRegDWORD HKLM "${UNINSTKEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINSTKEY}" "NoRepair" 1

  ; Optional: Open donation page (skip if silent install)
  IfSilent +2
    Exec "rundll32.exe url.dll,FileProtocolHandler https://librecad.org/donate.html"

SectionEnd

;--------------------------------
;Descriptions (optional)

  LangString DESC_SecInstall ${LANG_ENGLISH} "Main LibreCAD application files."

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  Delete "$INSTDIR\Uninstall.exe"
  Delete "$DESKTOP\LibreCAD.lnk"
  RMDir /r "$SMPROGRAMS\LibreCAD"
  RMDir /r "$INSTDIR"

  ; Remove empty parent directory if exists
  RMDir "$INSTDIR"

  DeleteRegKey /ifempty HKCU "Software\${AppKeyName}"
  DeleteRegKey HKLM "${UNINSTKEY}"

SectionEnd