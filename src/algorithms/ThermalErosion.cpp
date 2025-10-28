#include "ThermalErosion.h"
#include <algorithm>
#include <cmath>

void ThermalErosion::apply(HeightMap& heightMap, const Params& params, ThreadPool* pool) {
    if (params.iterations <= 0 || params.thermalRate < 0.001f) {
        return;  // Nothing to do
    }

    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    // Create work buffer for double-buffering
    HeightMap workBuffer(width, height);

    // Perform multiple erosion passes
    for (int iter = 0; iter < params.iterations; ++iter) {
        // Alternate between source and dest for double-buffering
        if (iter % 2 == 0) {
            thermalPass(heightMap, workBuffer, params, pool);
        } else {
            thermalPass(workBuffer, heightMap, params, pool);
        }
    }

    // Ensure final result is in heightMap
    if (params.iterations % 2 == 1) {
        heightMap = workBuffer;
    }
}

void ThermalErosion::thermalPass(HeightMap& source, HeightMap& dest, const Params& params, ThreadPool* pool) {
    int width = source.getWidth();
    int height = source.getHeight();

    // Copy source to dest first
    dest = source;

    // Precompute talus threshold (height difference that triggers erosion)
    // For 1 unit cell distance and talus angle, threshold = tan(angle)
    float talusThreshold = std::tan(params.talusAngle);

    auto processRow = [&](size_t y) {
        int yi = static_cast<int>(y);

        for (int x = 1; x < width - 1; ++x) {
            float current = source.at(x, yi);

            // Check 8 neighbors (Moore neighborhood)
            // Cardinal neighbors (distance = 1.0)
            const int dx[] = {-1,  0,  1, -1, 1, -1, 0, 1};
            const int dy[] = {-1, -1, -1,  0, 0,  1, 1, 1};
            const float distances[] = {
                1.414f, 1.0f, 1.414f,  // Top row (diagonal, straight, diagonal)
                1.0f,         1.0f,     // Middle row (straight, straight)
                1.414f, 1.0f, 1.414f   // Bottom row (diagonal, straight, diagonal)
            };

            float totalEroded = 0.0f;
            float depositAmounts[8] = {0};

            // Calculate material transfer to each neighbor
            for (int n = 0; n < 8; ++n) {
                int nx = x + dx[n];
                int ny = yi + dy[n];

                // Boundary check
                if (nx < 0 || nx >= width || ny < 0 || ny >= height) {
                    continue;
                }

                float neighbor = source.at(nx, ny);
                float heightDiff = current - neighbor;

                // Check if slope exceeds talus angle
                float slopeAngle = calculateSlopeAngle(heightDiff, distances[n]);

                if (slopeAngle > params.talusAngle) {
                    // Material needs to transfer
                    // Amount is proportional to how much slope exceeds threshold
                    float excessHeight = heightDiff - (talusThreshold * distances[n]);
                    float transferAmount = excessHeight * params.thermalRate;

                    // Limit transfer to avoid negative heights
                    transferAmount = std::min(transferAmount, heightDiff * 0.5f);

                    totalEroded += transferAmount;
                    depositAmounts[n] = transferAmount;
                }
            }

            // Apply erosion to current cell
            if (totalEroded > 0.0f) {
                dest.at(x, yi) = current - totalEroded;

                // Deposit material on neighbors
                for (int n = 0; n < 8; ++n) {
                    if (depositAmounts[n] > 0.0f) {
                        int nx = x + dx[n];
                        int ny = yi + dy[n];

                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            dest.at(nx, ny) += depositAmounts[n];
                        }
                    }
                }
            }
        }
    };

    // Process rows in parallel
    if (pool) {
        pool->parallelFor(1, height - 1, processRow);
    } else {
        // Single-threaded fallback
        for (int y = 1; y < height - 1; ++y) {
            processRow(y);
        }
    }
}
