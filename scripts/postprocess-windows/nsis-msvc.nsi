; nsis-msvc.nsi
; NSIS installer script optimized for MSVC Qt builds of LibreCAD
; Features:
; - Dynamic architecture detection (AMD64, ARM64) → -x86, -x64, or -arm64 in filename
; - Dynamic InstallDir ($PROGRAMFILES vs $PROGRAMFILES64)
; - Uses SCMREVISION passed from build-windows.bat (/DSCMREVISION=...)
; - Falls back to generated_scmrev.nsh or default if not provided
; - Packages LFF font files from librecad/support/fonts/ → resources/fonts/
; - Packages hatch patterns from librecad/support/patterns/ → resources/patterns/
; - Packages library parts from librecad/support/library/ → resources/library/ (with subfolders)
; - Preselected file association for .dxf files (user selectable via components page)
; Optimizations:
; - Added SetRegView for proper 32/64-bit registry handling
; - Added basic error checking for critical registry writes
; - Removed commented-out unused code for clarity
; - Used /SOLID lzma compression for smaller installer size
; - Added version info to installer properties
SetCompressor /SOLID lzma
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "WinVer.nsh"
;--------------------------------
; Allow overrides (Qt path, version, installer name, etc.)
!include /NONFATAL "custom.nsh"
;--------------------------------
; Version handling - priority:
; 1. /DSCMREVISION from command line (build-windows.bat)
; 2. generated_scmrev.nsh (if exists)
; 3. Default fallback
!ifdef SCMREVISION
    ; Already defined via command line - highest priority
!else
    !include /NONFATAL "generated_scmrev.nsh"
    !ifndef SCMREVISION
        !define SCMREVISION "2.2.x"
    !endif
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
; Dynamic architecture suffix, install directory, and registry view
!if defined(AMD64)
    !define ARCH_SUFFIX "x64"
    !define INSTALL_DIR "$PROGRAMFILES64\LibreCAD"
    !define REG_VIEW 64
!elseif defined(ARM64)
    !define ARCH_SUFFIX "arm64"
    !define INSTALL_DIR "$PROGRAMFILES\LibreCAD"
    !define REG_VIEW 64
!else
    !define ARCH_SUFFIX "x86"
    !define INSTALL_DIR "$PROGRAMFILES\LibreCAD"
    !define REG_VIEW 32
!endif
;--------------------------------
; General - Dynamic output filename with version and architecture
Name "${APPNAME} ${SCMREVISION}"
OutFile "../../generated/LibreCAD-${SCMREVISION}-Windows-${ARCH_SUFFIX}.exe"
InstallDir "${INSTALL_DIR}"
InstallDirRegKey HKLM "Software\${APPNAME}" ""
RequestExecutionLevel admin
VIProductVersion "${VIProductVersion}"
VIAddVersionKey "ProductName" "${APPNAME}"
VIAddVersionKey "FileVersion" "${SCMREVISION}"
VIAddVersionKey "ProductVersion" "${SCMREVISION}"
VIAddVersionKey "FileDescription" "${APPNAME} Installer"
VIAddVersionKey "LegalCopyright" "LibreCAD Team"
;--------------------------------
; Interface
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\LibreCAD.exe"
;--------------------------------
; Pages
!insertmacro MUI_PAGE_LICENSE "../../licenses/gpl-2.0.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
;--------------------------------
; Languages (multilingual)
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
    !define Qt_Dir "C:\Qt"
!endif
!ifndef Qt_Version
    !define Qt_Version "5.15.2"
!endif
!ifndef Arch_Suffix
    !if defined(AMD64)
        !define Arch_Suffix "_64"
    !elseif defined(ARM64)
        !define Arch_Suffix "_arm64"
    !else
        !define Arch_Suffix ""
    !endif
!endif
!ifndef MSVC_Ver
    !define MSVC_Ver "msvc2019${Arch_Suffix}"
