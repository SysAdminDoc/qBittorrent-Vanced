# Research — qBittorrent Vanced

## Executive Summary
qBittorrent Vanced is a Windows-first themed fork of qBittorrent Enhanced Edition v5.1.3.10, adding a Catppuccin Mocha desktop look, shimmer progress bars, a simplified main window, inline speed controls, low default upload, and stop-on-completion defaults. The strongest current shape is a small, opinionated desktop UX fork on top of a mature torrent engine; the highest-value direction is to make that fork trustworthy and cheap to rebase before adding more surface area. Top opportunities: (1) resolve license/version/release identity conflicts, (2) fix the Windows build entrypoint and add smoke coverage, (3) restore current-branch CI and WebUI dependency determinism, (4) rebase toward Enhanced 5.2.x/qBittorrent 5.2.x after guardrails exist, (5) move Vanced styling into the existing `.qbtheme` pipeline where possible, (6) harden WebUI session/CSP behavior, (7) close docs/i18n encoding and issue-routing gaps, (8) improve mobile WebUI add/manage flows, and (9) publish dependency/security provenance with releases.

## Product Map
- Core workflows: add magnet/torrent, manage transfer queue, monitor progress/status, control speeds, set categories/tags/locations, use WebUI remotely.
- User personas: desktop power users who like qBittorrent but want a quieter themed UI; seedbox/home-server users using WebUI; media-automation users integrating Sonarr/autobrr/qbit_manage; fork maintainers rebasing against Enhanced/upstream.
- Platforms and distribution: source builds via CMake/vcpkg/Qt6; Windows scripts and NSIS installer are present; README claims Windows 10/11 x64; roadmap already targets portable ZIP, Winget/Scoop, AppImage, and Flatpak.
- Key integrations and data flows: libtorrent session core, Qt Widgets desktop UI, WebUI/API under `src/webui`, optional RSS/search plugins, filesystem save paths, Windows installer metadata, bundled WebUI JavaScript/CSS.

## Competitive Landscape
- qBittorrent upstream
  - Does well: fast 5.2.x release cadence, documented `.qbtheme` format, current dependency disclosures, WebUI/mobile/theme fixes.
  - Learn: preserve upstream muscle memory and theme APIs instead of hard-coding more patched UI behavior.
  - Avoid: inheriting upstream regressions without a Vanced smoke suite.
- qBittorrent Enhanced Edition
  - Does well: parent anti-leecher fork, active 5.2.1.10 release assets, AppImage/zsync distribution, upstream backport cadence.
  - Learn: keep Vanced delta patchable and visible because Enhanced is the real integration base.
  - Avoid: ambiguous release identity between Vanced, Enhanced, and vanilla qBittorrent.
- Transmission
  - Does well: lean cross-platform client, simple RPC/WebUI mental model, active 4.1.x releases.
  - Learn: keep remote control predictable and lightweight.
  - Avoid: feature-thin categorization/tag workflows that media-automation users still need.
- Deluge and BiglyBT
  - Do well: plugin/daemon patterns, rich power-user features, recovery fixes in WebUI and plugin flows.
  - Learn: optional extension boundaries and explicit recovery behavior.
  - Avoid: importing a heavy plugin ecosystem into a small UX fork.
- Flood, VueTorrent, and alternate qBittorrent WebUIs
  - Do well: responsive/touch-ready WebUI, dark/system themes, modern list ergonomics.
  - Learn: mobile add/manage flows and theme parity are table-stakes for WebUI users.
  - Avoid: a full WebUI rewrite before core security/release guardrails are in place.
- autobrr, Sonarr, and qbit_manage
  - Do well: category/tag/indexer automation, regex/filter validation, operational cleanup.
  - Learn: category/tag workflows should be first-class and testable.
  - Avoid: turning Vanced into a full media manager.
- BitTorrent Web, uTorrent Web, Seedr, and put.io
  - Do well: streaming while downloading, mobile/cloud access, low-friction add/play flows, commercial support positioning.
  - Learn: quick add/play and mobile continuity drive perceived value.
  - Avoid: ads, bundled VPN/AV upsells, cloud-account dependency, or privacy-hostile telemetry.

