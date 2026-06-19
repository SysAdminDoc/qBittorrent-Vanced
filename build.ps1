param(
    [switch]$Test,
    [switch]$Verify,
    [string]$VSPath
)

$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot
if (-not $projectRoot) { $projectRoot = (Get-Location).Path }

if (-not $VSPath) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $VSPath = & $vswhere -latest -property installationPath 2>$null
    }
    if (-not $VSPath) {
        $candidates = @(
            "C:\Program Files\Microsoft Visual Studio\18\Community",
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
    if ($env:VCPKG_ROOT) { $vcpkgRoot = $env:VCPKG_ROOT }
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
} else {
    Write-Warning "qbittorrent.exe not found in build directory"
}

if ($Test) {
    Write-Output "=== Running tests ==="
    & $cmakeExe --build "$projectRoot\build" --target test
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Tests failed with exit code $LASTEXITCODE"
        exit 1
    }
    Write-Output "=== Tests passed ==="
}

if ($Verify -and $exe) {
    Write-Output "=== Verifying executable ==="
    & $exe.FullName --version
    Write-Output "=== Verification complete ==="
}
