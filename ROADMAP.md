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

## Research Expansion — 2026-06-19

Note: existing research wording that says `.qbttheme` is stale; qBittorrent's current documented bundle extension and this repo's loader use `.qbtheme`.

- [ ] Backport safe redirect scheme handling for torrent URL downloads
  Status: Proposed
  Category: Security
  Priority: P0
  Time horizon: Now
  User value: Prevents malicious torrent URL redirects from reaching unsupported/local schemes while preserving normal HTTP, HTTPS, and magnet flows.
  Implementation notes: Compare upstream qBittorrent 5.2.1 security patch `#24270`; update `Net::DownloadHandlerImpl::handleRedirection()` and/or `DownloadManager::hasSupportedScheme()` to allow only expected redirect schemes; add tests for `http -> https`, `http -> magnet`, `http -> file`, `http -> ftp`, relative redirects, and max-redirect behavior.
  Dependencies: Current `DownloadManager` test harness or new Qt network unit tests.
  Risks: Overly strict scheme handling could break legitimate torrent URL sources; preserve magnet redirect behavior explicitly.
  Acceptance criteria: Redirects to `file://`, `ftp://`, and unknown schemes fail with a clear error; `http(s)` and `magnet:` redirects still work; regression tests cover accepted and rejected redirects.
  Research references: `RESEARCH.md` > `Research Pass — 2026-06-19 Expansion` > `Feature Gap Analysis`; local `src/base/net/downloadhandlerimpl.cpp`, `src/base/net/downloadmanager.cpp`; qBittorrent v5.2.1 news.

- [ ] Create a qBittorrent/Enhanced 5.2.x backport matrix
  Status: Proposed
  Category: Developer Experience
  Priority: P0
  Time horizon: Now
  User value: Gives maintainers a deterministic upgrade path instead of mixing security, WebUI, theme, and packaging changes in one risky rebase.
  Implementation notes: Build a machine-readable matrix grouped by security, WebUI, WebAPI, RSS/search, theme, installer, dependencies, and platform; map each upstream/Enhanced change to Vanced status: present, absent, conflict, intentionally skipped, or superseded by Vanced.
  Dependencies: Existing roadmap items for current-branch CI and patch-series hygiene.
  Risks: The matrix can become stale unless generated or checked in CI against upstream tags.
  Acceptance criteria: A maintainer can select a 5.2.x patch group and know source commits, affected files, conflicts, tests, and whether the patch is required before feature work continues.
  Research references: `RESEARCH.md` > `Existing Roadmap and Research Observations`; qBittorrent v5.2.0-v5.2.2 news; Enhanced `v5_2_x` release stream.

- [ ] Add scoped WebAPI integration tokens
  Status: Proposed
  Category: Integration
  Priority: P1
  Time horizon: Next
  User value: Lets Sonarr, autobrr, qbit_manage, scripts, and alternate WebUIs authenticate without sharing the primary WebUI password or browser session.
  Implementation notes: Backport or adapt upstream 5.2.x API-key support; store hashed token metadata; expose create/revoke/list UI; add optional scopes if upstream structure allows; document examples for common automation clients.
  Dependencies: WebUI auth/session tests and release identity/security cleanup.
  Risks: Token UI can create false security if scopes are not enforced; revocation must be immediate and logged.
  Acceptance criteria: Users can create and revoke an integration token; WebAPI accepts valid token auth and rejects invalid/revoked tokens; tests cover token lifecycle and failed auth; docs include Sonarr/autobrr/qbit_manage examples.
  Research references: `RESEARCH.md` > `New Feature Opportunities`; qBittorrent v5.2.x WebAPI changes; Real-Debrid API contract; Sonarr issue #6777.

- [ ] Backport upstream 5.2.x WebUI performance and mobile baseline
  Status: Proposed
  Category: Performance
  Priority: P1
  Time horizon: Next
  User value: Makes the bundled WebUI usable for large torrent lists and mobile management before Vanced-specific WebUI styling is expanded.
  Implementation notes: Cherry-pick or reimplement upstream virtual list/table performance, hidden-page polling behavior, mobile footer/tab/window fixes, global checkbox performance, Add Torrent dialog improvements, and Safari transfer-list fixes.
  Dependencies: Pinned WebUI dependencies, WebUI lint, and mobile smoke harness.
  Risks: WebUI is MooTools-era code; partial cherry-picks can regress translations, context menus, or alternate WebUI behavior.
  Acceptance criteria: Large-list interactions remain responsive; 375px mobile viewport can use core transfer/add flows; WebUI lint passes; a mobile browser smoke covers add, select, filter, and delete.
  Research references: `RESEARCH.md` > `Feature Gap Analysis`; qBittorrent v5.2.0-v5.2.2 WebUI changelog; Flood; VueTorrent.

