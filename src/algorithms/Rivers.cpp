#include "Rivers.h"
#include <algorithm>
#include <cmath>

void Rivers::execute(HeightMap& map,
                    float intensity,
                    float width,
                    ThreadPool* pool) {
    (void)pool;  // Unused - reserved for future parallel implementation

    if (intensity < 0.01f) return;

    // Number of rivers based on intensity (3-11 rivers)
    int numRivers = static_cast<int>(intensity * 8) + 3;

    // Find lowest valley areas as destinations
    auto valleyTargets = findLowestValleys(map, numRivers);

    // Create rivers from edges to these valleys
    for (const auto& target : valleyTargets) {
        createRiverFromEdge(map, target, intensity, width);
    }
}

std::vector<Rivers::Point> Rivers::findLowestValleys(
    const HeightMap& map, int count) {

    std::vector<Point> valleys;

    int width = map.getWidth();
    int height = map.getHeight();

    // Sample interior points (not near edges)
    int margin = static_cast<int>(width * 0.15f);
    const int step = 8;  // Sample every 8th pixel for performance

    for (int y = margin; y < height - margin; y += step) {
        for (int x = margin; x < width - margin; x += step) {
            float h = map.at(x, y);

            // Look for low elevation areas (valleys)
            if (h < 0.35f) {
                valleys.push_back({x, y, h});
            }
        }
    }

    // Sort by elevation (lowest first)
    std::sort(valleys.begin(), valleys.end(),
        [](const Point& a, const Point& b) { return a.height < b.height; });

    // Select valleys that are spread out
    std::vector<Point> selectedValleys;
    float minSpacing = width * 0.2f;  // Minimum 20% of map width apart

    for (const auto& valley : valleys) {
        if (selectedValleys.size() >= static_cast<size_t>(count)) break;

        // Check if far enough from already selected valleys
        bool tooClose = false;
        for (const auto& selected : selectedValleys) {
            float dx = static_cast<float>(valley.x - selected.x);
            float dy = static_cast<float>(valley.y - selected.y);
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < minSpacing) {
                tooClose = true;
                break;
            }
        }

        if (!tooClose) {
            selectedValleys.push_back(valley);
        }
    }

    return selectedValleys;
}

void Rivers::createRiverFromEdge(
    HeightMap& map,
    const Point& target,
    float intensity,
    float width) {

    int mapWidth = map.getWidth();
    int mapHeight = map.getHeight();

    // Find closest edge point to this valley
    std::vector<Point> edges = {
        {0, target.y, 0.0f},                    // Left edge
        {mapWidth - 1, target.y, 0.0f},        // Right edge
        {target.x, 0, 0.0f},                   // Top edge
        {target.x, mapHeight - 1, 0.0f}        // Bottom edge
    };

    // Find nearest edge
    Point closestEdge = edges[0];
    float minDist = std::numeric_limits<float>::infinity();

    for (const auto& edge : edges) {
        float dx = static_cast<float>(edge.x - target.x);
        float dy = static_cast<float>(edge.y - target.y);
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < minDist) {
            minDist = dist;
            closestEdge = edge;
        }
    }

    // Create river path from edge to valley
    carveRiverPath(map, closestEdge, target, intensity, width);
}

void Rivers::carveRiverPath(
    HeightMap& map,
    const Point& start,
    const Point& end,
    float intensity,
    float riverWidth) {

    float dx = static_cast<float>(end.x - start.x);
    float dy = static_cast<float>(end.y - start.y);
    float distance = std::sqrt(dx * dx + dy * dy);

    int steps = static_cast<int>(distance / 2.0f);
    if (steps < 2) return;

    float baseWidth = riverWidth * 800.0f;  // Convert to pixels

    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / steps;

        // Smooth interpolation (smoothstep)
        float smoothT = t * t * (3.0f - 2.0f * t);

        int x = static_cast<int>(start.x + dx * smoothT);
        int y = static_cast<int>(start.y + dy * smoothT);

        // Width stays relatively consistent (90-100%)
        float width = baseWidth * (0.9f + t * 0.1f);

        // Depth increases as river approaches valley
        float depth = 0.5f + t * 0.5f;  // 50% to 100%

        carveRiverSegment(map, x, y, width, depth, intensity);
    }
}

void Rivers::carveRiverSegment(
    HeightMap& map,
    int cx, int cy,
    float radius,
    float depth,
    float intensity) {

    int mapWidth = map.getWidth();
    int mapHeight = map.getHeight();

    int iRadius = static_cast<int>(std::max(2.0f, radius));

    for (int dy = -iRadius; dy <= iRadius; ++dy) {
        for (int dx = -iRadius; dx <= iRadius; ++dx) {
            int x = cx + dx;
            int y = cy + dy;

            // Bounds check
            if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) continue;

            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (dist > radius) continue;

            // Smooth falloff from center
            float falloff = std::pow(1.0f - (dist / radius), 1.8f);

            // Carve based on intensity and depth
            float carvingAmount = intensity * 0.12f * falloff * depth;

            // Lower the terrain (create river channel)
            map.at(x, y) = std::max(0.0f, map.at(x, y) - carvingAmount);
        }
    }
}
