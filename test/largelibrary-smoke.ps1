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
.PARAMETER MemoryPollSeconds
    Duration for the browser JS heap regression check (default: 300)
.PARAMETER MemoryPollIntervalSeconds
    Poll interval for JS heap samples (default: 15)
.PARAMETER HeapCeilingMB
    Peak used JS heap ceiling in MB (default: 500)
.PARAMETER BrowserPath
    Optional path to Edge/Chrome/Chromium executable for the memory check
.PARAMETER BrowserDebugPort
    Optional Chrome DevTools Protocol port. Defaults to an ephemeral free port.
.PARAMETER SkipBrowserMemory
    Skip the browser JS heap regression check
#>
param(
    [string]$BaseUrl = "http://localhost:8080",
    [string]$Username = "admin",
    [string]$Password = "adminadmin",
    [int]$Count = 10000,
    [switch]$SkipCleanup,
    [int]$MemoryPollSeconds = 300,
    [int]$MemoryPollIntervalSeconds = 15,
    [int]$HeapCeilingMB = 500,
    [string]$BrowserPath = "",
    [int]$BrowserDebugPort = 0,
    [switch]$SkipBrowserMemory
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

function ConvertTo-JsLiteral($value) {
    return ($value | ConvertTo-Json -Compress)
}

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

function Get-FreeTcpPort {
    $listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback, 0)
    try {
        $listener.Start()
        return $listener.LocalEndpoint.Port
    } finally {
        $listener.Stop()
    }
}

function Find-BrowserExecutable {
    if ($BrowserPath) {
        if (Test-Path -LiteralPath $BrowserPath) {
            return (Resolve-Path -LiteralPath $BrowserPath).ProviderPath
        }
        throw "BrowserPath does not exist: $BrowserPath"
    }

    $candidates = @()
    foreach ($root in @($env:ProgramFiles, ${env:ProgramFiles(x86)}, $env:LocalAppData)) {
        if (-not $root) { continue }
        $candidates += Join-Path $root "Microsoft\Edge\Application\msedge.exe"
        $candidates += Join-Path $root "Google\Chrome\Application\chrome.exe"
        $candidates += Join-Path $root "Chromium\Application\chrome.exe"
        $candidates += Join-Path $root "BraveSoftware\Brave-Browser\Application\brave.exe"
    }

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    throw "No Edge/Chrome/Chromium browser executable found. Pass -BrowserPath or use -SkipBrowserMemory."
}

function Wait-CdpReady($port) {
    $deadline = (Get-Date).AddSeconds(15)
    $lastError = $null

    while ((Get-Date) -lt $deadline) {
        try {
            return Invoke-RestMethod -Uri "http://127.0.0.1:$port/json/version" -UseBasicParsing -TimeoutSec 2
        } catch {
            $lastError = $_.Exception.Message
            Start-Sleep -Milliseconds 250
        }
    }

    throw "Browser DevTools endpoint did not become ready on port $port. Last error: $lastError"
}

function Get-CdpPageTarget($port) {
    try {
        $encodedUrl = [System.Uri]::EscapeDataString("about:blank")
        $target = Invoke-RestMethod -Method PUT -Uri "http://127.0.0.1:$port/json/new?$encodedUrl" -UseBasicParsing -TimeoutSec 5
        if ($target.webSocketDebuggerUrl) { return $target }
    } catch {}

    $targets = Invoke-RestMethod -Uri "http://127.0.0.1:$port/json/list" -UseBasicParsing -TimeoutSec 5
    $target = $targets | Where-Object { $_.type -eq "page" -and $_.webSocketDebuggerUrl } | Select-Object -First 1
    if (-not $target) {
        throw "No DevTools page target was available."
    }
    return $target
}

function Connect-CdpWebSocket($url) {
    $socket = [System.Net.WebSockets.ClientWebSocket]::new()
    [void]$socket.ConnectAsync([Uri]$url, [System.Threading.CancellationToken]::None).GetAwaiter().GetResult()
    return $socket
}

function Receive-CdpMessage($socket, $timeoutMs = 10000) {
    $buffer = [byte[]]::new(65536)
    $segment = [System.ArraySegment[byte]]::new($buffer)
    $stream = [System.IO.MemoryStream]::new()
    $cts = [System.Threading.CancellationTokenSource]::new()
    $cts.CancelAfter($timeoutMs)

    try {
        do {
            $result = $socket.ReceiveAsync($segment, $cts.Token).GetAwaiter().GetResult()
            if ($result.MessageType -eq [System.Net.WebSockets.WebSocketMessageType]::Close) {
                throw "Browser DevTools socket closed."
            }
            $stream.Write($buffer, 0, $result.Count)
        } until ($result.EndOfMessage)

        $json = [System.Text.Encoding]::UTF8.GetString($stream.ToArray())
        return $json | ConvertFrom-Json
    } finally {
        $cts.Dispose()
        $stream.Dispose()
    }
}

