#!/bin/bash
# Clean rebuild script for Ymirge
# This script removes the build directory and calls build.sh

echo "======================================"
echo "  Ymirge - Clean Rebuild"
echo "======================================"
echo ""

# Try to remove build directory
if [ -d "build" ]; then
    echo "Removing old build directory..."
    sleep 1
    rm -rf build

    if [ -d "build" ]; then
        echo ""
        echo "Warning: Could not remove build directory."
        echo "It may be in use by another process."
        echo "Please close any programs using it and try again."
        exit 1
    fi

    echo "Build directory cleaned successfully!"
else
    echo "No existing build directory found."
fi

echo ""
echo "======================================"
echo "  Running build.sh..."
echo "======================================"
echo ""

# Run the regular build script (stays in sync)
./build.sh
