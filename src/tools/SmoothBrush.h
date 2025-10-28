#pragma once

#include "BrushTool.h"

// Smooths terrain using 3x3 kernel averaging
class SmoothBrush : public BrushTool {
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

                float sum = 0.0f;
                int count = 0;

                for (int ny = -1; ny <= 1; ++ny) {
                    for (int nx = -1; nx <= 1; ++nx) {
                        int sx = x + nx;
                        int sy = y + ny;

                        if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
                            sum += map.at(sx, sy);
                            count++;
                        }
                    }
                }

                float average = (count > 0) ? (sum / count) : map.at(x, y);

                float& pixel = map.at(x, y);
                float blendFactor = strength_ * weight * deltaTime * 5.0f;
                blendFactor = std::min(1.0f, blendFactor);
                pixel = pixel * (1.0f - blendFactor) + average * blendFactor;
            }
        }
    }

    const char* getName() const override {
        return "Smooth";
    }
};
