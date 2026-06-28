<#
.SYNOPSIS
    Generate a quilt-style patch series of the Vanced delta.
.DESCRIPTION
    Extracts all SysAdminDoc commits since the fork point as numbered .patch
    files under patches/. Only includes commits that touch source code (excludes
    commits that only modify .md files or .claude/ paths). The series file
    lists patches in application order for mechanical rebasing.
.PARAMETER OutputDir
    Directory for patch output (default: patches)
.PARAMETER ForkBase
    Commit hash of the last upstream commit before Vanced changes
#>
param(
    [string]$OutputDir = "patches",
    [string]$ForkBase = ""
)

$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot
if (-not $projectRoot) { $projectRoot = (Get-Location).Path }

if (-not $ForkBase) {
    $firstVanced = git log --reverse --author="SysAdminDoc" --format="%H" | Select-Object -First 1
    if (-not $firstVanced) {
        Write-Error "No SysAdminDoc commits found."
        exit 1
    }
    $ForkBase = git rev-parse "$firstVanced~1" 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Cannot determine fork base from first Vanced commit."
        exit 1
    }
}

Write-Host "Fork base: $(git log --oneline -1 $ForkBase)" -ForegroundColor Cyan

$patchDir = Join-Path $projectRoot $OutputDir
if (Test-Path $patchDir) { Remove-Item $patchDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path $patchDir | Out-Null

$commits = git log --reverse --author="SysAdminDoc" --format="%H" "$ForkBase..HEAD"
$sourceCommits = @()

foreach ($hash in $commits) {
    $files = git diff-tree --no-commit-id --name-only -r $hash
    $hasSource = $false
    foreach ($f in $files) {
        if ($f -and ($f -notmatch '\.md$') -and ($f -notmatch '^\.claude/') -and ($f -notmatch '^CLAUDE\.md$')) {
            $hasSource = $true
            break
        }
    }
    if ($hasSource) {
        $sourceCommits += $hash
    }
}

Write-Host "Total Vanced commits: $($commits.Count)" -ForegroundColor DarkGray
Write-Host "Source-code commits: $($sourceCommits.Count)" -ForegroundColor DarkGray

$series = @()
$index = 0

foreach ($hash in $sourceCommits) {
    $index++
    $subject = git log --format="%s" -1 $hash
    $safeSubject = $subject -replace '[^\w\-]', '-' -replace '-+', '-' -replace '^-|-$', ''
    $safeSubject = $safeSubject.Substring(0, [Math]::Min($safeSubject.Length, 60))
    $fileName = "{0:D4}-{1}.patch" -f $index, $safeSubject

    git format-patch -1 $hash --stdout | Out-File -FilePath (Join-Path $patchDir $fileName) -Encoding utf8
    $series += $fileName
    Write-Host "  $fileName" -ForegroundColor DarkGray
}

$seriesPath = Join-Path $patchDir "series"
$series | Out-File -FilePath $seriesPath -Encoding utf8

Write-Host "`nGenerated $($series.Count) patches in $OutputDir/" -ForegroundColor Green
Write-Host "Apply with: cd <upstream-checkout> && git am patches/*.patch" -ForegroundColor Yellow
