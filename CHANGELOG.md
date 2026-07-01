# Changelog

## Unreleased

- Added WebUI torrent-options dialog aggregating download/upload limits, sequential download, first/last piece priority, auto torrent management, and super seeding in one view.
- Added user-supplied GeoIP/MMDB guard: peer country resolution is disabled by default, requires a user-configured `.mmdb` path with validation and clear error text, and no longer auto-downloads databases.
- Added WebUI content-tab copy-path context menu action using ClipboardJS with multi-select/newline output and failure feedback.
- Added centralized logging for category and tag mutations with torrent name, hash, old value, and new value across desktop, WebAPI, batch, and auto-category paths.
- Added WebUI remote-access security negative smoke tests covering CSRF origin/referer rejection, SameSite=Strict cookie policy, CORS credential isolation, and X-Forwarded-Host bypass prevention.
- Removed disabled RSS, Search, and TorrentCreator module surfaces from the WebUI; added negative smoke tests for disabled-module endpoints and UI controls.
- Added AppCast-based program update checks with a non-modal in-app release toast and packaged AppCast checksums.
- Added WinGet and Scoop manifests for the v1.0.2 Windows setup and portable release artifacts.
- Added scoped transfer-list search tokens for fields such as seeders, size, category, ratio, speed, status, tracker, and paths.
- Added inline Category column editing via double-click with existing-category and Uncategorized choices.
- Added multi-row drag reordering for queued torrents without adding new keyboard shortcuts.
- Added Compact, Media Server, and Debug transfer-list column presets from the column header menu.
- Restored discoverable Filters Sidebar access with a toolbar toggle and cleared hidden sidebar filters when the pane is turned off.
- Added compact progress-bar glyph overlays for queued, checking, and stalled torrents without introducing additional state colors.
- Added `ProgressBar.*` theme color keys so custom `config.json` themes can rebrand progress groove, text, shimmer, focus, and per-state bar colors without recompiling.
- Documented the existing simple progress bar accessibility mode for users who prefer solid bars and percent text over shimmer animation.
- Added a built-in theme picker tab with live Catppuccin flavor preview, sample torrent rows, and shimmer progress bars.
- Added first-launch Catppuccin flavor selection that defaults to Latte for light system color schemes and Mocha otherwise.
- Added selectable built-in Catppuccin Latte, Frappe, Macchiato, and Mocha desktop theme flavors backed by a centralized palette table.
- Added a WebUI `prefers-color-scheme` fallback so the main UI uses Catppuccin Mocha tokens when system dark mode applies and no explicit light class is set.
- Added alternate WebUI static-bundle validation to the WebAPI smoke script for VueTorrent compatibility checks.
- Added WebUI CSP nonce plumbing for built-in HTML responses while keeping the existing inline-script policy during migration.
- Added TRaSH Guides-aligned hints when category save paths look like media-library roots.
- Added `--portable` startup mode for zero-install ZIP runs that keep profile data beside the executable.
- Added a configurable WebUI idle-session warning banner with Refresh and Logout actions.
- Added a browser-driven WebUI JS heap regression check to the large-library smoke script.
- Added visible Catppuccin-blue focus rings to custom-painted progress cells and torrent cards.
- Added release-gate advisory status checks for Qt, OpenSSL, and libtorrent resolved package versions.
- Added `bump-version.ps1` to synchronize Vanced release metadata across build, installer, manifest, README, package, vcpkg, and working-note files.
