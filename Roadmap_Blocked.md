# Blocked Roadmap Items

Items moved here from ROADMAP.md because they depend on external resources, upstream cherry-picks, build environment verification, or decisions that require human judgment. Move items back to ROADMAP.md once the blocker is resolved.

## Blocked by local-only GitHub policy

- [ ] P0 - Add current-branch Windows CI before rebase automation
  Status: Blocked
  Category: Developer Experience
  Priority: P0
  Blocker: Repository policy prohibits GitHub Actions workflows; all build, test, release, and artifact verification must run locally on this machine.
  Original description: GitHub Actions builds Windows Release from a clean checkout, runs CMake tests when enabled, runs WebUI lint with locked dependencies, and uploads logs/artifacts for failed release builds.

## Blocked on upstream rebase / repo access

- [ ] P0 — Rebase onto Enhanced Edition v5.2.1.10
  Status: Proposed
  Category: Platform
  Priority: P0
  Blocker: Requires adding Enhanced Edition as upstream remote, XL rebase across all source files, and build verification. Vanced-specific files (uithememanager.cpp, progressbarpainter.cpp, mainwindow.cpp, torrentcardswidget.cpp, traypopupwidget.cpp, statusbar.cpp, batchoperationsdialog.cpp, portablebackupdialog.cpp, installer.nsi, build scripts, CI) will have merge conflicts.
  Original description: Enhanced v5.2.1.10 (May 31, 2026) incorporates upstream qBittorrent v5.2.0 features + v5.2.1 SSRF fix. Absorbs WebAPI key auth, WebUI virtual list rendering, per-category share limits, mobile improvements, and theme system expansion.

- [ ] P0 — Extract Catppuccin Mocha colors into a bundled .qbtheme file
  Status: Proposed
  Category: Theme
  Priority: P0
  Blocker: Requires build environment to verify .qbtheme compilation and theme loading. Also benefits from rebase first, since upstream 5.2.0 expanded .qbtheme to support ProgressBar and PiecesBar color customization. Hard-coded colors in 5 files: uithememanager.cpp, progressbarpainter.cpp, torrentcardswidget.cpp, traypopupwidget.cpp, statusbar.cpp.
  Original description: Move Vanced's palette into the existing .qbtheme system to eliminate hundreds of lines of hard-coded values, enable flavor switching, and align with upstream architecture.

- [ ] P0 — Remove CSP unsafe-inline from WebUI
  Status: Proposed
  Category: Security
  Priority: P0
  Blocker: Upstream qBittorrent 5.2.x still uses unsafe-inline. The WebUI has 40+ inline event handlers (onclick, onchange, etc.) across HTML files that must be refactored to external addEventListener calls. Cannot verify without running WebUI. Should be coordinated with rebase to avoid double-work.
  Original description: Refactor MooTools-based WebUI to use external scripts with nonce-based CSP instead of unsafe-inline.

## Blocked on upstream 5.2.x analysis / cherry-picks

- [ ] P0 — Create a qBittorrent/Enhanced 5.2.x backport matrix
  Status: Proposed
  Category: Developer Experience
  Priority: P0
  Blocker: Requires systematic analysis of upstream qBittorrent v5.2.0-v5.2.2 and Enhanced v5.2.1.10 commits, which needs repo access and commit-by-commit review.
  Original description: Build a machine-readable matrix grouped by security, WebUI, WebAPI, RSS/search, theme, installer, dependencies, and platform; map each upstream/Enhanced change to Vanced status.

- [ ] P1 — Harden WebUI session and CSP boundaries
  Status: Proposed
  Category: Security
  Priority: P1
  Blocker: Requires analysis of upstream qBittorrent issue #17598 patches and 5.2.x session/CSP changes to determine safe backport path.
  Original description: Default WebUI no longer requires broad unsafe-inline, session validation has explicit tested rule, CSRF behavior is covered.

