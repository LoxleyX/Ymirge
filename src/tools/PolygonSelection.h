#pragma once

#include "../core/HeightMap.h"
#include <vector>
#include <glm/glm.hpp>

/**
 * PolygonSelection
 *
 * Allows selecting regions of the terrain using polygon drawing.
 * Uses point-in-polygon testing for area-based operations.
 *
 * Features:
 * - Click to add vertices
 * - Auto-close polygon
 * - Point-in-polygon test using ray casting
 * - Generate selection mask
 * - Invert selection
 */
class PolygonSelection {
public:
    PolygonSelection();

    /**
     * Add a vertex to the polygon
     */
    void addVertex(float x, float y);

    /**
     * Remove last vertex
     */
    void removeLastVertex();

    /**
     * Clear all vertices
     */
    void clear();

    /**
     * Close the polygon (connect last to first)
     */
    void close();

    /**
     * Check if polygon is closed
     */
    bool isClosed() const { return closed_; }

    /**
     * Get vertices
     */
    const std::vector<glm::vec2>& getVertices() const { return vertices_; }

    /**
     * Test if point is inside polygon (ray casting algorithm)
     *
     * @param x, y Point coordinates
     * @return true if inside polygon
     */
    bool isPointInside(float x, float y) const;

    /**
     * Generate selection mask from polygon
     *
     * @param width, height Mask dimensions
     * @return HeightMap where 1.0 = selected, 0.0 = not selected
     */
    HeightMap generateMask(int width, int height) const;

    /**
     * Generate feathered selection mask with smooth edges
     *
     * @param width, height Mask dimensions
     * @param featherRadius Feather distance in pixels
     * @return HeightMap where 1.0 = selected, 0.0-1.0 = feathered edge
     */
    HeightMap generateFeatheredMask(int width, int height, float featherRadius) const;

    /**
     * Get bounding box of polygon
     *
     * @param minX, minY, maxX, maxY Output bounds
     */
    void getBounds(float& minX, float& minY, float& maxX, float& maxY) const;

private:
    std::vector<glm::vec2> vertices_;
    bool closed_;

    /**
     * Calculate signed distance from point to polygon edge
     * (negative = inside, positive = outside)
     */
    float signedDistanceToPolygon(float x, float y) const;
};
