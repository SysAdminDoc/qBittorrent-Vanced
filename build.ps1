param(
    [switch]$Test,
    [switch]$Verify,
    [switch]$ReleaseGate,
    [string]$VSPath
)

$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot
if (-not $projectRoot) { $projectRoot = (Get-Location).Path }

function Get-RegexValue {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Label
    )

    $match = [regex]::Match($Text, $Pattern)
    if (-not $match.Success) {
        throw "Could not read $Label from version metadata."
    }

    return $match.Groups[1].Value
}

function Get-ReleaseInfo {
    $versionHeader = Get-Content (Join-Path $projectRoot "src\base\version.h.in") -Raw

    $baseMajor = Get-RegexValue $versionHeader '#define QBT_VERSION_MAJOR\s+(\d+)' "base major version"
    $baseMinor = Get-RegexValue $versionHeader '#define QBT_VERSION_MINOR\s+(\d+)' "base minor version"
    $baseBugfix = Get-RegexValue $versionHeader '#define QBT_VERSION_BUGFIX\s+(\d+)' "base bugfix version"
    $baseBuild = Get-RegexValue $versionHeader '#define QBT_VERSION_BUILD\s+(\d+)' "base build version"
    $vancedMajor = Get-RegexValue $versionHeader '#define QBT_VANCED_VERSION_MAJOR\s+(\d+)' "Vanced major version"
    $vancedMinor = Get-RegexValue $versionHeader '#define QBT_VANCED_VERSION_MINOR\s+(\d+)' "Vanced minor version"
    $vancedBugfix = Get-RegexValue $versionHeader '#define QBT_VANCED_VERSION_BUGFIX\s+(\d+)' "Vanced bugfix version"
    $upstream = Get-RegexValue $versionHeader '#define QBT_UPSTREAM_VERSION_2\s+"([^"]+)"' "upstream version"

    $baseVersion = "$baseMajor.$baseMinor.$baseBugfix"
    if ($baseBuild -ne "0") {
        $baseVersion = "$baseVersion.$baseBuild"
    }

    return [pscustomobject]@{
        Base = $baseVersion
        Vanced = "$vancedMajor.$vancedMinor.$vancedBugfix"
        Upstream = $upstream
    }
}

function Assert-FileContains {
    param(
        [string]$Path,
        [string]$Needle,
        [string]$Label
    )

    $fullPath = Join-Path $projectRoot $Path
    if (-not (Test-Path $fullPath)) {
        throw "$Label missing: $Path"
    }

    $text = Get-Content $fullPath -Raw
    if (-not $text.Contains($Needle)) {
        throw "$Label mismatch in ${Path}: expected '$Needle'."
    }
}

function Test-ManifestDependency {
    param(
        [object]$Manifest,
        [string]$Name
    )

    foreach ($dependency in $Manifest.dependencies) {
        if (($dependency -is [string]) -and ($dependency -eq $Name)) {
            return $true
        }
        if (($dependency -isnot [string]) -and ($dependency.name -eq $Name)) {
            return $true
        }
    }

    return $false
}

