#pragma once

#include "HeightMap.h"
#include "ThreadPool.h"
#include <vector>

// Rivers flow from map edges into lowest valleys using straight-line paths
class Rivers {
public:
    static void execute(HeightMap& map,
                       float intensity,
                       float width,
                       ThreadPool* pool);

private:
    struct Point {
        int x, y;
        float height;
    };

    static std::vector<Point> findLowestValleys(
        const HeightMap& map, int count);

    static void createRiverFromEdge(
        HeightMap& map,
        const Point& target,
        float intensity,
        float width);

    static void carveRiverPath(
        HeightMap& map,
        const Point& start,
        const Point& end,
        float intensity,
        float width);

    static void carveRiverSegment(
        HeightMap& map,
        int cx, int cy,
        float radius,
        float depth,
        float intensity);
};