- [ ] P1 — Add scoped WebAPI integration tokens
  Status: Proposed
  Category: Integration
  Priority: P1
  Blocker: Requires backporting or adapting upstream 5.2.x API-key support; needs upstream commit analysis.
  Original description: Let Sonarr, autobrr, qbit_manage authenticate without sharing the primary WebUI password.

- [ ] P1 — Backport upstream 5.2.x WebUI performance and mobile baseline
  Status: Proposed
  Category: Performance
  Priority: P1
  Blocker: Requires cherry-picking upstream virtual list/table performance patches, mobile fixes, Safari fixes from 5.2.x branch.
  Original description: Cherry-pick or reimplement upstream virtual list/table performance, hidden-page polling, mobile footer/tab fixes.

## Blocked on build environment / testing infrastructure

- [ ] P0 - Run C++ test suite in the local Windows build
  Status: Blocked
  Category: Testing
  Priority: P0
  Blocker: Qt6 Test (`qtbase[testlib]`) is not installed in the default vcpkg manifest tree. A direct attempt to add `testlib` triggered a local vcpkg `qtbase` debug rebuild failure; `build.ps1 -Test` now reaches `check` and fails with explicit setup guidance instead of failing CMake configure.
  Original description: Optional Windows test-build support for the local build entrypoint.

- [ ] P2 — Add QAccessibleValueInterface to custom progress bars
  Status: Proposed
  Category: Accessibility
  Priority: P2
  Blocker: Qt's built-in QAccessibleTable already exposes cell DisplayRole text ("45.2%") to screen readers. Adding QAccessibleValueInterface to a delegate-based table cell requires intercepting Qt's internal QAccessibleTableCell factory, which needs build environment testing with NVDA/Narrator to verify correctness.
  Original description: Screen readers should read progress percentage for custom-painted progress bar cells. WCAG 2.2 criterion 1.4.11 contrast verification needed.

- [ ] P1 — Add Vanced-specific theme regression fixtures
  Status: Proposed
  Category: Testing
  Priority: P1
  Blocker: Requires Qt6 test framework setup, build environment, and ability to run automated theme/paint tests.
  Original description: Automated fixtures verify built-in theme colors, progress states, and .qbtheme load/export behavior on Qt6.

- [ ] P1 — Replace the hand-rolled NSIS installer with the upstream Windows installer pipeline
  Status: Proposed
  Category: Packaging
  Priority: P1
  Blocker: Requires reconciling custom installer.nsi with upstream dist/windows scripts, and testing on a Windows machine with NSIS installed.
  Original description: One Windows installer path produces a branded Vanced setup with license page, silent install/uninstall support.

- [ ] P1 — Add accessibility coverage for Vanced custom controls
  Status: Proposed
  Category: UX
  Priority: P1
  Blocker: Requires Qt accessibility testing tools and assistive technology verification; depends on theme regression fixtures.
  Original description: Inline speed controls have descriptive accessible names, keyboard operation, visible focus.

## Blocked on build environment / multi-layer implementation

- [ ] P2 — Add category/tag automation presets for media and tracker workflows
  Status: Proposed
  Category: Integration
  Priority: P2
  Blocker: Requires new C++ data model, JSON persistence, WebAPI endpoints, and GUI dialog across multiple architectural layers. Cannot verify without build environment.
  Original description: Users can save named category/tag presets, apply them during add/import, and use the same presets from WebUI/API without right-click-only workflows.

## Blocked on prerequisite roadmap items


- [ ] P3 — Add SBOM generation to release pipeline
  Status: Proposed
  Category: Supply Chain
  Priority: P3
  Blocker: Qt 6.8+ required for SPDX v2.3 SBOM generation (-sbom configure flag). Current build targets Qt 6.5+. Depends on rebase to update Qt minimum.
  Original description: Include SPDX JSON SBOM with releases listing bundled dependencies and their versions.

- [ ] P2 — Add first-run interface profiles
  Status: Proposed
  Category: UX
  Priority: P2
  Blocker: Depends on column preset picker and filters-sidebar toggle (both in Planned Features).
  Original description: Minimal, Power User, and Automation profiles that set filters sidebar, status bar, column presets.