function Assert-ReleaseMetadata {
    param([object]$ReleaseInfo)

    Write-Output "=== Checking release metadata ==="

    Assert-FileContains "README.md" "Vanced-v$($ReleaseInfo.Vanced)" "README badge"
    Assert-FileContains "README.md" "license-GPL--2.0--or--later" "README license badge"
    Assert-FileContains "README.md" "qBittorrent Vanced v$($ReleaseInfo.Vanced) (base: qBittorrent Enhanced Edition v$($ReleaseInfo.Base); upstream: qBittorrent v$($ReleaseInfo.Upstream))" "README smoke output"
    Assert-FileContains "installer.nsi" "!define VANCED_VERSION `"$($ReleaseInfo.Vanced)`"" "NSIS Vanced version"
    Assert-FileContains "installer.nsi" "!define VANCED_VERSION_4 `"$($ReleaseInfo.Vanced).0`"" "NSIS four-part Vanced version"
    Assert-FileContains "dist\windows\config.nsh" "!define /ifndef QBT_VANCED_VERSION `"$($ReleaseInfo.Vanced)`"" "Windows installer config version"
    Assert-FileContains "src\qbittorrent.exe.manifest" "version=`"$($ReleaseInfo.Vanced).0`"" "Windows application manifest version"

    foreach ($licensePath in @("LICENSE", "COPYING", "COPYING.GPLv2", "COPYING.GPLv3")) {
        if (-not (Test-Path (Join-Path $projectRoot $licensePath))) {
            throw "License file missing: $licensePath"
        }
    }

    $vcpkgJson = Get-Content (Join-Path $projectRoot "vcpkg.json") -Raw | ConvertFrom-Json
    if ($vcpkgJson.'version-string' -ne $ReleaseInfo.Vanced) {
        throw "vcpkg.json version-string '$($vcpkgJson.'version-string')' does not match Vanced $($ReleaseInfo.Vanced)."
    }
    if ($vcpkgJson.description -notlike "*v$($ReleaseInfo.Vanced)*") {
        throw "vcpkg.json description does not mention Vanced v$($ReleaseInfo.Vanced)."
    }
    if ($vcpkgJson.license -ne "GPL-2.0-or-later") {
        throw "vcpkg.json license '$($vcpkgJson.license)' does not match GPL-2.0-or-later source licensing."
    }
    if (-not $vcpkgJson.'builtin-baseline') {
        throw "vcpkg.json must pin builtin-baseline for reproducible releases."
    }

    foreach ($dependency in @("boost-circular-buffer", "boost-stacktrace", "libtorrent", "openssl", "qtbase", "qtsvg", "qttools", "zlib")) {
        if (-not (Test-ManifestDependency $vcpkgJson $dependency)) {
            throw "vcpkg.json is missing required dependency '$dependency'."
        }
    }

    $workflowDir = Join-Path $projectRoot ".github\workflows"
    if ((Test-Path $workflowDir) -and (Get-ChildItem -Path $workflowDir -File -ErrorAction SilentlyContinue)) {
        throw "GitHub Actions workflows are present; releases must be verified locally in this repo."
    }

    Write-Output "=== Release metadata OK ==="
}

function Invoke-WebUILint {
    $webuiPath = Join-Path $projectRoot "src\webui\www"
    if (-not (Test-Path (Join-Path $webuiPath "package-lock.json"))) {
        throw "WebUI package-lock.json missing; cannot run deterministic lint."
    }

    Push-Location $webuiPath
    try {
        Write-Output "=== Installing WebUI lint dependencies ==="
        & npm ci
        if ($LASTEXITCODE -ne 0) {
            throw "npm ci failed with exit code $LASTEXITCODE"
        }

        Write-Output "=== Running WebUI lint ==="
        & npm run lint
        if ($LASTEXITCODE -ne 0) {
            throw "npm run lint failed with exit code $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }
}

$releaseInfo = Get-ReleaseInfo

if ($ReleaseGate) {
    Assert-ReleaseMetadata $releaseInfo
    Invoke-WebUILint
}

if (-not $VSPath) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $VSPath = & $vswhere -products * -latest -property installationPath 2>$null
    }
    if (-not $VSPath) {
        $candidates = @(
            "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools",
            "C:\Program Files\Microsoft Visual Studio\18\BuildTools",
            "C:\Program Files\Microsoft Visual Studio\18\Community",
            "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools",
            "C:\Program Files\Microsoft Visual Studio\2022\BuildTools",
            "C:\Program Files\Microsoft Visual Studio\2022\Community",
            "C:\Program Files\Microsoft Visual Studio\2022\Professional",
            "C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
        )
        foreach ($c in $candidates) {
            if (Test-Path "$c\VC\Auxiliary\Build\vcvars64.bat") {
                $VSPath = $c
                break
            }
        }
    }
    if (-not $VSPath) {
        Write-Error "Could not find Visual Studio. Pass -VSPath or install VS with C++ workload."
        exit 1
    }
}

$vcvarsall = "$VSPath\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvarsall)) {
    Write-Error "vcvars64.bat not found at $vcvarsall"
    exit 1
}