## Security, Privacy, and Reliability
- (Verified) License identity conflicts are user-trust risks: `README.md` has an MIT badge, `LICENSE` is MIT, `COPYING` says GPLv2+/GPLv3+ conditions, `dist/windows/license.txt` is GPL, and `installer.nsi` declares GPL-2.0.
- (Verified) Version identity is split across `README.md`, repo `CLAUDE.md`, `src/base/version.h.in`, `dist/windows/config.nsh`, `installer.nsi`, and `vcpkg.json`; release artifacts can report different product/upstream versions.
- (Verified) `build.ps1` changes into `C:\Users\--\repos\qBittorrent-Enhanced-Edition`, while the README tells users to run it from this repo; that blocks reproducible Windows builds from a clean checkout.
- (Verified) There is no `.github/` workflow tree, while the project is a fork of two active upstreams and carries UI/security-sensitive patches.
- (Verified) WebUI hardening is incomplete: `src/webui/webapplication.cpp` still allows global `unsafe-inline` in CSP and has `// TODO: Additional session check`; upstream users have open CSRF/session concerns.
- (Verified) `src/webui/www/package.json` uses wildcard devDependencies and no lockfile, so lint/format/security behavior can drift between releases.
- (Verified) `doc/ru/*.md` contains mojibake and `doc/en/*.md` still route bug reports to upstream qBittorrent; docs need an explicit Vanced/Enhanced/upstream issue policy.
- Missing guardrails: release SBOM/dependency provenance, checksum publication, installer license page consistency, current-branch CI, WebUI mobile smoke tests, and a documented rollback path for rebasing off Enhanced.
- Recovery and rollback needs: keep Vanced customizations as patchable theme/build deltas, archive release manifests with dependency versions, and make build scripts runnable from any checkout path.

## Architecture Assessment
- Theme boundary: `src/gui/uithememanager.cpp`, `src/gui/progressbarpainter.cpp`, tray/status styles, and inline speed controls embed Vanced colors in code even though `src/gui/uithemecommon.h`, `src/gui/uithemesource.*`, and the upstream wiki support `.qbtheme` assets with `config.json` and `stylesheet.qss`.
- Build/release boundary: `installer.nsi` duplicates the richer `dist/windows/*.nsh` installer pipeline and hard-codes product/version/license fields; use one source of truth for release metadata.
- WebUI boundary: `src/webui/www/private` remains upstream-style HTML/CSS/JS; mobile UX and CSP changes should be incremental and tested against the existing WebAPI rather than replaced wholesale.
- Refactor candidates: `build.ps1` path/configuration handling, version/license constants across `src/base/version.h.in` and `dist/windows/config.nsh`, WebUI security header construction in `src/webui/webapplication.cpp`, theme color routing out of `src/gui/uithememanager.cpp`, and docs generation for `doc/**`.
- Test gaps: no current CI workflow, no deterministic WebUI dependency install, no release smoke proving the branded executable reports the intended version/license, no visual/theme regression fixture for progress bars, and no mobile WebUI smoke.
- Documentation gaps: README build/distribution claims are ahead of verified automation, issue-reporting routes are inconsistent, and localized manpage sources need UTF-8 recovery.
- Category coverage: security is centered on WebUI/release trust, accessibility is already represented by the existing progress alt-style roadmap and needs regression coverage, i18n/l10n is the `doc/ru` repair, observability is CI/release logs plus provenance, testing is current-branch CI and fixtures, docs are issue-routing/manpage fixes, distribution is installer/provenance, plugin ecosystem and multi-user expansion are rejected as misfit for now, mobile is WebUI-first, offline/resilience is release rollback and local builds, migration/upgrade strategy is patchable rebasing toward Enhanced/qBittorrent 5.2.x.

## Rejected Ideas
- Bundled VPN, antivirus scanning, ad removal tiers, or paid support upsells (BitTorrent/uTorrent): contradicts the local-first GPL desktop fork and privacy posture.
- Cloud torrent storage/account features (Seedr/put.io): useful commercially but misfit for an offline desktop/WebUI client.
- Full WebUI rewrite now (Flood/VueTorrent/qbtmud): good inspiration, but too much drift before build, release, and WebUI security guardrails exist.
- Big plugin ecosystem (Deluge/BiglyBT): high maintenance cost and dependency surface for a small UX fork.
- DHT crawler/discovery tooling (BEP 51 and DHT research): privacy/legal risk and not aligned with the project’s streamlined-client purpose.
- Native mobile app: mobile WebUI fixes and optional alternate WebUI packaging solve the verified pain with lower maintenance.
- Tracker-health dashboard as first wave: already on the roadmap as a nice-to-have; release trust and WebUI hardening rank higher.