function Invoke-CdpCommand($socket, $method, $params = $null) {
    if (-not $script:cdpCommandId) { $script:cdpCommandId = 0 }
    $script:cdpCommandId++
    $id = $script:cdpCommandId

    $payload = [ordered]@{ id = $id; method = $method }
    if ($null -ne $params) { $payload.params = $params }

    $json = $payload | ConvertTo-Json -Depth 20 -Compress
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($json)
    $segment = [System.ArraySegment[byte]]::new($bytes)
    [void]$socket.SendAsync($segment, [System.Net.WebSockets.WebSocketMessageType]::Text, $true, [System.Threading.CancellationToken]::None).GetAwaiter().GetResult()

    while ($true) {
        $message = Receive-CdpMessage $socket
        if ($message.id -ne $id) { continue }
        if ($message.error) {
            throw "CDP $method failed: $($message.error.message)"
        }
        return $message.result
    }
}

function Close-CdpSocket($socket) {
    if (-not $socket) { return }
    try {
        if ($socket.State -eq [System.Net.WebSockets.WebSocketState]::Open) {
            [void]$socket.CloseAsync([System.Net.WebSockets.WebSocketCloseStatus]::NormalClosure, "done", [System.Threading.CancellationToken]::None).GetAwaiter().GetResult()
        }
    } catch {}
    [void]$socket.Dispose()
}

function Get-JsHeapUsedBytes($socket) {
    $metrics = Invoke-CdpCommand $socket "Performance.getMetrics"
    $heapMetric = $metrics.metrics | Where-Object { $_.name -eq "JSHeapUsedSize" } | Select-Object -First 1
    if ($heapMetric) {
        return [double]$heapMetric.value
    }

    $eval = Invoke-CdpCommand $socket "Runtime.evaluate" @{
        expression = "performance.memory ? performance.memory.usedJSHeapSize : 0"
        returnByValue = $true
    }
    return [double]$eval.result.value
}

