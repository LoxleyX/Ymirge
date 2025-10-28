#ifdef YMIRGE_SDL_UI_ENABLED

#include "PerlinNoiseGPU.h"
#include "GPUCompute.h"
#include "ComputeShader.h"
#include <random>
#include <algorithm>
#include <vector>
#include "GPUBuffer.h"
#include <iostream>

PerlinNoiseGPU::PerlinNoiseGPU() = default;
PerlinNoiseGPU::~PerlinNoiseGPU() = default;

bool PerlinNoiseGPU::isAvailable() {
    return GPUCompute::isAvailable();
}

void PerlinNoiseGPU::uploadPermutationTable(const PerlinNoise& noise) {
    // Get permutation table from CPU noise generator
    // Note: We'll need to add a getter to PerlinNoise class
    // For now, create a temporary one with the same seed
    std::vector<int> perm(512);

    // The permutation table is generated in PerlinNoise constructor
    // We need access to the p_ array. For now, we'll generate our own.
    // TODO: Add getPermutationTable() method to PerlinNoise

    // Create temporary noise with same seed to get permutation
    // This is a workaround - ideally we'd expose the permutation table
    std::vector<int> tempPerm(256);
    for (int i = 0; i < 256; i++) {
        tempPerm[i] = i;
    }

    // Simple shuffle using the same algorithm
    std::mt19937 rng(12345); // Default seed for now
    std::shuffle(tempPerm.begin(), tempPerm.end(), rng);

    // Duplicate for wrapping
    for (int i = 0; i < 256; i++) {
        perm[i] = tempPerm[i];
        perm[i + 256] = tempPerm[i];
    }

    // Upload to GPU
    if (!permutationBuffer_) {
        permutationBuffer_ = std::make_unique<GPUBuffer>(512 * sizeof(int), perm.data());
    } else {
        permutationBuffer_->upload(perm.data(), 512 * sizeof(int));
    }
}

void PerlinNoiseGPU::generate(HeightMap& map, float scale, int octaves,
                               float persistence, float lacunarity, uint32_t seed) {
    if (!isAvailable()) {
        std::cerr << "GPU Perlin noise not available" << std::endl;
        return;
    }

    try {
        // Create shader if needed
        if (!shader_) {
            shader_ = std::make_unique<ComputeShader>("gpu_shaders/perlin_noise.comp");
            if (!shader_->isValid()) {
                std::cerr << "Failed to load Perlin noise shader" << std::endl;
                return;
            }
        }

        // Create CPU noise generator to get permutation table
        PerlinNoise cpuNoise(seed);
        uploadPermutationTable(cpuNoise);

        // Create buffer for heightmap
        size_t dataSize = map.getWidth() * map.getHeight() * sizeof(float);
        GPUBuffer heightBuffer(dataSize);

        // Bind shader and set uniforms
        shader_->bind();
        shader_->setUniform("width", map.getWidth());
        shader_->setUniform("height", map.getHeight());
        shader_->setUniform("scale", scale);
        shader_->setUniform("octaves", octaves);
        shader_->setUniform("persistence", persistence);
        shader_->setUniform("lacunarity", lacunarity);

        // Bind buffers
        heightBuffer.bind(0);
        permutationBuffer_->bind(1);

        // Dispatch compute shader
        int groupsX = (map.getWidth() + 15) / 16;
        int groupsY = (map.getHeight() + 15) / 16;
        shader_->dispatch(groupsX, groupsY);

        // Read back results
        heightBuffer.download(map.getData(), dataSize);

    } catch (const std::exception& e) {
        std::cerr << "GPU Perlin noise generation failed: " << e.what() << std::endl;
    }
}

#endif // YMIRGE_SDL_UI_ENABLED
