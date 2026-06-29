# ROADMAP

qBittorrent Vanced — Catppuccin Mocha theme, custom shimmer progress bars, streamlined interface, and inline speed controls on top of qBittorrent Enhanced Edition v5.1.3.10.

## Planned Features

### Theme
- Centralize the palette in a `Palette` struct; swap Catppuccin Latte/Frappe/Macchiato/Mocha at runtime
- Respect system dark/light preference on first launch
- Theme picker dialog with live preview of the torrent list and progress bars

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
- Winget + Scoop manifests
- Self-update via AppCast XML on GitHub Releases with in-app toast on new release
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

## Research-Driven Additions — 2026-06-29

Note: items in Roadmap_Blocked.md (rebase, .qbtheme extraction, CSP unsafe-inline removal, libtorrent 2.0.13 bump, scoped API tokens, Qt Test setup, SBOM) are not repeated here. The following are new items not covered by any existing roadmap or blocked entry.

- [ ] P1 — Add WCAG 2.2 focus-appearance compliance for custom-painted controls
  Why: Custom-painted progress bars in `progressbarpainter.cpp` and torrent cards in `torrentcardswidget.cpp` must draw visible focus rings per WCAG 2.2 criterion 2.4.11 (Focus Appearance). Currently, progress bar cells have no visible focus indicator when navigated via keyboard. Torrent cards have StrongFocus policy but no drawn focus ring.
  Evidence: WCAG 2.2 criterion 2.4.11 requires >= 2px focus indicator with 3:1 contrast; Qt accessibility docs (doc.qt.io/qt-6/accessible-qwidget.html); `torrentcardswidget.cpp:41` sets `setFocusPolicy(Qt::StrongFocus)` but `paintEvent` does not draw a focus rectangle.
  Touches: src/gui/progressbarpainter.cpp (paintSimple/paintFancy), src/gui/torrentcardswidget.cpp (paintEvent)
  Acceptance: keyboard-navigated progress bar cells and torrent cards show a visible 2px focus ring in the Catppuccin blue (#89b4fa) with 3:1 contrast against the background.
  Complexity: S

- [ ] P2 — Add WebUI memory-leak regression check to large-library smoke
  Why: Upstream qBittorrent issues #20675, #21502, #23806 report unbounded WebUI RAM growth with tabs open. Vanced's largelibrary-smoke.ps1 measures sync timing but doesn't monitor browser memory.
  Evidence: qBittorrent issues #20675, #21502; VueTorrent exists partly because the stock WebUI leaks memory at scale.
  Touches: test/largelibrary-smoke.ps1, possibly add a Playwright-based memory snapshot step
  Acceptance: smoke script reports peak JS heap size after loading 10k torrents and 5 minutes of polling; fails if heap exceeds a documented ceiling (e.g., 500MB).
  Complexity: M

- [ ] P2 — Add WebUI session idle timeout warning
  Why: WebUI sessions persist until explicit logout or cookie expiration. Users behind reverse proxies may leave sessions open indefinitely. qBittorrent 5.2.0 added SameSite cookie enforcement but no idle timeout warning.
  Evidence: src/webui/webapplication.cpp session handling; authcontroller.cpp ban logic; no idle warning exists in any qBittorrent version.
  Touches: src/webui/www/private/scripts/client.js (add idle timer), src/webui/www/private/css/style.css (warning banner style)
  Acceptance: WebUI shows a non-blocking warning banner after N minutes of inactivity (configurable, default 30min); banner includes "Refresh" and "Logout" buttons.
  Complexity: S

- [ ] P2 — Add `--portable` CLI flag for zero-install portable mode
  Why: Portable ZIP users must manually set qBittorrent to use the local profile directory. A `--portable` flag would auto-configure the profile path relative to the executable, matching Transmission's portable mode.
  Evidence: Transmission 4.x portable mode; existing portable backup dialog in portablebackupdialog.cpp; users extract the ZIP and expect it to work without registry/AppData writes.
  Touches: src/app/application.cpp (CLI argument parsing), src/app/main.cpp (profile path override)
  Acceptance: `qbittorrent.exe --portable` stores all config/data in a `data/` subdirectory next to the executable; no writes to %APPDATA% or registry.
  Complexity: M

- [ ] P2 — Add TRaSH Guides-aligned category validation hints
  Why: TRaSH Guides are the de facto standard for *arr automation setup. Users following TRaSH recommendations expect subfolder-only category paths and Automatic torrent management mode. Vanced already warns on cross-filesystem paths but doesn't guide users toward the recommended setup.
  Evidence: TRaSH Guides qBittorrent setup (trash-guides.info), existing cross-filesystem warning in torrentcategorydialog.cpp.
  Touches: src/gui/torrentcategorydialog.cpp (add hint label when save path contains the media library root), src/webui/www/private/newcategory.html
  Acceptance: category dialog shows an informational hint when the save path looks like a media library root (e.g., contains `/movies/` or `/tv/` in the path) suggesting subfolder-only paths per TRaSH guidelines.
  Complexity: S

- [ ] P2 — Add CSP nonce plumbing to WebUI response generator
  Why: Even before removing all 91 inline handlers, the WebUI can begin using nonce-based CSP for its `<script>` tags. This is the prerequisite for eventually removing `'unsafe-inline'` from `script-src`. OWASP CSP cheat sheet recommends nonce-based CSP for server-rendered pages.
  Evidence: OWASP CSP Cheat Sheet (cheatsheetseries.owasp.org); web.dev strict CSP guide; src/webui/webapplication.cpp:465 current CSP.
  Touches: src/webui/webapplication.cpp (generate nonce per response, inject into script tags), src/base/utils/random.h (nonce generation)
  Acceptance: all `<script>` tags in WebUI responses include a `nonce` attribute; CSP header includes `'nonce-...'` alongside existing `'unsafe-inline'` (additive first, removal later); WebUI continues to function identically.
  Complexity: M

- [ ] P3 — Add VueTorrent alternate WebUI compatibility smoke
  Why: VueTorrent (v2.34.0) is the most popular alternate WebUI for qBittorrent. Users may install it alongside Vanced's built-in WebUI. Compatibility has never been validated.
  Evidence: VueTorrent GitHub (4.5k+ stars), qBittorrent alternate WebUI wiki; existing webapi-smoke.ps1 only tests the built-in WebUI.
  Touches: test/webapi-smoke.ps1 (add a `--alt-webui` parameter that validates API-only compatibility without the built-in HTML), documentation in README.md
  Acceptance: smoke script confirms VueTorrent can authenticate, list torrents, add/remove magnets, and read sync data against a running Vanced instance.
  Complexity: S

- [ ] P3 — Add `prefers-color-scheme` media query to WebUI for non-`.dark` class fallback
  Why: The WebUI hard-codes `class="dark"` on the HTML element. Users who override this or use the WebUI in a non-dark system preference context get no fallback. The login page already has `@media (prefers-color-scheme: light)` but the main UI does not.
  Evidence: src/webui/www/private/index.html:3 hardcodes `class="dark"`; src/webui/www/public/css/login.css:24 has prefers-color-scheme; style.css has `.dark` block but no `@media (prefers-color-scheme: dark)` fallback.
  Touches: src/webui/www/private/css/style.css, src/webui/www/private/scripts/color-scheme.js
  Acceptance: WebUI respects system dark/light preference when no explicit theme class is set; dark class remains the default but can be overridden.
  Complexity: S
