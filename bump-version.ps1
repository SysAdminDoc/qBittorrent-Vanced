param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+\.\d+\.\d+$')]
    [string]$Vanced
)

$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot
if (-not $projectRoot) { $projectRoot = (Get-Location).Path }
$projectRoot = [System.IO.Path]::GetFullPath($projectRoot)
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)

function Resolve-ProjectPath {
    param([string]$Path)

    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $projectRoot $Path))
    $rootPrefix = $projectRoot.TrimEnd('\') + '\'
    if (($fullPath -ne $projectRoot) -and (-not $fullPath.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase))) {
        throw "Refusing to edit outside project root: $fullPath"
    }

    return $fullPath
}

function Read-ProjectText {
    param([string]$Path)

    $fullPath = Resolve-ProjectPath $Path
    if (-not (Test-Path -LiteralPath $fullPath)) {
        throw "Required version file is missing: $Path"
    }

    return [System.IO.File]::ReadAllText($fullPath)
}

function Set-ProjectText {
    param(
        [string]$Path,
        [string]$Text
    )

    [System.IO.File]::WriteAllText((Resolve-ProjectPath $Path), $Text, $utf8NoBom)
}

function Replace-Required {
    param(
        [string]$Path,
        [string]$Text,
        [string]$Pattern,
        [string]$Replacement,
        [string]$Label
    )

    $regex = [regex]::new($Pattern, [System.Text.RegularExpressions.RegexOptions]::Multiline)
    if ($regex.Matches($Text).Count -eq 0) {
        throw "Could not update $Label in $Path; expected pattern was not found."
    }

    return $regex.Replace($Text, $Replacement)
}

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

$versionHeader = Read-ProjectText "src\base\version.h.in"
$baseMajor = Get-RegexValue $versionHeader '#define QBT_VERSION_MAJOR\s+(\d+)' "base major version"
$baseMinor = Get-RegexValue $versionHeader '#define QBT_VERSION_MINOR\s+(\d+)' "base minor version"
$baseBugfix = Get-RegexValue $versionHeader '#define QBT_VERSION_BUGFIX\s+(\d+)' "base bugfix version"
$baseBuild = Get-RegexValue $versionHeader '#define QBT_VERSION_BUILD\s+(\d+)' "base build version"
$upstream = Get-RegexValue $versionHeader '#define QBT_UPSTREAM_VERSION_2\s+"([^"]+)"' "upstream version"

$base = "$baseMajor.$baseMinor.$baseBugfix"
if ($baseBuild -ne "0") {
    $base = "$base.$baseBuild"
}

$vancedParts = $Vanced.Split('.')
$vancedFourPart = "$Vanced.0"
$releaseIdentity = "qBittorrent Vanced v$Vanced (base: qBittorrent Enhanced Edition v$base; upstream: qBittorrent v$upstream)"
$vcpkgDescription = "qBittorrent Vanced v$Vanced, based on qBittorrent Enhanced Edition v$base and qBittorrent v$upstream."

$paths = @(
    "src\base\version.h.in",
    "vcpkg.json",
    "README.md",
    "installer.nsi",
    "dist\windows\config.nsh",
    "src\qbittorrent.exe.manifest",
    "package.ps1"
)

if (Test-Path -LiteralPath (Resolve-ProjectPath "CLAUDE.md")) {
    $paths += "CLAUDE.md"
}

$original = @{}
$updated = @{}
foreach ($path in $paths) {
    $text = Read-ProjectText $path
    $original[$path] = $text

    switch ($path) {
        "src\base\version.h.in" {
            $text = Replace-Required $path $text '^#define QBT_VANCED_VERSION_MAJOR\s+\d+' "#define QBT_VANCED_VERSION_MAJOR $($vancedParts[0])" "Vanced major version"
            $text = Replace-Required $path $text '^#define QBT_VANCED_VERSION_MINOR\s+\d+' "#define QBT_VANCED_VERSION_MINOR $($vancedParts[1])" "Vanced minor version"
            $text = Replace-Required $path $text '^#define QBT_VANCED_VERSION_BUGFIX\s+\d+' "#define QBT_VANCED_VERSION_BUGFIX $($vancedParts[2])" "Vanced bugfix version"
        }
        "vcpkg.json" {
            $text = Replace-Required $path $text '"version-string":\s*"[^"]+"' "`"version-string`": `"$Vanced`"" "vcpkg version-string"
            $text = Replace-Required $path $text '"description":\s*"[^"]*"' "`"description`": `"$vcpkgDescription`"" "vcpkg description"
        }
        "README.md" {
            $text = Replace-Required $path $text 'Vanced-v\d+\.\d+\.\d+' "Vanced-v$Vanced" "README Vanced badge"
            $text = Replace-Required $path $text 'qBittorrent Vanced v\d+\.\d+\.\d+ \(base: qBittorrent Enhanced Edition v[^;]+; upstream: qBittorrent v[^\)]+\)' $releaseIdentity "README smoke output"
        }
        "installer.nsi" {
            $text = Replace-Required $path $text '(!define VANCED_VERSION\s+)"[^"]+"' "`$1`"$Vanced`"" "NSIS Vanced version"
            $text = Replace-Required $path $text '(!define VANCED_VERSION_4\s+)"[^"]+"' "`$1`"$vancedFourPart`"" "NSIS four-part Vanced version"
        }
        "dist\windows\config.nsh" {
            $text = Replace-Required $path $text '(!define /ifndef QBT_VANCED_VERSION\s+)"[^"]+"' "`$1`"$Vanced`"" "Windows installer config version"
        }
        "src\qbittorrent.exe.manifest" {
            $text = Replace-Required $path $text '^    version="\d+\.\d+\.\d+\.0"$' "    version=`"$vancedFourPart`"" "Windows application manifest version"
        }
        "package.ps1" {
            $text = Replace-Required $path $text '(\[string\]\$Version\s*=\s*)"[^"]+"' "`$1`"$Vanced`"" "package default version"
        }
        "CLAUDE.md" {
            $text = Replace-Required $path $text 'Vanced version: v\d+\.\d+\.\d+' "Vanced version: v$Vanced" "CLAUDE overview version"
            $text = Replace-Required $path $text 'qBittorrent Vanced v\d+\.\d+\.\d+ \(base: qBittorrent Enhanced Edition v[^;]+; upstream: qBittorrent v[^\)]+\)' $releaseIdentity "CLAUDE smoke output"
        }
    }

    $updated[$path] = $text
}

$written = New-Object System.Collections.Generic.List[string]
try {
    foreach ($path in $paths) {
        Set-ProjectText $path $updated[$path]
        $written.Add($path)
    }
}
catch {
    foreach ($path in $written) {
        Set-ProjectText $path $original[$path]
    }
    throw
}

Write-Output "Updated qBittorrent Vanced release metadata to v$Vanced."
Write-Output "Base: qBittorrent Enhanced Edition v$base"
Write-Output "Upstream: qBittorrent v$upstream"
Write-Output "Next verification: powershell -ExecutionPolicy Bypass -File build.ps1 -ReleaseGate"
