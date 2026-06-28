param(
    [string]$Version = "1.0.2",
    [string]$BaseVersion = "5.1.3.10",
    [string]$BuildDir = "build",
    [string]$DistDir = "dist_package",
    [string]$OutputDir = "release",
    [switch]$Clean,
    [switch]$BuildInstaller
)

$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot
if (-not $projectRoot) { $projectRoot = (Get-Location).Path }
$projectRoot = [System.IO.Path]::GetFullPath($projectRoot)

function Resolve-ProjectPath {
    param([string]$Path)

    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $projectRoot $Path))
    $rootPrefix = $projectRoot.TrimEnd('\') + '\'
    if (($fullPath -ne $projectRoot) -and (-not $fullPath.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase))) {
        throw "Refusing to operate outside project root: $fullPath"
    }

    return $fullPath
}

function Remove-ProjectPath {
    param([string]$Path)

    $fullPath = Resolve-ProjectPath $Path
    if (Test-Path -LiteralPath $fullPath) {
        Remove-Item -LiteralPath $fullPath -Recurse -Force
        Write-Output "Removed stale artifact: $fullPath"
    }
}

function Get-Sha256 {
    param([string]$Path)

    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    $stream = [System.IO.File]::OpenRead($Path)
    try {
        $hashBytes = $sha256.ComputeHash($stream)
        return ([System.BitConverter]::ToString($hashBytes)).Replace("-", "").ToLowerInvariant()
    }
    finally {
        $stream.Dispose()
        $sha256.Dispose()
    }
}

