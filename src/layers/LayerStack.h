#pragma once

#include "LayerBase.h"
#include "TerrainLayer.h"
#include "LayerGroup.h"
#include "HeightMap.h"
#include <vector>
#include <memory>
#include <string>

/**
 * Layer stack manager
 *
 * Manages a hierarchical stack of layers and groups.
 * Supports both flat layers and nested groups.
 * Layers are stored bottom-to-top (layer 0 is base, higher indices are on top).
 */
class LayerStack {
public:
    /**
     * Create a new layer stack
     *
     * @param width Heightmap width (all layers must match this)
     * @param height Heightmap height (all layers must match this)
     */
    LayerStack(int width, int height);

    // Layer management
    /**
     * Add a new layer or group to the top of the stack
     */
    void addLayer(std::unique_ptr<LayerBase> layer);

    /**
     * Insert a layer or group at a specific index
     */
    void insertLayer(size_t index, std::unique_ptr<LayerBase> layer);

    /**
     * Remove a layer/group from the stack
     */
    void removeLayer(size_t index);

    /**
     * Remove a layer/group and return ownership (for undo)
     */
    std::unique_ptr<LayerBase> removeAndReturnLayer(size_t index);

    /**
     * Move a layer/group to a new position
     */
    void moveLayer(size_t fromIndex, size_t toIndex);

    /**
     * Duplicate a layer (groups not supported for duplication yet)
     */
    std::unique_ptr<LayerBase> duplicateLayer(size_t index);

    /**
     * Merge a layer down into the layer below it
     */
    void mergeDown(size_t index);

    /**
     * Clear all layers (for loading new projects)
     */
    void clear();

    // Layer access
    size_t getLayerCount() const { return layers_.size(); }

    LayerBase* getLayer(size_t index);
    const LayerBase* getLayer(size_t index) const;

    // Helper to get as TerrainLayer (returns nullptr if it's a group)
    TerrainLayer* getLayerAsTerrainLayer(size_t index);
    const TerrainLayer* getLayerAsTerrainLayer(size_t index) const;

    // Active layer (currently being edited)
    size_t getActiveLayerIndex() const { return activeLayerIndex_; }
    void setActiveLayerIndex(size_t index);
    LayerBase* getActiveLayer();
    const LayerBase* getActiveLayer() const;

    // Get active layer as TerrainLayer (returns nullptr if active is a group)
    TerrainLayer* getActiveTerrainLayer();
    const TerrainLayer* getActiveTerrainLayer() const;

    // Compositing
    /**
     * Composite all visible layers into output heightmap
     *
     * This recursively composites groups and layers.
     */
    void composite(HeightMap& output);

    // Dimensions
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    std::vector<std::unique_ptr<LayerBase>> layers_;
    size_t activeLayerIndex_;
    int width_, height_;
};
