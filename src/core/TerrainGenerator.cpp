#include "TerrainGenerator.h"
#include "../algorithms/ValleyFlattening.h"
#include "../algorithms/EdgeSmoothing.h"
#include "../algorithms/Peaks.h"
#include "../algorithms/Rivers.h"
#include "../algorithms/ValleyConnectivity.h"
#include "../algorithms/TerrainSoftening.h"
#include "../algorithms/ThermalErosion.h"
#include "../algorithms/HydraulicErosion.h"
#include "../algorithms/RiverEnhancements.h"
#include <cmath>

TerrainGenerator::TerrainGenerator(int width, int height, ThreadPool* threadPool)
    : width_(width)
    , height_(height)
    , heightMap_(width, height)
    , workBuffer_(width, height)
    , threadPool_(threadPool)
    , generating_(false)
    , perlin_(nullptr) {
}

const HeightMap& TerrainGenerator::getHeightMap() const {
    std::lock_guard<std::mutex> lock(heightMapMutex_);
    return heightMap_;
}

HeightMap TerrainGenerator::getHeightMapCopy() const {
    std::lock_guard<std::mutex> lock(heightMapMutex_);
    return heightMap_;
}

std::future<void> TerrainGenerator::generateAsync(const TerrainParams& params) {
    return threadPool_->enqueue([this, params]() {
        this->generate(params);
    });
}

void TerrainGenerator::generate(const TerrainParams& params) {
    generating_.store(true);

    // Initialize noise generator with seed
    perlin_ = std::make_unique<PerlinNoise>(params.seed);

    // Clear heightmap
    {
        std::lock_guard<std::mutex> lock(heightMapMutex_);
        heightMap_.clear();
    }

    // Generate base noise
    generateBaseNoise(params);

    // Apply valley effect
    if (params.valleyStrength > 0.01f) {
        applyValleys(params);
    }

    // Apply erosion
    if (params.erosion > 0.01f) {
        applyErosion(params);
    }

    // Apply mountain peaks
    if (params.peaks > 0.01f) {
        applyPeaks(params);
    }

    // Apply island mode
    if (params.island > 0.01f) {
        applyIslandMask(params);
    }

    // Apply terracing
    if (params.terracing > 0) {
        applyTerracing(params);
    }

    if (params.edgePadding > 0.01f) {
        applyEdgePadding(params);
    }

    if (params.terrainSmoothness > 0.01f) {
        softenTerrain(params);
    }

    if (params.riverIntensity > 0.01f) {
        applyRivers(params);
    }

    // Normalize to 0-1 range
    {
        std::lock_guard<std::mutex> lock(heightMapMutex_);
        heightMap_.normalize();
    }

    generating_.store(false);
}

void TerrainGenerator::generateBaseNoise(const TerrainParams& params) {
    std::lock_guard<std::mutex> lock(heightMapMutex_);

    threadPool_->parallelFor(0, height_, [this, &params](size_t y) {
        for (int x = 0; x < width_; ++x) {
            float nx = x / params.scale;
            float ny = y / params.scale;

            float height = perlin_->octaveNoise(nx, ny,
                                               params.octaves,
                                               params.persistence,
                                               params.lacunarity);

            // Normalize from [-1, 1] to [0, 1]
            height = (height + 1.0f) * 0.5f;

            // Apply curve for gradual transitions
            height = std::pow(height, 1.2f);

            heightMap_.at(x, static_cast<int>(y)) = height;
        }
    }, 16);  // Process 16 rows per task
}

void TerrainGenerator::applyValleys(const TerrainParams& params) {
    // Valley generation is handled by valley flattening later in pipeline
    // This placeholder is kept for potential future multi-pass valley effects
    (void)params;
}

void TerrainGenerator::smoothValleyFloors(const TerrainParams& params) {
    // Valley smoothing is handled by valley flattening's 3-pass system
    (void)params;
}

