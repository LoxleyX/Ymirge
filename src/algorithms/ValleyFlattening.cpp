#include "ValleyFlattening.h"
#include <algorithm>
#include <cmath>
#include <limits>

void ValleyFlattening::execute(HeightMap& map, float strength, ThreadPool* pool) {
    if (strength < 0.01f) return;

    auto valleyFloors = detectValleyFloors(map, strength);
    if (valleyFloors.empty()) return;

    applyFlattening(map, valleyFloors, strength, pool);
    smoothTransitions(map, valleyFloors, strength, 4, pool);
}

float ValleyFlattening::calculateThreshold(const HeightMap& map, float strength) {
    // Dynamic threshold: affects lower 35-70% of terrain
    // Higher strength = more area gets flattened
    std::vector<float> sorted(map.getData(), map.getData() + map.getSize());

    size_t thresholdIndex = static_cast<size_t>(
        sorted.size() * (0.35f + strength * 0.35f));

    std::nth_element(sorted.begin(),
                    sorted.begin() + thresholdIndex,
                    sorted.end());

    return sorted[thresholdIndex];
}

std::unordered_map<int, float> ValleyFlattening::detectValleyFloors(
    const HeightMap& map, float strength) {

    std::unordered_map<int, float> floors;

    float threshold = calculateThreshold(map, strength);

    // Search radius for finding valley floor
    const int searchRadius = 10;

    int width = map.getWidth();
    int height = map.getHeight();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float currentHeight = map.at(x, y);

            if (currentHeight < threshold) {
                float minNeighbor = currentHeight;

                for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
                    for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
                        int nx = std::clamp(x + dx, 0, width - 1);
                        int ny = std::clamp(y + dy, 0, height - 1);

                        float neighborHeight = map.at(nx, ny);

                        if (neighborHeight < threshold) {
                            minNeighbor = std::min(minNeighbor, neighborHeight);
                        }
                    }
                }

                floors[y * width + x] = minNeighbor;
            }
        }
    }

    return floors;
}

void ValleyFlattening::applyFlattening(
    HeightMap& map,
    const std::unordered_map<int, float>& valleyFloors,
    float strength,
    ThreadPool* pool) {

    float threshold = calculateThreshold(map, strength);
    int width = map.getWidth();
    int height = map.getHeight();

    HeightMap tempMap = map;

    pool->parallelFor(0, height, [&](size_t y) {
        int yi = static_cast<int>(y);
        for (int x = 0; x < width; ++x) {
            int idx = static_cast<int>(y * width + x);
            float currentHeight = map.at(x, yi);

            if (currentHeight < threshold && valleyFloors.count(idx)) {
                float valleyFloor = valleyFloors.at(idx);
                float depthBelowThreshold = (threshold - currentHeight) / threshold;

                // Base 85% + up to 15% more based on depth
                float flattenFactor = strength * (0.85f + depthBelowThreshold * 0.15f);

                tempMap.at(x, yi) = currentHeight * (1.0f - flattenFactor) +
                                   valleyFloor * flattenFactor;
            }
        }
    }, 8);

    map = std::move(tempMap);
}

float ValleyFlattening::findBoundaryDistance(
    const HeightMap& map,
    const std::unordered_map<int, float>& valleyFloors,
    int x, int y,
    int searchRadius) {

    float minDist = std::numeric_limits<float>::infinity();
    int width = map.getWidth();
    int height = map.getHeight();
    int idx = y * width + x;

    bool isValley = valleyFloors.count(idx) > 0;

    for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
        for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
            int nx = std::clamp(x + dx, 0, width - 1);
            int ny = std::clamp(y + dy, 0, height - 1);
            int nIdx = ny * width + nx;

            bool neighborIsValley = valleyFloors.count(nIdx) > 0;

            if (isValley != neighborIsValley) {
                float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                minDist = std::min(minDist, dist);
            }
        }
    }

    return minDist;
}

void ValleyFlattening::smoothTransitions(
    HeightMap& map,
    const std::unordered_map<int, float>& valleyFloors,
    float strength,
    int rounds,
    ThreadPool* pool) {
    (void)strength;

    const int smoothRadius = 10;
    const int transitionZone = 20;
    const int boundaryRadius = 25;

    int width = map.getWidth();
    int height = map.getHeight();

    for (int round = 0; round < rounds; ++round) {
        HeightMap tempMap = map;

        pool->parallelFor(0, height, [&](size_t y) {
            int yi = static_cast<int>(y);
            for (int x = 0; x < width; ++x) {
                float distToEdge = findBoundaryDistance(
                    map, valleyFloors, x, yi, boundaryRadius);

                if (distToEdge < transitionZone) {
                    float sum = 0.0f;
                    float weightSum = 0.0f;

                    for (int dy = -smoothRadius; dy <= smoothRadius; ++dy) {
                        for (int dx = -smoothRadius; dx <= smoothRadius; ++dx) {
                            int maxX = static_cast<int>(width) - 1;
                            int maxY = static_cast<int>(height) - 1;
                            int nx = std::clamp<int>(x + dx, 0, maxX);
                            int ny = std::clamp<int>(y + dy, 0, maxY);

                            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                            if (dist > smoothRadius) continue;

                            float weight = std::exp(-(dist * dist) /
                                (smoothRadius * smoothRadius * 0.5f));

                            sum += map.at(nx, ny) * weight;
                            weightSum += weight;
                        }
                    }

                    float smoothed = sum / weightSum;
                    float blendFactor = (1.0f - (distToEdge / transitionZone)) * 0.95f;

                    tempMap.at(x, yi) = map.at(x, yi) * (1.0f - blendFactor) +
                                       smoothed * blendFactor;
                }
            }
        }, 4);

        map = std::move(tempMap);
    }
}
