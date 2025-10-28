#pragma once

#include "HeightMap.h"
#include <cmath>

// Base class for terrain sculpting brushes with radius, falloff, and strength
class BrushTool {
public:
    enum class FalloffType {
        LINEAR,
        SMOOTH,
        CONSTANT
    };

    virtual ~BrushTool() = default;

    virtual void apply(HeightMap& map, int centerX, int centerY, float deltaTime) = 0;
    virtual const char* getName() const = 0;

    void setRadius(int radius) {
        radius_ = std::max(1, std::min(radius, 100));
    }

    void setStrength(float strength) {
        strength_ = std::max(0.0f, std::min(strength, 1.0f));
    }

    void setFalloff(FalloffType falloff) {
        falloff_ = falloff;
    }

    int getRadius() const { return radius_; }
    float getStrength() const { return strength_; }
    FalloffType getFalloff() const { return falloff_; }

protected:
    float calculateFalloff(int dx, int dy) const {
        float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
        float radius = static_cast<float>(radius_);

        if (distance > radius) {
            return 0.0f;
        }

        float t = distance / radius;

        switch (falloff_) {
            case FalloffType::LINEAR:
                return 1.0f - t;

            case FalloffType::SMOOTH:
                return 1.0f - (t * t * (3.0f - 2.0f * t));

            case FalloffType::CONSTANT:
                return 1.0f;

            default:
                return 1.0f - t;
        }
    }

    int radius_ = 10;
    float strength_ = 0.5f;
    FalloffType falloff_ = FalloffType::SMOOTH;
};
