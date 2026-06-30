# Research - qBittorrent Vanced

## Executive Summary
[Verified] qBittorrent Vanced is a Windows-first C++/Qt6 fork of qBittorrent Enhanced Edition v5.1.3.10 on upstream qBittorrent v5.1.3, with Catppuccin desktop/WebUI styling, custom progress painting, local-only release gates, Windows installer/portable packaging, and Enhanced anti-leecher behavior. Its strongest current shape is a polished, privacy-preserving qBittorrent build for desktop and WebUI users who value no telemetry and predictable automation compatibility. Highest-value direction: keep the active roadmap focused on trust and mergeability before novelty. Top opportunities: (1) resolve stale WebUI RSS/Search surfaces that remain visible after those modules were removed; (2) add current WebUI CSRF/CORS/SameSite/X-Forwarded-Host negative tests; (3) log category/tag mutations for recovery; (4) finish the existing Linux AppImage/Flatpak item using upstream Flathub/AppImage patterns; (5) keep 5.2.x rebase, API-key auth, CSP `unsafe-inline` removal, libtorrent 2.0.13, Qt Test, SBOM, and theme extraction in `Roadmap_Blocked.md` until their blockers clear; (6) add user-supplied GeoIP/MMDB guardrails before peer ASN/country badges; (7) backfill small WebUI parity actions such as copying content file paths.

## Product Map
- Core workflows: add torrents/magnets/URLs, manage transfer queue and categories/tags, inspect files/peers/trackers, tune speed limits, operate via WebUI/WebAPI, package Windows releases, create/restore portable backups.
- User personas: Windows desktop users wanting a polished dark qBittorrent; Enhanced Edition users wanting anti-leecher defaults; self-hosted users behind reverse proxies; Sonarr/Radarr/autobrr/qbit_manage users relying on stable WebAPI behavior; maintainer rebasing a branded fork across qBittorrent and Enhanced.
- Platforms and distribution: Windows x64 NSIS installer and portable ZIP are shipped; Linux AppImage/Flatpak remains active roadmap work; inherited Unix/macOS build files exist but are not the release center.
- Key integrations and data flows: libtorrent session core, Qt6 Widgets GUI, MooTools WebUI under `src/webui/www`, WebAPI controllers under `src/webui/api`, vcpkg manifest dependencies, Windows WinGet/Scoop/AppCast release metadata.

## Competitive Landscape
- qBittorrent upstream: does well at conservative client behavior, WebUI/WebAPI parity, SameSite/X-Forwarded-Host fixes, RSS fixes, and current distribution. Learn from 5.2.2 WebUI/security fixes and avoid drifting from upstream muscle memory.
- qBittorrent Enhanced Edition: does well at anti-leecher defaults and now ships AppImage/static Linux assets. Learn from its Linux asset matrix and keep Enhanced deltas intact during rebases; avoid implementing features that bypass the parent fork.
- VueTorrent, qui, and Flood: do well at modern responsive WebUIs, alternate WebUI compatibility, API-key adoption, content-tab copy/range actions, and multi-instance management. Learn small WebUI parity actions and smoke compatibility; avoid a full rewrite while the fork is still behind 5.2.x.
- Transmission: does well at lean remote access and fast security response; 4.1.3 specifically fixed remote-access CSRF/CORS problems. Learn negative remote-access tests; avoid overcomplicating the control surface.
- PeerBanHelper and Enhanced anti-leecher ecosystem: do well at rule updates and peer-ban specialization. Learn auditability around peer decisions; avoid bundling opaque cloud rule behavior into Vanced.
- qbit_manage, cross-seed, autobrr, and TRaSH Guides: do well at dry-run automation, category/tag conventions, hardlink diagnostics, and tolerant qBittorrent API handling. Learn compatibility and logging patterns; avoid turning Vanced into a media manager.
- Deluge and BiglyBT: do well at plugins, rich tags/categories, RSS/search subscriptions, and advanced privacy features. Learn why plugin ecosystems need lifecycle governance; avoid a large plugin platform in this focused fork.

## Security, Privacy, and Reliability
- [Verified] `CLAUDE.md` states RSS/Search/TorrentCreator modules were removed, but `src/webui/www/private/index.html` still exposes Search/RSS menu entries and tab links, while `src/webui/api` lacks RSS/search controllers and `src/base` has only old RSS preference/backup remnants. This is a trust bug before any RSS tester work.
- [Verified] `src/webui/webapplication.cpp` implements CSRF origin/referer checks, SameSite cookie policy, and X-Forwarded-Host gating, but `test/webapi-smoke.ps1` and `test/testwebapplicationsecurity.cpp` do not exercise Origin/Referer/CORS/SameSite/X-Forwarded-Host failures.
- [Verified] `src/base/bittorrent/torrentimpl.cpp` records `oldCategory`, `src/base/bittorrent/sessionimpl.cpp` emits it, and `src/webui/api/synccontroller.cpp` receives and ignores it; category/tag changes are not logged even though Vanced now has inline category editing, batch operations, and auto-category paths.
- [Verified] `vcpkg.json` still pins a baseline behind upstream qBittorrent 5.2.2's libtorrent 2.0.13/Qt/OpenSSL stack. This is already in `Roadmap_Blocked.md`; do not duplicate it in the active roadmap.
- [Verified] `src/webui/webapplication.cpp` still uses CSP `unsafe-inline` because many WebUI inline handlers remain. This is already blocked pending upstream/rebase work.
- [Verified] MaxMind GeoLite data requires EULA/account/update compliance and has redistribution restrictions; the active peer ASN/country idea should use a user-supplied or separately licensed database, not bundle GeoLite silently.
- [Likely] Linux distribution should reuse upstream qBittorrent Flathub/AppImage patterns, appstream/desktop validation, and KDE runtime permissions; the active Linux packaging item is the right place for that work.

