#include "EdgeSmoothing.h"
#include <cmath>
#include <algorithm>

void EdgeSmoothing::execute(HeightMap& map,
                           float edgePadding,
                           float islandShape,
                           uint32_t seed,
                           ThreadPool* pool) {

    if (edgePadding < 0.01f) return;

    // Pass 1: Calculate distance map with noise
    auto distanceMap = calculateDistanceMap(map, islandShape, seed);

    // Pass 2: Multiple rounds of aggressive smoothing (3 passes)
    smoothEdges(map, distanceMap, edgePadding, 3, pool);

    // Pass 3: Apply triple smoothstep for ultra-smooth taper
    applyTripleSmoothstep(map, distanceMap, edgePadding, pool);
}

std::vector<float> EdgeSmoothing::calculateDistanceMap(
    const HeightMap& map,
    float islandShape,
    uint32_t seed) {

    int width = map.getWidth();
    int height = map.getHeight();
    std::vector<float> distMap(width * height);

    float centerX = width * 0.5f;
    float centerY = height * 0.5f;

    // Create noise generator for natural edge variation
    PerlinNoise edgeNoise(seed);
    const float noiseScale = 15.0f;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Normalize coordinates to [-1, 1]
            float nx = (x - centerX) / centerX;
            float ny = (y - centerY) / centerY;

            // Minkowski distance (controls shape)
            // p=1 → diamond, p=2 → circle, p>2 → square with rounded corners
            float p = 1.0f + (islandShape - 1.0f) * 1.5f;
            float minkowskiDist = std::pow(
                std::pow(std::abs(nx), p) + std::pow(std::abs(ny), p),
                1.0f / p
            );

            // Add subtle noise for natural variation
            float noiseValue = edgeNoise.octaveNoise(
                x / noiseScale, y / noiseScale, 3, 0.5f, 2.0f);

            // CRITICAL: Noise strength fades out near edges
            // This prevents jagged pillars at coastlines
            float noiseStrength = std::min(1.0f, minkowskiDist * 2.0f);

            // Only 3% noise influence (reduced from 15% to fix pillars)
            float noisyDist = minkowskiDist + noiseValue * 0.03f * noiseStrength;

            // Convert to 0-1 range (0 = ocean, 1 = island center)
            distMap[y * width + x] = std::max(0.0f, 1.0f - noisyDist);
        }
    }

    return distMap;
}

void EdgeSmoothing::smoothEdges(
    HeightMap& map,
    const std::vector<float>& distanceMap,
    float edgePadding,
    int rounds,
    ThreadPool* pool) {

    int width = map.getWidth();
    int height = map.getHeight();

    // Expanded padding zone for wider smoothing area
    float expandedPadding = edgePadding * 3.5f;

    // Multiple rounds of smoothing
    for (int round = 0; round < rounds; ++round) {
        HeightMap tempMap = map;

        pool->parallelFor(0, height, [&](size_t y) {
            int yi = static_cast<int>(y);
            for (int x = 0; x < width; ++x) {
                int idx = static_cast<int>(y * width + x);
                float normalizedDist = distanceMap[idx];

                // Only smooth within the edge zone
                if (normalizedDist < expandedPadding * 0.7f) {
                    float t = normalizedDist / (expandedPadding * 0.7f);

                    // Apply Gaussian blur
                    float sum = 0.0f;
                    float weightSum = 0.0f;
                    const int radius = 8;

                    for (int dy = -radius; dy <= radius; ++dy) {
                        for (int dx = -radius; dx <= radius; ++dx) {
                            int maxX = static_cast<int>(width) - 1;
                            int maxY = static_cast<int>(height) - 1;
                            int nx = std::clamp<int>(x + dx, 0, maxX);
                            int ny = std::clamp<int>(y + dy, 0, maxY);

                            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                            if (dist > radius) continue;

                            // Gaussian weight
                            float weight = 1.0f - (dist / radius);
                            sum += map.at(nx, ny) * weight;
                            weightSum += weight;
                        }
                    }

                    float smoothed = sum / weightSum;

                    // Extremely aggressive blending (up to 99% smoothed at edges)
                    float blendFactor = 1.0f - t;
                    tempMap.at(x, yi) = map.at(x, yi) * (1.0f - blendFactor * 0.99f) +
                                       smoothed * blendFactor * 0.99f;
                }
            }
        }, 4);

        map = std::move(tempMap);
    }
}

float EdgeSmoothing::smoothstep(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

void EdgeSmoothing::applyTripleSmoothstep(
    HeightMap& map,
    const std::vector<float>& distanceMap,
    float edgePadding,
    ThreadPool* pool) {

    int width = map.getWidth();
    int height = map.getHeight();
    float expandedPadding = edgePadding * 3.5f;

    pool->parallelFor(0, height, [&](size_t y) {
        int yi = static_cast<int>(y);
        for (int x = 0; x < width; ++x) {
            int idx = static_cast<int>(y * width + x);
            float normalizedDist = distanceMap[idx];

            float edgeFade = 1.0f;

            if (normalizedDist < expandedPadding) {
                float t = normalizedDist / expandedPadding;

                // Triple smoothstep for ultra-gradual taper
                float smooth1 = smoothstep(t);
                float smooth2 = smoothstep(smooth1);
                float smooth3 = smoothstep(smooth2);

                edgeFade = smooth3;
            }

            // Apply the edge fade
            map.at(x, yi) *= edgeFade;
        }
    }, 16);
}
