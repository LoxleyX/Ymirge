#include "ValleyConnectivity.h"
#include <algorithm>
#include <cmath>
#include <queue>

void ValleyConnectivity::execute(HeightMap& map,
                                 float connectivity,
                                 float valleyThreshold,
                                 ThreadPool* pool) {
    (void)pool;  // Unused - reserved for future parallel implementation

    if (connectivity < 0.01f) return;

    // Identify valley regions using flood-fill
    auto regions = identifyValleyRegions(map, valleyThreshold);

    if (regions.size() <= 1) return;  // No need to connect if only one region

    // Limit to top 6 largest regions for performance
    std::sort(regions.begin(), regions.end(),
        [](const Region& a, const Region& b) { return a.size() > b.size(); });

    if (regions.size() > 6) {
        regions.resize(6);
    }

    // Find connections between regions
    auto connections = findValleyConnections(regions, map);

    // Limit connections based on connectivity strength (max 8 corridors)
    int maxConnections = std::min(static_cast<int>(connectivity * 8),
                                  static_cast<int>(connections.size()));

    // Calculate corridor width based on connectivity strength
    float baseWidth = 8.0f + connectivity * 12.0f;  // 8-20 pixels wide

    // Create corridors between disconnected regions
    for (int i = 0; i < maxConnections; ++i) {
        const auto& conn = connections[i];
        createCorridor(map, conn.from, conn.to, baseWidth, valleyThreshold);
    }
}

std::vector<ValleyConnectivity::Region> ValleyConnectivity::identifyValleyRegions(
    const HeightMap& map, float threshold) {

    std::vector<Region> regions;
    std::unordered_set<int> visited;

    int width = map.getWidth();
    int height = map.getHeight();

    // Sample grid (every 4th pixel for performance)
    const int step = 4;

    for (int y = 0; y < height; y += step) {
        for (int x = 0; x < width; x += step) {
            int idx = y * width + x;

            if (!visited.count(idx) && map.at(x, y) < threshold) {
                // Start flood-fill for new region
                Region region = floodFillRegion(map, x, y, threshold, visited);

                // Only keep regions with reasonable size
                if (region.size() > 50) {
                    regions.push_back(region);
                }
            }
        }
    }

    return regions;
}

ValleyConnectivity::Region ValleyConnectivity::floodFillRegion(
    const HeightMap& map,
    int startX, int startY,
    float threshold,
    std::unordered_set<int>& visited) {

    Region region;
    std::queue<Point> queue;

    int width = map.getWidth();
    int height = map.getHeight();
    int startIdx = startY * width + startX;

    queue.push({startX, startY, startIdx});
    visited.insert(startIdx);

    // Limit region size for performance
    const int maxRegionSize = 500;
    const int step = 2;  // Skip every other pixel

    while (!queue.empty() && region.size() < maxRegionSize) {
        Point current = queue.front();
        queue.pop();

        region.points.push_back(current);

        // Check 4-connected neighbors
        const Point neighbors[] = {
            {current.x - step, current.y, 0},
            {current.x + step, current.y, 0},
            {current.x, current.y - step, 0},
            {current.x, current.y + step, 0}
        };

        for (const auto& neighbor : neighbors) {
            int nx = neighbor.x;
            int ny = neighbor.y;

            // Bounds check
            if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;

            int nIdx = ny * width + nx;

            if (!visited.count(nIdx) && map.at(nx, ny) < threshold) {
                visited.insert(nIdx);
                queue.push({nx, ny, nIdx});
            }
        }
    }

    return region;
}

std::vector<ValleyConnectivity::Connection> ValleyConnectivity::findValleyConnections(
    const std::vector<Region>& regions,
    const HeightMap& map) {

    std::vector<Connection> connections;

    // For each pair of regions, find closest points
    for (size_t i = 0; i < regions.size(); ++i) {
        for (size_t j = i + 1; j < regions.size(); ++j) {
            const auto& regionA = regions[i].points;
            const auto& regionB = regions[j].points;

            float minDist = std::numeric_limits<float>::infinity();
            Connection closestPair = {};

            // Sample points to find closest pair
            size_t sampleSize = std::min<size_t>(20, regionA.size());
            size_t stepA = std::max<size_t>(1, regionA.size() / sampleSize);
            size_t stepB = std::max<size_t>(1, regionB.size() / sampleSize);

            for (size_t a = 0; a < regionA.size(); a += stepA) {
                for (size_t b = 0; b < regionB.size(); b += stepB) {
                    const auto& pointA = regionA[a];
                    const auto& pointB = regionB[b];

                    float dx = static_cast<float>(pointA.x - pointB.x);
                    float dy = static_cast<float>(pointA.y - pointB.y);
                    float dist = std::sqrt(dx * dx + dy * dy);

                    if (dist < minDist) {
                        minDist = dist;
                        closestPair = {pointA, pointB, dist};
                    }
                }
            }

            // Only connect if reasonably close (max 40% of map width)
            float maxConnectionDist = map.getWidth() * 0.4f;
            if (minDist < maxConnectionDist) {
                connections.push_back(closestPair);
            }
        }
    }

    // Sort by distance (connect closest regions first)
    std::sort(connections.begin(), connections.end(),
        [](const Connection& a, const Connection& b) {
            return a.distance < b.distance;
        });

    return connections;
}

void ValleyConnectivity::createCorridor(
    HeightMap& map,
    const Point& from,
    const Point& to,
    float width,
    float threshold) {

    float dx = static_cast<float>(to.x - from.x);
    float dy = static_cast<float>(to.y - from.y);
    float distance = std::sqrt(dx * dx + dy * dy);

    int steps = static_cast<int>(distance / 3.0f);  // Sample every 3 pixels
    if (steps < 2) {
        steps = 2;
    }

    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / steps;

        int x = static_cast<int>(from.x + dx * t);
        int y = static_cast<int>(from.y + dy * t);

        flattenCorridorPoint(map, x, y, width / 2.0f, threshold);
    }
}

void ValleyConnectivity::flattenCorridorPoint(
    HeightMap& map,
    int cx, int cy,
    float radius,
    float threshold) {

    int width = map.getWidth();
    int height = map.getHeight();
    int iRadius = static_cast<int>(std::ceil(radius));

    for (int dy = -iRadius; dy <= iRadius; ++dy) {
        for (int dx = -iRadius; dx <= iRadius; ++dx) {
            int x = cx + dx;
            int y = cy + dy;

            // Bounds check
            if (x < 0 || x >= width || y < 0 || y >= height) continue;

            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (dist > radius) continue;

            // Smooth flattening with falloff
            float falloff = 1.0f - (dist / radius);
            float smoothFalloff = falloff * falloff * (3.0f - 2.0f * falloff);  // Smoothstep

            // Target height is slightly below threshold
            float targetHeight = threshold * 0.7f;
            float currentHeight = map.at(x, y);

            // Only flatten if currently higher than target
            if (currentHeight > targetHeight) {
                map.at(x, y) = currentHeight * (1.0f - smoothFalloff * 0.8f) +
                               targetHeight * smoothFalloff * 0.8f;
            }
        }
    }
}
