#pragma once

#ifdef YMIRGE_SDL_UI_ENABLED

#include "HeightMap.h"
#include <memory>

class ComputeShader;
class GPUBuffer;

class GaussianBlurGPU {
public:
    GaussianBlurGPU();
    ~GaussianBlurGPU();

    // Apply Gaussian blur to heightmap
    void blur(HeightMap& map, int radius, float sigma);

    // Check if GPU blur is available
    static bool isAvailable();

private:
    void computeGaussianWeights(int radius, float sigma);
    void blurPass(GPUBuffer& input, GPUBuffer& output, int width, int height, int radius, bool horizontal);

    std::unique_ptr<ComputeShader> shader_;
    float weights_[17];  // Max radius 8
    int currentRadius_;
    float currentSigma_;
};

#endif // YMIRGE_SDL_UI_ENABLED
