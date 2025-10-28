#pragma once

#include "HeightMap.h"
#include "ThreadPool.h"
#include <vector>
#include <unordered_set>

/**
 * Valley Connectivity Algorithm
 *
 * Connects isolated flat valley areas with corridors for better gameplay.
 * Uses flood-fill to detect regions, then creates simple straight-line paths.
 */
class ValleyConnectivity {
public:
    /**
     * Execute valley connectivity
     *
     * @param map The heightmap to modify
     * @param connectivity Connectivity strength (0.0-1.0)
     * @param valleyThreshold Height threshold for valleys
     * @param pool ThreadPool for parallel processing
     */
    static void execute(HeightMap& map,
                       float connectivity,
                       float valleyThreshold,
                       ThreadPool* pool);

private:
    struct Point {
        int x, y, idx;
    };

    struct Region {
        std::vector<Point> points;
        int size() const { return static_cast<int>(points.size()); }
    };

    struct Connection {
        Point from, to;
        float distance;
    };

    // Identify separate valley regions using flood-fill
    static std::vector<Region> identifyValleyRegions(
        const HeightMap& map, float threshold);

    // Flood-fill from a starting point
    static Region floodFillRegion(
        const HeightMap& map,
        int startX, int startY,
        float threshold,
        std::unordered_set<int>& visited);

    // Find connections between regions
    static std::vector<Connection> findValleyConnections(
        const std::vector<Region>& regions,
        const HeightMap& map);

    // Create corridor between two points
    static void createCorridor(
        HeightMap& map,
        const Point& from,
        const Point& to,
        float width,
        float threshold);

    // Flatten a single corridor point
    static void flattenCorridorPoint(
        HeightMap& map,
        int cx, int cy,
        float radius,
        float threshold);
};
