#pragma once

#include "HeightMap.h"
#include <string>
#include <memory>

/**
 * Layer types
 */
enum class LayerType {
    PROCEDURAL,     // Generated from terrain parameters
    SCULPT,         // Manual brush edits
    STAMP,          // Imported heightmap or stamp
    ADJUSTMENT,     // Effects (blur, sharpen, levels)
    GROUP           // Container for other layers
};

/**
 * Blend modes for layer compositing
 *
 * Determines how this layer combines with layers below it
 */
enum class BlendMode {
    NORMAL,         // Alpha blend (default)
    ADD,            // Add height values
    SUBTRACT,       // Subtract height values
    MULTIPLY,       // Multiply height values
    SCREEN,         // Inverse multiply (brightening)
    MAX,            // Take maximum height
    MIN,            // Take minimum height
    OVERLAY         // Combination of multiply and screen
};

/**
 * Individual terrain layer
 *
 * Stores heightmap data, mask, and properties for non-destructive editing.
 * Similar to Photoshop layers or Gaea layers.
 */
class TerrainLayer {
public:
    /**
     * Create a new terrain layer
     *
     * @param name Layer name (displayed in UI)
     * @param type Layer type
     * @param width Heightmap width
     * @param height Heightmap height
     */
    TerrainLayer(const std::string& name, LayerType type, int width, int height);

    // Core data access
    std::string getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    LayerType getType() const { return type_; }

    HeightMap& getHeightMap() { return heightMap_; }
    const HeightMap& getHeightMap() const { return heightMap_; }

    HeightMap& getMask() { return mask_; }
    const HeightMap& getMask() const { return mask_; }

    // Properties
    BlendMode getBlendMode() const { return blendMode_; }
    void setBlendMode(BlendMode mode) { blendMode_ = mode; }

    float getOpacity() const { return opacity_; }
    void setOpacity(float opacity);

    bool isVisible() const { return visible_; }
    void setVisible(bool visible) { visible_ = visible; }

    bool isLocked() const { return locked_; }
    void setLocked(bool locked) { locked_ = locked; }

    // Mask operations
    bool hasMask() const { return hasMask_; }
    void createMask();
    void deleteMask();
    void invertMask();

    // Utility
    int getWidth() const { return heightMap_.getWidth(); }
    int getHeight() const { return heightMap_.getHeight(); }

private:
    std::string name_;
    LayerType type_;
    HeightMap heightMap_;
    HeightMap mask_;              // Optional layer mask (white = full effect, black = no effect)
    BlendMode blendMode_;
    float opacity_;               // 0.0 to 1.0
    bool visible_;
    bool locked_;
    bool hasMask_;
};

/**
 * Convert layer type to string (for UI display)
 */
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
 * Convert blend mode to string (for UI display)
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
