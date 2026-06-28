<#
.SYNOPSIS
    Large-library WebUI and queue performance smoke test.
.DESCRIPTION
    Creates a synthetic profile of 10,000+ paused magnets, then measures
    sync/maindata response time, torrents/info pagination, torrents/count,
    and reports whether each stays under documented ceilings.

    WARNING: This test adds 10,000 dummy torrents. Run against a test
    instance, not your production library.
.PARAMETER BaseUrl
    WebUI base URL (default: http://localhost:8080)
.PARAMETER Username
    WebUI username (default: admin)
.PARAMETER Password
    WebUI password (default: adminadmin)
.PARAMETER Count
    Number of synthetic torrents to create (default: 10000)
.PARAMETER SkipCleanup
    Keep synthetic torrents after the test
#>
param(
    [string]$BaseUrl = "http://localhost:8080",
    [string]$Username = "admin",
    [string]$Password = "adminadmin",
    [int]$Count = 10000,
    [switch]$SkipCleanup
)

$ErrorActionPreference = "Stop"
$session = $null
$results = @()
$TAG = "_vanced_perf_smoke"

# Time ceilings (seconds)
$CEILING_SYNC_FULL = 10.0
$CEILING_SYNC_DELTA = 2.0
$CEILING_INFO_PAGE = 5.0
$CEILING_COUNT = 2.0

function Invoke-Api($method, $path, $body = $null) {
    $uri = "$BaseUrl/api/v2/$path"
    $params = @{ Method = $method; Uri = $uri; WebSession = $script:session }
    if ($body) { $params.Body = $body }
    return Invoke-WebRequest @params -UseBasicParsing
}

function Measure-Api($label, $method, $path, $body = $null) {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $resp = Invoke-Api $method $path $body
    $sw.Stop()
    return @{ Label = $label; Seconds = $sw.Elapsed.TotalSeconds; StatusCode = $resp.StatusCode; Size = $resp.Content.Length }
}

# --- Auth ---
Write-Host "`nLarge-Library Performance Smoke — $BaseUrl ($Count torrents)`n" -ForegroundColor Cyan

try {
    $loginResp = Invoke-WebRequest -Method POST -Uri "$BaseUrl/api/v2/auth/login" `
        -Body @{ username = $Username; password = $Password } `
        -SessionVariable session -UseBasicParsing
    $script:session = $session
    if ($loginResp.Content -notmatch "Ok\.") { throw "Login failed" }
} catch {
    Write-Host "  Cannot connect to $BaseUrl — start qBittorrent Vanced first." -ForegroundColor Red
    exit 1
}

# --- Create tag for cleanup ---
try { Invoke-Api POST "torrents/createTags" @{ tags = $TAG } | Out-Null } catch {}

# --- Seed synthetic torrents ---
Write-Host "Adding $Count synthetic paused magnets (batch of 50)..." -ForegroundColor Yellow
$batchSize = 50
$added = 0

for ($i = 0; $i -lt $Count; $i += $batchSize) {
    $batch = @()
    $end = [Math]::Min($i + $batchSize, $Count)
    for ($j = $i; $j -lt $end; $j++) {
        $hash = "{0:x40}" -f ($j + 1)
        $batch += "magnet:?xt=urn:btih:$hash&dn=perf-smoke-$j"
    }
    $urls = $batch -join "`n"
    try {
        Invoke-Api POST "torrents/add" @{ urls = $urls; stopped = "true"; tags = $TAG } | Out-Null
    } catch {}
    $added = $end
    if ($added % 1000 -eq 0) {
        Write-Host "  $added / $Count added" -ForegroundColor DarkGray
    }
}

Write-Host "  Waiting for torrents to settle..." -ForegroundColor DarkGray
Start-Sleep -Seconds 5

# --- Verify count ---
try {
    $m = Measure-Api "torrents/count" GET "torrents/count"
    $totalCount = [int]$((Invoke-Api GET "torrents/count").Content)
    $ok = $totalCount -ge $Count -and $m.Seconds -lt $CEILING_COUNT
    $results += @{ Label = "torrents/count"; Seconds = $m.Seconds; Ceiling = $CEILING_COUNT; Ok = $ok; Detail = "count=$totalCount" }
    Write-Host "  Total torrents: $totalCount" -ForegroundColor DarkGray
} catch {
    $results += @{ Label = "torrents/count"; Seconds = 0; Ceiling = $CEILING_COUNT; Ok = $false; Detail = $_.Exception.Message }
}

# --- Full sync ---
try {
    $m = Measure-Api "sync/maindata (full)" GET "sync/maindata?rid=0"
    $ok = $m.Seconds -lt $CEILING_SYNC_FULL
    $results += @{ Label = "sync/maindata (full, rid=0)"; Seconds = $m.Seconds; Ceiling = $CEILING_SYNC_FULL; Ok = $ok; Detail = "$([Math]::Round($m.Size / 1024))KB" }
} catch {
    $results += @{ Label = "sync/maindata (full)"; Seconds = 0; Ceiling = $CEILING_SYNC_FULL; Ok = $false; Detail = $_.Exception.Message }
}

# --- Delta sync ---
try {
    $ridResp = (Invoke-Api GET "sync/maindata?rid=0").Content | ConvertFrom-Json
    $rid = $ridResp.rid
    $m = Measure-Api "sync/maindata (delta)" GET "sync/maindata?rid=$rid"
    $ok = $m.Seconds -lt $CEILING_SYNC_DELTA
    $results += @{ Label = "sync/maindata (delta)"; Seconds = $m.Seconds; Ceiling = $CEILING_SYNC_DELTA; Ok = $ok; Detail = "$([Math]::Round($m.Size / 1024))KB" }
} catch {
    $results += @{ Label = "sync/maindata (delta)"; Seconds = 0; Ceiling = $CEILING_SYNC_DELTA; Ok = $false; Detail = $_.Exception.Message }
}

# --- Paginated torrents/info ---
try {
    $m = Measure-Api "torrents/info (first 100)" GET "torrents/info?limit=100&offset=0"
    $ok = $m.Seconds -lt $CEILING_INFO_PAGE
    $results += @{ Label = "torrents/info (limit=100)"; Seconds = $m.Seconds; Ceiling = $CEILING_INFO_PAGE; Ok = $ok; Detail = "$([Math]::Round($m.Size / 1024))KB" }
} catch {
    $results += @{ Label = "torrents/info (limit=100)"; Seconds = 0; Ceiling = $CEILING_INFO_PAGE; Ok = $false; Detail = $_.Exception.Message }
}

# --- Filtered by tag ---
try {
    $m = Measure-Api "torrents/info (tag filter)" GET "torrents/info?tag=$TAG&limit=100"
    $ok = $m.Seconds -lt $CEILING_INFO_PAGE
    $results += @{ Label = "torrents/info (tag filter)"; Seconds = $m.Seconds; Ceiling = $CEILING_INFO_PAGE; Ok = $ok; Detail = "$([Math]::Round($m.Size / 1024))KB" }
} catch {
    $results += @{ Label = "torrents/info (tag filter)"; Seconds = 0; Ceiling = $CEILING_INFO_PAGE; Ok = $false; Detail = $_.Exception.Message }
}

# --- Cleanup ---
if (-not $SkipCleanup) {
    Write-Host "`nCleaning up $Count synthetic torrents..." -ForegroundColor DarkGray
    try {
        $allSmoke = (Invoke-Api GET "torrents/info?tag=$TAG").Content | ConvertFrom-Json
        $hashes = ($allSmoke | ForEach-Object { $_.hash }) -join "|"
        if ($hashes) {
            Invoke-Api POST "torrents/delete" @{ hashes = $hashes; deleteFiles = "true" } | Out-Null
        }
        Invoke-Api POST "torrents/deleteTags" @{ tags = $TAG } | Out-Null
    } catch {
        Write-Host "  Cleanup failed: $($_.Exception.Message)" -ForegroundColor Yellow
    }
}

try { Invoke-Api POST "auth/logout" | Out-Null } catch {}

# --- Report ---
Write-Host "`n--- Performance Results ---" -ForegroundColor Cyan
Write-Host ("{0,-35} {1,8} {2,8} {3,6} {4}" -f "Test", "Time(s)", "Ceiling", "Result", "Detail")
Write-Host ("{0,-35} {1,8} {2,8} {3,6} {4}" -f "----", "-------", "-------", "------", "------")

$allPassed = $true
foreach ($r in $results) {
    $status = if ($r.Ok) { "PASS" } else { "FAIL" }
    $color = if ($r.Ok) { "Green" } else { "Red" }
    if (-not $r.Ok) { $allPassed = $false }
    Write-Host ("{0,-35} {1,8:N3} {2,8:N1} {3,6} {4}" -f $r.Label, $r.Seconds, $r.Ceiling, $status, $r.Detail) -ForegroundColor $color
}

Write-Host ""
exit $(if ($allPassed) { 0 } else { 1 })