- [ ] P2 — Add automation recipe import/export for categories, tags, and save paths
  Status: Proposed
  Category: Integration
  Priority: P2
  Blocker: Depends on category/tag preset work (P2 roadmap item) and path validation work.
  Original description: Define a small JSON schema for named recipes; include sample recipes for common workflows.

- [ ] P2 — Add alternate WebUI compatibility smoke matrix
  Status: Proposed
  Category: Integration
  Priority: P2
  Blocker: Depends on WebAPI token work and WebUI test harness; requires external WebUI testing (VueTorrent, Flood).
  Original description: Document supported alternate WebUI install paths; run smoke checks for core operations.

## Blocked on vcpkg baseline verification

- [ ] P2 - Bump dependency baseline for libtorrent web seed credential hardening
  Status: Proposed
  Category: Security
  Priority: P2
  Blocker: Current baseline pins libtorrent 2.0.11; needs a baseline that includes 2.0.13+. Bumping the vcpkg baseline requires a full rebuild to verify Qt/Boost/libtorrent compatibility. Cannot verify without build environment.
  Original description: libtorrent 2.0.13 clears HTTP credentials on redirected web seeds and fixes Merkle proof cleanup; Vanced's vcpkg baseline should not lag known torrent-core security fixes.

## Blocked on design / human judgment

- [ ] P3 — Evaluate dual libtorrent 1.2 and 2.x release flavors with explicit v2 torrent labeling
  Status: Proposed
  Category: Platform
  Priority: P3
  Blocker: Requires architectural decision on whether to ship lt12/lt20 variants; needs maintainer judgment.
  Original description: A decision record and prototype build matrix show whether Vanced should ship lt12/lt20 variants.

- [ ] P3 — Add redacted diagnostics bundle with optional support-ready summary
  Status: Needs Validation
  Category: Diagnostics
  Priority: P3
  Blocker: Requires design decisions about redaction scope, output format, and privacy guarantees. Complex feature with data-loss risk if redaction fails.
  Original description: Export app version, library versions, build metadata with redaction; optionally generate a support-ready prompt.

## Blocked — Planned Features (need design + implementation scope)

### Theme
- Centralize the palette in a `Palette` struct; swap Catppuccin Latte/Frappe/Macchiato/Mocha at runtime
- Respect system dark/light preference on first launch
- Theme picker dialog with live preview of the torrent list and progress bars
- WebUI theme parity — several Qt-driven list items still leak stock colors into the served CSS

### Progress bars
- Per-state color keys exposed to theme JSON so downstream forks can rebrand without recompiling
- Optional glyph overlay for stalled / queued / checking states instead of inventing new colors

### UX
- Alt+Up/Down keyboard shortcut for queue reordering
  Status: Blocked
  Category: UX
  Priority: P2
  Blocker: Repository instructions prohibit adding new keyboard shortcuts. Multi-row drag queue reorder is implemented without adding a shortcut.
  Original description: Multi-row drag queue reorder with Alt+Up/Down keyboard shortcut.
- Inline editable category column (double-click) instead of right-click > set category
- Search-bar scoped filter: `seeders>10 size<4gb category:movies` mini-DSL on the main list

### Platform
- Linux AppImage and Flatpak so the fork isn't Windows-only

### Upstream hygiene
- Publish the divergence as a quilt-style patch series under `patches/` so individual tweaks are reusable and rebase is mechanical
- Weekly CI rebase against Enhanced Edition; auto-file an issue on conflict

### Nice-to-Haves
- Play-while-downloading panel using mpv + sequential piece priority
- Built-in tracker-health dashboard (announce latency, last seen, error rate per tracker)
- RSS filter regex tester that runs against the current feed items
- Peer-list ASN/country badges using a bundled offline GeoLite DB
- Crash-dump toggle (opt-in, local-only) — no telemetry, ever