## Sources
### Project and Upstream
- https://www.qbittorrent.org/download
- https://www.qbittorrent.org/news
- https://github.com/qbittorrent/qBittorrent/wiki/Create-custom-themes-for-qBittorrent
- https://github.com/qbittorrent/qBittorrent/wiki/List-of-known-alternate-WebUIs
- https://github.com/c0re100/qBittorrent-Enhanced-Edition/releases
- https://github.com/qbittorrent/qBittorrent/issues/8887
- https://github.com/qbittorrent/qBittorrent/issues/17598
- https://github.com/qbittorrent/qBittorrent/issues/23900

### Competitors and Adjacent Projects
- https://transmissionbt.com/
- https://deluge.readthedocs.io/en/deluge-2.2.0/changelog.html
- https://github.com/BiglySoftware/BiglyBT/releases
- https://flood.js.org/
- https://github.com/VueTorrent/VueTorrent/wiki/Installation
- https://autobrr.com/introduction
- https://autobrr.com/3rd-party-tools/manage-torrents
- https://github.com/Sonarr/Sonarr/issues/6777
- https://trash-guides.info/Downloaders/qBittorrent/How-to-add-categories/

### Commercial and Community
- https://www.bittorrent.com/products/win/web-premium/
- https://www.seedr.cc/
- https://put.io/
- https://www.reddit.com/r/selfhosted/comments/w3xsao/preferred_torrent_client_with_mobile_friendly/

### Standards, Dependencies, and Packaging
- https://www.bittorrent.org/beps/bep_0052.html
- https://www.bittorrent.org/beps/bep_0051.html
- https://jhalderm.com/pub/papers/dht-woot10.pdf
- https://github.com/arvidn/libtorrent/releases
- https://openssl-library.org/news/timeline/
- https://www.qt.io/blog/qt-6.10.3-released
- https://www.boost.org/releases/
- https://devblogs.microsoft.com/cppblog/whats-new-in-vcpkg-may-2026/
- https://learn.microsoft.com/en-us/windows/package-manager/package/manifest

## Open Questions
- What canonical release identity should Vanced expose: a Vanced app version, the Enhanced base version, the vanilla qBittorrent version, or all three in distinct fields?
- Is Vanced intentionally Windows-first for the next release, or should CI and release metadata immediately validate Linux/macOS targets promised by the roadmap?

## Research Pass — 2026-06-19 Expansion

### Executive Summary
This pass preserves the prior research and sharpens the roadmap around evidence that was either new or not converted into actionable work. qBittorrent Vanced is still best treated as a small, privacy-preserving UX fork, not a cloud torrent service or media manager. The most urgent incremental findings are: upstream qBittorrent 5.2.1 fixed an HTTP redirect SSRF class that this fork's `DownloadHandlerImpl::handleRedirection()` does not appear to allowlist yet; upstream 5.2.0/5.2.2 added a dense set of WebUI, WebAPI, mobile, theme, RSS, and installer changes that should be backported deliberately; comparable WebUIs make responsive/touch-first flows table-stakes; commercial products monetize streaming, mobile continuity, antivirus/VPN, and cloud storage, but only the low-friction add/play, diagnostics, and trust patterns fit this local GPL fork.

### Current Project Understanding
- Project type: C++20/Qt6 Widgets desktop BitTorrent client fork with bundled WebUI, CMake/Ninja/vcpkg build, NSIS Windows packaging, and upstream qBittorrent/libtorrent core.
- Current target: README and build scripts are Windows-first, while inherited `dist/mac`, `dist/unix`, `qbittorrent-nox`, and roadmap content still imply broader platform ambitions.
- Core workflows: add torrent/magnet/URL, configure save path/category/tags, monitor transfers, filter/search/sort, inspect trackers/peers/files, control global speed, use WebUI/API, package Windows installer/portable builds.
- Target users: users who prefer qBittorrent/Enhanced behavior but want a darker, cleaner desktop; self-hosted users controlling qBittorrent remotely; media-automation users whose workflows depend on categories/tags; maintainers rebasing a branded fork.
- Hard constraints: GPL-family licensing inherited from qBittorrent, Qt6/libtorrent/vcpkg dependency floor, no telemetry posture, and high rebase risk because Vanced sits on Enhanced, which sits on qBittorrent.

