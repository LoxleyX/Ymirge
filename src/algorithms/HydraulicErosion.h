#pragma once

#include "../core/HeightMap.h"
#include "../core/ThreadPool.h"

/**
 * HydraulicErosion
 *
 * Particle-based hydraulic erosion simulation using water droplets.
 * Creates realistic gullies, valleys, and river beds.
 *
 * Algorithm:
 * 1. Spawn water droplets at random positions
 * 2. Each droplet flows downhill, eroding terrain
 * 3. Carries sediment proportional to velocity
 * 4. Deposits sediment when velocity decreases
 * 5. Evaporates over time
 *
 * References:
 * - "Fast Hydraulic Erosion Simulation and Visualization on GPU" (Mei et al. 2007)
 * - Sebastian Lague's hydraulic erosion tutorial
 */
class HydraulicErosion {
public:
    struct Params {
        // Simulation parameters
        int num_droplets = 5000;         // Droplets per iteration
        int max_lifetime = 50;            // Steps before droplet stops
        float inertia = 0.3f;             // Direction persistence (0-1)

        // Erosion/deposition
        float capacity_factor = 3.0f;     // Sediment capacity multiplier
        float min_capacity = 0.01f;       // Minimum sediment capacity
        float erosion_rate = 0.3f;        // Terrain erosion speed
        float deposition_rate = 0.3f;     // Sediment deposition speed

        // Water physics
        float evaporation_rate = 0.01f;   // Water loss per step
        float gravity = 4.0f;             // Downward acceleration
        float initial_water = 1.0f;       // Starting water volume
        float initial_speed = 1.0f;       // Starting velocity

        // Erosion brush
        int erosion_radius = 3;           // Radius of erosion effect
    };

    /**
     * Apply hydraulic erosion to heightmap
     *
     * @param heightMap Terrain to erode (modified in place)
     * @param params Erosion parameters
     * @param pool Thread pool for parallel processing (optional)
     * @param iterations Number of erosion passes (default 1)
     */
    static void apply(HeightMap& heightMap, const Params& params, ThreadPool* pool = nullptr, int iterations = 1);

private:
    /**
     * Simulate single water droplet
     *
     * @param heightMap Terrain heightmap
     * @param params Erosion parameters
     * @param startX, startY Starting position
     */
    static void simulateDroplet(HeightMap& heightMap, const Params& params, float startX, float startY);

    /**
     * Calculate height and gradient at position (bilinear interpolation)
     *
     * @param heightMap Terrain heightmap
     * @param x, y Position (can be fractional)
     * @param gradX, gradY Output gradient
     * @return Interpolated height
     */
    static float calculateHeightAndGradient(const HeightMap& heightMap, float x, float y, float& gradX, float& gradY);

    /**
     * Erode terrain at position with given amount
     *
     * @param heightMap Terrain heightmap
     * @param x, y Position (can be fractional)
     * @param amount Erosion amount
     * @param radius Brush radius
     */
    static void erodeAt(HeightMap& heightMap, float x, float y, float amount, int radius);

    /**
     * Deposit sediment at position
     *
     * @param heightMap Terrain heightmap
     * @param x, y Position (can be fractional)
     * @param amount Deposition amount
     * @param radius Brush radius
     */
    static void depositAt(HeightMap& heightMap, float x, float y, float amount, int radius);
};
