#include "HydraulicErosion.h"
#include <random>
#include <cmath>
#include <algorithm>

void HydraulicErosion::apply(HeightMap& heightMap, const Params& params, ThreadPool* pool, int iterations) {
    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    // Random number generator (thread-local for parallel safety)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(0.0f, static_cast<float>(width - 1));
    std::uniform_real_distribution<float> distY(0.0f, static_cast<float>(height - 1));

    for (int iter = 0; iter < iterations; ++iter) {
        // Spawn droplets
        for (int i = 0; i < params.num_droplets; ++i) {
            float startX = distX(gen);
            float startY = distY(gen);

            simulateDroplet(heightMap, params, startX, startY);
        }
    }
}

void HydraulicErosion::simulateDroplet(HeightMap& heightMap, const Params& params, float startX, float startY) {
    float x = startX;
    float y = startY;
    float dirX = 0.0f;
    float dirY = 0.0f;
    float speed = params.initial_speed;
    float water = params.initial_water;
    float sediment = 0.0f;

    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    for (int lifetime = 0; lifetime < params.max_lifetime; ++lifetime) {
        // Get current cell
        int cellX = static_cast<int>(x);
        int cellY = static_cast<int>(y);

        // Stop if out of bounds
        if (cellX < 0 || cellX >= width - 1 || cellY < 0 || cellY >= height - 1) {
            break;
        }

        // Calculate height and gradient
        float gradX, gradY;
        float currentHeight = calculateHeightAndGradient(heightMap, x, y, gradX, gradY);

        // Update direction (blend with gradient using inertia)
        dirX = dirX * params.inertia - gradX * (1.0f - params.inertia);
        dirY = dirY * params.inertia - gradY * (1.0f - params.inertia);

        // Normalize direction
        float len = std::sqrt(dirX * dirX + dirY * dirY);
        if (len > 0.0f) {
            dirX /= len;
            dirY /= len;
        }

        // Update position
        float newX = x + dirX;
        float newY = y + dirY;

        // Stop if new position is out of bounds
        if (newX < 0.0f || newX >= width - 1 || newY < 0.0f || newY >= height - 1) {
            break;
        }

        // Calculate new height
        float newHeight = calculateHeightAndGradient(heightMap, newX, newY, gradX, gradY);

        // Calculate height difference
        float deltaHeight = newHeight - currentHeight;

        // Calculate sediment capacity (proportional to speed and height difference)
        float capacity = std::max(-deltaHeight, params.min_capacity) * speed * water * params.capacity_factor;

        // Erode or deposit
        if (sediment > capacity || deltaHeight > 0.0f) {
            // Deposit sediment
            float amountToDeposit = (deltaHeight > 0.0f) ?
                (std::min)(deltaHeight, sediment) :
                (sediment - capacity) * params.deposition_rate;

            sediment -= amountToDeposit;
            depositAt(heightMap, x, y, amountToDeposit, params.erosion_radius);
        } else {
            // Erode terrain
            float amountToErode = (std::min)((capacity - sediment) * params.erosion_rate, -deltaHeight);

            erodeAt(heightMap, x, y, amountToErode, params.erosion_radius);
            sediment += amountToErode;
        }

        // Update speed (gravity and slope)
        speed = std::sqrt(speed * speed + deltaHeight * params.gravity);

        // Evaporate water
        water *= (1.0f - params.evaporation_rate);

        // Move to new position
        x = newX;
        y = newY;
    }
}

float HydraulicErosion::calculateHeightAndGradient(const HeightMap& heightMap, float x, float y, float& gradX, float& gradY) {
    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    // Get cell coordinates
    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int x1 = std::min(x0 + 1, width - 1);
    int y1 = std::min(y0 + 1, height - 1);

    // Fractional parts
    float fx = x - x0;
    float fy = y - y0;

    // Sample 4 corners
    float h00 = heightMap.sample(x0, y0);
    float h10 = heightMap.sample(x1, y0);
    float h01 = heightMap.sample(x0, y1);
    float h11 = heightMap.sample(x1, y1);

    // Bilinear interpolation for height
    float h0 = h00 * (1.0f - fx) + h10 * fx;
    float h1 = h01 * (1.0f - fx) + h11 * fx;
    float interpolatedHeight = h0 * (1.0f - fy) + h1 * fy;

    // Calculate gradient (central difference)
    gradX = (h10 - h00) * (1.0f - fy) + (h11 - h01) * fy;
    gradY = (h01 - h00) * (1.0f - fx) + (h11 - h10) * fx;

    return interpolatedHeight;
}

void HydraulicErosion::erodeAt(HeightMap& heightMap, float x, float y, float amount, int radius) {
    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    int cellX = static_cast<int>(x);
    int cellY = static_cast<int>(y);

    // Apply erosion with circular brush
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int nx = cellX + dx;
            int ny = cellY + dy;

            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                // Calculate distance from center
                float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));

                if (dist <= radius) {
                    // Weight erosion by distance (inverse square)
                    float weight = std::max(0.0f, 1.0f - dist / radius);
                    float erosion = amount * weight;

                    heightMap.at(nx, ny) -= erosion;
                }
            }
        }
    }
}

void HydraulicErosion::depositAt(HeightMap& heightMap, float x, float y, float amount, int radius) {
    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    int cellX = static_cast<int>(x);
    int cellY = static_cast<int>(y);

    // Apply deposition with circular brush
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int nx = cellX + dx;
            int ny = cellY + dy;

            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                // Calculate distance from center
                float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));

                if (dist <= radius) {
                    // Weight deposition by distance
                    float weight = std::max(0.0f, 1.0f - dist / radius);
                    float deposition = amount * weight;

                    heightMap.at(nx, ny) += deposition;
                }
            }
        }
    }
}
