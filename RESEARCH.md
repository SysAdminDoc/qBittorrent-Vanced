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
