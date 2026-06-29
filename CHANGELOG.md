# Changelog

## Unreleased

- Added a WebUI `prefers-color-scheme` fallback so the main UI uses Catppuccin Mocha tokens when system dark mode applies and no explicit light class is set.
- Added alternate WebUI static-bundle validation to the WebAPI smoke script for VueTorrent compatibility checks.
- Added WebUI CSP nonce plumbing for built-in HTML responses while keeping the existing inline-script policy during migration.
- Added TRaSH Guides-aligned hints when category save paths look like media-library roots.
- Added `--portable` startup mode for zero-install ZIP runs that keep profile data beside the executable.
- Added a configurable WebUI idle-session warning banner with Refresh and Logout actions.
- Added a browser-driven WebUI JS heap regression check to the large-library smoke script.
- Added visible Catppuccin-blue focus rings to custom-painted progress cells and torrent cards.
- Added release-gate advisory status checks for Qt, OpenSSL, and libtorrent resolved package versions.
- Added `bump-version.ps1` to synchronize Vanced release metadata across build, installer, manifest, README, package, vcpkg, and working-note files.
