# qBittorrent Vanced

## Overview
Custom fork of qBittorrent Enhanced Edition (v5.1.3.10) with Catppuccin Mocha dark theme, green shimmer progress bars, streamlined UI, and quality-of-life defaults. Vanced version: v1.0.0.

## Tech Stack
- **Language**: C++ (Qt6 Widgets)
- **Build**: CMake + Ninja + MSVC (Visual Studio 2026 v18)
- **Dependencies**: vcpkg manifest mode (vcpkg.json) - Qt6, libtorrent, Boost, OpenSSL, zlib
- **Installer**: NSIS (installer.nsi)
- **Packaging**: windeployqt copies Qt DLLs into dist_package/

## Build
```powershell
powershell -ExecutionPolicy Bypass -File build.ps1
```
- Loads MSVC env via vcvars64.bat, configures CMake, builds with Ninja
- Output: `build/qbittorrent.exe`
- Kill running instance first: `taskkill /f /im qbittorrent.exe`

## Package & Release
```powershell
# Copy exe + deploy Qt DLLs
cp build/qbittorrent.exe dist_package/
build/vcpkg_installed/x64-windows/tools/Qt6/bin/windeployqt.exe --release --no-translations --no-opengl-sw --no-system-d3d-compiler dist_package/qbittorrent.exe
# Copy vcpkg runtime DLLs
cp build/vcpkg_installed/x64-windows/bin/*.dll dist_package/
# Build installer
"/c/Program Files (x86)/NSIS/makensis.exe" installer.nsi
# Zip portable
Compress-Archive -Path 'dist_package\*' -DestinationPath 'qBittorrent-Vanced-vX.X.X-x64-portable.zip' -Force
```

## Key Files
- `src/gui/uithememanager.cpp` - Catppuccin Mocha theme (applyBuiltInDarkTheme, ~line 294). CSS split into 4 raw string parts due to MSVC 16380 char limit.
- `src/gui/progressbarpainter.cpp` - Custom QPainter progress bars with green gradient + shimmer animation via QElapsedTimer
- `src/gui/mainwindow.cpp` - Window title ("qBittorrent Vanced"), inline speed buttons in PropTabBar, filter bar setup, toolbar customization
- `src/gui/mainwindow.h` - Member vars for speed buttons (m_inlineDlSpeedBtn, m_inlineUlSpeedBtn)
- `src/gui/properties/proptabbar.h` - addStatusWidget() inline method for embedding widgets in tab bar
- `src/gui/statusbar.cpp` - Tinted speed labels (blue DL, green UL)
- `src/gui/aboutdialog.cpp` - About dialog branding
- `src/gui/traypopupwidget.cpp` - Tray popup branding
- `src/base/bittorrent/sessionimpl.cpp` - User-Agent string, default upload limit (20 KiB/s)
- `src/base/preferences.cpp` - Default prefs (stop on completion=true, filters sidebar=false, status bar=false)
- `src/qbittorrent.rc` - Windows version info
- `dist/windows/config.nsh` - NSIS installer branding
- `installer.nsi` - NSIS installer script
- `build.ps1` - Build script

## Customizations Applied
1. **Theme**: Full Catppuccin Mocha via QPalette + QSS in uithememanager.cpp
2. **Progress bars**: Green gradient with 3D banding + shimmer sweep (2s cycle). Distinct colors for downloading/complete/stopped.
3. **Removed**: Dashboard tab, Filter by dropdown, lock button from toolbar, filters sidebar (default off), status bar (default off), RSS/Search/TorrentCreator modules, program updater, stats dialog, donation links
4. **Added**: Inline speed limit buttons with dropdown presets in properties tab bar, tray popup widget, batch operations dialog, portable backup dialog, torrent cards widget
5. **Defaults**: Upload limit 20 KiB/s, stop on completion enabled
6. **Branding**: All user-visible strings changed from "qBittorrent Enhanced Edition" to "qBittorrent Vanced"

## Gotchas
- MSVC C2026: Raw string literals max 16380 chars. CSS in uithememanager.cpp must be split into multiple parts concatenated together.
- `taskkill /f /im qbittorrent.exe` before building or you get LNK1104 (can't open exe).
- proptabbar.h: addStatusWidget must be `public:` inline, NOT in `signals:` block (causes AutoMoc errors).
- QComboBox was removed from mainwindow - don't re-add the forward declaration or include.
- The exe alone won't run - needs Qt DLLs via windeployqt + vcpkg bin DLLs.

## GitHub
- Repo: SysAdminDoc/qBittorrent-Vanced
- Local branch `v5_1_x` pushes to remote `vanced` as `main`
- Branch protection enabled on main
- Dependabot disabled, all upstream CI/community files removed