### Existing Feature Inventory
- Desktop branding and UX: Vanced title/about/tray branding, Catppuccin Mocha built-in theme, compact UI, hidden status/filters defaults, custom inline DL/UL speed buttons in `src/gui/mainwindow.cpp`.
- Transfer visualization: green custom shimmer progress bars in `src/gui/progressbarpainter.cpp`, hard-coded progress/state colors, existing roadmap item for reduced-motion/alt style.
- WebUI/API: upstream-style WebUI under `src/webui/www`, WebAPI controllers under `src/webui/api`, existing security toggles for CSRF, Host header validation, SameSite cookies, HTTPS, alternate WebUI folder.
- Automation primitives: categories, tags, watch folders, RSS/search code still present, qBittorrent WebAPI compatibility, existing `AutoCategoryManager`, and media-stack compatibility via existing WebAPI.
- Packaging/build: CMake feature flags, `build.ps1`, `build.bat`, upstream `dist/windows` installer scripts, separate root `installer.nsi`, vcpkg manifest, 15 test executables under `test/` but `TESTING=OFF` in the documented build.

### Existing Roadmap and Research Observations
- Supported by new research: the existing P0/P1 items on license/version identity, build entrypoint, CI, WebUI reproducibility, WebUI session/CSP hardening, theme regression fixtures, installer consolidation, mobile WebUI, category/tag presets, and dependency provenance remain valid.
- Modified by new research: upstream qBittorrent uses `.qbtheme`, not `.qbttheme`; existing roadmap wording that says `.qbttheme` is stale terminology and should be interpreted as `.qbtheme` when implemented.
- Supported but not duplicated: existing items for WebUI theme parity, progress alt style, palette centralization, portable ZIP, Winget/Scoop, AppImage/Flatpak, patch series, weekly rebase, RSS regex tester, and tracker dashboard already cover those opportunities.
- Newly separated: the upstream 5.2.1 HTTP redirect SSRF fix is a concrete security backport, not just a general "rebase someday" concern.
- Newly separated: upstream 5.2.x WebAPI API-key support is a specific integration/security opportunity that aligns with Sonarr/autobrr/qbit_manage usage and should not be bundled with visual WebUI polish.

### Comparable Paid Products Researched
| Product | URL | What it does | Relevant features or pricing | Why it matters here | Applicability |
| --- | --- | --- | --- | --- | --- |
| BitTorrent Web Premium | https://www.bittorrent.com/products/win/web-premium/ | Commercial browser-centered torrent client. | Web Pro at $19.95/year, Pro+VPN at $69.95/year; commercial positioning around threat protection, privacy/VPN, and premium support. | Shows users pay for trust cues, low-friction Web UX, streaming, and no-ad focus. | Partially applicable: trust/diagnostics and add/play UX fit; VPN/AV/ads do not. |
| Bitport.io | https://bitport.io/pricelist | Cloud torrent downloader/storage/streamer. | Free 1 GB tier; paid 75 GB/250 GB/1 TB tiers from $5/$10/$15 monthly; HTTPS download, antivirus checked downloads, online streaming, RSS, Google Drive sync on higher tiers. | Paid competitors package safety, streaming, RSS automation, and remote access as user value. | Partially applicable: RSS/import/reporting patterns fit; cloud storage/antivirus scanning do not. |
| Seedr | https://www.seedr.cc/pricing/ | Cloud fetching, storage, and streaming service. | Plans emphasize mobile/device access, streaming, FTP/WebDAV on higher tiers. | Reinforces mobile continuity and "fetch now, watch anywhere" as high-value flows. | Partially applicable: mobile WebUI and WebDAV-style handoff patterns fit; cloud account model does not. |
| Premiumize.me | https://www.premiumize.me/premium | Paid cloud storage/debrid service. | 1 TB storage, remote upload by URL, cloud sync, WebDAV/FTP/SFTP, RSS automation, developer API; monthly and yearly pricing. | Its pricing page exposes automation and API access as premium differentiators. | Partially applicable: API ergonomics, RSS automation, and exportable recipes fit; service backend does not. |
| put.io | https://put.io/ | Paid cloud torrent/file fetching and streaming service. | WebDAV/FTP, browser extensions, apps, API, console/TV playback. | Browser/magnet capture, API access, and device continuity are recurring paid UX themes. | Partially applicable: magnet-handler polish and API docs fit; hosted file storage does not. |
| Real-Debrid | https://real-debrid.com/ and https://api.real-debrid.com/ | Paid unrestricted downloader with torrent/cache APIs. | REST API has clear error codes, rate limits, torrent add/select/delete endpoints, streaming metadata endpoints. | Demonstrates robust API contracts for automation clients and external integrations. | Partially applicable: API-key and error-contract design fit; debrid/hoster integration does not. |

