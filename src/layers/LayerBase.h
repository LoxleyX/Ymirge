#pragma once

#include "HeightMap.h"
#include <string>
#include <memory>

enum class LayerType {
    PROCEDURAL,
    SCULPT,
    STAMP,
    ADJUSTMENT,
    GROUP
};

enum class BlendMode {
    NORMAL,
    ADD,
    SUBTRACT,
    MULTIPLY,
    SCREEN,
    MAX,
    MIN,
    OVERLAY
};

class LayerBase {
public:
    virtual ~LayerBase() = default;

    virtual LayerType getType() const = 0;
    virtual bool isGroup() const = 0;

    virtual std::string getName() const { return name_; }
    virtual void setName(const std::string& name) { name_ = name; }

    virtual BlendMode getBlendMode() const { return blendMode_; }
    virtual void setBlendMode(BlendMode mode) { blendMode_ = mode; }

    virtual float getOpacity() const { return opacity_; }
    virtual void setOpacity(float opacity);

    virtual bool isVisible() const { return visible_; }
    virtual void setVisible(bool visible) { visible_ = visible; }

    virtual bool isLocked() const { return locked_; }
    virtual void setLocked(bool locked) { locked_ = locked; }

    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;

    virtual void composite(HeightMap& output, const HeightMap& below) = 0;

protected:
    std::string name_;
    BlendMode blendMode_ = BlendMode::NORMAL;
    float opacity_ = 1.0f;
    bool visible_ = true;
    bool locked_ = false;
};

inline const char* layerTypeToString(LayerType type) {
    switch (type) {
        case LayerType::PROCEDURAL: return "Procedural";
        case LayerType::SCULPT:     return "Sculpt";
        case LayerType::STAMP:      return "Stamp";
        case LayerType::ADJUSTMENT: return "Adjustment";
        case LayerType::GROUP:      return "Group";
        default:                    return "Unknown";
    }
}

/**
 * Convert blend mode to string
 */
inline const char* blendModeToString(BlendMode mode) {
    switch (mode) {
        case BlendMode::NORMAL:   return "Normal";
        case BlendMode::ADD:      return "Add";
        case BlendMode::SUBTRACT: return "Subtract";
        case BlendMode::MULTIPLY: return "Multiply";
        case BlendMode::SCREEN:   return "Screen";
        case BlendMode::MAX:      return "Max";
        case BlendMode::MIN:      return "Min";
        case BlendMode::OVERLAY:  return "Overlay";
        default:                  return "Unknown";
    }
}
