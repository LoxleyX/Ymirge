#pragma once

#ifdef YMIRGE_SDL_UI_ENABLED

#include <glad/glad.h>
#include <string>

class HeightMap;
struct PerlinParams;
struct TerrainParams;

// GPU compute manager - handles OpenGL compute shader execution
class GPUCompute {
public:
    // Check if GPU compute is available on this system
    static bool isAvailable();

    // Initialize GPU compute system (call once at startup)
    static bool initialize();

    // Shutdown GPU compute system (call at exit)
    static void shutdown();

    // Check if initialized
    static bool isInitialized() { return initialized_; }

private:
    static bool initialized_;
    static bool available_;
};

#endif // YMIRGE_SDL_UI_ENABLED
