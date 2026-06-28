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

- [ ] P2 - Add hardlink and atomic-move setup validation
  Why: media automation users rely on qBittorrent categories and save paths preserving hardlinks; valid writable paths can still cause slow copies or broken imports across filesystems.
  Evidence: TRaSH hardlink/atomic-move guide, autobrr category/save-path docs, src/webui/api/torrentscontroller.cpp, category dialogs.
  Touches: src/gui/torrentcategorydialog.*, src/gui/addnewtorrentdialog.*, src/webui/api/torrentscontroller.cpp, WebUI category/location dialogs.
  Acceptance: category/save-location flows warn when paths cross filesystem roots or cannot support hardlinks, show the resolved category save path before apply, and keep advanced users able to proceed after an explicit warning.
  Complexity: M

