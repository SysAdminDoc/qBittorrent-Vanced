# qBittorrent Vanced

![Vanced](https://img.shields.io/badge/Vanced-v1.0.2-blue) ![Base](https://img.shields.io/badge/Enhanced_Edition-v5.1.3.10-blue) ![License](https://img.shields.io/badge/license-GPL--2.0--or--later-green) ![Platform](https://img.shields.io/badge/platform-Windows_x64-lightgrey)

A customized build of [qBittorrent Enhanced Edition](https://github.com/c0re100/qBittorrent-Enhanced-Edition) with a modern dark theme, streamlined interface, and quality-of-life improvements.

Based on qBittorrent Enhanced Edition v5.1.3.10 (which itself is based on [qBittorrent](https://github.com/qbittorrent/qBittorrent) v5.1.3).

## What's Different

### Catppuccin Theme

- Complete Catppuccin styling across all widgets, menus, toolbars, dialogs, and the WebUI, with selectable Latte, Frappe, Macchiato, and Mocha desktop flavors
- First launch chooses Latte on a light system color scheme and Mocha otherwise; saved flavor choices are preserved
- The built-in theme picker previews Catppuccin flavors live with sample torrent rows and shimmer progress bars
- Glassmorphism-inspired styling with subtle transparency and accent colors
- Branded scrollbars, condensed spacing, and polished UI elements throughout
- Refined focus, hover, selected, disabled, empty, and warning states across desktop widgets

### Green Progress Bars with Shimmer Effect

- Custom-painted progress bars replace the default Qt style
- Green gradient with 3D vertical banding (similar to classic Windows Explorer file transfers)
- Animated shimmer highlight sweeps across active downloads
- Optional simple mode uses solid bars with percent text for users who prefer less motion
- Distinct colors for downloading (green), completed (bright green), and stopped/error (muted gray)
- Queued, checking, and stalled torrents show compact glyph overlays without adding extra state colors
- Custom `config.json` themes can override `ProgressBar.*` keys for groove, text, shimmer, focus, and per-state bar colors

### Streamlined Interface

- Removed the Dashboard tab
- Removed the "Filter by" dropdown from the toolbar (search bar filters by name)
- Filters sidebar is hidden by default but available from the toolbar or View menu for Status, Category, Tag, and Tracker panes
- Search bar supports scoped filters such as `seeders>10`, `size<4gb`, and `category:movies` while preserving plain name search
- Condensed vertical spacing throughout (tab bars, toolbars, table rows, headers, buttons)
- Wider default column sizes so column titles aren't clipped
- Transfer-list column header menu includes Compact, Media Server, and Debug presets for one-click layout switching
- Selected queued torrents can be dragged together in the transfer list to reorder the queue
- Category cells can be double-clicked to assign an existing category or clear the category inline
- Cleaner batch operations, portable backup, torrent-card, toolbar, and dialog flows with clearer labels and feedback
- Mobile-first WebUI add/manage dialogs for torrent links, local torrent files, category/tag edits, save-location edits, and toolbar/filter controls
- Category save-path hints flag media-library-root paths and steer *arr/TRaSH-style setups toward subfolder-only category paths with Automatic torrent management

### Inline Speed Controls

- Download and upload speed displayed directly in the properties tab bar
- Click either button for a dropdown with quick speed limit presets (Unlimited, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000 KiB/s)
- Real-time display of current speed and active limit

### Sensible Defaults

- Upload speed limited to 20 KiB/s by default (instead of unlimited)
- Torrents stop on completion by default

### Inherited from Enhanced Edition

- Auto ban BitTorrent leechers and unwanted peers (Xunlei, QQDownload, etc.)
- Auto ban unknown peers from private trackers
- Enhanced peer ID/client name detection
- All features from upstream qBittorrent

## Building

### Requirements

- Visual Studio 2022+ with C++ workload
- CMake 3.16+
- vcpkg (included with Visual Studio)

### Windows

```powershell
powershell -ExecutionPolicy Bypass -File build.ps1
```

The build script discovers Visual Studio Build Tools/Community, CMake, Ninja, and vcpkg, then configures and builds this checkout with Ninja. It deploys the Qt runtime beside `build\qbittorrent.exe` and verifies the result with `--version`.

Expected smoke output:

```text
qBittorrent Vanced v1.0.2 (base: qBittorrent Enhanced Edition v5.1.3.10; upstream: qBittorrent v5.1.3)
```

The batch wrapper exposes the same release smoke:

```bat
build.bat --verify
```

To bump Vanced release metadata before packaging, run:

```powershell
powershell -ExecutionPolicy Bypass -File bump-version.ps1 -Vanced 1.0.3
```

The version bump updates the C++ version header, vcpkg manifest, README badge/smoke text, NSIS metadata, Windows manifest, package defaults, and local working notes. Run the release gate immediately after a bump.

Optional C++ tests use `build.ps1 -Test` or `build.bat --test`. Standalone tests run without Qt6 Test; older QtTest-based cases run only when the local vcpkg tree includes the Qt6 Test component (`qtbase[testlib]`).

For a local release verification gate, run:

```powershell
powershell -ExecutionPolicy Bypass -File build.ps1 -ReleaseGate
```

The gate checks version/license/dependency metadata, installs the pinned WebUI lint toolchain, runs WebUI lint, builds and verifies `qbittorrent.exe`, cleans stale release artifacts, builds the portable ZIP and NSIS installer, then writes `release\SHA256SUMS.txt`, `release\RELEASE-PROVENANCE.txt`, and `release\ADVISORY-CHECK.txt`. The advisory file includes resolved Qt, OpenSSL, and libtorrent versions with known-security status before the raw `vcpkg update` and `npm audit` output.

Portable ZIP users can launch `qbittorrent.exe --portable` to keep configuration, cache, session data, and default downloads under a `data\qBittorrent\` folder beside the executable. `--portable` is mutually exclusive with `--profile=<dir>`.

### WebUI Checks

```powershell
cd src\webui\www
npm ci
npm run lint
```

The WebUI toolchain is pinned in `package.json` and `package-lock.json` so `npm ci` installs the same lint stack on each machine.
Mobile WebUI smoke checks should include a 375px viewport for `download.html`, `upload.html`, `newcategory.html`, `newtag.html`, `setlocation.html`, and the main `index.html` toolbar/filter shell.
The WebUI shows a non-blocking idle-session warning after 30 minutes of inactivity by default; override the local preference or URL query key `session_idle_warning_minutes` for shorter or longer browser-side warning windows.
Built-in WebUI HTML responses emit per-response CSP script nonces and apply the matching `nonce` attribute to script tags as additive hardening before the later unsafe-inline cleanup.
Run `test\webapi-smoke.ps1 -AltWebUIPath <VueTorrent-dist>` against a running instance configured for that alternate WebUI to verify the supplied bundle is served and the API paths used by VueTorrent still pass.
The large-library smoke (`test\largelibrary-smoke.ps1`) includes a dependency-free Edge/Chrome DevTools heap check that loads the WebUI after seeding 10k torrents and fails if peak used JS heap exceeds the documented ceiling (`-HeapCeilingMB`, default 500 MB).

## License

Licensed under the [GNU General Public License v2.0](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html) (or later), same as upstream qBittorrent.

## Credits

- [qBittorrent](https://github.com/qbittorrent/qBittorrent) - The original open-source BitTorrent client
- [qBittorrent Enhanced Edition](https://github.com/c0re100/qBittorrent-Enhanced-Edition) - Enhanced fork with anti-leecher features