### Comparable Open Source Projects Researched
| Project | URL | What it does | Relevant features or patterns | Why it matters here | Applicability |
| --- | --- | --- | --- | --- | --- |
| qBittorrent upstream | https://www.qbittorrent.org/news | Canonical upstream BitTorrent client. | v5.2.2 uses Qt 6.10.3, libtorrent 1.2.20/2.0.13, Boost 1.86/1.91; v5.2.x adds mobile/performance WebUI changes, API keys, progress/piece color customization, category share limits, RSS/search fixes, and SSRF redirect fix. | Vanced inherits its security, compatibility, and user expectations. | Directly applicable as the primary backport source. |
| qBittorrent Enhanced Edition | https://github.com/c0re100/qBittorrent-Enhanced-Edition/releases | Parent fork with anti-leecher and tracker enhancements. | Current branch is `v5_2_x`; latest releases package Windows and Linux artifacts. | Vanced should track Enhanced before vanilla qBittorrent where patches conflict. | Directly applicable as the integration base. |
| Transmission | https://transmissionbt.com/ | Lean multi-platform torrent client with web interface and RPC. | Simple interface, web UI, tracker editing, privacy controls; recent 4.1.x releases emphasize bug/performance fixes. | Sets the bar for lightweight remote control without overwhelming UI. | Partially applicable: keep Vanced simple while restoring power-user paths. |
| Deluge | https://deluge.readthedocs.io/en/deluge-2.2.0/changelog.html | libtorrent-based client with daemon/WebUI/plugin model. | Plugin ecosystem, daemonized WebUI, recent cache/WebUI/security fixes. | Good cautionary evidence for plugin/security surface and recovery testing. | Partially applicable: test/recovery patterns fit; plugin expansion is not recommended now. |
| BiglyBT | https://www.biglybt.com/features.php | Feature-rich Java BitTorrent client. | Advanced tag system, per-tag/per-peer/per-network rate limits, swarm merging, I2P, VPN detection, plugin depth. | Shows power-user demand for tag/rate automation and privacy cues. | Partially applicable: tag/rate presets fit; swarm merging/I2P/plugin bloat does not. |
| VueTorrent | https://github.com/VueTorrent/VueTorrent | Modern qBittorrent WebAPI frontend. | Responsive WebUI skin compatible with qBittorrent automation and alternate WebUI deployment. | Validates mobile/responsive WebUI as a core user expectation. | Directly applicable for compatibility testing and UX patterns. |
| Flood | https://flood.js.org/ | Modern WebUI for rTorrent, Transmission, and qBittorrent. | Touch-ready responsive layout, collapsible sidebar, light/dark/system theme support. | Shows UX patterns Vanced can adopt without a full rewrite. | Partially applicable: responsive patterns fit; Node service architecture does not. |
| autobrr | https://autobrr.com/introduction | Modern RSS/IRC download automation tool. | Filter-driven grabs, actions to download clients, private tracker workflows. | Vanced users likely integrate with it through qBittorrent categories/tags/API. | Partially applicable: recipe presets and docs fit; duplicating autobrr does not. |
| qbit_manage | https://github.com/StuffAnThings/qbit_manage | qBittorrent automation/cleanup tool. | Tag/category automation, orphan cleanup, unregistered torrent removal, share-limit management. | Its existence proves repetitive category/tag cleanup is a pain point. | Directly applicable for presets, dry-run, and safer bulk actions. |
| Tribler | https://www.tribler.org/index.html | Decentralized/privacy-focused torrent client. | Search without websites, onion-style routing, explicit privacy caveats. | Useful as a privacy warning source: anonymity claims must stay careful. | Not suitable for near-term implementation; informs wording and privacy guardrails only. |

