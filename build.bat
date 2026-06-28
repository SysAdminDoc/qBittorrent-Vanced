@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"
set "TEST_BUILD=OFF"

:parse_args
if "%~1"=="" goto after_args
if /i "%~1"=="--test" set "TEST_BUILD=ON"
if /i "%~1"=="--verify" rem accepted for parity with build.ps1; verification runs after every build
shift
goto parse_args
:after_args

:: Find Visual Studio via vswhere if available
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto after_vswhere
for /f "tokens=*" %%i in ('"%VSWHERE%" -products * -latest -property installationPath 2^>nul') do set "VS_PATH=%%i"
:after_vswhere
if not defined VS_PATH if exist "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools"
if not defined VS_PATH if exist "C:\Program Files\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VS_PATH=C:\Program Files\Microsoft Visual Studio\18\BuildTools"
if not defined VS_PATH if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" set "VS_PATH=C:\Program Files\Microsoft Visual Studio\18\Community"
if not defined VS_PATH if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
if not defined VS_PATH if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\BuildTools"
if not defined VS_PATH if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community"
if not defined VS_PATH if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional"
if not defined VS_PATH if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"

set "VCVARSALL=%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VCVARSALL%" goto vcvars_found
echo ERROR: vcvars64.bat not found at %VCVARSALL%
exit /b 1
:vcvars_found

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

set "VS_VCPKG_ROOT=%VS_PATH%\VC\vcpkg"
if exist "%VS_VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" set "VCPKG_ROOT=%VS_VCPKG_ROOT%"
if defined VCPKG_ROOT if exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" goto vcpkg_found
if exist "%USERPROFILE%\repos\vcpkg\scripts\buildsystems\vcpkg.cmake" set "VCPKG_ROOT=%USERPROFILE%\repos\vcpkg"
if defined VCPKG_ROOT if exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" goto vcpkg_found
echo ERROR: vcpkg not found
exit /b 1
:vcpkg_found

echo === Configuring with CMake (source: %PROJECT_ROOT%) ===
"%CMAKE_EXE%" -S "%PROJECT_ROOT%" -B "%PROJECT_ROOT%\build" ^
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
"%CMAKE_EXE%" --build "%PROJECT_ROOT%\build" --config Release -j %NUMBER_OF_PROCESSORS%

if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo === Build complete ===
for /f "delims=" %%i in ('dir /s /b "%PROJECT_ROOT%\build\qbittorrent.exe" 2^>nul') do if not defined QBT_EXE set "QBT_EXE=%%i"
if not defined QBT_EXE (
    echo ERROR: qbittorrent.exe not found in build directory
    exit /b 1
)
echo Built: %QBT_EXE%

set "WINDEPLOYQT_EXE=%PROJECT_ROOT%\build\vcpkg_installed\x64-windows\tools\Qt6\bin\windeployqt.exe"
if not exist "%WINDEPLOYQT_EXE%" goto skip_windeployqt
echo === Deploying Qt runtime plugins ===
"%WINDEPLOYQT_EXE%" --release --no-translations --no-compiler-runtime "%QBT_EXE%"
if errorlevel 1 (
    echo ERROR: windeployqt failed
    exit /b 1
)
goto after_windeployqt
:skip_windeployqt
echo WARNING: windeployqt.exe not found at %WINDEPLOYQT_EXE%; the executable may not launch without Qt plugins
:after_windeployqt

if "%TEST_BUILD%"=="ON" (
    echo === Running tests ===
    "%CMAKE_EXE%" --build "%PROJECT_ROOT%\build" --target check
    if errorlevel 1 (
        echo ERROR: Tests failed
        exit /b 1
    )
    echo === Tests passed ===
)

echo === Verifying executable ===
"%QBT_EXE%" --version
if errorlevel 1 (
    echo ERROR: Executable verification failed
    exit /b 1
)
echo === Verification complete ===
