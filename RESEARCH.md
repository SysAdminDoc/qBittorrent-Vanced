# Research - qBittorrent Vanced

## Executive Summary
qBittorrent Vanced is a Windows-first C++/Qt6 fork of qBittorrent Enhanced Edition v5.1.3.10 (upstream qBittorrent v5.1.3) shipping Catppuccin Mocha styling, custom shimmer progress bars, streamlined defaults, WebUI polish, and local-only packaging. Its strongest shape is an opinionated, privacy-preserving desktop+WebUI client for users who want a polished dark qBittorrent with anti-leecher defaults and no telemetry. The highest-value direction is closing the security and dependency gap with upstream 5.2.x (libtorrent 2.0.13, Qt 6.8 LTS, OpenSSL 3.5.x), extracting hard-coded theme colors into the `.qbtheme` system to reduce rebase friction, and hardening the WebUI session/CSP surface before adding new UX features. Top 8 opportunities in priority order: (1) upgrade vcpkg baseline to pull libtorrent 2.0.13 + Qt 6.8.8 + OpenSSL 3.5.7 security fixes; (2) add a version-bump script to eliminate manual version-string synchronization errors; (3) add WebUI session idle timeout enforcement with configurable warning; (4) add WCAG 2.2 focus-appearance compliance for custom-painted controls; (5) add WebUI `Content-Security-Policy` nonce plumbing for inline scripts; (6) add WebUI memory-leak regression smoke for long-running sessions; (7) add TRaSH Guides-aligned category/path validation presets; (8) add a `--portable` CLI flag for zero-install operation.

## Product Map
- Core workflows: add torrent/magnet/URL, manage queue and priorities, inspect peers/trackers/files, tune global speeds, set categories/tags/locations, operate through WebUI/API, batch edit torrents, portable backup/restore.
- User personas: Windows desktop users wanting polished dark qBittorrent; Enhanced Edition users wanting anti-leecher defaults; self-hosted users managing via WebUI behind reverse proxies; Sonarr/Radarr/autobrr/qbit_manage automation users depending on stable API behavior; single maintainer rebasing a branded fork against two upstreams.
- Platforms and distribution: Windows x64 (NSIS installer + portable ZIP), CMake/Ninja/MSVC/vcpkg source build; inherited Unix/macOS build files exist but are untested; WebUI serves all platforms.
- Key integrations: libtorrent 2.0.11 session core, Qt6 Widgets desktop, MooTools-based WebUI under `src/webui/www`, WebAPI controllers under `src/webui/api`, Windows `.torrent`/`magnet:` associations, release metadata from `src/base/version.h.in` + `dist/windows/config.nsh` + `installer.nsi` + `vcpkg.json`.

## Competitive Landscape

### qBittorrent upstream (5.2.2, Jun 2026)
- Leads with: API key auth, virtual list WebUI rendering, per-category share limits, X-Forwarded-Host hardening, SameSite cookie enforcement, libtorrent 2.0.13, Windows ARM64 installer.
- Learn from: backport stream discipline (5.2.1 SSRF, 5.2.2 reverse proxy), published library versions, theme wiki documentation.
- Avoid: drifting from expected qBittorrent behavior and breaking automation client compatibility.

### qBittorrent Enhanced Edition (v5.2.1.10, May 2026)
- Leads with: auto-ban Xunlei/QQ/Baidu/BitTorrent Player peers, shadow banning, peer whitelist/blacklist, tracker auto-update.
- Learn from: tracking Enhanced first then vanilla upstream during rebase; its v5.2.1.10 is on upstream 5.2.1, not yet 5.2.2.
- Avoid: losing the anti-leecher feature delta during rebases.

### VueTorrent (v2.34.0, alternate WebUI)
- Leads with: Vue 3 PWA-installable WebUI, virtual scrolling for 10k+ lists, dark/light/system themes, shift-click multi-select, API key auth support, configurable dashboard columns.
- Learn from: PWA installability pattern, virtual scrolling architecture for large libraries.
- Avoid: full WebUI rewrite — validate compatibility with VueTorrent as an alternate WebUI instead.

### Transmission (4.1.1, Jan 2026)
- Leads with: lower CPU/RAM footprint, sequential downloading, even peer bandwidth distribution, cross-platform parity.
- Learn from: lean RPC contract, predictable remote-control surface.
- Avoid: feature sprawl that makes simple workflows harder.

### Flood (v4.13.9) and qui (v1.18.0)
- Flood: multi-client backend abstraction, full keyboard accessibility, responsive collapsing sidebar.
- qui: multi-instance management, built-in cross-seed, transparent auth proxy for *arr apps.
- Learn from: keyboard-first UX patterns, multi-instance awareness for users running Vanced alongside other clients.

