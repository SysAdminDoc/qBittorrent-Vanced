Var uninstallerPath

Section "-hidden"

    ;Search if qBittorrent Vanced is already installed.
    FindFirst $0 $1 "$uninstallerPath\uninst.exe"
    FindClose $0
    StrCmp $1 "" done

    ;Run the uninstaller of the previous install.
    DetailPrint $(inst_unist)
    ExecWait '"$uninstallerPath\uninst.exe" /S _?=$uninstallerPath'
    Delete "$uninstallerPath\uninst.exe"
    RMDir "$uninstallerPath"

    done:

SectionEnd


Section $(inst_qbt_req) ;"qBittorrent Vanced (required)"

  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put files there
  File "${QBT_DIST_DIR}\qbittorrent.exe"
  File "qt.conf"
  File /nonfatal /r /x "${QBT_DIST_DIR}\qbittorrent.exe" "${QBT_DIST_DIR}\*.*"
  Exec '"cmd.exe" /c COMPACT /C "$INSTDIR\qbittorrent.pdb"'

  ; Write the installation path into the registry
  WriteRegStr HKLM "Software\qBittorrent Vanced" "InstallLocation" "$INSTDIR"

  ; Register qBittorrent Vanced as possible default program for .torrent files and magnet links
  WriteRegStr HKLM "Software\qBittorrent Vanced\Capabilities" "ApplicationDescription" "qBittorrent Vanced - A customized BitTorrent client in Qt"
  WriteRegStr HKLM "Software\qBittorrent Vanced\Capabilities" "ApplicationName" "qBittorrent Vanced"
  WriteRegStr HKLM "Software\qBittorrent Vanced\Capabilities\FileAssociations" ".torrent" "qBittorrentVanced.File.Torrent"
  WriteRegStr HKLM "Software\qBittorrent Vanced\Capabilities\UrlAssociations" "magnet" "qBittorrentVanced.Url.Magnet"
  WriteRegStr HKLM "Software\RegisteredApplications" "qBittorrent Vanced" "Software\qBittorrent Vanced\Capabilities"
  ; Register qBittorrent Vanced ProgIDs
  WriteRegStr HKLM "Software\Classes\qBittorrentVanced.File.Torrent" "" "Torrent File"
  WriteRegStr HKLM "Software\Classes\qBittorrentVanced.File.Torrent\DefaultIcon" "" '"$INSTDIR\qbittorrent.exe",1'
  WriteRegStr HKLM "Software\Classes\qBittorrentVanced.File.Torrent\shell\open\command" "" '"$INSTDIR\qbittorrent.exe" "%1"'
  WriteRegStr HKLM "Software\Classes\qBittorrentVanced.Url.Magnet" "" "Magnet URI"
  WriteRegStr HKLM "Software\Classes\qBittorrentVanced.Url.Magnet\DefaultIcon" "" '"$INSTDIR\qbittorrent.exe",1'
  WriteRegStr HKLM "Software\Classes\qBittorrentVanced.Url.Magnet\shell\open\command" "" '"$INSTDIR\qbittorrent.exe" "%1"'

  WriteRegStr HKLM "Software\Classes\.torrent" "Content Type" "application/x-bittorrent"
  WriteRegStr HKLM "Software\Classes\magnet" "" "URL:Magnet URI"
  WriteRegStr HKLM "Software\Classes\magnet" "Content Type" "application/x-magnet"
  WriteRegStr HKLM "Software\Classes\magnet" "URL Protocol" ""

  System::Call 'Shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, p 0, p 0)'

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "DisplayName" "qBittorrent Vanced"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "UninstallString" '"$INSTDIR\uninst.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "DisplayIcon" '"$INSTDIR\qbittorrent.exe",0'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "Publisher" "qBittorrent Vanced"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "URLInfoAbout" "https://github.com/SysAdminDoc/qBittorrent-Vanced"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "DisplayVersion" "${QBT_VANCED_VERSION} (base ${QBT_BASE_VERSION})"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "NoRepair" 1
  WriteUninstaller "uninst.exe"
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\qBittorrent Vanced" "EstimatedSize" "$0"