### Feature Gap Analysis
- Security gap: `src/base/net/downloadhandlerimpl.cpp` follows `QNetworkRequest::RedirectionTargetAttribute` through `DownloadManager::download()` without the upstream 5.2.1-style redirect scheme allowlist; `DownloadManager::hasSupportedScheme()` currently delegates to all schemes supported by Qt.
- WebAPI integration gap: local scan found no API-key/basic-auth additions from upstream 5.2.x, limiting clean integration for automation tools that should not reuse the full admin password.
- WebUI performance/mobile gap: upstream 5.2.x added virtual list performance, hidden-page polling behavior, mobile footer/tab/window fixes, search mobile improvements, tracker management, and new Add Torrent experience; this fork is still rooted in 5.1.3-era WebUI.
- Accessibility gap: Vanced custom inline speed buttons use custom style and short `D:`/`U:` text with tooltips but no explicit accessible names; custom shimmer and compact controls need keyboard/focus/non-text contrast checks against WCAG 2.2 principles.
- Automation gap: qBittorrent categories/tags exist, but no Vanced-specific guided presets for Arr/autobrr/qbit_manage workflows or safer dry-run previews for bulk cleanup.
- Recovery gap: local TODOs around `setLocation`/destination validation and existing destructive/bulk flows need clearer preflight, failure messages, and rollback expectations.
- Diagnostics gap: commercial products monetize trust/support; Vanced should offer local-only support bundles, dependency/version capture, and redaction rather than telemetry.

### Improvement Opportunities for Current Features
- Inline speed controls: add accessible names, keyboard shortcuts, overflow-resistant text, and theme-driven colors; avoid relying on tooltips as the only explanation.
- Catppuccin theme: keep moving hard-coded colors from `src/gui/uithememanager.cpp`, `src/gui/mainwindow.cpp`, `src/gui/progressbarpainter.cpp`, and `src/gui/trackerlist/trackerlistmodel.cpp` into the existing `.qbtheme`/color API.
- WebUI: backport upstream 5.2.x performance/mobile patches before building Vanced-specific visual parity; otherwise Vanced-specific CSS will be applied to stale structure.
- Add/move/delete flows: add path existence/permission checks, clearer validation errors, and explicit "what happens to files" copy before destructive actions.
- Docs: correct stale `.qbttheme` wording to `.qbtheme` in future edits, explain Vanced/Enhanced/vanilla version fields, and document supported alternate WebUI compatibility.

### New Feature Opportunities
- First-run interface profiles: let users choose Minimal, Power User, or Automation defaults so streamlined Vanced defaults do not hide categories/tags/status from users who need them.
- Scoped WebAPI integration tokens: adopt upstream API-key ideas for Sonarr/autobrr/qbit_manage without handing full WebUI session credentials to every tool.
- Automation recipe import/export: ship local presets for common category/tag/save-path workflows and let users export/share safe JSON recipes.
- Alternate WebUI compatibility matrix: test VueTorrent/Flood/other known WebUI frontends against Vanced WebAPI and document supported/unsupported deltas.
- Local diagnostics bundle with optional AI-ready summary: export redacted logs, versions, config fingerprints, and recent errors; optionally allow the user to generate a local-only summary without bundling cloud AI.

### Technical Improvement Opportunities
- Backport upstream security fixes as isolated patches before large rebase work.
- Add a 5.2.x backport matrix mapping upstream changes to Vanced status, dependencies, and conflict files.
- Pin WebUI tooling and run WebUI lint/tests in CI before editing WebUI CSS/JS heavily.
- Build a release smoke that checks `--version`, installer metadata, library versions, SHA256, and license display together.
- Add regression tests for `DownloadManager` redirects, WebAPI auth, path validation, and custom theme color lookup.

