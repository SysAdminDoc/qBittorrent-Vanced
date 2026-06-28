# ROADMAP

qBittorrent Vanced — Catppuccin Mocha theme, custom shimmer progress bars, streamlined interface, and inline speed controls on top of qBittorrent Enhanced Edition v5.1.3.10.

## Planned Features

### Theme
- Centralize the palette in a `Palette` struct; swap Catppuccin Latte/Frappe/Macchiato/Mocha at runtime
- Respect system dark/light preference on first launch
- Theme picker dialog with live preview of the torrent list and progress bars
- WebUI theme parity — several Qt-driven list items still leak stock colors into the served CSS

### Progress bars
- Accessibility: add an alt-style mode (solid color + text percent) for users who find the shimmer distracting
- Per-state color keys exposed to theme JSON so downstream forks can rebrand without recompiling
- Optional glyph overlay for stalled / queued / checking states instead of inventing new colors

### UX
- Bring back filters sidebar behind a toggle; some users rely on the Status / Category / Tags panes
- Column preset picker (Compact / Media Server / Debug) with a one-click switch
- Multi-row drag queue reorder with Alt+Up/Down keyboard shortcut
- Inline editable category column (double-click) instead of right-click > set category
- Search-bar scoped filter: `seeders>10 size<4gb category:movies` mini-DSL on the main list

### Platform
- Portable ZIP build alongside the NSIS installer
- Winget + Scoop manifests
- Self-update via AppCast XML on GitHub Releases with in-app toast on new release
- Linux AppImage and Flatpak so the fork isn't Windows-only

### Upstream hygiene
- Publish the divergence as a quilt-style patch series under `patches/` so individual tweaks are reusable and rebase is mechanical

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

## Research-Driven Additions

- [ ] P2 — Add dependency and security provenance to release artifacts
  Why: qBittorrent publishes library versions/checksums and recent OpenSSL/Qt/vcpkg releases show why binary provenance matters.
  Evidence: qBittorrent download page, OpenSSL security timeline, Qt 6.10.3 release notes, `vcpkg.json`
  Touches: release scripts, `vcpkg.json`, installer packaging, portable packaging
  Acceptance: every release includes SHA256 hashes, Qt/libtorrent/Boost/OpenSSL/zlib versions, vcpkg baseline, and advisory-check output next to installer/portable assets.
  Complexity: M

## Research Expansion — 2026-06-19

Note: existing research wording that says `.qbttheme` is stale; qBittorrent's current documented bundle extension and this repo's loader use `.qbtheme`.

- [ ] Add path/action validation and recovery for WebUI move/delete flows
  Status: Proposed
  Category: Reliability
  Priority: P2
  Time horizon: Next
  User value: Reduces data-loss and automation-breakage risk when users move torrents, change save locations, or run bulk actions remotely.
  Implementation notes: Resolve the local `setLocation` destination validation TODO; preflight path existence/permissions; improve WebUI error copy; add dry-run style summaries for bulk move/delete/category operations where feasible.
  Dependencies: WebAPI test harness and current mobile WebUI smoke.
  Risks: Strict validation may block legitimate paths not visible from the UI process; preserve advanced override with explicit warning if needed.
  Acceptance criteria: WebAPI returns actionable errors for invalid/unwritable destinations; WebUI displays recovery guidance; tests cover invalid path, permission failure, existing destination, and success.
  Research references: `RESEARCH.md` > `Improvement Opportunities for Current Features`; local `src/webui/api/torrentscontroller.cpp`; qBittorrent v5.2.x Add Torrent and WebUI recovery fixes.

## Research-Driven Additions

- [ ] P0 - Add redirect hardening regression coverage
  Why: Vanced now has a local redirect scheme allowlist, but the SSRF-class behavior needs a test so future upstream merges cannot reopen it.
  Evidence: qBittorrent 5.2.1 SSRF redirect fix, src/base/net/downloadhandlerimpl.cpp.
  Touches: src/base/net/downloadhandlerimpl.cpp, test/CMakeLists.txt, test/.
  Acceptance: tests prove HTTP/HTTPS redirects are followed, magnet redirects return the magnet handoff status, file/javascript/ftp redirects fail with the dangerous-protocol error, and max redirect count still fails closed.
  Complexity: M

