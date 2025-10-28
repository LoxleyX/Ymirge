#!/bin/bash
# Linux/macOS build script for Ymirge

echo "======================================"
echo "  Building Ymirge Terrain Generator"
echo "======================================"

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo ""
echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=Release ..
if [ $? -ne 0 ]; then
    echo ""
    echo "Error: CMake configuration failed!"
    echo "Please make sure you have CMake installed:"
    echo "  Ubuntu/Debian: sudo apt-get install cmake build-essential"
    echo "  macOS: brew install cmake"
    exit 1
fi

# Build
echo ""
echo "Building project..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
if [ $? -ne 0 ]; then
    echo ""
    echo "Error: Build failed!"
    exit 1
fi

echo ""
echo "======================================"
echo "  Build successful!"
echo "======================================"
echo ""
echo "Executables built:"
echo "  1. build/bin/ymirge     (Console test suite)"
echo "  2. build/bin/ymirge-ui  (Interactive UI application)"
echo ""
echo "Dependencies auto-downloaded:"
echo "  - raylib 5.0"
echo "  - stb_image_write.h"
echo ""
echo "======================================"
echo ""
echo "What would you like to run?"
echo "  1. UI Application (recommended)"
echo "  2. Console test suite"
echo "  3. Exit"
echo ""
read -p "Enter choice (1-3): " CHOICE

if [ "$CHOICE" = "1" ]; then
    cd bin
    echo ""
    echo "Launching Ymirge UI Application..."
    echo ""
    echo "Controls:"
    echo "  - Try the presets!"
    echo "  - Adjust sliders to customize"
    echo "  - Click 'Export Heightmap' to save PNG"
    echo "  - Click 'Export Splatmap' for texture maps"
    echo "  - Press G to regenerate terrain"
    echo ""
    ./ymirge-ui
    cd ..
    exit 0
fi

if [ "$CHOICE" = "2" ]; then
    cd bin
    echo ""
    echo "Running console test suite..."
    ./ymirge
    echo ""
    echo "Check the current directory for generated PPM files."
    cd ..
    exit 0
fi

cd ..
