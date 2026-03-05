; ══════════════════════════════════════════════════════════════════
;  Game Catalogue – NSIS Installer Script
;
;  Lives in:  GameCatalogue\installer\installer.nsi
;  Run via:   installer\package.bat  (handles cd to project root)
; ══════════════════════════════════════════════════════════════════

Unicode True

;------------------------------------------------------------------
; Metadata
;------------------------------------------------------------------
!define APP_NAME      "Game Catalogue"
!define APP_EXE       "Game Catalogue.exe"
!define APP_VERSION   "5.0"
!define APP_PUBLISHER "Your Name"
!define REG_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\GameCatalogue"

;------------------------------------------------------------------
; Modern UI
;------------------------------------------------------------------
!include "MUI2.nsh"

!define MUI_ABORTWARNING
!define MUI_ICON   "..\res\icon.ico"
!define MUI_UNICON "..\res\icon.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

;------------------------------------------------------------------
; Output settings
;------------------------------------------------------------------
Name             "${APP_NAME} ${APP_VERSION}"
OutFile          "..\docs\GameCatalogue-Setup-Latest.exe"
InstallDir       "$PROGRAMFILES64\${APP_NAME}"
InstallDirRegKey HKLM "${REG_KEY}" "InstallLocation"
RequestExecutionLevel admin
ShowInstDetails  show

;------------------------------------------------------------------
; Install
;------------------------------------------------------------------
Section "Game Catalogue" SEC_MAIN
    SectionIn RO

    SetOutPath "$INSTDIR"

    ; Main executable (icon + version info baked in via windres)
    File "..\${APP_EXE}"

    ; SDL2 runtime DLLs
    File "..\SDL2.dll"
    File "..\SDL2_ttf.dll"

    ; Custom font
    File "..\DejaVuSans.ttf"

    ; Uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; Add / Remove Programs registry entry
    WriteRegStr   HKLM "${REG_KEY}" "DisplayName"     "${APP_NAME}"
    WriteRegStr   HKLM "${REG_KEY}" "DisplayVersion"  "${APP_VERSION}"
    WriteRegStr   HKLM "${REG_KEY}" "Publisher"       "${APP_PUBLISHER}"
    WriteRegStr   HKLM "${REG_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr   HKLM "${REG_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegDWORD HKLM "${REG_KEY}" "NoModify"        1
    WriteRegDWORD HKLM "${REG_KEY}" "NoRepair"        1

SectionEnd

;------------------------------------------------------------------
; Start Menu shortcut
;------------------------------------------------------------------
Section "Start Menu Shortcut" SEC_SM
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortcut  "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" \
                    "$INSTDIR\${APP_EXE}"
    CreateShortcut  "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk" \
                    "$INSTDIR\Uninstall.exe"
SectionEnd

;------------------------------------------------------------------
; Desktop shortcut (opt-in)
;------------------------------------------------------------------
Section /o "Desktop Shortcut" SEC_DT
    CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
SectionEnd

;------------------------------------------------------------------
; Uninstall
;------------------------------------------------------------------
Section "Uninstall"

    Delete "$INSTDIR\Game Catalogue.exe"
    Delete "$INSTDIR\SDL2.dll"
    Delete "$INSTDIR\SDL2_ttf.dll"
    Delete "$INSTDIR\TeknafRegular.ttf"
    Delete "$INSTDIR\Uninstall.exe"

    ; Ask whether to delete save data
    IfFileExists "$INSTDIR\catalogue_save.bin" ask_save cleanup
    ask_save:
        MessageBox MB_YESNO "Delete your saved catalogue data (catalogue_save.bin)?" \
            IDNO cleanup
        Delete "$INSTDIR\catalogue_save.bin"
    cleanup:
        RMDir  "$INSTDIR"

    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk"
    RMDir  "$SMPROGRAMS\${APP_NAME}"
    Delete "$DESKTOP\${APP_NAME}.lnk"

    DeleteRegKey HKLM "${REG_KEY}"

SectionEnd
