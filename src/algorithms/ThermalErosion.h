#pragma once

#include "HeightMap.h"
#include "ThreadPool.h"
#include <cmath>

/**
 * Thermal Erosion - Talus Angle Method
 *
 * Simulates material collapse when slopes exceed angle of repose.
 * Creates natural scree slopes below cliffs and smooths unrealistic steep angles.
 *
 * Algorithm: For each pixel, transfer material to neighbors if slope > talus angle
 * Physics: Angle of repose for loose material is typically 35-45 degrees
 *
 * Performance: ~100ms for 512x512 with 30 iterations (parallelized)
 */
class ThermalErosion {
public:
    struct Params {
        float talusAngle = 0.7f;      // Angle of repose in radians (~40° = 0.7 rad)
        float thermalRate = 0.5f;     // Material transfer rate (0.0-1.0)
        int iterations = 30;           // Number of erosion passes
    };

    /**
     * Apply thermal erosion to heightmap
     * @param heightMap Terrain to erode (modified in-place)
     * @param params Erosion parameters
     * @param pool Thread pool for parallelization (optional)
     */
    static void apply(HeightMap& heightMap, const Params& params, ThreadPool* pool);

private:
    /**
     * Perform single thermal erosion pass
     * Transfers material from steep slopes to neighbors
     */
    static void thermalPass(HeightMap& source, HeightMap& dest, const Params& params, ThreadPool* pool);

    /**
     * Calculate slope angle between two heights
     * @param heightDiff Height difference
     * @param cellSize Distance between cells (normalized to 1.0)
     * @return Slope angle in radians
     */
    static inline float calculateSlopeAngle(float heightDiff, float cellSize = 1.0f) {
        return std::atan(heightDiff / cellSize);
    }
};