void TerrainGenerator::applyErosion(const TerrainParams& params) {
    if (params.erosion < 0.01f && !params.thermalErosionEnabled && !params.hydraulicErosionEnabled) {
        return;  // No erosion to apply
    }

    std::lock_guard<std::mutex> lock(heightMapMutex_);

    // Apply thermal erosion (cliff collapse, talus slopes)
    if (params.thermalErosionEnabled && params.thermalIterations > 0) {
        ThermalErosion::Params thermalParams;
        thermalParams.talusAngle = params.thermalTalusAngle;
        thermalParams.thermalRate = params.thermalRate * params.erosion;  // Scale by master erosion
        thermalParams.iterations = params.thermalIterations;

        ThermalErosion::apply(heightMap_, thermalParams, threadPool_);
    }

    // Apply hydraulic erosion (water droplet simulation)
    if (params.hydraulicErosionEnabled && params.hydraulicIterations > 0) {
        HydraulicErosion::Params hydraulicParams;
        hydraulicParams.num_droplets = params.hydraulicDroplets;
        hydraulicParams.max_lifetime = params.hydraulicLifetime;
        hydraulicParams.inertia = params.hydraulicInertia;
        hydraulicParams.capacity_factor = params.hydraulicCapacity;
        hydraulicParams.erosion_rate = params.hydraulicErosion * params.erosion;  // Scale by master erosion
        hydraulicParams.deposition_rate = params.hydraulicDeposition;

        HydraulicErosion::apply(heightMap_, hydraulicParams, threadPool_, params.hydraulicIterations);
    }

    // Apply legacy simple erosion if thermal is disabled
    if (!params.thermalErosionEnabled && params.erosion > 0.01f) {
        workBuffer_ = heightMap_;

        threadPool_->parallelFor(1, height_ - 1, [this, &params](size_t y) {
            int yi = static_cast<int>(y);
            for (int x = 1; x < width_ - 1; ++x) {
                float current = heightMap_.at(x, yi);

                // Sample 4-neighbors
                float top = heightMap_.at(x, yi - 1);
                float bottom = heightMap_.at(x, yi + 1);
                float left = heightMap_.at(x - 1, yi);
                float right = heightMap_.at(x + 1, yi);

                float avgNeighbor = (top + bottom + left + right) * 0.25f;

                // Erode high peaks
                if (current > avgNeighbor) {
                    float diff = (current - avgNeighbor) * params.erosion * 0.3f;
                    workBuffer_.at(x, yi) = current - diff;
                }
            }
        }, 8);

        heightMap_ = workBuffer_;
    }
}

void TerrainGenerator::applyPeaks(const TerrainParams& params) {
    std::lock_guard<std::mutex> lock(heightMapMutex_);
    Peaks::execute(heightMap_, params.peaks, params.seed, threadPool_);
}

void TerrainGenerator::applyIslandMask(const TerrainParams& params) {
    std::lock_guard<std::mutex> lock(heightMapMutex_);

    if (params.archipelagoMode) {
        // Generate multiple islands
        applyArchipelagoMask(params);
        return;
    }

    // Single island mode (original behavior)
    float centerX = width_ * 0.5f;
    float centerY = height_ * 0.5f;
    float maxDist = std::sqrt(centerX * centerX + centerY * centerY);

    threadPool_->parallelFor(0, height_, [this, &params, centerX, centerY, maxDist](size_t y) {
        int yi = static_cast<int>(y);
        for (int x = 0; x < width_; ++x) {
            float dx = x - centerX;
            float dy = y - centerY;
            float dist = std::sqrt(dx * dx + dy * dy);
            float normalizedDist = dist / maxDist;

            // Create gradient falloff
            float falloff = 1.0f - std::pow(normalizedDist, 1.5f);
            float islandEffect = std::max(0.0f, falloff);

            // Blend with island strength
            heightMap_.at(x, yi) *= (1.0f - params.island) + (islandEffect * params.island);
        }
    }, 8);
}

void TerrainGenerator::applyArchipelagoMask(const TerrainParams& params) {
    // Generate island centers using seeded random with minimum spacing
    std::vector<std::pair<float, float>> islandCenters;
    std::vector<float> islandRadii;

    std::mt19937 rng(params.seed + 999);  // Different seed for island placement
    std::uniform_real_distribution<float> posDist(0.1f, 0.9f);
    std::uniform_real_distribution<float> sizeDist(params.archipelagoMinSize, params.archipelagoMaxSize);
    std::uniform_real_distribution<float> noiseDist(-1.0f, 1.0f);

    // Try to place islands with minimum spacing
    int attempts = 0;
    int maxAttempts = params.archipelagoIslandCount * 50;

    while (islandCenters.size() < static_cast<size_t>(params.archipelagoIslandCount) && attempts < maxAttempts) {
        attempts++;

        float cx = posDist(rng);
        float cy = posDist(rng);

        // Check spacing from existing islands
        bool tooClose = false;
        for (size_t i = 0; i < islandCenters.size(); ++i) {
            float dx = cx - islandCenters[i].first;
            float dy = cy - islandCenters[i].second;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist < params.archipelagoSpacing + islandRadii[i] * 0.5f) {
                tooClose = true;
                break;
            }
        }

        if (!tooClose) {
            islandCenters.push_back({cx, cy});
            islandRadii.push_back(sizeDist(rng));
        }
    }

    // Create noise for irregular island shapes
    PerlinNoise shapeNoise(params.seed + 1000);

    // Apply multi-island mask
    threadPool_->parallelFor(0, height_, [this, &params, &islandCenters, &islandRadii, &shapeNoise](size_t y) {
        int yi = static_cast<int>(y);
        float ny = static_cast<float>(y) / height_;

        for (int x = 0; x < width_; ++x) {
            float nx = static_cast<float>(x) / width_;

            // Find contribution from all islands
            float totalIslandEffect = 0.0f;

            for (size_t i = 0; i < islandCenters.size(); ++i) {
                float cx = islandCenters[i].first;
                float cy = islandCenters[i].second;
                float radius = islandRadii[i];

                float dx = nx - cx;
                float dy = ny - cy;
                float dist = std::sqrt(dx * dx + dy * dy);

                // Add noise to radius for irregular shapes
                float angle = std::atan2(dy, dx);
                float noiseVal = shapeNoise.octaveNoise(
                    cx * 10.0f + std::cos(angle) * 3.0f,
                    cy * 10.0f + std::sin(angle) * 3.0f,
                    3, 0.5f, 2.0f);
                float noisyRadius = radius * (1.0f + noiseVal * params.archipelagoVariation * 0.4f);

                if (dist < noisyRadius) {
                    // Smooth falloff from center to edge
                    float normalizedDist = dist / noisyRadius;
                    float falloff = 1.0f - std::pow(normalizedDist, params.islandShape);
                    falloff = std::max(0.0f, falloff);

                    // Use max to combine overlapping islands
                    totalIslandEffect = std::max(totalIslandEffect, falloff);
                }
            }

            // Apply island mask
            float current = heightMap_.at(x, yi);
            float masked = current * ((1.0f - params.island) + (totalIslandEffect * params.island));

            // Push underwater areas deeper
            if (totalIslandEffect < 0.1f) {
                masked *= 0.3f;  // Ocean floor
            }

            heightMap_.at(x, yi) = masked;
        }
    }, 8);
}

