@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "TEST_BUILD=OFF"
if /i "%~1"=="--test" set "TEST_BUILD=ON"

:: Find Visual Studio via vswhere if available
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "tokens=*" %%i in ('"%VSWHERE%" -latest -property installationPath 2^>nul') do set "VS_PATH=%%i"
)
if not defined VS_PATH set "VS_PATH=C:\Program Files\Microsoft Visual Studio\18\Community"

set "VCVARSALL=%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARSALL%" (
    echo ERROR: vcvars64.bat not found at %VCVARSALL%
    exit /b 1
)

call "%VCVARSALL%"
if errorlevel 1 (
    echo ERROR: Failed to set up MSVC environment
    exit /b 1
)

:: Find tools - prefer VS-bundled, fall back to PATH
set "CMAKE_EXE=%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "%CMAKE_EXE%" for %%i in (cmake.exe) do set "CMAKE_EXE=%%~$PATH:i"

set "NINJA_EXE=%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
if not exist "%NINJA_EXE%" for %%i in (ninja.exe) do set "NINJA_EXE=%%~$PATH:i"

set "VCPKG_ROOT=%VS_PATH%\VC\vcpkg"
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    if defined VCPKG_ROOT (
        echo Using VCPKG_ROOT from environment: %VCPKG_ROOT%
    ) else (
        echo ERROR: vcpkg not found
        exit /b 1
    )
)

echo === Configuring with CMake (source: %PROJECT_ROOT%) ===
"%CMAKE_EXE%" -S "%PROJECT_ROOT%" -B "%PROJECT_ROOT%build" ^
    -G Ninja ^
    -DCMAKE_MAKE_PROGRAM="%NINJA_EXE%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DWEBUI=ON ^
    -DGUI=ON ^
    -DSTACKTRACE=OFF ^
    -DTESTING=%TEST_BUILD%

if errorlevel 1 (
    echo ERROR: CMake configure failed
    exit /b 1
)

echo === Building ===
"%CMAKE_EXE%" --build "%PROJECT_ROOT%build" --config Release -j %NUMBER_OF_PROCESSORS%

if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo === Build complete ===
dir /s /b "%PROJECT_ROOT%build\qbittorrent.exe" 2>nul

if "%TEST_BUILD%"=="ON" (
    echo === Running tests ===
    "%CMAKE_EXE%" --build "%PROJECT_ROOT%build" --target test
    if errorlevel 1 (
        echo ERROR: Tests failed
        exit /b 1
    )
    echo === Tests passed ===
)