- [ ] P1 - Backport reverse-proxy Host/X-Forwarded-Host handling
  Why: qBittorrent 5.2.2 limits X-Forwarded-Host use to explicit reverse-proxy mode, while community issues show WebUI Host/CSRF behavior remains fragile behind Cloudflare and Nginx.
  Evidence: qBittorrent 5.2.2 changelog entry for #24457, qBittorrent issues #21106 and #21673, src/webui/webapplication.cpp.
  Touches: src/webui/webapplication.cpp, src/base/preferences.*, src/webui/api/appcontroller.*, WebUI options text.
  Acceptance: reverse-proxy disabled ignores X-Forwarded-Host, reverse-proxy enabled accepts configured forwarded hosts, CSRF/Host failures produce actionable log messages, and local/non-proxy WebUI behavior is unchanged.
  Complexity: M

- [ ] P1 - Add large-library WebUI and queue performance smoke
  Why: 10k+ torrent sessions expose CPU, queueing, checkbox, and sync-update regressions that ordinary smoke tests miss.
  Evidence: qBittorrent issue #23384, qBittorrent 5.2.2 WebUI global-checkbox performance fix, src/webui/api/synccontroller.cpp, src/webui/www/private/scripts/dynamicTable.js.
  Touches: src/base/bittorrent/sessionimpl.cpp, src/webui/api/synccontroller.cpp, src/webui/www/private/scripts/*.js, test fixtures.
  Acceptance: a synthetic profile with at least 10,000 torrent records can load the WebUI transfer list, toggle global selection, apply queue filters, and request sync data under documented CPU/time ceilings.
  Complexity: L

- [ ] P1 - Add WebAPI compatibility smoke for automation clients
  Why: qBittorrent 5.2.x clients and tools expect stable categories, tags, auth, and WebAPI version behavior, and Vanced currently has no compatibility fixture.
  Evidence: qBittorrent API-key documentation, qbittorrent-api v5.2.2 support, autobrr qBittorrent action docs, qbit_manage category/tag cleanup docs.
  Touches: src/webui/api/*controller.*, src/webui/webapplication.cpp, src/base/preferences.*, test or tooling scripts.
  Acceptance: smoke script authenticates, reads app/version data, creates categories/tags, adds a paused magnet or fixture torrent, sets save path/category/tags, queries transfer info, and records the supported WebAPI version without requiring real tracker traffic.
  Complexity: M

- [ ] P2 - Bump dependency baseline for libtorrent web seed credential hardening
  Why: libtorrent 2.0.13 clears HTTP credentials on redirected web seeds and fixes Merkle proof cleanup; Vanced's vcpkg baseline should not lag known torrent-core security fixes.
  Evidence: libtorrent 2.0.13 release notes, vcpkg.json, qBittorrent 5.2.2 library versions.
  Touches: vcpkg.json, build scripts, release provenance output, smoke documentation.
  Acceptance: local build uses a baseline containing libtorrent 2.0.13 or an explicitly patched equivalent, release provenance prints the resolved libtorrent version, and web seed redirect behavior is covered by a targeted smoke/test.
  Complexity: M

- [ ] P2 - Add hardlink and atomic-move setup validation
  Why: media automation users rely on qBittorrent categories and save paths preserving hardlinks; valid writable paths can still cause slow copies or broken imports across filesystems.
  Evidence: TRaSH hardlink/atomic-move guide, autobrr category/save-path docs, src/webui/api/torrentscontroller.cpp, category dialogs.
  Touches: src/gui/torrentcategorydialog.*, src/gui/addnewtorrentdialog.*, src/webui/api/torrentscontroller.cpp, WebUI category/location dialogs.
  Acceptance: category/save-location flows warn when paths cross filesystem roots or cannot support hardlinks, show the resolved category save path before apply, and keep advanced users able to proceed after an explicit warning.
  Complexity: M

- [ ] P2 - Add official-source and checksum cues to installer and About surfaces
  Why: upstream qBittorrent warns about repackaged store clones; Vanced should make official GitHub releases, GPL source availability, and artifact hashes visible without telemetry or marketing.
  Evidence: qBittorrent 5.2.2 scam warning, installer.nsi, dist/windows/config.nsh, src/gui/aboutdialog.cpp.
  Touches: installer.nsi, dist/windows/config.nsh, src/gui/aboutdialog.*, package/provenance scripts.
  Acceptance: installer and About dialog show the official repository URL and release identity, package output includes SHA256 files beside artifacts, and no user-facing path suggests Microsoft Store or third-party download channels.
  Complexity: S
