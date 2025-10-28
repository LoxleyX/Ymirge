#include "TerrainLayer.h"
#include <algorithm>
#include <cmath>

TerrainLayer::TerrainLayer(const std::string& name, LayerType type, int width, int height)
    : name_(name)
    , type_(type)
    , heightMap_(width, height)
    , mask_(width, height)
    , blendMode_(BlendMode::NORMAL)
    , opacity_(1.0f)
    , visible_(true)
    , locked_(false)
    , hasMask_(false) {

    // Initialize heightmap to zero (empty layer)
    heightMap_.clear();

    // Initialize mask to white (full effect) but mark as not having a mask yet
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            mask_.at(x, y) = 1.0f;
        }
    }
}

void TerrainLayer::setOpacity(float opacity) {
    opacity_ = std::clamp(opacity, 0.0f, 1.0f);
}

void TerrainLayer::createMask() {
    if (hasMask_) {
        return;  // Already has a mask
    }

    // Initialize mask to white (full effect everywhere)
    int width = heightMap_.getWidth();
    int height = heightMap_.getHeight();

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            mask_.at(x, y) = 1.0f;
        }
    }

    hasMask_ = true;
}

void TerrainLayer::deleteMask() {
    if (!hasMask_) {
        return;  // No mask to delete
    }

    // Reset mask to white (so it doesn't affect compositing if accidentally used)
    int width = heightMap_.getWidth();
    int height = heightMap_.getHeight();

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            mask_.at(x, y) = 1.0f;
        }
    }

    hasMask_ = false;
}

void TerrainLayer::invertMask() {
    if (!hasMask_) {
        return;  // No mask to invert
    }

    int width = heightMap_.getWidth();
    int height = heightMap_.getHeight();

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            mask_.at(x, y) = 1.0f - mask_.at(x, y);
        }
    }
}
