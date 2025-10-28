#pragma once

#include <cstdint>

struct TerrainParams {
    // Base terrain
    uint32_t seed = 12345;
    float scale = 100.0f;
    int octaves = 6;
    float persistence = 0.5f;
    float lacunarity = 2.0f;

    // Valleys
    float valleyStrength = 0.3f;
    float valleySharpness = 2.0f;
    float valleyWidth = 0.3f;
    float flattenValleys = 0.0f;
    float valleyConnectivity = 0.0f;

    // Rivers
    float riverIntensity = 0.4f;
    float riverWidth = 0.02f;

    // River Enhancements (Phase 2)
    bool enableRiverEnhancements = false;
    bool useGradientFlow = true;
    float flowSmoothing = 0.3f;
    bool enableTributaries = true;
    int tributariesPerRiver = 2;
    float tributaryWidth = 0.4f;
    bool enableWetlands = true;
    float wetlandRadius = 30.0f;
    float wetlandStrength = 0.3f;

    // Effects
    float erosion = 0.2f;
    int terracing = 0;
    float peaks = 0.3f;
    float island = 0.0f;
    float edgePadding = 0.15f;
    float islandShape = 1.5f;

    // Thermal Erosion (Phase 2.2)
    bool thermalErosionEnabled = false;
    float thermalTalusAngle = 0.7f;     // Radians (~40 degrees)
    float thermalRate = 0.5f;            // Material transfer rate
    int thermalIterations = 30;          // Number of passes

    // Hydraulic Erosion (Phase 2.2)
    bool hydraulicErosionEnabled = false;
    int hydraulicDroplets = 5000;        // Number of droplets per pass
    int hydraulicLifetime = 50;          // Max steps per droplet
    float hydraulicInertia = 0.3f;       // Direction persistence
    float hydraulicCapacity = 3.0f;      // Sediment capacity factor
    float hydraulicErosion = 0.3f;       // Erosion rate
    float hydraulicDeposition = 0.3f;    // Deposition rate
    int hydraulicIterations = 1;         // Number of passes

    // New for archipelago
    float islandDensity = 0.5f;
    float islandSize = 100.0f;

    // Rendering
    float seaLevel = 0.25f;  // Height of sea plane (0-1)

    bool operator==(const TerrainParams& other) const {
        return seed == other.seed &&
               scale == other.scale &&
               octaves == other.octaves &&
               persistence == other.persistence &&
               lacunarity == other.lacunarity &&
               valleyStrength == other.valleyStrength &&
               valleySharpness == other.valleySharpness &&
               valleyWidth == other.valleyWidth &&
               flattenValleys == other.flattenValleys &&
               valleyConnectivity == other.valleyConnectivity &&
               riverIntensity == other.riverIntensity &&
               riverWidth == other.riverWidth &&
               erosion == other.erosion &&
               terracing == other.terracing &&
               peaks == other.peaks &&
               island == other.island &&
               edgePadding == other.edgePadding &&
               islandShape == other.islandShape &&
               islandDensity == other.islandDensity &&
               islandSize == other.islandSize &&
               seaLevel == other.seaLevel;
    }

    bool operator!=(const TerrainParams& other) const {
        return !(*this == other);
    }
};