$envBlock = cmd.exe /c "`"$vcvarsall`" >nul 2>&1 && set" 2>&1
foreach ($line in $envBlock) {
    if ($line -match '^([^=]+)=(.*)$') {
        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}

Write-Output "=== MSVC Environment loaded from $VSPath ==="
Write-Output "cl.exe: $(Get-Command cl.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source)"

$cmakeExe = "$VSPath\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not (Test-Path $cmakeExe)) { $cmakeExe = (Get-Command cmake -ErrorAction SilentlyContinue).Source }
if (-not $cmakeExe) { Write-Error "cmake not found"; exit 1 }

$ninjaExe = "$VSPath\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
if (-not (Test-Path $ninjaExe)) { $ninjaExe = (Get-Command ninja -ErrorAction SilentlyContinue).Source }
if (-not $ninjaExe) { Write-Error "ninja not found"; exit 1 }

$vcpkgRoot = "$VSPath\VC\vcpkg"
if (-not (Test-Path "$vcpkgRoot\scripts\buildsystems\vcpkg.cmake")) {
    $repoVcpkgRoot = Join-Path $env:USERPROFILE "repos\vcpkg"
    if ($env:VCPKG_ROOT) { $vcpkgRoot = $env:VCPKG_ROOT }
    elseif (Test-Path "$repoVcpkgRoot\scripts\buildsystems\vcpkg.cmake") { $vcpkgRoot = $repoVcpkgRoot }
    else { Write-Error "vcpkg not found at $vcpkgRoot and VCPKG_ROOT not set"; exit 1 }
}

$testingFlag = if ($Test) { "ON" } else { "OFF" }

Write-Output "=== Configuring with CMake (source: $projectRoot) ==="
& $cmakeExe -S $projectRoot -B "$projectRoot\build" `
    -G Ninja `
    "-DCMAKE_MAKE_PROGRAM=$ninjaExe" `
    -DCMAKE_BUILD_TYPE=Release `
    "-DCMAKE_TOOLCHAIN_FILE=$vcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
    -DWEBUI=ON `
    -DGUI=ON `
    -DSTACKTRACE=OFF `
    "-DTESTING=$testingFlag"

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configure failed with exit code $LASTEXITCODE"
    exit 1
}

Write-Output "=== Building ==="
& $cmakeExe --build "$projectRoot\build" --config Release -j $env:NUMBER_OF_PROCESSORS

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed with exit code $LASTEXITCODE"
    exit 1
}

Write-Output "=== Build complete ==="
$exe = Get-ChildItem -Path "$projectRoot\build" -Recurse -Filter "qbittorrent.exe" | Select-Object -First 1
if ($exe) {
    Write-Output "Built: $($exe.FullName)"
    $windeployqtExe = Join-Path $projectRoot "build\vcpkg_installed\x64-windows\tools\Qt6\bin\windeployqt.exe"
    if (Test-Path $windeployqtExe) {
        Write-Output "=== Deploying Qt runtime plugins ==="
        & $windeployqtExe --release --no-translations --no-compiler-runtime $exe.FullName
        if ($LASTEXITCODE -ne 0) {
            Write-Error "windeployqt failed with exit code $LASTEXITCODE"
            exit 1
        }
    }
    else {
        Write-Warning "windeployqt.exe not found at $windeployqtExe; the executable may not launch without Qt plugins"
    }
} else {
    Write-Error "qbittorrent.exe not found in build directory"
    exit 1
}

if ($Test) {
    Write-Output "=== Running tests ==="
    & $cmakeExe --build "$projectRoot\build" --target check
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Tests failed with exit code $LASTEXITCODE"
        exit 1
    }
    Write-Output "=== Tests passed ==="
}

if ($exe) {
    Write-Output "=== Verifying executable ==="
    & $exe.FullName --version
    if (($null -ne $LASTEXITCODE) -and ($LASTEXITCODE -ne 0)) {
        Write-Error "Executable verification failed with exit code $LASTEXITCODE"
        exit 1
    }
    $expectedVersion = "qBittorrent Vanced v$($releaseInfo.Vanced) (base: qBittorrent Enhanced Edition v$($releaseInfo.Base); upstream: qBittorrent v$($releaseInfo.Upstream))"
    $versionInfo = (Get-Item $exe.FullName).VersionInfo
    if (($versionInfo.FileVersion -ne $releaseInfo.Vanced) -or ($versionInfo.ProductVersion -ne $expectedVersion)) {
        Write-Error "Executable version resource mismatch. Expected '$expectedVersion', got file '$($versionInfo.FileVersion)' product '$($versionInfo.ProductVersion)'."
        exit 1
    }
    Write-Output $expectedVersion
    Write-Output "=== Verification complete ==="
}

if ($ReleaseGate) {
    Write-Output "=== Packaging release artifacts ==="
    & (Join-Path $projectRoot "package.ps1") -Version $releaseInfo.Vanced -BaseVersion $releaseInfo.Base -Clean -BuildInstaller
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Packaging failed with exit code $LASTEXITCODE"
        exit 1
    }
    Write-Output "=== Release gate complete ==="
}