### Automation ecosystem (autobrr, qbit_manage, TRaSH Guides)
- autobrr: requires multi-instance connection pooling, health monitoring, categories, re-announce, API key auth (qBit 5.2+).
- qbit_manage: requires tag/category CRUD by tracker/save path, share limit enforcement, orphan cleanup, unregistered torrent removal.
- TRaSH Guides: recommends per-media-type categories with subfolder-only save paths, Automatic torrent management mode, unified `/data` structure for hardlinks.
- Learn from: their preset/recipe patterns and dry-run habits.
- Avoid: turning Vanced into a media manager; provide the API surface and let tools handle workflows.

## Security, Privacy, and Reliability

### Verified findings
- `vcpkg.json` pins libtorrent 2.0.11. Upstream qBittorrent 5.2.2 ships 2.0.13 which clears HTTP credentials on cross-origin web seed redirects (PR arvidn/libtorrent#8455). No CVE assigned but this is a credential-leak fix.
- Qt minimum is 6.5.0 (`CMakeLists.txt:11`). Qt 6.8.8 (current LTS patch) fixes CVE-2026-6210 (SVG heap overflow), CVE-2025-14575 (untrusted OpenSSL cert search path on Windows), CVE-2025-14576 (QML injection), CVE-2025-6338 (Schannel TLS cleanup). The vcpkg baseline determines which Qt version actually builds.
- OpenSSL 3.5.7 fixes CVE-2026-45447 (HIGH: heap UAF in PKCS7_verify). OpenSSL 3.0 EOL is Sep 2026. The vcpkg baseline pins the actual OpenSSL version.
- `src/webui/webapplication.cpp:465`: CSP still includes `script-src 'unsafe-inline'`. 91 inline event handlers across 14 WebUI HTML files block removal. `preferences.html` alone has 49.
- `src/webui/webapplication.cpp`: X-Forwarded-Host is now gated on reverse-proxy mode (commit 4aad73ab6), matching qBittorrent 5.2.2 behavior. Verified.
- `src/base/net/downloadhandlerimpl.cpp`: redirect scheme allowlist is in place with regression tests (commit cfcc53fca). Verified.
- Auth rate limiting exists in `src/webui/api/authcontroller.cpp` with configurable max fail count and ban duration. Verified.
- Hard-coded Catppuccin colors in 3 GUI files (`progressbarpainter.cpp`, `torrentcardswidget.cpp`, `traypopupwidget.cpp`) and QSS in `uithememanager.cpp`. These resist rebase and prevent runtime theme switching.
- WebUI memory leaks are a known upstream issue (#20675, #21502, #23806). Vanced has no monitoring for this.

### Missing guardrails
- No WebUI session idle timeout warning — sessions persist until explicit logout or cookie expiration.
- No dependency security scanning in the local release gate (vcpkg outdated check, npm audit).
- No automated version-bump workflow — 7+ files must be manually synchronized on every release.
- No WCAG 2.2 focus-appearance testing for custom-painted progress bars and torrent cards.
- No WebUI memory monitoring in the large-library smoke test.

### Recovery and rollback
- Release gate (`build.ps1 -ReleaseGate`) is comprehensive: version checks, WebUI lint, build smoke, portable ZIP, NSIS installer, SHA256SUMS, provenance, advisory check.
- No automated rollback for failed releases — manual git revert is the only path.
- `generate-patches.ps1` enables mechanical rebase via quilt-style patch series.

## Architecture Assessment

### Module boundaries needing attention
- **Theme data**: Hard-coded colors in `progressbarpainter.cpp` (8 colors), `torrentcardswidget.cpp` (5 colors), `traypopupwidget.cpp` (3 colors), and ~400 lines of QSS in `uithememanager.cpp`. The `.qbtheme` system (upstream since 4.6.0) supports `config.json` palette routing, `stylesheet.qss`, and custom resources — Vanced should use it.
- **Version strings**: 7+ files (`version.h.in`, `vcpkg.json`, `README.md`, `installer.nsi`, `config.nsh`, `qbittorrent.exe.manifest`, `CLAUDE.md`) must stay synchronized. The release gate catches mismatches but doesn't automate the bump.
- **WebUI inline handlers**: 91 across 14 HTML files. `preferences.html` (3000 lines, 49 handlers) is the worst offender. CSP nonce support requires refactoring these to `addEventListener` calls, which should be coordinated with rebase onto 5.2.x (where upstream may have done partial work).
- **dynamicTable.js**: 152KB, 3200 lines — the WebUI's performance-critical table renderer. Upstream 5.2.0 replaced this with virtual list rendering. Vanced's 5.1.3 base has the old implementation.

### Refactor candidates
- `src/gui/progressbarpainter.cpp`: Extract 8 hard-coded `QColor(0x...)` constants into a `ProgressBarColors` struct loaded from `.qbtheme` config.json or defaulting to Catppuccin Mocha.
- `src/gui/torrentcardswidget.cpp:54-60`: Same pattern — 5 state colors should come from the theme system.
- `src/gui/traypopupwidget.cpp:104-122`: 3 hard-coded colors for the tray popup background and borders.

### Test gaps
- C++ test execution is blocked on Qt Test availability (`qtbase[testlib]` vcpkg feature triggers rebuild failure).
- `testdownloadhandlerimpl.cpp` is the only Vanced-authored C++ test. It runs standalone without Qt Test.
- `webapi-smoke.ps1` covers auth, categories, tags, magnets, sync, and path validation — but requires a running instance.
- `largelibrary-smoke.ps1` seeds 10k magnets and measures sync timing — but doesn't monitor WebUI memory.
- No integration test for the WebUI login flow, CSRF rejection, or reverse-proxy X-Forwarded-Host behavior.

## Rejected Ideas
- Bundled VPN, AV scanning, ads, paid tiers (BitTorrent/uTorrent): contradicts GPL/local-first trust. Source: BitTorrent Web Premium feature page.
- Cloud storage/streaming (Seedr, put.io, Premiumize): wrong for a desktop fork. Source: Seedr/put.io/Premiumize feature pages.
- Full WebUI rewrite to Vue/React (VueTorrent/Flood): increases drift before security and API gates are stable. Source: VueTorrent GitHub.
- Native mobile app: responsive WebUI + alternate WebUI compatibility addresses mobile need. Source: Reddit r/selfhosted threads.
- Large plugin ecosystem (Deluge/BiglyBT): too much maintenance surface for a focused fork. Source: Deluge plugin architecture docs.
- I2P/anonymity claims (BiglyBT/Tribler): Vanced cannot verify end-to-end. Source: BiglyBT features page.
- DHT crawling dashboard (BEP 51): privacy/legal risk exceeds value. Source: BEP 51 specification.
- Encrypted P2P chat (Tixati): niche feature, closed-source reference, no ecosystem demand. Source: Tixati feature list.
- Dual libtorrent 1.2/2.x release flavors: architectural complexity for marginal user benefit; already in Roadmap_Blocked.md for maintainer judgment. Source: libtorrent release notes.
- WiX/MSIX installer replacement: NSIS 3.12+ addresses CVE-2025-43715; WiX migration would be high-effort for marginal gain with current single-platform focus. Source: NSIS releases, WiX documentation.

## Sources

### Project and Upstream
- https://github.com/qbittorrent/qBittorrent/blob/master/Changelog
- https://github.com/qbittorrent/qBittorrent/wiki/API-Key-Authentication-%28%E2%89%A5v5.2.0%29
- https://github.com/qbittorrent/qBittorrent/wiki/Create-custom-themes-for-qBittorrent
- https://github.com/qbittorrent/qBittorrent/pull/24270
- https://github.com/c0re100/qBittorrent-Enhanced-Edition/releases

### Competitors and WebUIs
- https://github.com/VueTorrent/VueTorrent/releases/tag/v2.34.0
- https://github.com/jesec/flood/releases
- https://github.com/autobrr/qui
- https://github.com/transmission/transmission/releases/
- https://deluge.readthedocs.io/en/deluge-2.2.0/changelog.html
- https://www.biglybt.com/features.php

### Automation Ecosystem
- https://github.com/StuffAnThings/qbit_manage
- https://autobrr.com/filters/actions
- https://trash-guides.info/Downloaders/qBittorrent/Basic-Setup/
- https://trash-guides.info/File-and-Folder-Structure/Hardlinks-and-Instant-Moves/

### Security and Dependencies
- https://github.com/arvidn/libtorrent/releases
- https://github.com/arvidn/libtorrent/pull/8455
- https://wiki.qt.io/List_of_known_vulnerabilities_in_Qt_products
- https://openssl-library.org/news/vulnerabilities/
- https://nvd.nist.gov/vuln/detail/CVE-2025-43715
- https://cheatsheetseries.owasp.org/cheatsheets/Content_Security_Policy_Cheat_Sheet.html

### Standards and Accessibility
- https://www.w3.org/TR/WCAG22/
- https://doc.qt.io/qt-6/accessible-qwidget.html
- https://www.bittorrent.org/beps/bep_0052.html

### Community Signal
- https://github.com/qbittorrent/qBittorrent/issues/20675
- https://github.com/qbittorrent/qBittorrent/issues/21502
- https://github.com/qbittorrent/qBittorrent/issues/23384
- https://www.reddit.com/r/selfhosted/comments/w3xsao/preferred_torrent_client_with_mobile_friendly/

## Open Questions
- Should Vanced target Qt 6.8 LTS explicitly (pinning vcpkg baseline to a commit with Qt 6.8.8+), or continue accepting whatever the baseline provides (currently Qt 6.5+)?
- Should the `.qbtheme` extraction happen before or after the Enhanced Edition 5.2.x rebase? Before reduces rebase conflicts; after gets upstream theme system improvements.
- Should Vanced validate compatibility with VueTorrent as an alternate WebUI (it's the most popular alternate, and qui handles the multi-instance case)?
