# Research - qBittorrent Vanced

## Executive Summary
qBittorrent Vanced is a Windows-first C++/Qt6 fork of qBittorrent Enhanced Edition v5.1.3.10 that adds Catppuccin Mocha styling, custom progress painting, streamlined defaults, mobile WebUI polish, and local packaging around a mature qBittorrent/libtorrent core. Its strongest current shape is an opinionated, privacy-preserving desktop/WebUI fork; the highest-value direction is to make local release verification, upstream 5.2.x backports, and WebAPI/WebUI compatibility repeatable before adding larger UX surface. Top opportunities: P0 local release gate after GitHub Actions removal; P0 regression coverage for recent redirect hardening; P1 reverse-proxy/WebUI host-header backport from qBittorrent 5.2.2; P1 large-library performance smoke for 10k+ torrent sessions; P1 WebAPI compatibility smoke against current automation clients; P2 libtorrent/vcpkg bump focused on web seed credential handling; P2 hardlink/atomic-move setup validation for media automation users; P2 stronger official-source/checksum cues in installer and release assets.

## Product Map
- Core workflows: add torrent/magnet/URL, manage queue and priorities, inspect peers/trackers/files, tune global speeds, set categories/tags/locations, operate through WebUI/API.
- User personas: Windows desktop users who want qBittorrent with a polished dark UI; Enhanced Edition users who want anti-leecher behavior; self-hosted users managing qBittorrent through WebUI; Sonarr/Radarr/autobrr/qbit_manage users depending on stable categories, tags, paths, and API behavior; maintainers rebasing a branded fork.
- Platforms and distribution: CMake/Ninja/MSVC/vcpkg source build, Qt6 Widgets desktop app, bundled WebUI, NSIS installer, portable packaging scripts, Windows x64 README target; inherited Unix/macOS files remain in tree but Vanced release automation is Windows-centered.
- Key integrations and data flows: libtorrent session core, Qt desktop UI, WebUI under `src/webui/www`, WebAPI controllers under `src/webui/api`, filesystem paths and hardlinks, Windows associations for `.torrent`/`magnet:`, release metadata from `src/base/version.h.in`, `dist/windows/config.nsh`, `installer.nsi`, and `vcpkg.json`.

## Competitive Landscape
- qBittorrent upstream: strong 5.2.2 cadence, published library versions, WebUI/WebAPI fixes, API keys, and reverse-proxy header hardening. Learn from its backport stream and disclosure discipline; avoid drifting away from expected qBittorrent behavior.
- qBittorrent Enhanced Edition: direct parent fork with anti-leecher defaults and 5.2.1.10 ecosystem packaging. Learn by tracking Enhanced first, then vanilla upstream; avoid losing the Enhanced feature delta during rebases.
- Transmission: lean multi-platform client with a stable RPC model and recent fixes for filesystem/RPC error reporting and queue persistence. Learn from small, predictable remote-control contracts; avoid feature sprawl that makes simple workflows harder.
- Deluge: daemon/WebUI/plugin architecture with 2.2.0 security fixes, v2 torrent support, dark/light themes, and WebUI recovery fixes. Learn from explicit plugin boundaries and WebUI security fixes; avoid importing a heavy plugin system into this fork.
- BiglyBT: power-user client with advanced tags, rate limits by tag/peer/network, I2P, VPN detection, and swarm merging. Learn from tag/rate automation and privacy cues; avoid anonymity claims or network features Vanced cannot safely verify.
- VueTorrent and Flood: responsive alternate WebUIs prove mobile/touch ergonomics, dark/system themes, and high-density list workflows are table-stakes. Learn through compatibility tests and incremental WebUI backports; avoid a full rewrite before security and API guardrails.
- autobrr, qbit_manage, and TRaSH Guides: automation ecosystem treats qBittorrent categories, tags, save paths, hardlinks, and cleanup as operational contracts. Learn from their presets and dry-run habits; avoid turning Vanced into a media manager.
- BitTorrent Web, Seedr, put.io, and Premiumize: commercial products monetize streaming, mobile continuity, support, antivirus/VPN, cloud access, RSS automation, WebDAV/FTP, and API access. Learn trust cues and low-friction add/play patterns; avoid ads, bundled VPN/AV, telemetry, accounts, or cloud storage.

