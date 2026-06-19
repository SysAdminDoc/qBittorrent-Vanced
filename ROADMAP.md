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
- Weekly CI rebase against Enhanced Edition; auto-file an issue on conflict

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

- [ ] P0 — Resolve license and release identity conflicts
  Why: Users and package managers see incompatible MIT/GPL and v1.0.0/v5.1.3.10/v5.1.0 metadata today.
  Evidence: `README.md`, `LICENSE`, `COPYING`, `installer.nsi`, `src/base/version.h.in`, `dist/windows/config.nsh`, `vcpkg.json`
  Touches: `README.md`, `LICENSE`, `installer.nsi`, `dist/windows/config.nsh`, `src/base/version.h.in`, `vcpkg.json`
  Acceptance: README badges, installer metadata, About/version output, package metadata, and license files expose one coherent license policy and distinct Vanced/base/upstream versions.
  Complexity: M

- [ ] P0 — Fix the Windows build entrypoint and add a release smoke
  Why: The documented PowerShell build path changes into a different Enhanced Edition checkout, so a clean Vanced checkout is not independently reproducible.
  Evidence: `README.md`, `build.ps1`, `build.bat`
  Touches: `build.ps1`, `build.bat`, `CMakePresets.json`, `test/CMakeLists.txt`
  Acceptance: `powershell -ExecutionPolicy Bypass -File build.ps1` configures this checkout from any current directory, supports an optional test build, and verifies the produced executable with `--version`.
  Complexity: M

- [ ] P0 — Add current-branch Windows CI before rebase automation
  Why: The fork carries security- and UI-sensitive deltas but has no `.github/` build guard after upstream/Enhanced moved to 5.2.x.
  Evidence: missing `.github/`, qBittorrent 5.2.2 releases, Enhanced 5.2.1.10 releases, recent git history
  Touches: `.github/workflows/*`, `build.ps1`, `vcpkg.json`, `src/webui/www/package.json`
  Acceptance: GitHub Actions builds Windows Release from a clean checkout, runs CMake tests when enabled, runs WebUI lint with locked dependencies, and uploads logs/artifacts for failed release builds.
  Complexity: L

- [ ] P1 — Make WebUI dependency and lint runs reproducible
  Why: Wildcard WebUI devDependencies make lint/format/security behavior drift between releases and machines.
  Evidence: `src/webui/www/package.json`
  Touches: `src/webui/www/package.json`, WebUI lockfile, CI workflow
  Acceptance: a pinned lockfile is committed, `npm ci` or equivalent works offline from the lock, and `npm run lint` is deterministic in CI.
  Complexity: S

- [ ] P1 — Harden WebUI session and CSP boundaries
  Why: The WebUI still permits global inline scripts/styles and contains a session-check TODO while upstream users track CSRF/session issues.
  Evidence: `src/webui/webapplication.cpp`, qBittorrent issue #17598
  Touches: `src/webui/webapplication.*`, `src/webui/www/private/index.html`, `src/webui/www/private/scripts/*`, WebAPI tests
  Acceptance: default WebUI no longer requires broad `unsafe-inline`, session validation has an explicit tested rule, CSRF behavior is covered, and alternate WebUI compatibility is documented.
  Complexity: L

- [ ] P1 — Add Vanced-specific theme regression fixtures
  Why: Vanced colors and shimmer painting bypass the theme bundle path, and upstream 5.2.x changed theme behavior.
  Evidence: `src/gui/uithememanager.cpp`, `src/gui/progressbarpainter.cpp`, qBittorrent theme wiki, qBittorrent issue #23900
  Touches: `src/gui/uithememanager.*`, `src/gui/progressbarpainter.*`, `src/gui/transferlistdelegate.*`, `test/`
  Acceptance: automated fixtures verify built-in theme colors, progress states, and `.qbtheme` load/export behavior on Qt6 without relying on manual screenshots.
  Complexity: M

- [ ] P1 — Replace the hand-rolled NSIS installer with the upstream Windows installer pipeline
  Why: The custom installer duplicates `dist/windows` scripts and hard-codes product, version, license, and install-path fields.
  Evidence: `installer.nsi`, `dist/windows/config.nsh`, Enhanced release assets, Winget manifest rules
  Touches: `installer.nsi`, `dist/windows/*.nsh`, release scripts
  Acceptance: one Windows installer path produces a branded Vanced setup with license page, silent install/uninstall support, matching DisplayVersion, and SHA256 output.
  Complexity: M

- [ ] P1 — Restore localized manpage encoding and issue routing
  Why: Russian manpage sources are mojibake and generated docs still point bug reports at upstream qBittorrent without a Vanced policy.
  Evidence: `doc/ru/qbittorrent.1.md`, `doc/ru/qbittorrent-nox.1.md`, `doc/en/qbittorrent.1.md`
  Touches: `doc/**`, `dist/unix/CMakeLists.txt`, documentation generation commands
  Acceptance: localized manpage markdown is valid UTF-8, generated manpages are refreshed, and every bug-report URL intentionally routes to Vanced, Enhanced, or upstream.
  Complexity: S

- [ ] P2 — Add a mobile-first WebUI add/manage flow
  Why: qBittorrent users repeatedly report mobile WebUI friction, while Flood/VueTorrent and commercial clients compete on touch-friendly add/play flows.
  Evidence: qBittorrent issue #8887; Flood; VueTorrent; BitTorrent Web
  Touches: `src/webui/www/private/index.html`, `src/webui/www/private/css/style.css`, `src/webui/www/private/scripts/*`, WebAPI tests
  Acceptance: a 375px viewport can add magnets/torrents, set category/tag/save path, search/filter, select/delete, and recover from validation errors without horizontal trapping; mobile smoke tests cover the flow.
  Complexity: L

- [ ] P2 — Add category/tag automation presets for media and tracker workflows
  Why: Sonarr, autobrr, TRaSH guides, and qbit_manage all rely on qBittorrent categories/tags for import, tracker separation, and cleanup.
  Evidence: Sonarr issue #6777, TRaSH category guide, qbit_manage docs, `src/gui/transferlistwidget.cpp`
  Touches: `src/gui/optionsdialog.*`, `src/gui/transferlistwidget.*`, WebUI preferences/API, add-torrent dialog
  Acceptance: users can save named category/tag presets, apply them during add/import, and use the same presets from WebUI/API without right-click-only workflows.
  Complexity: M

- [ ] P2 — Add dependency and security provenance to release artifacts
  Why: qBittorrent publishes library versions/checksums and recent OpenSSL/Qt/vcpkg releases show why binary provenance matters.
  Evidence: qBittorrent download page, OpenSSL security timeline, Qt 6.10.3 release notes, `vcpkg.json`
  Touches: release scripts, `vcpkg.json`, installer packaging, portable packaging
  Acceptance: every release includes SHA256 hashes, Qt/libtorrent/Boost/OpenSSL/zlib versions, vcpkg baseline, and advisory-check output next to installer/portable assets.
  Complexity: M

- [ ] P3 — Evaluate dual libtorrent 1.2 and 2.x release flavors with explicit v2 torrent labeling
  Why: qBittorrent ships separate libtorrent choices and BEP 52/libtorrent 2.x support changes compatibility, performance, and user expectations.
  Evidence: qBittorrent download page, BEP 52, libtorrent releases
  Touches: `CMakeLists.txt`, `vcpkg.json`, release scripts, About dialog, documentation
  Acceptance: a decision record and prototype build matrix show whether Vanced should ship lt12/lt20 variants, and the UI clearly labels BitTorrent v2/hybrid behavior when enabled.
  Complexity: XL