### UX Improvement Opportunities
- Preserve Vanced's clean default while making hidden power controls discoverable through profiles and view presets.
- Use upstream 5.2.x Add Torrent and mobile WebUI patterns to reduce modal/window overflow and mobile keyboard friction.
- Add focus-visible styling and screen-reader metadata for custom buttons and icon-heavy controls.
- Expose category/tag presets in the add-torrent path, not only context menus after the torrent exists.
- Add warnings and confirmations around actions that can move/delete files or break automation imports.

### Risks, Constraints, and Assumptions
- Security assumption: absence of a local redirect allowlist in the scanned handler is enough to prioritize a backport; implementation should compare upstream patch `#24270` before coding.
- Architecture constraint: Vanced should avoid service/cloud features that require accounts, remote storage, or bundled paid security products.
- Licensing constraint: GPL identity must be resolved before broad package distribution; paid-product patterns cannot be copied if they imply closed-source service coupling.
- Rebase risk: upstream 5.2.x changed WebUI, WebAPI, theme, installer, and dependency behavior; cherry-picking without CI will increase regressions.
- Privacy constraint: any AI-assisted feature must be opt-in, local/export-only by default, and must never send torrent names, paths, IPs, trackers, cookies, or logs to a cloud endpoint automatically.

### Findings Not Added as Roadmap Items
- Bundled VPN/antivirus/cloud storage: not suitable for a local GPL fork; keep as rejected in prior research.
- Full plugin ecosystem: Deluge/BiglyBT show value but the maintenance/security surface is too large for Vanced now.
- Native mobile app: mobile WebUI and alternate WebUI compatibility are lower-risk.
- Tribler-style anonymity or decentralized search: privacy claims and architecture changes are too risky for this fork.
- Swarm merging/WebTorrent streaming as core bets: interesting, but existing roadmap already has play-while-downloading and libtorrent flavor evaluation; security/rebase work ranks higher.

### Source List
- qBittorrent news: https://www.qbittorrent.org/news
- qBittorrent downloads/library versions: https://www.qbittorrent.org/download
- qBittorrent theme wiki: https://github.com/qbittorrent/qBittorrent/wiki/Create-custom-themes-for-qBittorrent
- qBittorrent alternate WebUI wiki: https://github.com/qbittorrent/qBittorrent/wiki/List-of-known-alternate-WebUIs
- qBittorrent Enhanced releases: https://github.com/c0re100/qBittorrent-Enhanced-Edition/releases
- BitTorrent Web Premium: https://www.bittorrent.com/products/win/web-premium/
- Bitport pricing: https://bitport.io/pricelist
- Seedr pricing: https://www.seedr.cc/pricing/
- Seedr WebDAV: https://www.seedr.cc/products/webdav/
- Premiumize pricing/features: https://www.premiumize.me/premium
- put.io: https://put.io/
- Real-Debrid: https://real-debrid.com/
- Real-Debrid API: https://api.real-debrid.com/
- Transmission: https://transmissionbt.com/
- Deluge changelog: https://deluge.readthedocs.io/en/deluge-2.2.0/changelog.html
- BiglyBT features: https://www.biglybt.com/features.php
- VueTorrent: https://github.com/VueTorrent/VueTorrent
- Flood: https://flood.js.org/
- autobrr introduction: https://autobrr.com/introduction
- qbit_manage: https://github.com/StuffAnThings/qbit_manage
- TRaSH qBittorrent categories: https://trash-guides.info/Downloaders/qBittorrent/How-to-add-categories/
- Sonarr qBittorrent category/tag request: https://github.com/Sonarr/Sonarr/issues/6777
- qBittorrent WebUI API wiki: https://github.com/qbittorrent/qBittorrent/wiki/WebUI-API-%28qBittorrent-4.1%29
- WCAG 2.2: https://www.w3.org/TR/WCAG22/
- WAI accessible names/descriptions: https://www.w3.org/WAI/ARIA/apg/practices/names-and-descriptions/
- Qt accessibility overview: https://www.qt.io/development/qt-framework/ui-accessibility
