#pragma once

#include "BrushTool.h"

// Flattens terrain to target height sampled from first click
class FlattenBrush : public BrushTool {
public:
    void setTargetHeight(float height) {
        targetHeight_ = std::max(0.0f, std::min(height, 1.0f));
    }

    float getTargetHeight() const {
        return targetHeight_;
    }

    void apply(HeightMap& map, int centerX, int centerY, float deltaTime) override {
        int width = map.getWidth();
        int height = map.getHeight();

        for (int y = centerY - radius_; y <= centerY + radius_; ++y) {
            for (int x = centerX - radius_; x <= centerX + radius_; ++x) {
                if (x < 0 || x >= width || y < 0 || y >= height) {
                    continue;
                }

                int dx = x - centerX;
                int dy = y - centerY;
                float weight = calculateFalloff(dx, dy);

                if (weight <= 0.0f) {
                    continue;
                }

                float& pixel = map.at(x, y);
                float blendFactor = strength_ * weight * deltaTime * 3.0f;
                blendFactor = std::min(1.0f, blendFactor);
                pixel = pixel * (1.0f - blendFactor) + targetHeight_ * blendFactor;
            }
        }
    }

    const char* getName() const override {
        return "Flatten";
    }

private:
    float targetHeight_ = 0.5f;
};
