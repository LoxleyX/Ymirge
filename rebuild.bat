@echo off
REM Clean rebuild script for Ymirge
REM This script removes the build directory and calls build.bat

echo ======================================
echo   Ymirge - Clean Rebuild
echo ======================================
echo.

REM Close any running processes that might lock the build directory
echo Attempting to clean build directory...

REM Try to remove build directory (wait a moment first)
if exist build (
    echo Removing old build directory...
    timeout /t 1 /nobreak >nul
    rmdir /s /q build 2>nul

    REM Check if removal succeeded
    if exist build (
        echo.
        echo Warning: Could not remove build directory.
        echo Please:
        echo   1. Close any File Explorer windows showing the build folder
        echo   2. Close Visual Studio or any IDEs
        echo   3. Try running this script again
        echo.
        echo Or manually delete the 'build' folder and run build.bat
        pause
        exit /b 1
    )

    echo Build directory cleaned successfully!
) else (
    echo No existing build directory found.
)

echo.
echo ======================================
echo   Running build.bat...
echo ======================================
echo.

REM Run the regular build script (stays in sync)
call build.bat