SectionEnd

; Optional section (can be disabled by the user)
Section /o $(inst_desktop) ;"Create Desktop Shortcut"

  CreateShortCut "$DESKTOP\qBittorrent Vanced.lnk" "$INSTDIR\qbittorrent.exe"

SectionEnd

Section $(inst_startmenu) ;"Create Start Menu Shortcut"

  CreateDirectory "$SMPROGRAMS\qBittorrent Vanced"
  CreateShortCut "$SMPROGRAMS\qBittorrent Vanced\qBittorrent Vanced.lnk" "$INSTDIR\qbittorrent.exe"
  CreateShortCut "$SMPROGRAMS\qBittorrent Vanced\$(inst_uninstall_link_description).lnk" "$INSTDIR\uninst.exe"

SectionEnd

Section /o $(inst_startup) ;"Start qBittorrent on Windows start up"

  !insertmacro UAC_AsUser_Call Function inst_startup_user ${UAC_SYNCREGISTERS}|${UAC_SYNCOUTDIR}|${UAC_SYNCINSTDIR}

SectionEnd

Function inst_startup_user

  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "qBittorrent Vanced" "$INSTDIR\qbittorrent.exe"

FunctionEnd

Section $(inst_firewall)

  DetailPrint $(inst_firewallinfo)
  nsisFirewallW::AddAuthorizedApplication "$INSTDIR\qbittorrent.exe" "qBittorrent Vanced"

SectionEnd

Section $(inst_pathlimit) ;"Disable Windows path length limit (260 character MAX_PATH limitation, requires Windows 10 1607 or later)"

  WriteRegDWORD HKLM "SYSTEM\CurrentControlSet\Control\FileSystem" "LongPathsEnabled" 1

SectionEnd

;--------------------------------

Function .onInit

  !insertmacro Init "installer"
  !insertmacro MUI_LANGDLL_DISPLAY

  ${IfNot} ${AtLeastWaaS} 1809 ; Windows 10 (1809) / Windows Server 2019. Min supported version by Qt6
    MessageBox MB_OK|MB_ICONEXCLAMATION $(inst_requires_win10) /SD IDOK
    SetErrorLevel 1654 # WinError.h: `ERROR_INSTALL_REJECTED`
    Abort
  ${EndIf}

  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONEXCLAMATION $(inst_requires_64bit) /SD IDOK
    SetErrorLevel 1654 # WinError.h: `ERROR_INSTALL_REJECTED`
    Abort
  ${EndIf}

  ;Search if qBittorrent Vanced is already installed.
  FindFirst $0 $1 "$INSTDIR\uninst.exe"
  FindClose $0
  StrCmp $1 "" done

  ;Copy old value to var so we can call the correct uninstaller
  StrCpy $uninstallerPath $INSTDIR

  ;Inform the user
  MessageBox MB_OKCANCEL|MB_ICONINFORMATION $(inst_uninstall_question) /SD IDOK IDOK done
  Quit

  done:

FunctionEnd

Function check_instance

  check:
  FindProcDLL::FindProc "qbittorrent.exe"
  StrCmp $R0 "1" 0 notfound
  MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION $(inst_warning) /SD IDCANCEL IDRETRY check IDCANCEL canceled

  canceled:
  SetErrorLevel 15618 # WinError.h: `ERROR_PACKAGES_IN_USE`
  Abort

  notfound:

FunctionEnd

Function PageFinishRun

  !insertmacro UAC_AsUser_ExecShell "" "$INSTDIR\qbittorrent.exe" "" "" ""

FunctionEnd

Function .onInstSuccess
  SetErrorLevel 0
FunctionEnd
