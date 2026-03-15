@echo off
setlocal

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 (
    echo ERROR: Failed to set up MSVC environment
    exit /b 1
)

set VCPKG_EXE=C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg\vcpkg.exe
set VCPKG_ROOT=C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg
set CMAKE_EXE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
set NINJA_EXE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe

echo === Configuring with CMake ===
"%CMAKE_EXE%" -S . -B build ^
    -G Ninja ^
    -DCMAKE_MAKE_PROGRAM="%NINJA_EXE%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DWEBUI=ON ^
    -DGUI=ON ^
    -DSTACKTRACE=OFF ^
    -DTESTING=OFF

if errorlevel 1 (
    echo ERROR: CMake configure failed
    exit /b 1
)

echo === Building ===
"%CMAKE_EXE%" --build build --config Release -j %NUMBER_OF_PROCESSORS%

if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo === Build complete ===
dir /s /b build\*.exe 2>nul
