;NSIS Modern User Interface
;Final version with explicit plugin copy to fix DLL loading issues
;Updated: Multilingual installer UI + explicit LibreCAD .qm handling with error check

!include "LogicLib.nsh"  ; Required for existence checks

;--------------------------------
;Include custom settings if exists
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

  !define MUI_LICENSEPAGE_BUTTON $(^NextBtn)
  !define MUI_LICENSEPAGE_TEXT_BOTTOM "You are now aware of your rights. Click Next to continue."

;--------------------------------
;General
  Name "${APPNAME}"
  OutFile "../../generated/${InstallerName}.exe"
  InstallDir "${ProgramsFolder}\LibreCAD"
  InstallDirRegKey HKCU "Software\${AppKeyName}" ""
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
;Languages (Multilingual support with auto-detection)
  !insertmacro MUI_LANGUAGE "English"     ; Default/fallback
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
  !insertmacro MUI_LANGUAGE "Arabic"      ; RTL support

Function .onInit
  Push $R0
  Push $R1
  Push $R2
  userInfo::getAccountType
  Pop $0
  StrCpy $R2 $0 5
  ${If} ${IsWin2000}
    StrCmp $R2 "Admin" +3
    MessageBox MB_OK "Sorry, this installer requires Administrator privileges."
    Quit
  ${EndIf}
  ${If} ${IsWinXP}
    StrCmp $R2 "Admin" +3
    MessageBox MB_OK "Sorry, this installer requires Administrator privileges."
    Quit
  ${EndIf}
  Pop $R2
  Pop $R1
  Pop $R0
FunctionEnd

;--------------------------------
; Dynamic Qt paths for CI (MSVC)
!ifndef Qt_Dir
    !define Qt_Dir "..\\Qt"
!endif
!ifndef Qt_Version
    !define Qt_Version "5.15.2"
!endif
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

!define QT_BIN_DIR          "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\bin"
!define PLUGINS_DIR         "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\plugins"
!define TRANSLATIONS_DIR    "${Qt_Dir}\${Qt_Version}\${MSVC_Ver}\translations"

;--------------------------------
;Installer Sections
Section "Install Section" SecInstall

  SetOutPath "$INSTDIR"
  File /r "..\..\windows\*.*"

  ; Explicit critical plugin copies
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

  ;--- Explicit LibreCAD application translation files (.qm) ---
  SetOutPath "$INSTDIR\ts"

  ClearErrors
  FindFirst $0 $1 "..\..\windows\ts\librecad_*.qm"
  ${If} ${Errors}
    MessageBox MB_OK|MB_ICONSTOP "ERROR: No LibreCAD translation files (.qm) found in build output!$\r$\n\
      The lrelease step may have failed or been skipped.$\r$\n\
      Installation cannot continue."
    Abort "Missing LibreCAD translation files"
  ${EndIf}
  FindClose $0

  File /r "..\..\windows\ts\*.qm"

  SetOutPath "$INSTDIR"

  WriteRegStr HKCU "Software\LibreCAD" "" $INSTDIR
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  CreateShortCut "$DESKTOP\LibreCAD.lnk" "$INSTDIR\LibreCAD.exe"
  CreateDirectory "$SMPROGRAMS\LibreCAD"
  CreateShortCut "$SMPROGRAMS\LibreCAD\LibreCAD.lnk" "$INSTDIR\LibreCAD.exe"
  CreateShortCut "$SMPROGRAMS\LibreCAD\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

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

  IfSilent +2
    Exec "rundll32.exe url.dll,FileProtocolHandler https://librecad.org/donate.html"

SectionEnd

LangString DESC_SecInstall ${LANG_ENGLISH} "Main LibreCAD application files."

Section "Uninstall"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$DESKTOP\LibreCAD.lnk"
  RMDir /r "$SMPROGRAMS\LibreCAD"
  RMDir /r "$INSTDIR"
  RMDir "$INSTDIR"
  DeleteRegKey /ifempty HKCU "Software\${AppKeyName}"
  DeleteRegKey HKLM "${UNINSTKEY}"
SectionEnd