#pragma once

#include "HeightMap.h"
#include "PerlinNoise.h"
#include "ThreadPool.h"
#include <vector>

// Three-pass island edge smoothing: distance map, aggressive smoothing, triple smoothstep
class EdgeSmoothing {
public:
    static void execute(HeightMap& map,
                       float edgePadding,
                       float islandShape,
                       uint32_t seed,
                       ThreadPool* pool);

private:
    static std::vector<float> calculateDistanceMap(
        const HeightMap& map,
        float islandShape,
        uint32_t seed);

    static void smoothEdges(
        HeightMap& map,
        const std::vector<float>& distanceMap,
        float edgePadding,
        int rounds,
        ThreadPool* pool);

    static void applyTripleSmoothstep(
        HeightMap& map,
        const std::vector<float>& distanceMap,
        float edgePadding,
        ThreadPool* pool);

    static float smoothstep(float t);
};
