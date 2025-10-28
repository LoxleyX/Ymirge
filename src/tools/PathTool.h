#pragma once

#include "../core/HeightMap.h"
#include <vector>
#include <glm/glm.hpp>

/**
 * PathTool
 *
 * Draw smooth paths/splines on terrain for roads, rivers, trails, etc.
 * Uses Catmull-Rom splines for smooth interpolation between control points.
 *
 * Features:
 * - Click to add control points
 * - Automatic smooth curve generation
 * - Adjustable width and falloff
 * - Carve (lower terrain) or raise (build roads)
 * - Flatten along path
 */
class PathTool {
public:
    enum class PathMode {
        CARVE,    // Lower terrain (rivers, canyons)
        RAISE,    // Raise terrain (roads, bridges)
        FLATTEN   // Flatten to path height
    };

    struct Params {
        PathMode mode = PathMode::CARVE;
        float width = 5.0f;           // Path width in pixels
        float depth = 0.1f;           // Depth/height change
        float falloff = 3.0f;         // Edge smoothing distance
        float smoothness = 10.0f;     // Curve smoothness (points per segment)
        bool autoFlatten = true;      // Flatten along path
    };

    PathTool();

    /**
     * Add control point to path
     */
    void addPoint(float x, float y);

    /**
     * Remove last control point
     */
    void removeLastPoint();

    /**
     * Clear all control points
     */
    void clear();

    /**
     * Get control points
     */
    const std::vector<glm::vec2>& getControlPoints() const { return controlPoints_; }

    /**
     * Generate smooth spline from control points
     * Uses Catmull-Rom interpolation
     *
     * @param smoothness Points per segment
     * @return Interpolated spline points
     */
    std::vector<glm::vec2> generateSpline(float smoothness = 10.0f) const;

    /**
     * Apply path to heightmap
     *
     * @param heightMap Terrain to modify
     * @param params Path parameters
     */
    void applyToHeightMap(HeightMap& heightMap, const Params& params) const;

private:
    std::vector<glm::vec2> controlPoints_;

    /**
     * Catmull-Rom spline interpolation
     *
     * @param p0, p1, p2, p3 Control points (p1-p2 is the segment)
     * @param t Interpolation factor (0-1)
     * @return Interpolated point
     */
    static glm::vec2 catmullRom(const glm::vec2& p0, const glm::vec2& p1, 
                                  const glm::vec2& p2, const glm::vec2& p3, float t);

    /**
     * Calculate distance from point to line segment
     */
    static float distanceToSegment(const glm::vec2& point, const glm::vec2& a, const glm::vec2& b);

    /**
     * Get height at position for flattening
     */
    float getPathHeight(const HeightMap& heightMap, const std::vector<glm::vec2>& splinePoints) const;
};
