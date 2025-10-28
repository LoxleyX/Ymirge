#pragma once

#ifdef YMIRGE_SDL_UI_ENABLED

#include "HeightMap.h"
#include "PerlinNoise.h"
#include <memory>

class ComputeShader;
class GPUBuffer;

// GPU-accelerated Perlin noise generator
class PerlinNoiseGPU {
public:
    PerlinNoiseGPU();
    ~PerlinNoiseGPU();

    // Generate Perlin noise into heightmap
    void generate(HeightMap& map, float scale, int octaves,
                  float persistence, float lacunarity, uint32_t seed);

    // Check if GPU implementation is available
    static bool isAvailable();

private:
    void uploadPermutationTable(const PerlinNoise& noise);

    std::unique_ptr<ComputeShader> shader_;
    std::unique_ptr<GPUBuffer> permutationBuffer_;
};

#endif // YMIRGE_SDL_UI_ENABLED
