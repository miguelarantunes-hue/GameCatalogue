; ══════════════════════════════════════════════════════════════════
;  installer\installer.nsi  –  NSIS installer script for Game Catalogue
;  Run via:  installer\package.bat  (from project root)
; ══════════════════════════════════════════════════════════════════

Unicode True

;------------------------------------------------------------------
; General metadata
;------------------------------------------------------------------
!define APP_NAME        "Game Catalogue"
!define APP_EXE         "Game Catalogue.exe"
!define PUBLISHER       "miguelarantunes-hue"
!define REG_KEY         "Software\Microsoft\Windows\CurrentVersion\Uninstall\GameCatalogue"
!define INSTALL_DIR     "$PROGRAMFILES64\Game Catalogue"
!define OUTPUT_FILE     "..\docs\GameCatalogue-Setup-Latest.exe"

Name            "${APP_NAME}"
OutFile         "${OUTPUT_FILE}"
InstallDir      "${INSTALL_DIR}"
InstallDirRegKey HKLM "${REG_KEY}" "InstallLocation"
RequestExecutionLevel admin
SetCompressor   /SOLID lzma

;------------------------------------------------------------------
; Modern UI
;------------------------------------------------------------------
!include "MUI2.nsh"

!define MUI_ABORTWARNING
!define MUI_ICON          "..\res\icon.ico"
!define MUI_UNICON        "..\res\icon.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

;------------------------------------------------------------------
; Version info
;------------------------------------------------------------------
VIProductVersion "1.0.0.0"
VIAddVersionKey "ProductName"      "${APP_NAME}"
VIAddVersionKey "CompanyName"      "${PUBLISHER}"
VIAddVersionKey "FileDescription"  "${APP_NAME} Installer"
VIAddVersionKey "FileVersion"      "1.0.0"
VIAddVersionKey "LegalCopyright"   "© ${PUBLISHER}"

;------------------------------------------------------------------
; Install section
;------------------------------------------------------------------
Section "Install"

    SetOutPath "$INSTDIR"

    ; Main executable
    File "..\${APP_EXE}"

    ; Runtime dependencies
    File "..\SDL2.dll"
    File "..\SDL2_ttf.dll"
    File "..\DejaVuSans.ttf"

    ; Start Menu shortcut
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortcut  "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" \
                    "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0

    ; Desktop shortcut
    CreateShortcut  "$DESKTOP\${APP_NAME}.lnk" \
                    "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0

    ; Write uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; Registry: Add/Remove Programs entry
    WriteRegStr   HKLM "${REG_KEY}" "DisplayName"          "${APP_NAME}"
    WriteRegStr   HKLM "${REG_KEY}" "UninstallString"      "$INSTDIR\Uninstall.exe"
    WriteRegStr   HKLM "${REG_KEY}" "InstallLocation"      "$INSTDIR"
    WriteRegStr   HKLM "${REG_KEY}" "Publisher"            "${PUBLISHER}"
    WriteRegStr   HKLM "${REG_KEY}" "DisplayIcon"          "$INSTDIR\${APP_EXE}"
    WriteRegDWORD HKLM "${REG_KEY}" "NoModify"             1
    WriteRegDWORD HKLM "${REG_KEY}" "NoRepair"             1

SectionEnd

;------------------------------------------------------------------
; Uninstall section
;------------------------------------------------------------------
Section "Uninstall"

    ; Remove installed files
    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\SDL2.dll"
    Delete "$INSTDIR\SDL2_ttf.dll"
    Delete "$INSTDIR\DejaVuSans.ttf"
    Delete "$INSTDIR\Uninstall.exe"

    ; Remove shortcuts
    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    RMDir  "$SMPROGRAMS\${APP_NAME}"
    Delete "$DESKTOP\${APP_NAME}.lnk"

    ; Remove install directory (only if empty after above)
    RMDir  "$INSTDIR"

    ; Remove registry entry
    DeleteRegKey HKLM "${REG_KEY}"

SectionEnd