function Remove-TempBrowserProfile($profilePath) {
    if (-not $profilePath -or -not (Test-Path -LiteralPath $profilePath)) { return }

    $resolvedProfile = (Resolve-Path -LiteralPath $profilePath).ProviderPath
    $resolvedTemp = (Resolve-Path -LiteralPath ([System.IO.Path]::GetTempPath())).ProviderPath
    if (-not $resolvedProfile.StartsWith($resolvedTemp, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove browser profile outside temp: $resolvedProfile"
    }

    Remove-Item -LiteralPath $resolvedProfile -Recurse -Force -ErrorAction SilentlyContinue
}

function Measure-WebUIBrowserHeap {
    if ($MemoryPollSeconds -lt 1) { throw "MemoryPollSeconds must be at least 1." }
    if ($MemoryPollIntervalSeconds -lt 1) { throw "MemoryPollIntervalSeconds must be at least 1." }
    if ($HeapCeilingMB -lt 1) { throw "HeapCeilingMB must be at least 1." }

    $browser = Find-BrowserExecutable
    $debugPort = if ($BrowserDebugPort -gt 0) { $BrowserDebugPort } else { Get-FreeTcpPort }
    $profilePath = Join-Path ([System.IO.Path]::GetTempPath()) ("qbt-vanced-webui-cdp-" + [Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $profilePath -Force | Out-Null

    $process = $null
    $socket = $null

    try {
        $args = @(
            "--remote-debugging-port=$debugPort",
            "--user-data-dir=$profilePath",
            "--headless=new",
            "--disable-gpu",
            "--disable-extensions",
            "--disable-background-networking",
            "--no-first-run",
            "--no-default-browser-check",
            "--enable-precise-memory-info",
            "about:blank"
        )

        $process = Start-Process -FilePath $browser -ArgumentList $args -PassThru -WindowStyle Hidden
        Wait-CdpReady $debugPort | Out-Null
        $target = Get-CdpPageTarget $debugPort
        $socket = Connect-CdpWebSocket $target.webSocketDebuggerUrl

        Invoke-CdpCommand $socket "Page.enable" | Out-Null
        Invoke-CdpCommand $socket "Runtime.enable" | Out-Null
        Invoke-CdpCommand $socket "Network.enable" | Out-Null
        Invoke-CdpCommand $socket "Performance.enable" | Out-Null

        Invoke-CdpCommand $socket "Page.navigate" @{ url = $BaseUrl } | Out-Null
        Start-Sleep -Seconds 2

        $usernameLiteral = ConvertTo-JsLiteral $Username
        $passwordLiteral = ConvertTo-JsLiteral $Password
        $loginExpression = @"
(async () => {
  const params = new URLSearchParams();
  params.set("username", $usernameLiteral);
  params.set("password", $passwordLiteral);
  const response = await fetch("/api/v2/auth/login", {
    method: "POST",
    credentials: "include",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: params.toString()
  });
  return await response.text();
})()
"@

        $loginResult = Invoke-CdpCommand $socket "Runtime.evaluate" @{
            expression = $loginExpression
            awaitPromise = $true
            returnByValue = $true
        }
        if ($loginResult.result.value -notmatch "Ok\.") {
            throw "Browser login failed: $($loginResult.result.value)"
        }

        Invoke-CdpCommand $socket "Page.navigate" @{ url = $BaseUrl } | Out-Null
        Start-Sleep -Seconds 5

        $peakBytes = 0.0
        $samples = 0
        $deadline = (Get-Date).AddSeconds($MemoryPollSeconds)
        $pollExpression = "fetch('/api/v2/sync/maindata?rid=0', { credentials: 'include' }).then(r => r.text()).then(t => t.length).catch(() => -1)"

        while ($true) {
            Invoke-CdpCommand $socket "Runtime.evaluate" @{
                expression = $pollExpression
                awaitPromise = $true
                returnByValue = $true
            } | Out-Null

            $usedBytes = Get-JsHeapUsedBytes $socket
            if ($usedBytes -gt $peakBytes) { $peakBytes = $usedBytes }
            $samples++

            $peakMB = [Math]::Round($peakBytes / 1MB, 1)
            Write-Host ("  JS heap sample {0}: peak={1:N1}MB" -f $samples, $peakMB) -ForegroundColor DarkGray

            $remaining = [int][Math]::Ceiling(($deadline - (Get-Date)).TotalSeconds)
            if ($remaining -le 0) { break }
            Start-Sleep -Seconds ([Math]::Min($MemoryPollIntervalSeconds, $remaining))
        }

        return @{
            PeakMB = [Math]::Round($peakBytes / 1MB, 1)
            Samples = $samples
            Browser = [System.IO.Path]::GetFileName($browser)
        }
    } finally {
        Close-CdpSocket $socket
        if ($process -and -not $process.HasExited) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        }
        Remove-TempBrowserProfile $profilePath
    }
}

# --- Auth ---
Write-Host ("`nLarge-Library Performance Smoke - {0} ({1} torrents)`n" -f $BaseUrl, $Count) -ForegroundColor Cyan

try {
    $loginResp = Invoke-WebRequest -Method POST -Uri "$BaseUrl/api/v2/auth/login" `
        -Body @{ username = $Username; password = $Password } `
        -SessionVariable session -UseBasicParsing
    $script:session = $session
    if ($loginResp.Content -notmatch "Ok\.") { throw "Login failed" }
} catch {
    Write-Host ("  Cannot connect to {0} - start qBittorrent Vanced first." -f $BaseUrl) -ForegroundColor Red
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

# --- Browser JS heap regression check ---
if (-not $SkipBrowserMemory) {
    Write-Host "`nMeasuring WebUI JS heap for $MemoryPollSeconds seconds..." -ForegroundColor Yellow
    try {
        $heap = Measure-WebUIBrowserHeap
        $ok = $heap.PeakMB -le $HeapCeilingMB
        $results += @{
            Label = "WebUI JS heap peak"
            Seconds = $heap.PeakMB
            Ceiling = $HeapCeilingMB
            Unit = "MB"
            Ok = $ok
            Detail = "samples=$($heap.Samples), browser=$($heap.Browser), poll=${MemoryPollSeconds}s"
        }
    } catch {
        $results += @{
            Label = "WebUI JS heap peak"
            Seconds = 0
            Ceiling = $HeapCeilingMB
            Unit = "MB"
            Ok = $false
            Detail = $_.Exception.Message
        }
    }
} else {
    Write-Host "`nSkipping browser JS heap check (-SkipBrowserMemory)." -ForegroundColor Yellow
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
Write-Host ("{0,-35} {1,8} {2,8} {3,6} {4,4} {5}" -f "Test", "Value", "Ceiling", "Result", "Unit", "Detail")
Write-Host ("{0,-35} {1,8} {2,8} {3,6} {4,4} {5}" -f "----", "-----", "-------", "------", "----", "------")

$allPassed = $true
foreach ($r in $results) {
    $status = if ($r.Ok) { "PASS" } else { "FAIL" }
    $color = if ($r.Ok) { "Green" } else { "Red" }
    $unit = if ($r.ContainsKey("Unit")) { $r.Unit } else { "s" }
    if (-not $r.Ok) { $allPassed = $false }
    Write-Host ("{0,-35} {1,8:N3} {2,8:N1} {3,6} {4,4} {5}" -f $r.Label, $r.Seconds, $r.Ceiling, $status, $unit, $r.Detail) -ForegroundColor $color
}

Write-Host ""
exit $(if ($allPassed) { 0 } else { 1 })
