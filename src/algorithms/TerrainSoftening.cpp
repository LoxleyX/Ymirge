#include "TerrainSoftening.h"
#include <algorithm>
#include <cmath>
#include <vector>

void TerrainSoftening::execute(HeightMap& map, float strength, float threshold,
                                int smoothRadius, int passes, ThreadPool* pool) {
    if (strength < 0.01f) return;

    float elevationThreshold = calculateElevationThreshold(map, threshold);

    for (int pass = 0; pass < passes; pass++) {
        applySmoothingPass(map, elevationThreshold, strength, smoothRadius, pool);
    }
}

float TerrainSoftening::calculateElevationThreshold(const HeightMap& map, float threshold) {
    std::vector<float> sorted(map.getData(), map.getData() + map.getSize());
    size_t thresholdIndex = static_cast<size_t>(sorted.size() * threshold);
    thresholdIndex = std::min(thresholdIndex, sorted.size() - 1);
    std::nth_element(sorted.begin(), sorted.begin() + thresholdIndex, sorted.end());
    return sorted[thresholdIndex];
}

void TerrainSoftening::applySmoothingPass(HeightMap& map, float elevationThreshold,
                                          float strength, int smoothRadius, ThreadPool* pool) {
    int width = map.getWidth();
    int height = map.getHeight();

    HeightMap smoothed = map;

    pool->parallelFor(0, height, [&](size_t y) {
        int yi = static_cast<int>(y);
        for (int x = 0; x < width; x++) {
            float sum = 0.0f;
            float weightSum = 0.0f;

            for (int dy = -smoothRadius; dy <= smoothRadius; dy++) {
                for (int dx = -smoothRadius; dx <= smoothRadius; dx++) {
                    int nx = std::clamp(x + dx, 0, width - 1);
                    int ny = std::clamp(yi + dy, 0, height - 1);

                    float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                    if (dist > smoothRadius) continue;

                    float sigma = smoothRadius / 3.0f;
                    float weight = std::exp(-(dist * dist) / (2.0f * sigma * sigma));

                    sum += map.at(nx, ny) * weight;
                    weightSum += weight;
                }
            }

            smoothed.at(x, yi) = sum / weightSum;
        }
    });

    pool->parallelFor(0, height, [&](size_t y) {
        int yi = static_cast<int>(y);
        for (int x = 0; x < width; x++) {
            float originalHeight = map.at(x, yi);
            float smoothedHeight = smoothed.at(x, yi);

            float transitionWidth = 0.15f;
            float lowerBound = elevationThreshold - transitionWidth;
            float upperBound = elevationThreshold + transitionWidth;

            float blendFactor = 0.0f;
            if (originalHeight < lowerBound) {
                blendFactor = 1.0f;
            } else if (originalHeight < upperBound) {
                float t = (originalHeight - lowerBound) / (upperBound - lowerBound);
                blendFactor = 1.0f - t;
            }

            blendFactor *= strength;
            map.at(x, yi) = originalHeight * (1.0f - blendFactor) + smoothedHeight * blendFactor;
        }
    });
}