- [ ] Add accessibility coverage for Vanced custom controls
  Status: Proposed
  Category: UX
  Priority: P1
  Time horizon: Next
  User value: Keeps the custom compact theme usable with keyboard navigation, screen readers, high contrast needs, and motion sensitivity.
  Implementation notes: Add explicit accessible names/descriptions for inline speed buttons, verify focus order, provide focus-visible styling, check non-text contrast for custom controls, and include reduced-motion handling for shimmer progress bars.
  Dependencies: Theme regression fixture item and progress alt-style roadmap item.
  Risks: Some Qt accessibility metadata may vary by platform; tests may need to combine unit checks with manual assistive-tech notes.
  Acceptance criteria: Inline speed controls have descriptive accessible names, keyboard operation, visible focus, and no tooltip-only meaning; reduced-motion/alt progress mode is reachable; documented checks align with WCAG 2.2 and Qt accessibility guidance.
  Research references: `RESEARCH.md` > `UX Improvement Opportunities`; local `src/gui/mainwindow.cpp`, `src/gui/progressbarpainter.cpp`; WCAG 2.2; WAI accessible names; Qt accessibility overview.

- [ ] Add first-run interface profiles
  Status: Proposed
  Category: UX
  Priority: P2
  Time horizon: Next
  User value: Lets users keep Vanced's clean default or opt into power/automation layouts without hunting through menus after first launch.
  Implementation notes: Add Minimal, Power User, and Automation profiles that set filters sidebar, status bar, column presets, WebUI visibility hints, category/tag emphasis, and speed control defaults; make profiles reversible from settings.
  Dependencies: Existing column preset and filters-sidebar roadmap work.
  Risks: Profiles can surprise existing users if applied after first run; gate profile selection to new profiles or explicit reset.
  Acceptance criteria: New profiles see a one-time profile choice; each profile maps to documented preference keys; switching profiles does not delete torrents or categories; settings expose current profile-derived choices.
  Research references: `RESEARCH.md` > `Current Project Understanding` and `UX Improvement Opportunities`; Flood responsive/sidebar patterns; Transmission lightweight UX.

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

- [ ] Add automation recipe import/export for categories, tags, and save paths
  Status: Proposed
  Category: Integration
  Priority: P2
  Time horizon: Later
  User value: Makes common Arr/autobrr/qbit_manage workflows repeatable without manual category/tag setup mistakes.
  Implementation notes: Define a small JSON schema for named recipes; include sample local-only recipes for movies, TV, private trackers, manual downloads, and cross-seed; validate paths before import; expose apply/revert preview.
  Dependencies: Category/tag preset roadmap item and path validation work.
  Risks: Recipes can encode user-specific paths or tracker names; exports should support redaction and comments.
  Acceptance criteria: Users can export current category/tag/save-path rules, import a recipe with validation, preview changes before applying, and revert imported changes.
  Research references: `RESEARCH.md` > `New Feature Opportunities`; autobrr; qbit_manage; TRaSH category guide; Sonarr issue #6777.

- [ ] Add alternate WebUI compatibility smoke matrix
  Status: Proposed
  Category: Integration
  Priority: P2
  Time horizon: Later
  User value: Lets self-hosted users choose VueTorrent/Flood-style frontends with clear support expectations instead of guessing whether Vanced's WebAPI diverges.
  Implementation notes: Document supported alternate WebUI install paths; run smoke checks for login, torrent list, add magnet, set category/tag, delete, preferences read, and token auth once available; record unsupported API differences.
  Dependencies: WebAPI token work and WebUI test harness.
  Risks: Third-party WebUIs move quickly; support should be compatibility best-effort, not bundled maintenance.
  Acceptance criteria: At least two known alternate WebUIs have a documented compatibility result; failures are linked to missing API endpoints or known third-party issues; Vanced WebAPI changes include compatibility notes.
  Research references: `RESEARCH.md` > `Comparable Open Source Projects Researched`; qBittorrent alternate WebUI wiki; VueTorrent; Flood.

- [ ] Add redacted diagnostics bundle with optional AI-ready summary
  Status: Needs Validation
  Category: AI
  Priority: P3
  Time horizon: Later
  User value: Gives users a local support artifact for build/runtime/WebUI problems without telemetry or automatic cloud upload.
  Implementation notes: Export app version, library versions, build metadata, installer metadata, recent log excerpts, WebUI settings fingerprints, and dependency baseline; redact torrent names, paths, tracker URLs, peer IPs, cookies, and tokens; optionally generate a local plain-language summary or AI-ready prompt file the user controls.
  Dependencies: Release provenance item and logging/diagnostics review.
  Risks: Redaction failures can leak sensitive torrent or network data; do not call cloud AI services from the app.
  Acceptance criteria: A support bundle can be generated offline; redaction tests cover common sensitive fields; output includes a warning and preview; no network request is made; optional summary is clearly local/export-only.
  Research references: `RESEARCH.md` > `New Feature Opportunities` and `Risks, Constraints, and Assumptions`; local `src/app/filelogger.cpp`, `src/gui/executionlogwidget.*`; paid-product support/trust patterns.
