#pragma once

#include "BrushTool.h"

class LowerBrush : public BrushTool {
public:
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
                float delta = strength_ * weight * deltaTime * 2.0f;
                pixel = std::max(0.0f, pixel - delta);
            }
        }
    }

    const char* getName() const override {
        return "Lower";
    }
};
