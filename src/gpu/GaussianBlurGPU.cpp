#ifdef YMIRGE_SDL_UI_ENABLED

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include "GaussianBlurGPU.h"
#include "GPUCompute.h"
#include "ComputeShader.h"
#include "GPUBuffer.h"
#include <cmath>
#include <algorithm>
#include <iostream>

GaussianBlurGPU::GaussianBlurGPU()
    : currentRadius_(0)
    , currentSigma_(0.0f) {
}

GaussianBlurGPU::~GaussianBlurGPU() = default;

bool GaussianBlurGPU::isAvailable() {
    return GPUCompute::isAvailable();
}

void GaussianBlurGPU::computeGaussianWeights(int radius, float sigma) {
    // Only recompute if parameters changed
    if (radius == currentRadius_ && sigma == currentSigma_) {
        return;
    }

    currentRadius_ = radius;
    currentSigma_ = sigma;

    // Compute Gaussian kernel
    float sum = 0.0f;
    for (int i = -radius; i <= radius; i++) {
        float x = static_cast<float>(i);
        float weight = std::exp(-(x * x) / (2.0f * sigma * sigma));
        weights_[i + radius] = weight;
        sum += weight;
    }

    // Normalize weights
    for (int i = 0; i <= 2 * radius; i++) {
        weights_[i] /= sum;
    }
}

void GaussianBlurGPU::blurPass(GPUBuffer& input, GPUBuffer& output, int width, int height, int radius, bool horizontal) {
    if (!shader_) {
        shader_ = std::make_unique<ComputeShader>("gpu_shaders/gaussian_blur.comp");
        if (!shader_->isValid()) {
            std::cerr << "Failed to load Gaussian blur shader" << std::endl;
            return;
        }
    }

    shader_->bind();
    shader_->setUniform("width", width);
    shader_->setUniform("height", height);
    shader_->setUniform("radius", radius);
    shader_->setUniform("horizontal", horizontal ? 1 : 0);

    // Upload weights
    for (int i = 0; i <= 2 * radius; i++) {
        std::string uniformName = "weights[" + std::to_string(i) + "]";
        shader_->setUniform(uniformName, weights_[i]);
    }

    input.bind(0);
    output.bind(1);

    // Dispatch
    int totalPixels = width * height;
    int groupsX = (totalPixels + 255) / 256;
    shader_->dispatch(groupsX, 1, 1);
}

void GaussianBlurGPU::blur(HeightMap& map, int radius, float sigma) {
    if (!isAvailable()) {
        std::cerr << "GPU Gaussian blur not available" << std::endl;
        return;
    }

    // Clamp radius
    radius = (std::min)(radius, 8);
    if (radius <= 0) return;

    try {
        // Compute Gaussian weights
        computeGaussianWeights(radius, sigma);

        size_t dataSize = map.getWidth() * map.getHeight() * sizeof(float);

        // Create buffers
        GPUBuffer buffer1(dataSize, map.getData());
        GPUBuffer buffer2(dataSize);

        // Horizontal pass (buffer1 -> buffer2)
        blurPass(buffer1, buffer2, map.getWidth(), map.getHeight(), radius, true);

        // Vertical pass (buffer2 -> buffer1)
        blurPass(buffer2, buffer1, map.getWidth(), map.getHeight(), radius, false);

        // Download result
        buffer1.download(map.getData(), dataSize);

    } catch (const std::exception& e) {
        std::cerr << "GPU Gaussian blur failed: " << e.what() << std::endl;
    }
}

#endif // YMIRGE_SDL_UI_ENABLED