void TerrainGenerator::applyTerracing(const TerrainParams& params) {
    std::lock_guard<std::mutex> lock(heightMapMutex_);

    int steps = params.terracing;
    if (steps <= 0) return;

    for (int i = 0; i < width_ * height_; ++i) {
        float h = heightMap_.getData()[i];
        float stepped = std::floor(h * steps) / steps;
        heightMap_.getData()[i] = stepped;
    }
}

void TerrainGenerator::applyEdgePadding(const TerrainParams& params) {
    std::lock_guard<std::mutex> lock(heightMapMutex_);
    EdgeSmoothing::execute(heightMap_, params.edgePadding, params.islandShape,
                          params.seed + 1, threadPool_);
}

void TerrainGenerator::flattenLowAreas(const TerrainParams& params) {
    std::lock_guard<std::mutex> lock(heightMapMutex_);
    ValleyFlattening::execute(heightMap_, params.flattenValleys, threadPool_);
}

void TerrainGenerator::softenTerrain(const TerrainParams& params) {
    std::lock_guard<std::mutex> lock(heightMapMutex_);
    TerrainSoftening::execute(heightMap_, params.terrainSmoothness,
                              params.softeningThreshold, 8, 3, threadPool_);
}

void TerrainGenerator::connectValleys(const TerrainParams& params) {
    std::lock_guard<std::mutex> lock(heightMapMutex_);

    // Calculate valley threshold (same as ValleyFlattening uses)
    std::vector<float> sorted(heightMap_.getData(),
                              heightMap_.getData() + heightMap_.getSize());
    size_t thresholdIndex = static_cast<size_t>(
        sorted.size() * (0.35f + params.flattenValleys * 0.35f));
    std::nth_element(sorted.begin(),
                    sorted.begin() + thresholdIndex,
                    sorted.end());
    float threshold = sorted[thresholdIndex];

    ValleyConnectivity::execute(heightMap_, params.valleyConnectivity,
                               threshold, threadPool_);
}

void TerrainGenerator::applyRivers(const TerrainParams& params) {
    std::lock_guard<std::mutex> lock(heightMapMutex_);
    
    if (params.enableRiverEnhancements) {
        // Use enhanced river system with gradients, tributaries, and wetlands
        RiverEnhancements::Params enhancedParams;
        enhancedParams.intensity = params.riverIntensity;
        enhancedParams.width = params.riverWidth;
        enhancedParams.useGradientFlow = params.useGradientFlow;
        enhancedParams.flowSmoothing = params.flowSmoothing;
        enhancedParams.enableTributaries = params.enableTributaries;
        enhancedParams.tributariesPerRiver = params.tributariesPerRiver;
        enhancedParams.tributaryWidth = params.tributaryWidth;
        enhancedParams.enableWetlands = params.enableWetlands;
        enhancedParams.wetlandRadius = params.wetlandRadius;
        enhancedParams.wetlandStrength = params.wetlandStrength;
        
        RiverEnhancements::apply(heightMap_, enhancedParams, threadPool_);
    } else {
        // Use original simple river system
        Rivers::execute(heightMap_, params.riverIntensity, params.riverWidth, threadPool_);
    }
}
