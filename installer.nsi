!include "MUI2.nsh"

; --- Shell notification constants for file association refresh ---
!define SHCNE_ASSOCCHANGED 0x8000000
!define SHCNF_IDLIST 0

; --- Version defines (override via makensis /DVANCED_VERSION=x.y.z /DBASE_VERSION=a.b.c.d) ---
!ifndef VANCED_VERSION
  !define VANCED_VERSION "1.0.1"
!endif
!ifndef BASE_VERSION
  !define BASE_VERSION "5.1.3.10"
!endif

; --- General ---
Name "qBittorrent Vanced"
OutFile "qBittorrent-Vanced-v${VANCED_VERSION}-base${BASE_VERSION}-x64-setup.exe"
InstallDir "$PROGRAMFILES64\qBittorrent Vanced"
InstallDirRegKey HKLM "Software\qBittorrent Vanced" "InstallLocation"
RequestExecutionLevel admin
Unicode True

; --- Version Info ---
VIProductVersion "${BASE_VERSION}"
VIAddVersionKey "ProductName" "qBittorrent Vanced"
VIAddVersionKey "FileDescription" "qBittorrent Vanced Installer"
VIAddVersionKey "FileVersion" "${VANCED_VERSION}"
VIAddVersionKey "ProductVersion" "${VANCED_VERSION} (base: Enhanced Edition ${BASE_VERSION})"
VIAddVersionKey "LegalCopyright" "Copyright 2006-2026 The qBittorrent project, GPLv2+"

; --- Interface ---
!define MUI_ABORTWARNING
!define MUI_ICON "src\icons\qbittorrent.ico"
!define MUI_UNICON "src\icons\qbittorrent.ico"

; --- Pages ---
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "COPYING"
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
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "DisplayVersion" "${VANCED_VERSION} (${BASE_VERSION})"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "NoRepair" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "EstimatedSize" 150000

    ; Register as handler for .torrent files and magnet: links
    WriteRegStr HKLM "Software\qBittorrent Vanced\Capabilities" "ApplicationDescription" "qBittorrent Vanced - A Customized BitTorrent Client"
    WriteRegStr HKLM "Software\qBittorrent Vanced\Capabilities" "ApplicationName" "qBittorrent Vanced"
    WriteRegStr HKLM "Software\qBittorrent Vanced\Capabilities\FileAssociations" ".torrent" "qBittorrentVanced.File.Torrent"
    WriteRegStr HKLM "Software\qBittorrent Vanced\Capabilities\UrlAssociations" "magnet" "qBittorrentVanced.Url.Magnet"
    WriteRegStr HKLM "Software\RegisteredApplications" "qBittorrent Vanced" "Software\qBittorrent Vanced\Capabilities"

    ; ProgID for .torrent files
    WriteRegStr HKLM "Software\Classes\qBittorrentVanced.File.Torrent" "" "Torrent File"
    WriteRegStr HKLM "Software\Classes\qBittorrentVanced.File.Torrent\DefaultIcon" "" '"$INSTDIR\qbittorrent.exe",1'
    WriteRegStr HKLM "Software\Classes\qBittorrentVanced.File.Torrent\shell\open\command" "" '"$INSTDIR\qbittorrent.exe" "%1"'

    ; ProgID for magnet: protocol
    WriteRegStr HKLM "Software\Classes\qBittorrentVanced.Url.Magnet" "" "Magnet URI"
    WriteRegStr HKLM "Software\Classes\qBittorrentVanced.Url.Magnet\DefaultIcon" "" '"$INSTDIR\qbittorrent.exe",1'
    WriteRegStr HKLM "Software\Classes\qBittorrentVanced.Url.Magnet\shell\open\command" "" '"$INSTDIR\qbittorrent.exe" "%1"'

    ; .torrent file type
    WriteRegStr HKLM "Software\Classes\.torrent" "" "qBittorrentVanced.File.Torrent"
    WriteRegStr HKLM "Software\Classes\.torrent" "Content Type" "application/x-bittorrent"

    ; magnet: protocol
    WriteRegStr HKLM "Software\Classes\magnet" "" "URL:Magnet URI"
    WriteRegStr HKLM "Software\Classes\magnet" "Content Type" "application/x-magnet"
    WriteRegStr HKLM "Software\Classes\magnet" "URL Protocol" ""
    WriteRegStr HKLM "Software\Classes\magnet\DefaultIcon" "" '"$INSTDIR\qbittorrent.exe",1'
    WriteRegStr HKLM "Software\Classes\magnet\shell\open\command" "" '"$INSTDIR\qbittorrent.exe" "%1"'

    ; Notify Windows shell of association changes
    System::Call 'Shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, p 0, p 0)'

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

    ; File associations — ProgIDs
    DeleteRegKey HKLM "Software\Classes\qBittorrentVanced.File.Torrent"
    DeleteRegKey HKLM "Software\Classes\qBittorrentVanced.Url.Magnet"
    DeleteRegValue HKLM "Software\RegisteredApplications" "qBittorrent Vanced"

    ; File associations — root class keys (only remove if still pointing to our handler)
    ReadRegStr $0 HKLM "Software\Classes\.torrent" ""
    StrCmp $0 "qBittorrentVanced.File.Torrent" 0 +2
        DeleteRegKey HKLM "Software\Classes\.torrent"
    ReadRegStr $0 HKLM "Software\Classes\magnet\shell\open\command" ""
    StrCmp $0 '"$INSTDIR\qbittorrent.exe" "%1"' 0 +2
        DeleteRegKey HKLM "Software\Classes\magnet"

    System::Call 'Shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, p 0, p 0)'
SectionEnd
