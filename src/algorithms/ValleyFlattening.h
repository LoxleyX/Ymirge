#pragma once

#include "HeightMap.h"
#include "ThreadPool.h"
#include <unordered_map>
#include <vector>

// Three-pass valley flattening: detect floors, extreme flattening (85-100%), smooth transitions
class ValleyFlattening {
public:
    static void execute(HeightMap& map, float strength, ThreadPool* pool);

private:
    static std::unordered_map<int, float> detectValleyFloors(
        const HeightMap& map, float strength);

    static void applyFlattening(
        HeightMap& map,
        const std::unordered_map<int, float>& valleyFloors,
        float strength,
        ThreadPool* pool);

    static void smoothTransitions(
        HeightMap& map,
        const std::unordered_map<int, float>& valleyFloors,
        float strength,
        int rounds,
        ThreadPool* pool);

    static float findBoundaryDistance(
        const HeightMap& map,
        const std::unordered_map<int, float>& valleyFloors,
        int x, int y,
        int searchRadius);

    static float calculateThreshold(const HeightMap& map, float strength);
};
