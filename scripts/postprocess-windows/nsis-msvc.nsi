; nsis-msvc.nsi
; NSIS installer script optimized for MSVC Qt builds of LibreCAD
; Features:
;   - Works reliably with MSVC-compiled LibreCAD
;   - Explicit (non-fatal) plugin copies as fallback
;   - Strong recommendation to use windeployqt (best practice)
;   - Robust .qm translation check
;   - All-users shortcuts
;   - Easy override via custom.nsh

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "WinVer.nsh"

;--------------------------------
; Allow overrides (Qt path, version, installer name, etc.)
!include /NONFATAL "custom.nsh"

;--------------------------------
; Version fallback
!include /NONFATAL "generated_scmrev.nsh"
!ifndef SCMREVISION
    !define SCMREVISION "2.2.1"
!endif

;--------------------------------
; Basic definitions
!ifndef APPNAME
    !define APPNAME "LibreCAD"
!endif
!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

!define MUI_ICON "..\..\librecad\res\main\librecad.ico"
!define MUI_UNICON "..\..\librecad\res\main\uninstall.ico"

;--------------------------------
; General
Name "${APPNAME} ${SCMREVISION}"
OutFile "../../generated/LibreCAD-${SCMREVISION}-Windows-x64.exe"
InstallDir "$PROGRAMFILES64\LibreCAD"
InstallDirRegKey HKCU "Software\${APPNAME}" ""
RequestExecutionLevel admin

;--------------------------------
; Interface
!define MUI_ABORTWARNING

;--------------------------------
; Pages
!insertmacro MUI_PAGE_LICENSE "../../licenses/gpl-2.0.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
; Languages
!insertmacro MUI_LANGUAGE "English"
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

;--------------------------------
; Qt paths (override in custom.nsh if needed)
!ifndef Qt_Dir
    !define Qt_Dir "C:\Qt"               ; Common root
!endif
!ifndef Qt_Version
    !define Qt_Version "5.15.2"          ; Recommended for 2.2.x
!endif
!ifndef Arch_Suffix
    !define Arch_Suffix "_64"
!endif
!ifndef MSVC_Ver
    !define MSVC_Ver "msvc2019${Arch_Suffix}"
!endif

!define QT_BIN_DIR "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\bin"
!define PLUGINS_DIR "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\plugins"
!define TRANSLATIONS_DIR "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\translations"

;--------------------------------
; Installer Section
Section "Main Section" SecMain
    SetOutPath "$INSTDIR"

    ; Copy the built application files
    File /r "..\..\windows\*.*"

    ; ==================================================================
    ; BEST PRACTICE: Run windeployqt BEFORE compiling this installer
    ; Example:
    ;   "${QT_BIN_DIR}\windeployqt.exe" --release "..\..\windows\LibreCAD.exe"
    ; This automatically copies all required DLLs, plugins, and Qt translations.
    ; ==================================================================

    ; Fallback: explicit copy of critical plugins (non-fatal if missing)
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

    ; LibreCAD translations (.qm files)
    SetOutPath "$INSTDIR\ts"
    ClearErrors
    File /r "..\..\windows\translations\*.qm"
    ${If} ${Errors}
        MessageBox MB_OK|MB_ICONSTOP "ERROR: No LibreCAD translation files (.qm) found in build output!$\r$\n$\r$\nRun 'lrelease' on the .ts files before building the installer.$\r$\n$\r$\nInstallation cannot continue."
        Abort
    ${EndIf}

    ; Registry, shortcuts, uninstaller
    WriteRegStr HKCU "Software\${APPNAME}" "" "$INSTDIR"
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    SetShellVarContext all
    CreateDirectory "$SMPROGRAMS\${APPNAME}"
    CreateShortCut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\LibreCAD.exe"
    CreateShortCut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
    CreateShortCut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\LibreCAD.exe"

    ; Add/Remove Programs entries
    WriteRegStr HKLM "${UNINSTKEY}" "DisplayName" "${APPNAME} ${SCMREVISION}"
    WriteRegStr HKLM "${UNINSTKEY}" "DisplayIcon" "$INSTDIR\LibreCAD.exe"
    WriteRegStr HKLM "${UNINSTKEY}" "DisplayVersion" "${SCMREVISION}"
    WriteRegStr HKLM "${UNINSTKEY}" "Publisher" "LibreCAD Team"
    WriteRegStr HKLM "${UNINSTKEY}" "URLInfoAbout" "https://librecad.org"
    WriteRegStr HKLM "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "${UNINSTKEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegDWORD HKLM "${UNINSTKEY}" "NoModify" 1
    WriteRegDWORD HKLM "${UNINSTKEY}" "NoRepair" 1
SectionEnd

;--------------------------------
Section "Uninstall"
    SetShellVarContext all

    Delete "$DESKTOP\${APPNAME}.lnk"
    RMDir /r "$SMPROGRAMS\${APPNAME}"
    RMDir /r "$INSTDIR"

    DeleteRegKey HKCU "Software\${APPNAME}"
    DeleteRegKey HKLM "${UNINSTKEY}"
SectionEnd