## Security, Privacy, and Reliability
- Verified: `src/base/net/downloadhandlerimpl.cpp` now allowlists redirect schemes for HTTP/HTTPS while preserving magnet handoff; this needs a regression test because upstream qBittorrent 5.2.1 shipped an SSRF redirect fix and Vanced carries its own adaptation.
- Verified: `src/webui/webapplication.cpp` still depends on broad inline-script-compatible CSP and reverse-proxy/session logic; qBittorrent 5.2.2 added an X-Forwarded-Host guard and the community continues to report CSRF/Host/reverse-proxy breakage.
- Verified: `src/webui/api/torrentscontroller.cpp` now validates set-location/save-path writability, but media automation workflows still need hardlink/atomic-move guidance so valid paths do not silently break Sonarr/Radarr imports.
- Verified: `vcpkg.json` is still pinned to Vanced 1.0.1 metadata while the repo, README, installer, and executable identity are 1.0.2; release gates should fail on that mismatch.
- Verified: `.github/workflows` is absent by policy, so the repo needs a local-only release gate that replaces deleted CI with deterministic build, lint, package, checksum, and smoke commands.
- Verified: the WebUI toolchain is now pinned in `src/webui/www/package.json` and lockfile, but WebAPI compatibility is unverified against current qBittorrent 5.2.x clients and automation tools.
- Missing guardrails: redirect regression test, reverse-proxy WebUI tests, 10k+ torrent performance fixture, WebAPI client compatibility smoke, dependency advisory capture, installer/source authenticity cues, and local release rollback instructions.
- Recovery needs: package output should be reproducible from one local command; release artifacts should include checksums, dependency versions, and an advisory snapshot; automation-sensitive path changes should explain hardlink and filesystem boundaries before the user commits.

## Architecture Assessment
- Build/release boundary: `build.ps1`, `build.bat`, `package.ps1`, `build_dist.sh`, `installer.nsi`, and `dist/windows/config.nsh` are separate paths that can diverge; consolidate verification around one local release gate without reintroducing remote CI.
- Version boundary: README badges, repo working notes, `vcpkg.json`, `src/base/version.h.in`, `src/qbittorrent.exe.manifest`, `dist/windows/config.nsh`, and `installer.nsi` should be checked together before every package.
- WebUI/API boundary: `src/webui/webapplication.cpp` and `src/webui/api/*controller.cpp` should be backported incrementally from qBittorrent 5.2.x with targeted tests for auth, Host/X-Forwarded-Host, API key behavior, categories/tags, and large transfer lists.
- Theme boundary: `src/gui/uithememanager.cpp`, `src/gui/progressbarpainter.cpp`, `src/gui/statusbar.cpp`, `src/gui/traypopupwidget.cpp`, and WebUI CSS still contain fork-specific color behavior; prior roadmap items to move color data into `.qbtheme` remain valid.
- Performance boundary: transfer list, sync payload, WebUI checkbox updates, and queue logic need a synthetic large-library fixture because upstream issue reports show 10k+ torrent sessions can expose CPU and queueing regressions.
- Test gaps: C++ test execution is blocked on Qt Test availability, but local release scripts can still run WebUI lint, executable version smoke, installer metadata checks, package hash/provenance checks, and API smoke when a fixture instance is available.
- Category coverage: security is focused on redirects, WebUI/reverse proxy, dependency updates, and release trust; accessibility remains covered by existing progress/control work plus WCAG focus/contrast references; i18n/l10n is a risk around inherited translations and installer/manpage text; observability is local logs/provenance; testing is local-only; docs are source/authenticity and automation setup; distribution is Windows installer/portable first; plugin ecosystem is intentionally rejected; mobile is WebUI/alternate-WebUI compatibility; offline/resilience is local packages and rollback; multi-user is limited to WebUI/API auth; migration/upgrade strategy is Enhanced/qBittorrent 5.2.x backporting.

