;NSIS Modern User Interface
;Updated for MSVC Qt builds + explicit plugin copy to fix DLL loading issues
;Multilingual installer UI + explicit LibreCAD .qm handling with error check
;Recommended: Use windeployqt before running NSIS for automatic plugin deployment
!include "LogicLib.nsh"
!include "FileFunc.nsh"  ; For GetParent if needed

;--------------------------------
;Include custom settings if exists (override Qt paths here)
  !include /NONFATAL "custom.nsh"

;--------------------------------
;Include version information
  !include /NONFATAL "generated_scmrev.nsh"
!ifndef SCMREVISION
    !define SCMREVISION "2.2.1.3"
!endif

;--------------------------------
;Include Modern UI
  !include "MUI2.nsh"
  !include "WinVer.nsh"
  !define MUI_ICON "..\..\librecad\res\main\librecad.ico"
  !define MUI_UNICON "..\..\librecad\res\main\uninstall.ico"
  !define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\LibreCAD"
  !define APPNAME "LibreCAD"

;--------------------------------
;General
  Name "${APPNAME} ${SCMREVISION}"
  OutFile "../../generated/LibreCAD-${SCMREVISION}-win64.exe"  ; Adjust name as needed
  InstallDir "$PROGRAMFILES64\LibreCAD"  ; 64-bit default
  InstallDirRegKey HKCU "Software\LibreCAD" ""
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
  !insertmacro MUI_LANGUAGE "English" ; Default
  !insertmacro MUI_LANGUAGE "French"
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Spanish"
  !insertmacro MUI_LANGUAGE "SpanishInternational"
  !insertmacro MUI_LANGUAGE "SimpChinese"
  !insertmacro MUI_LANGUAGE "TradChinese"
  !insertmacro MUI_LANGUAGE "Japanese"
  !insertmacro MUI_LANGUAGE "Korean"
  !insertmacro MUI_LANGUAGE "Italian"
  !insertmacro MUI_LANGUAGE "Dutch"
  !insertmacro MUI_LANGUAGE "Russian"
  !insertmacro MUI_LANGUAGE "Polish"
  !insertmacro MUI_LANGUAGE "Portuguese"
  !insertmacro MUI_LANGUAGE "PortugueseBR"
  !insertmacro MUI_LANGUAGE "Czech"
  !insertmacro MUI_LANGUAGE "Swedish"
  !insertmacro MUI_LANGUAGE "Finnish"
  !insertmacro MUI_LANGUAGE "Greek"
  !insertmacro MUI_LANGUAGE "Turkish"
  !insertmacro MUI_LANGUAGE "Arabic"

Function .onInit
  ; Admin check (keep for older Windows, optional now)
  userInfo::getAccountType
  Pop $0
  StrCpy $1 $0 5
  ${If} $1 != "Admin"
    MessageBox MB_OK|MB_ICONSTOP "This installer requires Administrator privileges."
    Quit
  ${EndIf}
FunctionEnd

;--------------------------------
; Qt paths for MSVC (override in custom.nsh if needed)
!ifndef Qt_Dir
    !define Qt_Dir "C:\Qt"  ; Common root, adjust if different
!endif
!ifndef Qt_Version
    !define Qt_Version "5.15.2"  ; Recommended for 2.2.x branch
!endif
!ifndef Arch_Suffix
    !define Arch_Suffix "_64"  ; Use "" for 32-bit
!endif
!ifndef MSVC_Ver
    !define MSVC_Ver "msvc2019${Arch_Suffix}"
!endif

!define QT_BIN_DIR "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\bin"
!define PLUGINS_DIR "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\plugins"
!define TRANSLATIONS_DIR "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\translations"

;--------------------------------
;Installer Section
Section "Main Section" SecMain
  SetOutPath "$INSTDIR"

  ; Copy built files (adjust if your build output is elsewhere)
  File /r "..\..\windows\*.*"

  ; === Best option: Use windeployqt (recommended for MSVC) ===
  ; Run this BEFORE compiling NSIS to auto-copy all needed DLLs/plugins to ..\..\windows\
  ; Example command:
  ; "${QT_BIN_DIR}\windeployqt.exe" --release "..\..\windows\LibreCAD.exe"
  ; This handles all plugins, DLLs, and translations automatically.

  ; === Fallback: Explicit critical plugin copies (non-fatal warnings if missing) ===
  SetOutPath "$INSTDIR\platforms"
  File /nonfatal "${PLUGINS_DIR}\platforms\qwindows.dll"
  File /nonfatal "${PLUGINS_DIR}\platforms\qminimal.dll"
  File /nonfatal "${PLUGINS_DIR}\platforms\qoffscreen.dll"

  SetOutPath "$INSTDIR\imageformats"
  File /nonfatal "${PLUGINS_DIR}\imageformats\qgif.dll"
  File /nonfatal "${PLUGINS_DIR}\imageformats\qico.dll"
  File /nonfatal "${PLUGINS_DIR}\imageformats\qjpeg.dll"
  File /nonfatal "${PLUGINS_DIR}\imageformats\qsvg.dll"
  File /nonfatal "${PLUGINS_DIR}\imageformats\qtiff.dll"

  SetOutPath "$INSTDIR\styles"
  File /nonfatal "${PLUGINS_DIR}\styles\qwindowsvistastyle.dll"

  SetOutPath "$INSTDIR\resources\qm"
  File /nonfatal "${TRANSLATIONS_DIR}\qt_*.qm"
  File /nonfatal "${TRANSLATIONS_DIR}\qtbase_*.qm"

  ; === LibreCAD translations (.qm) ===
  SetOutPath "$INSTDIR\ts"
  ClearErrors
  File /r "..\..\windows\ts\*.qm"
  ${If} ${Errors}
    MessageBox MB_OK|MB_ICONSTOP "ERROR: No LibreCAD translation files (.qm) found!$\r$\nRun 'lrelease' on .ts files before building installer."
    Abort
  ${EndIf}

  ; Registry + Shortcuts
  WriteRegStr HKCU "Software\LibreCAD" "" "$INSTDIR"
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  SetShellVarContext all  ; For all-users shortcuts
  CreateDirectory "$SMPROGRAMS\LibreCAD"
  CreateShortCut "$SMPROGRAMS\LibreCAD\LibreCAD.lnk" "$INSTDIR\LibreCAD.exe"
  CreateShortCut "$SMPROGRAMS\LibreCAD\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortCut "$DESKTOP\LibreCAD.lnk" "$INSTDIR\LibreCAD.exe"  ; Optional desktop

  ; Uninstall info
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayName" "LibreCAD ${SCMREVISION}"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayIcon" "$INSTDIR\LibreCAD.exe"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayVersion" "${SCMREVISION}"
  WriteRegStr HKLM "${UNINSTKEY}" "Publisher" "LibreCAD Team"
  WriteRegStr HKLM "${UNINSTKEY}" "URLInfoAbout" "https://librecad.org"
  WriteRegStr HKLM "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${UNINSTKEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKLM "${UNINSTKEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINSTKEY}" "NoRepair" 1
SectionEnd

Section "Uninstall"
  SetShellVarContext all
  Delete "$DESKTOP\LibreCAD.lnk"
  RMDir /r "$SMPROGRAMS\LibreCAD"
  RMDir /r "$INSTDIR"
  DeleteRegKey HKCU "Software\LibreCAD"
  DeleteRegKey HKLM "${UNINSTKEY}"
SectionEnd
