!include "MUI2.nsh"

; --- General ---
Name "qBittorrent Vanced"
OutFile "qBittorrent-Vanced-v1.0.0-x64-setup.exe"
InstallDir "$PROGRAMFILES64\qBittorrent Vanced"
InstallDirRegKey HKLM "Software\qBittorrent Vanced" "InstallLocation"
RequestExecutionLevel admin
Unicode True

; --- Version Info ---
VIProductVersion "5.1.3.0"
VIAddVersionKey "ProductName" "qBittorrent Vanced"
VIAddVersionKey "FileDescription" "qBittorrent Vanced Installer"
VIAddVersionKey "FileVersion" "1.0.0"
VIAddVersionKey "ProductVersion" "1.0.0"
VIAddVersionKey "LegalCopyright" "GPL-2.0"

; --- Interface ---
!define MUI_ABORTWARNING
!define MUI_ICON "src\icons\qbittorrent.ico"
!define MUI_UNICON "src\icons\qbittorrent.ico"

; --- Pages ---
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\qbittorrent.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch qBittorrent Vanced"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; --- Install Section ---
Section "Install"
    SetOutPath "$INSTDIR"

    ; Main exe and DLLs
    File "dist_package\qbittorrent.exe"
    File "dist_package\*.dll"

    ; Qt plugin directories
    SetOutPath "$INSTDIR\generic"
    File "dist_package\generic\*.*"

    SetOutPath "$INSTDIR\iconengines"
    File "dist_package\iconengines\*.*"

    SetOutPath "$INSTDIR\imageformats"
    File "dist_package\imageformats\*.*"

    SetOutPath "$INSTDIR\networkinformation"
    File "dist_package\networkinformation\*.*"

    SetOutPath "$INSTDIR\platforms"
    File "dist_package\platforms\*.*"

    SetOutPath "$INSTDIR\sqldrivers"
    File "dist_package\sqldrivers\*.*"

    SetOutPath "$INSTDIR\styles"
    File "dist_package\styles\*.*"

    SetOutPath "$INSTDIR\tls"
    File "dist_package\tls\*.*"

    ; Back to root for shortcuts
    SetOutPath "$INSTDIR"

    ; Registry
    WriteRegStr HKLM "Software\qBittorrent Vanced" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "DisplayName" "qBittorrent Vanced"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "DisplayIcon" "$INSTDIR\qbittorrent.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "Publisher" "qBittorrent Vanced"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "DisplayVersion" "1.0.0"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "NoRepair" 1

    ; Start Menu
    CreateDirectory "$SMPROGRAMS\qBittorrent Vanced"
    CreateShortcut "$SMPROGRAMS\qBittorrent Vanced\qBittorrent Vanced.lnk" "$INSTDIR\qbittorrent.exe"
    CreateShortcut "$SMPROGRAMS\qBittorrent Vanced\Uninstall.lnk" "$INSTDIR\uninstall.exe"

    ; Desktop shortcut
    CreateShortcut "$DESKTOP\qBittorrent Vanced.lnk" "$INSTDIR\qbittorrent.exe"

    ; Uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

; --- Uninstall Section ---
Section "Uninstall"
    ; Kill running instance
    nsExec::ExecToLog 'taskkill /f /im qbittorrent.exe'

    ; Remove files
    RMDir /r "$INSTDIR\generic"
    RMDir /r "$INSTDIR\iconengines"
    RMDir /r "$INSTDIR\imageformats"
    RMDir /r "$INSTDIR\networkinformation"
    RMDir /r "$INSTDIR\platforms"
    RMDir /r "$INSTDIR\sqldrivers"
    RMDir /r "$INSTDIR\styles"
    RMDir /r "$INSTDIR\tls"
    Delete "$INSTDIR\*.dll"
    Delete "$INSTDIR\qbittorrent.exe"
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"

    ; Shortcuts
    Delete "$SMPROGRAMS\qBittorrent Vanced\qBittorrent Vanced.lnk"
    Delete "$SMPROGRAMS\qBittorrent Vanced\Uninstall.lnk"
    RMDir "$SMPROGRAMS\qBittorrent Vanced"
    Delete "$DESKTOP\qBittorrent Vanced.lnk"

    ; Registry
    DeleteRegKey HKLM "Software\qBittorrent Vanced"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced"
SectionEnd
