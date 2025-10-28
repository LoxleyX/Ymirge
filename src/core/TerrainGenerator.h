#pragma once

#include "HeightMap.h"
#include "TerrainParams.h"
#include "PerlinNoise.h"
#include "ThreadPool.h"
#include <memory>
#include <future>
#include <atomic>
#include <mutex>

class TerrainGenerator {
public:
    TerrainGenerator(int width, int height, ThreadPool* threadPool);

    void generate(const TerrainParams& params);
    std::future<void> generateAsync(const TerrainParams& params);

    bool isGenerating() const { return generating_.load(); }

    const HeightMap& getHeightMap() const;
    HeightMap getHeightMapCopy() const;

    // For direct editing (brush tools). Use generate() for procedural generation.
    HeightMap& getHeightMapMutable() { return heightMap_; }

    void setHeightMap(const HeightMap& newMap) {
        std::lock_guard<std::mutex> lock(heightMapMutex_);
        heightMap_ = newMap;
        width_ = newMap.getWidth();
        height_ = newMap.getHeight();
        workBuffer_ = HeightMap(width_, height_);
    }

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    void generateBaseNoise(const TerrainParams& params);
    void applyValleys(const TerrainParams& params);
    void smoothValleyFloors(const TerrainParams& params);
    void applyErosion(const TerrainParams& params);
    void applyPeaks(const TerrainParams& params);
    void applyIslandMask(const TerrainParams& params);
    void applyTerracing(const TerrainParams& params);
    void applyEdgePadding(const TerrainParams& params);
    void flattenLowAreas(const TerrainParams& params);
    void connectValleys(const TerrainParams& params);
    void applyRivers(const TerrainParams& params);

    int width_, height_;
    HeightMap heightMap_;
    HeightMap workBuffer_;  // Scratch buffer for multi-pass operations

    ThreadPool* threadPool_;
    std::atomic<bool> generating_;
    mutable std::mutex heightMapMutex_;

    std::unique_ptr<PerlinNoise> perlin_;
};
