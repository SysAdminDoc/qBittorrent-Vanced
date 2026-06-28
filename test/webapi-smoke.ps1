<#
.SYNOPSIS
    WebAPI compatibility smoke test for qBittorrent Vanced.
.DESCRIPTION
    Authenticates, exercises core WebAPI endpoints used by automation clients
    (qbittorrent-api, autobrr, qbit_manage), and verifies stable behavior
    without requiring real tracker traffic.
.PARAMETER BaseUrl
    WebUI base URL (default: http://localhost:8080)
.PARAMETER Username
    WebUI username (default: admin)
.PARAMETER Password
    WebUI password (default: adminadmin)
#>
param(
    [string]$BaseUrl = "http://localhost:8080",
    [string]$Username = "admin",
    [string]$Password = "adminadmin"
)

$ErrorActionPreference = "Stop"
$passed = 0
$failed = 0
$session = $null

function Write-Result($name, $ok, $detail = "") {
    if ($ok) {
        Write-Host "  PASS  $name" -ForegroundColor Green
        $script:passed++
    } else {
        Write-Host "  FAIL  $name  $detail" -ForegroundColor Red
        $script:failed++
    }
}

function Invoke-Api($method, $path, $body = $null) {
    $uri = "$BaseUrl/api/v2/$path"
    $params = @{ Method = $method; Uri = $uri; WebSession = $script:session }
    if ($body) { $params.Body = $body }
    return Invoke-WebRequest @params -UseBasicParsing
}

# --- Auth ---
Write-Host "`nWebAPI Smoke Test — $BaseUrl`n" -ForegroundColor Cyan

try {
    $loginResp = Invoke-WebRequest -Method POST -Uri "$BaseUrl/api/v2/auth/login" `
        -Body @{ username = $Username; password = $Password } `
        -SessionVariable session -UseBasicParsing
    $script:session = $session
    Write-Result "auth/login" ($loginResp.Content -match "Ok\.")
} catch {
    Write-Host "  FAIL  auth/login — cannot connect to $BaseUrl" -ForegroundColor Red
    Write-Host "        Start qBittorrent Vanced with WebUI enabled before running this test." -ForegroundColor Yellow
    exit 1
}

# --- App info ---
try {
    $ver = (Invoke-Api GET "app/version").Content
    Write-Result "app/version" ($ver.Length -gt 0) $ver
} catch { Write-Result "app/version" $false $_.Exception.Message }

try {
    $apiVer = (Invoke-Api GET "app/webapiVersion").Content
    Write-Result "app/webapiVersion" ($apiVer -match "^\d+\.\d+") $apiVer
} catch { Write-Result "app/webapiVersion" $false $_.Exception.Message }

try {
    $buildInfo = (Invoke-Api GET "app/buildInfo").Content | ConvertFrom-Json
    $hasQt = $buildInfo.qt.Length -gt 0
    $hasLt = $buildInfo.libtorrent.Length -gt 0
    Write-Result "app/buildInfo" ($hasQt -and $hasLt) "qt=$($buildInfo.qt) lt=$($buildInfo.libtorrent)"
} catch { Write-Result "app/buildInfo" $false $_.Exception.Message }

try {
    $prefs = (Invoke-Api GET "app/preferences").Content | ConvertFrom-Json
    Write-Result "app/preferences" ($null -ne $prefs.save_path)
} catch { Write-Result "app/preferences" $false $_.Exception.Message }

# --- Transfer info ---
try {
    $transfer = (Invoke-Api GET "transfer/info").Content | ConvertFrom-Json
    Write-Result "transfer/info" ($null -ne $transfer.dl_info_speed)
} catch { Write-Result "transfer/info" $false $_.Exception.Message }

# --- Categories ---
$testCategory = "_vanced_smoke_test"
try {
    Invoke-Api POST "torrents/createCategory" @{ category = $testCategory; savePath = "" } | Out-Null
    $cats = (Invoke-Api GET "torrents/categories").Content | ConvertFrom-Json
    $found = $null -ne $cats.$testCategory
    Write-Result "torrents/createCategory" $found
} catch { Write-Result "torrents/createCategory" $false $_.Exception.Message }

# --- Tags ---
$testTag = "_vanced_smoke_tag"
try {
    Invoke-Api POST "torrents/createTags" @{ tags = $testTag } | Out-Null
    $tags = (Invoke-Api GET "torrents/tags").Content | ConvertFrom-Json
    $found = $tags -contains $testTag
    Write-Result "torrents/createTags" $found
} catch { Write-Result "torrents/createTags" $false $_.Exception.Message }

# --- Add paused magnet ---
$testMagnet = "magnet:?xt=urn:btih:0000000000000000000000000000000000000000&dn=vanced-smoke-test"
try {
    Invoke-Api POST "torrents/add" @{ urls = $testMagnet; stopped = "true"; category = $testCategory; tags = $testTag } | Out-Null
    Start-Sleep -Seconds 2
    $torrents = (Invoke-Api GET "torrents/info").Content | ConvertFrom-Json
    $smokeT = $torrents | Where-Object { $_.name -match "vanced-smoke-test" -or $_.hash -eq "0000000000000000000000000000000000000000" }
    Write-Result "torrents/add (paused magnet)" ($null -ne $smokeT)
} catch { Write-Result "torrents/add" $false $_.Exception.Message }

# --- Torrent info/count ---
try {
    $count = (Invoke-Api GET "torrents/count").Content
    Write-Result "torrents/count" ([int]$count -ge 0) "count=$count"
} catch { Write-Result "torrents/count" $false $_.Exception.Message }

# --- Sync maindata ---
try {
    $sync = (Invoke-Api GET "sync/maindata?rid=0").Content | ConvertFrom-Json
    Write-Result "sync/maindata" ($null -ne $sync.server_state)
} catch { Write-Result "sync/maindata" $false $_.Exception.Message }

# --- Cleanup ---
Write-Host "`nCleaning up smoke test data..." -ForegroundColor DarkGray
try {
    if ($smokeT) {
        Invoke-Api POST "torrents/delete" @{ hashes = $smokeT.hash; deleteFiles = "true" } | Out-Null
    }
    Invoke-Api POST "torrents/removeCategories" @{ categories = $testCategory } | Out-Null
    Invoke-Api POST "torrents/deleteTags" @{ tags = $testTag } | Out-Null
} catch {}

try {
    Invoke-Api POST "auth/logout" | Out-Null
} catch {}

# --- Summary ---
Write-Host "`n--- Results ---" -ForegroundColor Cyan
Write-Host "  Passed: $passed" -ForegroundColor Green
if ($failed -gt 0) {
    Write-Host "  Failed: $failed" -ForegroundColor Red
} else {
    Write-Host "  Failed: 0" -ForegroundColor Green
}
Write-Host "  WebAPI version: $apiVer"
Write-Host ""

exit $(if ($failed -gt 0) { 1 } else { 0 })
