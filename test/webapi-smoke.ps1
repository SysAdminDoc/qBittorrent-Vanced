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
.PARAMETER AltWebUIPath
    Optional path to a built alternate WebUI bundle, such as VueTorrent's dist folder.
    When set, the smoke verifies the running instance is serving that bundle at /.
.PARAMETER TimeoutSec
    Per-request timeout in seconds (default: 15).
#>
param(
    [string]$BaseUrl = "http://localhost:8080",
    [string]$Username = "admin",
    [string]$Password = "adminadmin",
    [Alias("AltWebUI", "alt-webui")]
    [string]$AltWebUIPath = "",
    [int]$TimeoutSec = 15
)

$ErrorActionPreference = "Stop"
$BaseUrl = $BaseUrl.TrimEnd("/")
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
    $params = @{ Method = $method; Uri = $uri; WebSession = $script:session; TimeoutSec = $TimeoutSec }
    if ($body) { $params.Body = $body }
    return Invoke-WebRequest @params -UseBasicParsing
}

function Get-AltWebUIAssetRefs($html) {
    $assetPattern = "(?i)(?:src|href)=['`"](?<path>[^'`"]+\.(?:js|css)(?:\?[^'`"]*)?)['`"]"
    $refs = New-Object System.Collections.Generic.List[string]

    foreach ($match in [regex]::Matches($html, $assetPattern)) {
        $ref = $match.Groups["path"].Value.Trim()
        if (($ref -match "^(https?:|data:|//|#)") -or [string]::IsNullOrWhiteSpace($ref)) {
            continue
        }
        if (-not $refs.Contains($ref)) {
            $refs.Add($ref)
        }
    }

    return @($refs)
}

function ConvertTo-WebUIAssetUrl($baseUrl, $assetRef) {
    $clean = $assetRef.Trim()
    $clean = $clean.Split("?")[0]
    $clean = $clean -replace "^[./\\]+", ""
    if ([string]::IsNullOrWhiteSpace($clean)) { return $null }
    return "$($baseUrl.TrimEnd('/'))/$clean"
}

function Test-AlternateWebUI($path) {
    if ([string]::IsNullOrWhiteSpace($path)) { return }

    try {
        $resolvedPath = (Resolve-Path -LiteralPath $path -ErrorAction Stop).Path
        if (-not (Test-Path -LiteralPath $resolvedPath -PathType Container)) {
            Write-Result "alt-webui/path" $false "not a directory: $resolvedPath"
            return
        }

        $indexPath = Join-Path $resolvedPath "index.html"
        if (-not (Test-Path -LiteralPath $indexPath -PathType Leaf)) {
            Write-Result "alt-webui/index.html" $false "missing index.html in $resolvedPath"
            return
        }

        $localIndex = Get-Content -LiteralPath $indexPath -Raw -Encoding UTF8
        $assetRefs = Get-AltWebUIAssetRefs $localIndex
        $rootResp = Invoke-WebRequest -Method GET -Uri "$BaseUrl/" -WebSession $script:session -UseBasicParsing -TimeoutSec $TimeoutSec
        $rootHtml = [string]$rootResp.Content

        $fingerprints = New-Object System.Collections.Generic.List[string]
        $titleMatch = [regex]::Match($localIndex, "(?is)<title>\s*(?<title>.*?)\s*</title>")
        if ($titleMatch.Success -and -not [string]::IsNullOrWhiteSpace($titleMatch.Groups["title"].Value)) {
            $fingerprints.Add($titleMatch.Groups["title"].Value.Trim())
        }
        foreach ($assetRef in ($assetRefs | Select-Object -First 5)) {
            $fingerprints.Add($assetRef)
        }

        $matchedFingerprints = 0
        foreach ($fingerprint in $fingerprints) {
            if ($rootHtml.Contains($fingerprint)) { $matchedFingerprints++ }
        }

        Write-Result "alt-webui/root serves supplied bundle" `
            (($rootResp.StatusCode -eq 200) -and ($matchedFingerprints -gt 0)) `
            "matched=$matchedFingerprints fingerprints=$($fingerprints.Count)"

        if ($assetRefs.Count -gt 0) {
            $assetUrl = ConvertTo-WebUIAssetUrl $BaseUrl $assetRefs[0]
            if ($assetUrl) {
                $assetResp = Invoke-WebRequest -Method GET -Uri $assetUrl -WebSession $script:session -UseBasicParsing -TimeoutSec $TimeoutSec
                $assetLength = if ($null -ne $assetResp.RawContentLength) { $assetResp.RawContentLength } else { 0 }
                if (($assetLength -le 0) -and ($null -ne $assetResp.Content)) { $assetLength = $assetResp.Content.Length }
                Write-Result "alt-webui/asset fetch" `
                    (($assetResp.StatusCode -eq 200) -and ($assetLength -gt 0)) `
                    $assetRefs[0]
            }
        } else {
            Write-Result "alt-webui/asset fetch" $false "no local JS/CSS asset references found"
        }
    } catch {
        Write-Result "alt-webui/static serving" $false $_.Exception.Message
    }
}

# --- Auth ---
Write-Host "`nWebAPI Smoke Test - $BaseUrl`n" -ForegroundColor Cyan

try {
    $loginResp = Invoke-WebRequest -Method POST -Uri "$BaseUrl/api/v2/auth/login" `
        -Body @{ username = $Username; password = $Password } `
        -SessionVariable session -UseBasicParsing -TimeoutSec $TimeoutSec
    $script:session = $session
    Write-Result "auth/login" ($loginResp.Content -match "Ok\.")
} catch {
    Write-Host "  FAIL  auth/login - cannot connect to $BaseUrl" -ForegroundColor Red
    Write-Host "        Start qBittorrent Vanced with WebUI enabled before running this test." -ForegroundColor Yellow
    exit 1
}

# --- Alternate WebUI static bundle ---
Test-AlternateWebUI $AltWebUIPath

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

# --- Path validation: empty setLocation ---
try {
    $resp = Invoke-Api POST "torrents/setLocation" @{ hashes = "all"; location = "" }
    Write-Result "setLocation (empty)" $false "expected error"
} catch {
    $ok = $_.Exception.Response.StatusCode.value__ -eq 400
    Write-Result "setLocation (empty -> 400)" $ok
}

# --- Path validation: invalid setLocation ---
try {
    $resp = Invoke-Api POST "torrents/setLocation" @{ hashes = "all"; location = "Z:\nonexistent_root_99\smoke" }
    Write-Result "setLocation (invalid path)" $false "expected error"
} catch {
    $code = $_.Exception.Response.StatusCode.value__
    $ok = ($code -eq 409) -or ($code -eq 403)
    Write-Result "setLocation (invalid -> 409/403)" $ok "status=$code"
}

# --- Path validation: empty setSavePath ---
try {
    $resp = Invoke-Api POST "torrents/setSavePath" @{ id = "all"; path = "" }
    Write-Result "setSavePath (empty)" $false "expected error"
} catch {
    $ok = $_.Exception.Response.StatusCode.value__ -eq 400
    Write-Result "setSavePath (empty -> 400)" $ok
}

# --- Sync maindata ---
try {
    $sync = (Invoke-Api GET "sync/maindata?rid=0").Content | ConvertFrom-Json
    Write-Result "sync/maindata" ($null -ne $sync.server_state)
} catch { Write-Result "sync/maindata" $false $_.Exception.Message }

# --- Remote-access security ---

# CSRF: POST with mismatched Origin header should be rejected
try {
    $csrfResp = Invoke-WebRequest -Method POST -Uri "$BaseUrl/api/v2/torrents/createCategory" `
        -Body @{ category = "_csrf_test"; savePath = "" } `
        -Headers @{ Origin = "http://evil.example.com" } `
        -WebSession $script:session -UseBasicParsing -TimeoutSec $TimeoutSec
    Write-Result "security/csrf cross-origin POST rejected" $false "expected rejection, got $($csrfResp.StatusCode)"
} catch {
    $code = $_.Exception.Response.StatusCode.value__
    $ok = ($code -eq 401) -or ($code -eq 403)
    Write-Result "security/csrf cross-origin POST rejected ($code)" $ok
}

# CSRF: POST with mismatched Referer header should also be rejected
try {
    $csrfRefResp = Invoke-WebRequest -Method POST -Uri "$BaseUrl/api/v2/torrents/createCategory" `
        -Body @{ category = "_csrf_ref_test"; savePath = "" } `
        -Headers @{ Referer = "http://evil.example.com/page" } `
        -WebSession $script:session -UseBasicParsing -TimeoutSec $TimeoutSec
    Write-Result "security/csrf cross-origin Referer rejected" $false "expected rejection"
} catch {
    $code = $_.Exception.Response.StatusCode.value__
    $ok = ($code -eq 401) -or ($code -eq 403)
    Write-Result "security/csrf cross-origin Referer rejected ($code)" $ok
}

# SameSite cookie: login Set-Cookie should contain SameSite=Strict and HttpOnly
try {
    $cookieLoginResp = Invoke-WebRequest -Method POST -Uri "$BaseUrl/api/v2/auth/login" `
        -Body @{ username = $Username; password = $Password } `
        -UseBasicParsing -TimeoutSec $TimeoutSec
    $setCookieHeader = $cookieLoginResp.Headers["Set-Cookie"]
    $cookieStr = if ($setCookieHeader -is [array]) { $setCookieHeader -join "; " } else { "$setCookieHeader" }
    $hasSameSiteStrict = $cookieStr -match "(?i)SameSite=Strict"
    $hasHttpOnly = $cookieStr -match "(?i)HttpOnly"
    Write-Result "security/cookie SameSite=Strict" $hasSameSiteStrict
    Write-Result "security/cookie HttpOnly" $hasHttpOnly
} catch { Write-Result "security/cookie attributes" $false $_.Exception.Message }

# CORS: preflight from foreign origin should not expose credentials
try {
    $corsResp = Invoke-WebRequest -Method OPTIONS -Uri "$BaseUrl/api/v2/auth/login" `
        -Headers @{
            Origin = "http://evil.example.com"
            "Access-Control-Request-Method" = "POST"
        } `
        -UseBasicParsing -TimeoutSec $TimeoutSec
    $acac = $corsResp.Headers["Access-Control-Allow-Credentials"]
    $noCredentials = ($null -eq $acac) -or ("$acac" -ne "true")
    Write-Result "security/cors no allow-credentials" $noCredentials
} catch {
    Write-Result "security/cors preflight rejected" $true
}

# X-Forwarded-Host: should be ignored without reverse-proxy mode
try {
    $xffResp = Invoke-WebRequest -Method POST -Uri "$BaseUrl/api/v2/torrents/createCategory" `
        -Body @{ category = "_xff_test"; savePath = "" } `
        -Headers @{ "X-Forwarded-Host" = "evil.example.com" } `
        -WebSession $script:session -UseBasicParsing -TimeoutSec $TimeoutSec
    $xffOk = $xffResp.StatusCode -eq 200
    Write-Result "security/x-forwarded-host ignored (no reverse proxy)" $xffOk
    # clean up test category
    try { Invoke-Api POST "torrents/removeCategories" @{ categories = "_xff_test" } | Out-Null } catch {}
} catch {
    $code = $_.Exception.Response.StatusCode.value__
    Write-Result "security/x-forwarded-host ignored (no reverse proxy)" $false "got $code, expected 200 (header should be ignored)"
}

# --- Disabled modules (RSS/Search) ---
try {
    $indexResp = Invoke-WebRequest -Method GET -Uri "$BaseUrl/" -WebSession $script:session -UseBasicParsing -TimeoutSec $TimeoutSec
    $html = [string]$indexResp.Content
    $noSearchTab = -not ($html -match 'id="searchTabLink"')
    $noRssTab = -not ($html -match 'id="rssTabLink"')
    $noSearchMenu = -not ($html -match 'id="showSearchEngineLink"')
    $noRssMenu = -not ($html -match 'id="showRssReaderLink"')
    Write-Result "disabled-modules/no search tab" $noSearchTab
    Write-Result "disabled-modules/no rss tab" $noRssTab
    Write-Result "disabled-modules/no search menu" $noSearchMenu
    Write-Result "disabled-modules/no rss menu" $noRssMenu
} catch { Write-Result "disabled-modules/html check" $false $_.Exception.Message }

try {
    $rssResp = Invoke-WebRequest -Method GET -Uri "$BaseUrl/api/v2/rss/items" -WebSession $script:session -UseBasicParsing -TimeoutSec $TimeoutSec
    Write-Result "disabled-modules/rss endpoint" $false "expected 404 or error"
} catch {
    $code = $_.Exception.Response.StatusCode.value__
    $ok = ($code -eq 404) -or ($code -eq 409)
    Write-Result "disabled-modules/rss endpoint ($code)" $ok
}

try {
    $searchResp = Invoke-WebRequest -Method GET -Uri "$BaseUrl/api/v2/search/status" -WebSession $script:session -UseBasicParsing -TimeoutSec $TimeoutSec
    Write-Result "disabled-modules/search endpoint" $false "expected 404 or error"
} catch {
    $code = $_.Exception.Response.StatusCode.value__
    $ok = ($code -eq 404) -or ($code -eq 409)
    Write-Result "disabled-modules/search endpoint ($code)" $ok
}

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