function Get-ToolPath {
    param(
        [string]$Name,
        [string[]]$Fallbacks = @()
    )

    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    foreach ($candidate in $Fallbacks) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

function Get-VcpkgStatusPackages {
    param([string]$StatusPath)

    if (-not (Test-Path $StatusPath)) {
        return @()
    }

    $records = New-Object System.Collections.Generic.List[object]
    $current = @{}
    foreach ($line in Get-Content $StatusPath) {
        if ([string]::IsNullOrWhiteSpace($line)) {
            if ($current.ContainsKey("Package") -and $current.ContainsKey("Version")) {
                $records.Add([pscustomobject]$current)
            }
            $current = @{}
            continue
        }

        if ($line -match '^([^:]+):\s*(.*)$') {
            $current[$matches[1]] = $matches[2]
        }
    }

    if ($current.ContainsKey("Package") -and $current.ContainsKey("Version")) {
        $records.Add([pscustomobject]$current)
    }

    return $records | Where-Object { $_.Status -eq "install ok installed" }
}

function Write-AdvisoryCheck {
    param(
        [string]$OutputPath,
        [string]$VcpkgRoot
    )

    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("qBittorrent Vanced release advisory check")
    $lines.Add("Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss K')")
    $lines.Add("")

    $vcpkgExe = $null
    if ($VcpkgRoot) {
        $vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
    }
    if ((-not $vcpkgExe) -or (-not (Test-Path $vcpkgExe))) {
        $vcpkgExe = Join-Path $env:USERPROFILE "repos\vcpkg\vcpkg.exe"
    }

    if (Test-Path $vcpkgExe) {
        $lines.Add("## vcpkg update")
        $vcpkgOutput = & $vcpkgExe update 2>&1
        $vcpkgExit = $LASTEXITCODE
        $lines.AddRange([string[]]$vcpkgOutput)
        $lines.Add("vcpkg update exit code: $vcpkgExit")
    }
    else {
        $lines.Add("## vcpkg update")
        $lines.Add("vcpkg.exe not found; dependency freshness check was not run.")
    }

    $lines.Add("")
    $webuiPath = Join-Path $projectRoot "src\webui\www"
    if (Test-Path (Join-Path $webuiPath "package-lock.json")) {
        Push-Location $webuiPath
        try {
            $lines.Add("## npm audit --omit=dev")
            $npmOutput = & npm audit --omit=dev 2>&1
            $npmExit = $LASTEXITCODE
            $lines.AddRange([string[]]$npmOutput)
            $lines.Add("npm audit exit code: $npmExit")
        }
        finally {
            Pop-Location
        }
    }
    else {
        $lines.Add("## npm audit --omit=dev")
        $lines.Add("package-lock.json not found; npm advisory check was not run.")
    }

    $lines | Out-File -FilePath $OutputPath -Encoding utf8
}

$buildPath = Resolve-ProjectPath $BuildDir
$distPath = Resolve-ProjectPath $DistDir
$outputPath = Resolve-ProjectPath $OutputDir

if ($Clean) {
    Remove-ProjectPath $DistDir
    Remove-ProjectPath $OutputDir
    Get-ChildItem -Path $projectRoot -Filter "qBittorrent-Vanced-v*-base*-x64-setup.exe" -File -ErrorAction SilentlyContinue | ForEach-Object {
        Remove-ProjectPath $_.Name
    }
}

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
$vcpkgRoot = $env:VCPKG_ROOT
if (-not $vcpkgRoot) {
    $repoVcpkgRoot = Join-Path $env:USERPROFILE "repos\vcpkg"
    if (Test-Path (Join-Path $repoVcpkgRoot "vcpkg.exe")) {
        $vcpkgRoot = $repoVcpkgRoot
    }
}

$windeployqt = Join-Path $buildPath "vcpkg_installed\x64-windows\tools\Qt6\bin\windeployqt.exe"
if (Test-Path $windeployqt) {
    Write-Output "=== Running windeployqt ==="
    & $windeployqt --release --no-translations --no-opengl-sw --no-system-d3d-compiler (Join-Path $distPath "qbittorrent.exe")
    if ($LASTEXITCODE -ne 0) {
        Write-Error "windeployqt failed with exit code $LASTEXITCODE"
        exit 1
    }
}

if (Test-Path $vcpkgBin) {
    Write-Output "=== Copying vcpkg runtime DLLs ==="
    Copy-Item "$vcpkgBin\*.dll" -Destination $distPath -Force
}

$portableZipName = "qBittorrent-Vanced-v$Version-base$BaseVersion-x64-portable.zip"
$portableZip = Join-Path $outputPath $portableZipName
Write-Output "=== Creating portable ZIP ==="
if (Test-Path $portableZip) { Remove-Item $portableZip -Force }
Compress-Archive -Path "$distPath\*" -DestinationPath $portableZip -Force

$releaseArtifacts = New-Object System.Collections.Generic.List[object]
$releaseArtifacts.Add([pscustomobject]@{
    Path = $portableZip
    Name = $portableZipName
    Hash = (Get-Sha256 $portableZip)
})

if ($BuildInstaller) {
    $makensis = Get-ToolPath "makensis.exe" @(
        "C:\Program Files (x86)\NSIS\makensis.exe",
        "C:\Program Files\NSIS\makensis.exe"
    )
    if (-not $makensis) {
        Write-Error "makensis.exe not found. Install NSIS before building installer artifacts."
        exit 1
    }

    Write-Output "=== Building NSIS installer ==="
    & $makensis (Join-Path $projectRoot "installer.nsi")
    if ($LASTEXITCODE -ne 0) {
        Write-Error "makensis failed with exit code $LASTEXITCODE"
        exit 1
    }

    $installerName = "qBittorrent-Vanced-v$Version-base$BaseVersion-x64-setup.exe"
    $installerRoot = Join-Path $projectRoot $installerName
    if (-not (Test-Path $installerRoot)) {
        Write-Error "Expected installer was not produced: $installerRoot"
        exit 1
    }

    $installerRelease = Join-Path $outputPath $installerName
    Copy-Item $installerRoot -Destination $installerRelease -Force
    $releaseArtifacts.Add([pscustomobject]@{
        Path = $installerRelease
        Name = $installerName
        Hash = (Get-Sha256 $installerRelease)
    })
}

$vcpkgJson = Get-Content (Join-Path $projectRoot "vcpkg.json") -Raw | ConvertFrom-Json
$vcpkgBaseline = $vcpkgJson.'builtin-baseline'
$statusPath = Join-Path $buildPath "vcpkg_installed\vcpkg\status"
$installedPackages = @(Get-VcpkgStatusPackages $statusPath)
$requiredPackages = @("boost-circular-buffer", "boost-stacktrace", "libtorrent", "openssl", "qtbase", "qtsvg", "qttools", "zlib")
$missingPackages = @($requiredPackages | Where-Object { $pkg = $_; -not ($installedPackages | Where-Object { $_.Package -eq $pkg }) })
if ($missingPackages.Count -gt 0) {
    Write-Error "Installed dependency set is missing required package(s): $($missingPackages -join ', ')"
    exit 1
}

$resolvedVersions = $installedPackages |
    Sort-Object Package |
    ForEach-Object {
        $packageVersion = $_.Version
        if ($_.'Port-Version') { $packageVersion = "$packageVersion#$($_.'Port-Version')" }
        "- $($_.Package): $packageVersion"
    }

$hashLines = $releaseArtifacts | ForEach-Object { "$($_.Hash)  $($_.Name)" }
$hashPath = Join-Path $outputPath "SHA256SUMS.txt"
$hashLines | Out-File -FilePath $hashPath -Encoding ascii

$advisoryPath = Join-Path $outputPath "ADVISORY-CHECK.txt"
Write-AdvisoryCheck -OutputPath $advisoryPath -VcpkgRoot $vcpkgRoot

$provenance = @"
qBittorrent Vanced v$Version Release Provenance
Base: qBittorrent Enhanced Edition v$BaseVersion

Build Info
- Build date: $(Get-Date -Format 'yyyy-MM-dd')
- Platform: Windows x64
- vcpkg baseline: $vcpkgBaseline

Artifact Checksums (SHA256)
$($hashLines -join "`n")

Resolved Dependency Versions
$($resolvedVersions -join "`n")

Advisory Check
- See ADVISORY-CHECK.txt

Verify
certutil -hashfile $portableZipName SHA256
"@

if ($BuildInstaller) {
    $provenance += "certutil -hashfile qBittorrent-Vanced-v$Version-base$BaseVersion-x64-setup.exe SHA256`n"
}

$provenancePath = Join-Path $outputPath "RELEASE-PROVENANCE.txt"
$provenance | Out-File -FilePath $provenancePath -Encoding utf8

Write-Output "=== Artifact hashes ==="
$hashLines | Write-Output
Write-Output "=== Provenance written to $provenancePath ==="
Write-Output "=== Advisory check written to $advisoryPath ==="
Write-Output "=== Packaging complete ==="
$global:LASTEXITCODE = 0
