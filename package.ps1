param(
    [string]$Version = "1.0.0",
    [string]$BaseVersion = "5.1.3.10",
    [string]$BuildDir = "build",
    [string]$DistDir = "dist_package",
    [string]$OutputDir = "release"
)

$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot
if (-not $projectRoot) { $projectRoot = (Get-Location).Path }

$buildPath = Join-Path $projectRoot $BuildDir
$distPath = Join-Path $projectRoot $DistDir
$outputPath = Join-Path $projectRoot $OutputDir

$exe = Join-Path $buildPath "qbittorrent.exe"
if (-not (Test-Path $exe)) {
    Write-Error "qbittorrent.exe not found at $exe. Run build.ps1 first."
    exit 1
}

New-Item -ItemType Directory -Force -Path $distPath | Out-Null
New-Item -ItemType Directory -Force -Path $outputPath | Out-Null

Write-Output "=== Packaging qBittorrent Vanced v$Version ==="

Copy-Item $exe -Destination $distPath -Force

$vcpkgBin = Join-Path $buildPath "vcpkg_installed\x64-windows\bin"
$windeployqt = Join-Path $buildPath "vcpkg_installed\x64-windows\tools\Qt6\bin\windeployqt.exe"

if (Test-Path $windeployqt) {
    Write-Output "=== Running windeployqt ==="
    & $windeployqt --release --no-translations --no-opengl-sw --no-system-d3d-compiler (Join-Path $distPath "qbittorrent.exe")
}

if (Test-Path $vcpkgBin) {
    Write-Output "=== Copying vcpkg runtime DLLs ==="
    Copy-Item "$vcpkgBin\*.dll" -Destination $distPath -Force
}

$portableZip = Join-Path $outputPath "qBittorrent-Vanced-v$Version-base$BaseVersion-x64-portable.zip"
Write-Output "=== Creating portable ZIP ==="
if (Test-Path $portableZip) { Remove-Item $portableZip -Force }
Compress-Archive -Path "$distPath\*" -DestinationPath $portableZip -Force

$zipHash = (Get-FileHash $portableZip -Algorithm SHA256).Hash
Write-Output "SHA256: $zipHash  $([System.IO.Path]::GetFileName($portableZip))"

$vcpkgJson = Get-Content (Join-Path $projectRoot "vcpkg.json") | ConvertFrom-Json
$vcpkgBaseline = $vcpkgJson.'builtin-baseline'

# Extract resolved dependency versions from vcpkg SPDX manifests
$resolvedVersions = @()
$vcpkgShareDir = Join-Path $buildPath "vcpkg_installed\x64-windows\share"
if (Test-Path $vcpkgShareDir) {
    Get-ChildItem -Path $vcpkgShareDir -Filter "vcpkg.spdx.json" -Recurse | ForEach-Object {
        try {
            $spdx = Get-Content $_.FullName | ConvertFrom-Json
            $pkgName = $spdx.name -replace '^.*:', ''
            $pkgVersion = $spdx.packages | Where-Object { $_.name -like "*:*" } | Select-Object -First 1 -ExpandProperty versionInfo -ErrorAction SilentlyContinue
            if (-not $pkgVersion) {
                $pkgVersion = $spdx.packages | Select-Object -First 1 -ExpandProperty versionInfo -ErrorAction SilentlyContinue
            }
            if ($pkgName -and $pkgVersion) {
                $resolvedVersions += "- ${pkgName}: ${pkgVersion}"
            }
        } catch {
            # Skip malformed SPDX files
        }
    }
}

$provenance = @"
# qBittorrent Vanced v$Version Release Provenance
# Base: qBittorrent Enhanced Edition v$BaseVersion

## Build Info
- Build date: $(Get-Date -Format 'yyyy-MM-dd')
- Platform: Windows x64
- vcpkg baseline: $vcpkgBaseline

## Artifact Checksums (SHA256)
$zipHash  $([System.IO.Path]::GetFileName($portableZip))
"@

$installerExe = Get-ChildItem -Path $projectRoot -Filter "*setup.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
if ($installerExe) {
    $installerHash = (Get-FileHash $installerExe.FullName -Algorithm SHA256).Hash
    $provenance += "`n$installerHash  $($installerExe.Name)"
}

if ($resolvedVersions.Count -gt 0) {
    $provenance += @"

## Resolved Dependency Versions
$($resolvedVersions | Sort-Object | Out-String -NoNewline)
"@
} else {
    $provenance += @"

## Dependency Versions
Check the About dialog (Help > About) for runtime library versions:
- Qt6
- libtorrent-rasterbar
- Boost
- OpenSSL
- zlib
"@
}

$provenance += @"

## Verify
``````powershell
# Verify portable ZIP
(Get-FileHash .\$([System.IO.Path]::GetFileName($portableZip)) -Algorithm SHA256).Hash -eq '$zipHash'
``````
"@

$provenancePath = Join-Path $outputPath "RELEASE-PROVENANCE.txt"
$provenance | Out-File -FilePath $provenancePath -Encoding utf8
Write-Output "=== Provenance written to $provenancePath ==="
Write-Output "=== Packaging complete ==="
