# ROADMAP

qBittorrent Vanced — Catppuccin Mocha theme, custom shimmer progress bars, streamlined interface, and inline speed controls on top of qBittorrent Enhanced Edition v5.1.3.10.

## Planned Features

### Platform
- Linux AppImage and Flatpak so the fork isn't Windows-only

## Competitive Research

- **qBittorrent upstream** — Reference behavior; never break muscle memory
- **qBittorrent Enhanced Edition** — Parent fork; auto-ban + peer detection must track upstream
- **Deluge** — Plugin model and auto-shutdown-when-done UX worth considering
- **Transmission 4** — Minimal RPC + WebUI; the bar for lightweight remote control

## Nice-to-Haves

- Play-while-downloading panel using mpv + sequential piece priority
- Built-in tracker-health dashboard (announce latency, last seen, error rate per tracker)
- RSS filter regex tester that runs against the current feed items
- Peer-list ASN/country badges using a bundled offline GeoLite DB
- Crash-dump toggle (opt-in, local-only) — no telemetry, ever

## Open-Source Research (Round 2)

### Related OSS Projects
- https://github.com/c0re100/qBittorrent-Enhanced-Edition — upstream Enhanced Edition base; auto-ban, tracker auto-update, API deltas
- https://github.com/qbittorrent/qBittorrent — vanilla upstream; origin of the `.qbttheme` system and config.json palette plumbing
- https://github.com/catppuccin/qbittorrent — Catppuccin official theme bundle; palette matches our scheme, saves hand-rolling QSS
- https://github.com/jagannatharjun/qbt-theme — multi-theme collection (DarkStyleSheet / Mumble / Breeze) — canonical icon-pack integration reference
- https://github.com/MahdiMirzadeh/qbittorrent — both Qt + WebUI theme bundles; good reference for keeping client + WebUI coherent
- https://github.com/witalihirsch/qBitTorrent-fluent-theme — Fluent dark/light; good reference for Windows 11 alignment
- https://github.com/maboroshin/qBittorrentDarktheme — minimal dark theme, useful reference for size/scope comparison
- https://draculatheme.com/qbittorrent — Dracula theme for qBittorrent; informs our palette alternatives

### Features to Borrow
- Ship Vanced theme as a signed `.qbttheme` (upstream theme wiki) — separate theme from patched binary so users can apply without rebuilding; automate in CI
- WebUI + Qt client shared color-token JSON (MahdiMirzadeh) — keeps the WebUI and desktop looking identical without two independent stylesheets that drift on version bumps
- Icon overrides under the `icons/` theme prefix (upstream wiki + jagannatharjun) — toolbar/tray icons via the theme bundle, not via patched resource paths
- config.json palette routing (upstream wiki) — move anything context-sensitive (Palette.Link, Palette.Highlight, etc.) to config.json instead of patched C++
- Fluent variant opt-in (witalihirsch) — optional Fluent light/dark bundle for Windows 11 users who don't want Catppuccin
- theme.park community hosting (Catppuccin / theme.park) — publish Vanced's WebUI theme there once stable, doubling reach without hosting
- RSS filter regex tester (on the existing roadmap) — upstream PR candidate; benefits the whole ecosystem

### Patterns & Architectures Worth Studying
- Theme-as-data vs theme-as-patch — moving Vanced's color/icon customizations out of source and into a `.qbttheme` minimizes rebase pain against c0re100 upstream
- QSS (structural) vs palette (context-sensitive colors) split — upstream draws this line clearly; audit every custom color in Vanced and route it to the right layer
- Shimmer-on-progress-bar as paintEvent override — already in place; document it as a standalone widget so other qBittorrent forks can pick it up via upstream PR
- Fork hygiene against the double-upstream (c0re100 on top of qbittorrent/qbittorrent) — keep our delta in patchable data + one isolated paintEvent override to stay mergeable with two ancestors

## Research Expansion — 2026-06-19

Note: existing research wording that says `.qbttheme` is stale; qBittorrent's current documented bundle extension and this repo's loader use `.qbtheme`.

## Audit-Driven Items — 2026-07-01

### P1
- [ ] P1 — Add Enter/Escape keyboard handlers and initial focus to download.html and upload.html dialogs
  Why: These are the two most-used dialogs but lack keyboard support that every other dialog has.
  Where: `src/webui/www/private/download.html`, `src/webui/www/private/upload.html`

- [ ] P1 — Add fetch error handling across all WebUI dialogs
  Why: 8+ dialogs silently ignore network errors and HTTP error responses. Users get no feedback when operations fail.
  Where: `download.html`, `upload.html`, `rename.html`, `rename_file.html`, `newcategory.html`, `newtag.html`, `shareratio.html`, `setlocation.html`

### P2
- [ ] P2 — Gate X-Forwarded-Host on trusted proxy IP in CSRF and Host validation
  Why: When reverse proxy is enabled, any direct client can forge X-Forwarded-Host without IP verification.
  Where: `src/webui/webapplication.cpp` lines 819-821, 864-865

- [ ] P2 — Add GeoIP database reload mutex to prevent use-after-free in concurrent lookup
  Why: `loadDatabase()` deletes `m_geoIPDatabase` on the main thread while libtorrent threads may call `lookup()`.
  Where: `src/base/net/geoipmanager.cpp`, `src/base/net/geoipdatabase.h` (mutable m_countries)

- [ ] P2 — Replace alert() with inline error display in rename_file.html
  Why: `alert()` blocks the UI thread and cannot be styled to match the theme.
  Where: `src/webui/www/private/rename_file.html`

- [ ] P2 — Add error display element to rename.html dialog
  Why: The rename dialog has no way to tell the user when renaming fails.
  Where: `src/webui/www/private/rename.html`

### P3
- [ ] P3 — Remove dead RSS/Search view HTML files and search.js from Qt resource bundle
  Why: The files are still compiled into the binary via webui.qrc even though the features are disabled.
  Where: `src/webui/www/webui.qrc`, `src/webui/www/private/views/rss.html`, `views/search.html`, `views/searchplugins.html`, `views/rssDownloader.html`, `views/installsearchplugin.html`, `scripts/search.js`

- [ ] P3 — Fix fragile double-encoding of save path in setLocation dialog
  Why: mocha-init.js double-encodes the path and setlocation.html double-decodes; if either side changes, paths with special characters break.
  Where: `src/webui/www/private/scripts/mocha-init.js` line 685, `src/webui/www/private/setlocation.html` line 36

## Research-Driven Additions — 2026-06-29

Note: items in Roadmap_Blocked.md (rebase, .qbtheme extraction, CSP unsafe-inline removal, libtorrent 2.0.13 bump, scoped API tokens, Qt Test setup, SBOM) are not repeated here. The following are new items not covered by any existing roadmap or blocked entry.

## Research-Driven Additions
