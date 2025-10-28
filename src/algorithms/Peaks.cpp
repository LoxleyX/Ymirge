#include "Peaks.h"
#include <cmath>

void Peaks::execute(HeightMap& map,
                   float intensity,
                   uint32_t seed,
                   ThreadPool* pool) {

    if (intensity < 0.01f) return;

    int width = map.getWidth();
    int height = map.getHeight();

    // Create noise generators for peaks and slopes
    PerlinNoise peakNoise(seed);
    PerlinNoise slopeNoise(seed * 5);  // Different seed for variety

    pool->parallelFor(0, height, [&](size_t y) {
        int yi = static_cast<int>(y);
        for (int x = 0; x < width; ++x) {
            float currentHeight = map.at(x, yi);

            // Only affect higher elevations (creates peaks on existing mountains)
            if (currentHeight > 0.3f) {
                // Normalized coordinates (larger scale for mountain features)
                float nx = x / 200.0f;  // Scale controls mountain frequency
                float ny = y / 200.0f;

                // Create sharp peaks using ridged noise
                float peakPattern = ridgedNoise(peakNoise, nx, ny);

                // Add gradient slopes for natural transitions
                float slopeGradient = slopeNoise.octaveNoise(
                    nx * 0.5f, ny * 0.5f, 3, 0.5f, 2.0f);

                // Normalize slope gradient to 0-1
                slopeGradient = (slopeGradient + 1.0f) * 0.5f;

                // Combine sharp peaks with gradual slopes
                // Sharp component: emphasizes ridges
                float sharpness = std::pow(peakPattern, 2.5f);

                // Gradual component: smooth transitions
                float gradualSlope = std::pow(peakPattern, 0.8f);

                // Blend: 40% sharp, 60% gradual for natural look
                float mountainShape = sharpness * 0.4f + gradualSlope * 0.6f;

                // Create smooth transition starting from mid-elevation
                // This prevents sudden height jumps
                float elevationFactor = (currentHeight - 0.3f) / 0.7f;
                float smoothTransition = std::pow(elevationFactor, 0.6f);

                // Apply height boost
                float heightBoost = mountainShape * intensity * 0.35f * smoothTransition;

                map.at(x, yi) = currentHeight + heightBoost;
            }
        }
    }, 8);
}

float Peaks::ridgedNoise(const PerlinNoise& noise, float x, float y) {
    // Generate multi-octave noise
    float value = noise.octaveNoise(x, y, 5, 0.6f, 2.5f);

    // Ridged effect: invert absolute value
    // This creates sharp peaks instead of smooth hills
    return 1.0f - std::abs(value);
}