## Architecture Assessment
- Module boundary: removed desktop modules are not consistently reflected in the WebUI. Either restore backend controllers or remove/hide stale WebUI views, local preferences, menus, and smoke-test that no visible tab calls missing `/api/v2/rss` or search endpoints.
- Security test boundary: WebUI security logic is in C++ but its highest-risk regressions are HTTP-level behaviors. Add a local smoke that drives a running instance with crafted headers rather than only unit-testing helper functions.
- Observability boundary: category/tag mutations occur through desktop widgets, WebAPI, batch operations, and automation managers. Centralize logging in `SessionImpl::handleTorrentCategoryChanged`, `handleTorrentTagAdded`, and `handleTorrentTagRemoved` so all entrypoints are covered once.
- WebUI UX boundary: `ClipboardJS` is already loaded and tracker copy exists, but the content/files tab lacks a copy-path action; qui v1.22.0 shows this is a small table-stakes parity action.
- Test gaps: no disabled-module WebUI smoke, no CSRF/CORS negative smoke, no category/tag mutation logging tests, no WebUI content-tab copy-path test, no Linux packaging validation gate.
- Categories consciously not expanded in the active roadmap: accessibility is already blocked on Qt accessibility verification; i18n/l10n should follow upstream rebase strings; plugin ecosystem is rejected; multi-user scoped tokens are blocked on 5.2.x API-key work; mobile/responsive work is covered by existing alternate WebUI/mobile smoke and blocked 5.2.x WebUI baseline; upgrade strategy is represented by blocked rebase/backport items.

## Rejected Ideas
- Bundled VPN, antivirus scanning, ads, or paid tiers: contradicts GPL/local-first trust. Source: BitTorrent/uTorrent commercial feature pages.
- Cloud torrent storage/streaming service: wrong product category for a desktop fork. Source: Seedr and put.io.
- Full Vue/React WebUI rewrite: higher drift than value while Vanced is still behind upstream 5.2.x. Source: VueTorrent, Flood, qui.
- Large Deluge-style plugin ecosystem: maintenance and compatibility surface is too large for this fork. Source: Deluge plugin docs.
- BiglyBT-style I2P/anonymity/social features: would create unverifiable privacy claims and product sprawl. Source: BiglyBT feature list.
- Built-in Let's Encrypt/ACME automation: useful upstream request, but unsafe without domain/DNS/storage design; reverse-proxy docs are safer for now. Source: qBittorrent issue #7172.
- Bundled GeoLite database: licensing/update obligations conflict with silent bundling; use user-supplied or separately licensed MMDB only. Source: MaxMind EULA/developer docs.
- DHT crawler/discovery dashboard: privacy and legal risk exceed value for Vanced's stated local-control shape. Source: BiglyBT discovery features and BEP ecosystem.

## Sources
### Project and Upstream
- https://github.com/qbittorrent/qBittorrent/releases/tag/release-5.2.2
- https://github.com/c0re100/qBittorrent-Enhanced-Edition/releases/tag/release-5.2.1.10
- https://github.com/qbittorrent/qBittorrent/issues/9796
- https://github.com/qbittorrent/qBittorrent/issues/18525
- https://github.com/qbittorrent/qBittorrent/issues/7172
- https://github.com/qbittorrent/qBittorrent/wiki/API-Key-Authentication-%28%E2%89%A5v5.2.0%29

### Competitors and Adjacent OSS
- https://github.com/transmission/transmission/releases/tag/4.1.3
- https://github.com/VueTorrent/VueTorrent/releases/tag/v2.34.0
- https://github.com/jesec/flood/releases/tag/v4.14.3
- https://github.com/autobrr/qui/releases/tag/v1.22.0
- https://deluge-torrent.org/development/plugins/
- https://www.biglybt.com/features.php
- https://github.com/PBH-BTN/PeerBanHelper/releases/tag/v9.3.15
- https://github.com/StuffAnThings/qbit_manage/releases/tag/v4.9.1
- https://github.com/cross-seed/cross-seed/releases/tag/v6.13.7
- https://autobrr.com/filters/actions
- https://trash-guides.info/Downloaders/qBittorrent/Basic-Setup/

### Distribution, Standards, and Security
- https://raw.githubusercontent.com/flathub/org.qbittorrent.qBittorrent/master/org.qbittorrent.qBittorrent.yaml
- https://docs.flatpak.org/en/latest/manifests.html
- https://docs.appimage.org/packaging-guide/from-source/native-binaries.html
- https://specifications.freedesktop.org/desktop-entry-spec/latest/
- https://www.freedesktop.org/software/appstream/docs/chap-Metadata.html
- https://github.com/arvidn/libtorrent/releases/tag/v2.0.13
- https://wiki.qt.io/List_of_known_vulnerabilities_in_Qt_products
- https://openssl-library.org/news/vulnerabilities/
- https://cheatsheetseries.owasp.org/cheatsheets/Content_Security_Policy_Cheat_Sheet.html
- https://www.maxmind.com/en/geolite/eula
- https://dev.maxmind.com/geoip/geolite2-free-geolocation-data/

### Commercial and Community
- https://www.seedr.cc/
- https://put.io/

## Open Questions
None blocking. Existing blocked items already capture the decisions or environments that require maintainer input.