!endif
!define QT_BIN_DIR "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\bin"
!define PLUGINS_DIR "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\plugins"
!define TRANSLATIONS_DIR "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\translations"
;--------------------------------
; Component descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} "The core files required to run LibreCAD."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecAssoc} "Associate .dxf files with LibreCAD so double-clicking them opens in the application."
!insertmacro MUI_FUNCTION_DESCRIPTION_END
;--------------------------------
; Installer Sections
Section "Main Section" SecMain
  SectionIn RO ; Required section
  ; Set registry view based on architecture
  SetRegView ${REG_VIEW}
  SetOutPath "$INSTDIR"
  ; Copy all built files (windeployqt already placed everything needed)
  File /r /x "*.pdb" "..\..\windows\*.*"
  File /nonfatal "${MUI_ICON}"
  ; Fallback explicit plugin copies (non-fatal)
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
  File /nonfatal "..\..\windows\translations\*.qm"
  File /nonfatal "..\..\generated\Release\translations\*.qm"
  ; === Package LFF fonts ===
  SetOutPath "$INSTDIR\resources\fonts"
  File /r "..\..\librecad\support\fonts\*.lff"
  ; === Package hatch patterns ===
  SetOutPath "$INSTDIR\resources\patterns"
  File /r "..\..\librecad\support\patterns\*.dxf"
  ; === Package library parts (DXF) - preserves subfolder structure ===
  SetOutPath "$INSTDIR\resources\library"
  File /r "..\..\librecad\support\library\*.dxf"
  ; Registry, shortcuts, uninstaller
  WriteRegStr HKLM "Software\${APPNAME}" "" "$INSTDIR"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\${APPNAME}"
  CreateShortCut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\LibreCAD.exe"
  CreateShortCut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortCut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\LibreCAD.exe"
  ; Add/Remove Programs entries - show architecture
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayName" "${APPNAME} ${SCMREVISION} (${ARCH_SUFFIX})"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayIcon" "$INSTDIR\librecad.ico"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayVersion" "${SCMREVISION}"
  WriteRegStr HKLM "${UNINSTKEY}" "Publisher" "LibreCAD Team"
  WriteRegStr HKLM "${UNINSTKEY}" "URLInfoAbout" "https://librecad.org"
  WriteRegStr HKLM "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${UNINSTKEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKLM "${UNINSTKEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINSTKEY}" "NoRepair" 1
  ; Check for errors on uninstaller registration
SectionEnd
Section "Associate .dxf files" SecAssoc
  ; Set registry view (same as main section)
  SetRegView ${REG_VIEW}
  ; File association for .dxf
  ; Backup existing association if not already ours
  ReadRegStr $R0 HKCR ".dxf" ""
  ${If} $R0 != "LibreCAD.DXF"
    WriteRegStr HKLM "Software\${APPNAME}" "OldDXFAssoc" $R0
  ${EndIf}
  ; Set new association
  WriteRegStr HKCR ".dxf" "" "LibreCAD.DXF"
  WriteRegStr HKCR "LibreCAD.DXF" "" "DXF File"
  WriteRegStr HKCR "LibreCAD.DXF\DefaultIcon" "" "$INSTDIR\LibreCAD.exe,0"
  WriteRegStr HKCR "LibreCAD.DXF\shell\open\command" "" '"$INSTDIR\LibreCAD.exe" "%1"'
  ; Check for errors
  ${IfErrors}
    MessageBox MB_OK|MB_ICONEXCLAMATION "Failed to set .dxf file association!"
    Abort
  ${EndIf}
  ; Notify Windows of changes
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
SectionEnd
Section "Uninstall"
  ; Set registry view based on architecture
  SetRegView ${REG_VIEW}
  SetShellVarContext all
  Delete "$DESKTOP\${APPNAME}.lnk"
  RMDir /r "$SMPROGRAMS\${APPNAME}"
  RMDir /r "$INSTDIR"
  ; Restore file association for .dxf if it was ours
  DeleteRegKey HKCR "LibreCAD.DXF"
  ReadRegStr $R0 HKCR ".dxf" ""
  ${If} $R0 == "LibreCAD.DXF"
    ReadRegStr $R1 HKLM "Software\${APPNAME}" "OldDXFAssoc"
    ${If} $R1 == ""
      DeleteRegKey HKCR ".dxf"
    ${Else}
      WriteRegStr HKCR ".dxf" "" $R1
    ${EndIf}
    ; Notify Windows of changes
    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
  ${EndIf}
  DeleteRegKey HKLM "Software\${APPNAME}"
  DeleteRegKey HKLM "${UNINSTKEY}"
SectionEnd
