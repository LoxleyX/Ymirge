#pragma once

#include "HeightMap.h"
#include "ThreadPool.h"

class TerrainSoftening {
public:
    static void execute(HeightMap& map, float strength, float threshold,
                       int smoothRadius, int passes, ThreadPool* pool);

private:
    static float calculateElevationThreshold(const HeightMap& map, float threshold);
    static void applySmoothingPass(HeightMap& map, float elevationThreshold,
                                   float strength, int smoothRadius, ThreadPool* pool);
};
