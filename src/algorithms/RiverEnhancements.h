#pragma once

#include "HeightMap.h"
#include "ThreadPool.h"
#include <vector>
#include <glm/glm.hpp>

/**
 * River Enhancements (Phase 2)
 *
 * Advanced river generation with:
 * - Realistic flow following terrain gradients
 * - Tributary generation (branching streams)
 * - Wetland zones around rivers
 */
class RiverEnhancements {
public:
    struct Params {
        // Main river parameters
        float intensity = 0.5f;          // Overall river strength (0-1)
        float width = 0.03f;             // Base river width

        // Flow algorithm
        bool useGradientFlow = true;     // Follow terrain gradients (vs straight lines)
        float flowSmoothing = 0.3f;      // Direction smoothing (0-1)

        // Tributaries
        bool enableTributaries = true;   // Generate branching streams
        int tributariesPerRiver = 2;     // Number of tributaries per main river (1-4)
        float tributaryWidth = 0.4f;     // Tributary width relative to main river

        // Wetlands
        bool enableWetlands = true;      // Generate marshy areas
        float wetlandRadius = 30.0f;     // Wetland spread distance (pixels)
        float wetlandStrength = 0.3f;    // Moisture effect strength
    };

    /**
     * Apply enhanced river generation to heightmap
     */
    static void apply(HeightMap& heightMap, const Params& params, ThreadPool* pool);

private:
    struct RiverPoint {
        glm::vec2 pos;        // Position (x, y)
        float width;          // River width at this point
        float depth;          // Carving depth
    };

    struct RiverPath {
        std::vector<RiverPoint> points;
        bool isMain;          // Main river vs tributary
    };

    // Flow field for gradient-based pathfinding
    struct FlowField {
        std::vector<glm::vec2> directions;  // Flow direction at each pixel
        int width, height;

        FlowField(int w, int h) : width(w), height(h) {
            directions.resize(w * h, glm::vec2(0.0f));
        }

        glm::vec2& at(int x, int y) {
            return directions[y * width + x];
        }

        const glm::vec2& at(int x, int y) const {
            return directions[y * width + x];
        }
    };

    // Calculate flow directions based on terrain gradient
    static FlowField calculateFlowField(const HeightMap& map);

    // Generate river path following flow field
    static RiverPath generateFlowBasedRiver(
        const HeightMap& map,
        const FlowField& flowField,
        const glm::vec2& start,
        const glm::vec2& target,
        const Params& params);

    // Generate tributary starting from point on main river
    static RiverPath generateTributary(
        const HeightMap& map,
        const FlowField& flowField,
        const glm::vec2& branchPoint,
        const Params& params);

    // Carve river path into heightmap
    static void carveRiverPath(
        HeightMap& map,
        const RiverPath& path,
        float intensity);

    // Apply wetland effect around river
    static void applyWetlands(
        HeightMap& map,
        const RiverPath& path,
        const Params& params);

    // Find spawn points for rivers (map edges + high elevation springs)
    static std::vector<glm::vec2> findRiverSources(
        const HeightMap& map,
        int numRivers);

    // Find river destinations (lowest valleys)
    static std::vector<glm::vec2> findRiverDestinations(
        const HeightMap& map,
        int numRivers);

    // Smooth river path to remove sharp turns
    static void smoothPath(RiverPath& path, float amount);
};
