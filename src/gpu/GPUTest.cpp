#ifdef YMIRGE_SDL_UI_ENABLED

#include "GPUTest.h"
#include "GPUCompute.h"
#include "ComputeShader.h"
#include "GPUBuffer.h"
#include <iostream>
#include <cmath>

namespace GPUTest {

bool testAddValue(HeightMap& map, float addValue) {
    if (!GPUCompute::isAvailable()) {
        std::cerr << "GPU compute not available for test" << std::endl;
        return false;
    }

    try {
        // Create buffer with heightmap data
        size_t dataSize = map.getWidth() * map.getHeight() * sizeof(float);
        GPUBuffer buffer(dataSize, map.getData());

        // Load and compile test shader
        ComputeShader shader("gpu_shaders/test.comp");
        if (!shader.isValid()) {
            std::cerr << "Failed to load test shader" << std::endl;
            return false;
        }

        // Bind shader and set uniforms
        shader.bind();
        shader.setUniform("width", map.getWidth());
        shader.setUniform("height", map.getHeight());
        shader.setUniform("addValue", addValue);

        // Bind buffer
        buffer.bind(0);

        // Dispatch compute shader (16x16 work groups)
        int groupsX = (map.getWidth() + 15) / 16;
        int groupsY = (map.getHeight() + 15) / 16;
        shader.dispatch(groupsX, groupsY);

        // Read back results
        buffer.download(map.getData(), dataSize);

        std::cout << "GPU test completed successfully" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "GPU test failed: " << e.what() << std::endl;
        return false;
    }
}

bool verifyResults(const HeightMap& result, const HeightMap& expected, float tolerance) {
    if (result.getWidth() != expected.getWidth() ||
        result.getHeight() != expected.getHeight()) {
        std::cerr << "Size mismatch" << std::endl;
        return false;
    }

    int errors = 0;
    float maxError = 0.0f;

    for (int y = 0; y < result.getHeight(); y++) {
        for (int x = 0; x < result.getWidth(); x++) {
            float r = result.sample(x, y);
            float e = expected.sample(x, y);
            float error = std::abs(r - e);

            if (error > tolerance) {
                errors++;
                maxError = std::max(maxError, error);
            }
        }
    }

    if (errors > 0) {
        std::cerr << "Verification failed: " << errors << " errors, max error: " << maxError << std::endl;
        return false;
    }

    std::cout << "Verification passed (tolerance: " << tolerance << ")" << std::endl;
    return true;
}

} // namespace GPUTest

#endif // YMIRGE_SDL_UI_ENABLED
