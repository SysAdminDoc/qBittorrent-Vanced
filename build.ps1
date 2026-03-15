$ErrorActionPreference = "Stop"

# Import MSVC environment
$vsPath = "C:\Program Files\Microsoft Visual Studio\18\Community"
$vcvarsall = "$vsPath\VC\Auxiliary\Build\vcvars64.bat"

# Run vcvars64.bat and capture environment variables
$envBlock = cmd.exe /c "`"$vcvarsall`" >nul 2>&1 && set" 2>&1
foreach ($line in $envBlock) {
    if ($line -match '^([^=]+)=(.*)$') {
        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}

Write-Output "=== MSVC Environment loaded ==="
Write-Output "cl.exe: $(Get-Command cl.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source)"

$cmakeExe = "$vsPath\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$ninjaExe = "$vsPath\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
$vcpkgRoot = "$vsPath\VC\vcpkg"

Set-Location "C:\Users\--\repos\qBittorrent-Enhanced-Edition"

Write-Output "=== Configuring with CMake ==="
& $cmakeExe -S . -B build `
    -G Ninja `
    "-DCMAKE_MAKE_PROGRAM=$ninjaExe" `
    -DCMAKE_BUILD_TYPE=Release `
    "-DCMAKE_TOOLCHAIN_FILE=$vcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
    -DWEBUI=ON `
    -DGUI=ON `
    -DSTACKTRACE=OFF `
    -DTESTING=OFF

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configure failed with exit code $LASTEXITCODE"
    exit 1
}

Write-Output "=== Building ==="
& $cmakeExe --build build --config Release -j $env:NUMBER_OF_PROCESSORS

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed with exit code $LASTEXITCODE"
    exit 1
}

Write-Output "=== Build complete ==="
Get-ChildItem -Path build -Recurse -Filter "*.exe" | ForEach-Object { Write-Output $_.FullName }
