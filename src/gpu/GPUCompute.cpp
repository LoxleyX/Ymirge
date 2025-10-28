#ifdef YMIRGE_SDL_UI_ENABLED

#include "GPUCompute.h"
#include "HeightMap.h"
#include <iostream>

bool GPUCompute::initialized_ = false;
bool GPUCompute::available_ = false;

bool GPUCompute::isAvailable() {
    if (!initialized_) {
        return false;
    }
    return available_;
}

bool GPUCompute::initialize() {
    if (initialized_) {
        return available_;
    }

    // Check OpenGL version (need 4.3+ for compute shaders)
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    if (major < 4 || (major == 4 && minor < 3)) {
        std::cerr << "GPU Compute unavailable: OpenGL 4.3+ required (have "
                  << major << "." << minor << ")" << std::endl;
        initialized_ = true;
        available_ = false;
        return false;
    }

    // Check compute shader support
    GLint computeSupport = 0;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &computeSupport);
    if (computeSupport == 0) {
        std::cerr << "GPU Compute unavailable: No compute shader support" << std::endl;
        initialized_ = true;
        available_ = false;
        return false;
    }

    // Check SSBO support
    GLint maxSSBOBindings = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxSSBOBindings);
    if (maxSSBOBindings < 2) {
        std::cerr << "GPU Compute unavailable: Insufficient SSBO bindings" << std::endl;
        initialized_ = true;
        available_ = false;
        return false;
    }

    std::cout << "GPU Compute initialized successfully" << std::endl;
    std::cout << "  OpenGL Version: " << major << "." << minor << std::endl;
    std::cout << "  Max Work Group Invocations: " << computeSupport << std::endl;
    std::cout << "  Max SSBO Bindings: " << maxSSBOBindings << std::endl;

    initialized_ = true;
    available_ = true;
    return true;
}

void GPUCompute::shutdown() {
    initialized_ = false;
    available_ = false;
}

#endif // YMIRGE_SDL_UI_ENABLED
