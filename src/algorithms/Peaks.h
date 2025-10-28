#pragma once

#include "HeightMap.h"
#include "PerlinNoise.h"
#include "ThreadPool.h"

// Sharp mountain peaks using ridged noise with gradual slopes
class Peaks {
public:
    static void execute(HeightMap& map,
                       float intensity,
                       uint32_t seed,
                       ThreadPool* pool);

private:
    static float ridgedNoise(const PerlinNoise& noise, float x, float y);
};