## Rejected Ideas
- Bundled VPN, antivirus scanning, ads, paid support tiers, or monetized installers (BitTorrent/uTorrent): contradicts GPL/local-first trust and adds high-risk third-party dependencies.
- Cloud torrent storage, account sync, hosted streaming, or remote WebDAV/FTP storage (Seedr, put.io, Premiumize): valuable commercially but wrong for a desktop/WebUI fork.
- Full WebUI replacement as the next step (VueTorrent/Flood): useful as compatibility and UX evidence, but a rewrite would increase drift before security, API, and release gates are stable.
- Large plugin ecosystem (Deluge/BiglyBT): useful pattern, but too much maintenance/security surface for a focused fork.
- DHT crawling/discovery dashboard (BEP 51 and academic DHT crawling): privacy and legal risk exceed the value for Vanced users.
- Native mobile app: responsive WebUI and alternate WebUI support address the verified mobile need with lower maintenance.
- Anonymity/I2P claims (BiglyBT/Tribler): Vanced should avoid privacy claims it cannot implement and verify end-to-end.

## Sources
### Project and Upstream
- https://www.qbittorrent.org/news
- https://github.com/qbittorrent/qBittorrent/wiki/API-Key-Authentication-%28%E2%89%A5v5.2.0%29
- https://github.com/qbittorrent/qBittorrent/wiki/Create-custom-themes-for-qBittorrent
- https://github.com/qbittorrent/qBittorrent/wiki/List-of-known-alternate-WebUIs
- https://github.com/c0re100/qBittorrent-Enhanced-Edition/releases
- https://github.com/qbittorrent/qBittorrent/issues/17504
- https://github.com/qbittorrent/qBittorrent/issues/9143
- https://github.com/qbittorrent/qBittorrent/issues/21106
- https://github.com/qbittorrent/qBittorrent/issues/21673
- https://github.com/qbittorrent/qBittorrent/issues/23384

### Competitors and Automation
- https://github.com/transmission/transmission/releases/
- https://deluge.readthedocs.io/en/deluge-2.2.0/changelog.html
- https://www.biglybt.com/features.php
- https://flood.js.org/
- https://vuetorrent.com/
- https://github.com/StuffAnThings/qbit_manage
- https://autobrr.com/filters/actions
- https://trash-guides.info/Downloaders/qBittorrent/How-to-add-categories/
- https://trash-guides.info/File-and-Folder-Structure/Hardlinks-and-Instant-Moves/

### Commercial and Community
- https://www.bittorrent.com/products/win/web-premium/
- https://www.seedr.cc/
- https://put.io/
- https://www.premiumize.me/features
- https://www.reddit.com/r/selfhosted/comments/w3xsao/preferred_torrent_client_with_mobile_friendly/

### Standards, Dependencies, and Security
- https://www.bittorrent.org/beps/bep_0052.html
- https://www.bittorrent.org/beps/bep_0051.html
- https://github.com/arvidn/libtorrent/releases
- https://www.qt.io/blog/qt-6.10.3-released
- https://devblogs.microsoft.com/cppblog/whats-new-in-vcpkg-may-2026/
- https://www.w3.org/TR/WCAG22/

## Open Questions
- Should Vanced stay Windows-only for the next release, or should inherited Linux/macOS packaging be actively validated before it remains in user-facing docs?
- Should Vanced backport qBittorrent 5.2.x WebAPI API-key support exactly, or expose only a smaller integration-token subset until the full Enhanced rebase lands?
