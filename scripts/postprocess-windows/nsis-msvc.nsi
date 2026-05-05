; nsis-msvc.nsi
; NSIS installer script for qmake + MSVC builds of LibreCAD (build-windows.bat)
; - Architecture-aware: x86, x64, ARM64 via /DAMD64 or /DARM64 from build-windows.bat
; - Per-arch registry keys allow x64 and ARM64 to coexist
; - Qt translations from Qt install dir + LibreCAD/plugin .qm from source tree
; - windeployqt output in windows\ is the primary file source
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
!define MUI_ICON  "..\..\librecad\res\images\librecad.ico"
!define MUI_UNICON "..\..\desktop\res_old\main\uninstall.ico"
;--------------------------------
; Architecture suffix, install directory, and registry view
; /DAMD64 or /DARM64 passed by build-windows.bat; default is x86
!if defined(AMD64)
    !define ARCH_SUFFIX "x64"
    !define INSTALL_DIR "$PROGRAMFILES64\LibreCAD"
    !define REG_VIEW 64
!elseif defined(ARM64)
    !define ARCH_SUFFIX "arm64"
    !define INSTALL_DIR "$PROGRAMFILES64\LibreCAD"
    !define REG_VIEW 64
!else
    !define ARCH_SUFFIX "x86"
    !define INSTALL_DIR "$PROGRAMFILES\LibreCAD"
    !define REG_VIEW 32
!endif
; Per-architecture registry paths — allows x64 and ARM64 to coexist
!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\LibreCAD-${ARCH_SUFFIX}"
!define APPREG    "Software\LibreCAD\${ARCH_SUFFIX}"
;--------------------------------
; General - Dynamic output filename with version and architecture
Name "${APPNAME} ${SCMREVISION}"
OutFile "../../generated/LibreCAD-${SCMREVISION}-Windows-${ARCH_SUFFIX}.exe"
InstallDir "${INSTALL_DIR}"
InstallDirRegKey HKLM "${APPREG}" ""
RequestExecutionLevel admin
!ifndef VIProductVersion
    !define VIProductVersion "2.2.2.0"
!endif
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
    !define MSVC_Ver "msvc2022${Arch_Suffix}"
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
  ; Copy all files from windeployqt output
  File /r /x "*.pdb" /x "translations" "..\..\windows\*.*"
  
  ; Ensure application icon is installed (required for shortcuts and Add/Remove Programs)
  File "${MUI_ICON}"
  
  ; Fallback Qt plugin copies (non-fatal; windeployqt should have handled these)
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
  ; Translations — all three sources into resources\qm (where rs_system searches)
  SetOutPath "$INSTDIR\resources\qm"
  File /nonfatal "${TRANSLATIONS_DIR}\qt_*.qm"
  File /nonfatal "${TRANSLATIONS_DIR}\qtbase_*.qm"
  File /nonfatal "..\..\librecad\ts\*.qm"
  File /nonfatal "..\..\plugins\ts\*.qm"
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
  WriteRegStr HKLM "${APPREG}" "" "$INSTDIR"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\${APPNAME}"
  CreateShortCut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\LibreCAD.exe" "" "$INSTDIR\librecad.ico"
  CreateShortCut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe" "" "${MUI_UNICON}"
  CreateShortCut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\LibreCAD.exe" "" "$INSTDIR\librecad.ico"
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
    WriteRegStr HKLM "${APPREG}" "OldDXFAssoc" $R0
  ${EndIf}
  ClearErrors
  ; Set new association
  WriteRegStr HKCR ".dxf" "" "LibreCAD.DXF"
  WriteRegStr HKCR "LibreCAD.DXF" "" "DXF File"
  WriteRegStr HKCR "LibreCAD.DXF\DefaultIcon" "" "$INSTDIR\LibreCAD.exe,0"
  WriteRegStr HKCR "LibreCAD.DXF\shell\open\command" "" '"$INSTDIR\LibreCAD.exe" "%1"'
  ; Check for errors
  ${If} ${Errors}
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
    ReadRegStr $R1 HKLM "${APPREG}" "OldDXFAssoc"
    ${If} $R1 == ""
      DeleteRegKey HKCR ".dxf"
    ${Else}
      WriteRegStr HKCR ".dxf" "" $R1
    ${EndIf}
    ; Notify Windows of changes
    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
  ${EndIf}
  DeleteRegKey HKLM "${APPREG}"
  DeleteRegKey HKLM "${UNINSTKEY}"
SectionEnd
