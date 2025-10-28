@echo off
REM Windows build script for Ymirge

echo ======================================
echo   Building Ymirge Terrain Generator
echo ======================================

REM Check if build directory exists
if not exist build mkdir build
cd build

REM Configure with CMake
echo.
echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64

REM Check if CMake generated the solution file (more reliable than errorlevel)
if not exist Ymirge.sln (
    echo.
    echo Error: CMake configuration failed!
    echo Please make sure you have:
    echo   - CMake installed (https://cmake.org/download/)
    echo   - Visual Studio 2022 or later
    echo.
    pause
    exit /b 1
)
echo.
echo Configuration successful!

REM Build
echo.
echo Building project...
cmake --build . --config Release

REM Check if executables were built successfully
if not exist bin\Release\ymirge.exe (
    echo.
    echo Error: Build failed - ymirge.exe not created!
    echo Check the output above for compilation errors.
    echo.
    pause
    exit /b 1
)
if not exist bin\Release\ymirge-ui.exe (
    echo.
    echo Error: Build failed - ymirge-ui.exe not created!
    echo Check the output above for compilation errors.
    echo.
    pause
    exit /b 1
)
echo.
echo Build successful!

echo.
echo ======================================
echo   Build successful!
echo ======================================
echo.
echo Executables built:
echo   1. build\bin\Release\ymirge.exe     (Console test suite)
echo   2. build\bin\Release\ymirge-ui.exe  (Interactive UI application)
echo.
echo Dependencies auto-downloaded:
echo   - raylib 5.0
echo   - stb_image_write.h
echo.
echo ======================================
echo.
echo What would you like to run?
echo   1. UI Application (recommended)
echo   2. Console test suite
echo   3. Exit
echo.
set /p CHOICE=Enter choice (1-3):

if "%CHOICE%"=="1" (
    cd bin\Release
    echo.
    echo Launching Ymirge UI Application...
    echo.
    echo Controls:
    echo   - Try the presets!
    echo   - Adjust sliders to customize
    echo   - Click "Export Heightmap" to save PNG
    echo   - Click "Export Splatmap" for texture maps
    echo   - Press G to regenerate terrain
    echo.
    start ymirge-ui.exe
    cd ..\..
    exit /b 0
)

if "%CHOICE%"=="2" (
    cd bin\Release
    echo.
    echo Running console test suite...
    ymirge.exe
    echo.
    echo Check the current directory for generated PPM files.
    pause
    cd ..\..
    exit /b 0
)

cd ..\..
