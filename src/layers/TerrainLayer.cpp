#include "TerrainLayer.h"
#include <algorithm>
#include <cmath>

TerrainLayer::TerrainLayer(const std::string& name, LayerType type, int width, int height)
    : type_(type)
    , heightMap_(width, height)
    , mask_(width, height)
    , hasMask_(false) {

    // Initialize base class members
    name_ = name;
    blendMode_ = BlendMode::NORMAL;
    opacity_ = 1.0f;
    visible_ = true;
    locked_ = false;

    // Initialize heightmap to zero (empty layer)
    heightMap_.clear();

    // Initialize mask to white (full effect) but mark as not having a mask yet
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            mask_.at(x, y) = 1.0f;
        }
    }
}

void TerrainLayer::composite(HeightMap& output, const HeightMap& below) {
    if (!visible_ || opacity_ < 0.01f) {
        // Layer not visible, just copy below
        output = below;
        return;
    }

    int width = heightMap_.getWidth();
    int height = heightMap_.getHeight();

    // Apply blend mode
    applyBlendMode(output, below, blendMode_, opacity_);
}

void TerrainLayer::applyBlendMode(HeightMap& output, const HeightMap& below, BlendMode mode, float opacity) {
    int width = heightMap_.getWidth();
    int height = heightMap_.getHeight();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float belowValue = below.at(x, y);
            float layerValue = heightMap_.at(x, y);
            float maskValue = hasMask_ ? mask_.at(x, y) : 1.0f;

            float blended = belowValue;

            switch (mode) {
                case BlendMode::NORMAL:
                    blended = belowValue + (layerValue - belowValue) * opacity * maskValue;
                    break;

                case BlendMode::ADD:
                    blended = belowValue + layerValue * opacity * maskValue;
                    break;

                case BlendMode::SUBTRACT:
                    blended = belowValue - layerValue * opacity * maskValue;
                    break;

                case BlendMode::MULTIPLY:
                    blended = belowValue * (1.0f + (layerValue - 1.0f) * opacity * maskValue);
                    break;

                case BlendMode::SCREEN:
                    {
                        float invLayer = 1.0f - layerValue * opacity * maskValue;
                        float invBelow = 1.0f - belowValue;
                        blended = 1.0f - (invBelow * invLayer);
                    }
                    break;

                case BlendMode::MAX:
                    {
                        float maxVal = std::max(belowValue, layerValue);
                        blended = belowValue + (maxVal - belowValue) * opacity * maskValue;
                    }
                    break;

                case BlendMode::MIN:
                    {
                        float minVal = std::min(belowValue, layerValue);
                        blended = belowValue + (minVal - belowValue) * opacity * maskValue;
                    }
                    break;

                case BlendMode::OVERLAY:
                    {
                        float result;
                        if (belowValue < 0.5f) {
                            result = 2.0f * belowValue * layerValue;
                        } else {
                            result = 1.0f - 2.0f * (1.0f - belowValue) * (1.0f - layerValue);
                        }
                        blended = belowValue + (result - belowValue) * opacity * maskValue;
                    }
                    break;
            }

            output.at(x, y) = blended;
        }
    }
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
