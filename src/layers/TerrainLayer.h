#pragma once

#include "LayerBase.h"
#include "HeightMap.h"
#include <string>
#include <memory>

/**
 * Individual terrain layer
 *
 * Stores heightmap data, mask, and properties for non-destructive editing.
 * Similar to Photoshop layers or Gaea layers.
 */
class TerrainLayer : public LayerBase {
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

    // LayerBase interface implementation
    LayerType getType() const override { return type_; }
    bool isGroup() const override { return false; }
    int getWidth() const override { return heightMap_.getWidth(); }
    int getHeight() const override { return heightMap_.getHeight(); }

    void composite(HeightMap& output, const HeightMap& below) override;

    // Layer-specific data access
    HeightMap& getHeightMap() { return heightMap_; }
    const HeightMap& getHeightMap() const { return heightMap_; }

    HeightMap& getMask() { return mask_; }
    const HeightMap& getMask() const { return mask_; }

    // Mask operations
    bool hasMask() const { return hasMask_; }
    void createMask();
    void deleteMask();
    void invertMask();

private:
    LayerType type_;
    HeightMap heightMap_;
    HeightMap mask_;              // Optional layer mask (white = full effect, black = no effect)
    bool hasMask_;

    // Helper for compositing with blend modes
    void applyBlendMode(HeightMap& output, const HeightMap& below, BlendMode mode, float opacity);
};
