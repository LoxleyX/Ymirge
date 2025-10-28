#pragma once

#ifdef YMIRGE_SDL_UI_ENABLED

#include "HeightMap.h"

// GPU compute test utilities
namespace GPUTest {
    // Simple test: add a value to all heights
    bool testAddValue(HeightMap& map, float addValue);

    // Verify GPU computation matches expected result
    bool verifyResults(const HeightMap& result, const HeightMap& expected, float tolerance = 0.001f);
}

#endif // YMIRGE_SDL_UI_ENABLED
