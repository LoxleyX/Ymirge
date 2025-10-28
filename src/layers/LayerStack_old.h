#pragma once

#include "TerrainLayer.h"
#include "HeightMap.h"
#include <vector>
#include <memory>
#include <string>

/**
 * Layer stack manager
 *
 * Manages a stack of terrain layers and handles compositing them together.
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
     * Add a new layer to the top of the stack
     */
    void addLayer(std::unique_ptr<TerrainLayer> layer);

    /**
     * Insert a layer at a specific index
     *
     * @param index Position to insert (0 = bottom, getLayerCount() = top)
     * @param layer Layer to insert
     */
    void insertLayer(size_t index, std::unique_ptr<TerrainLayer> layer);

    /**
     * Remove a layer from the stack
     *
     * @param index Layer index to remove
     */
    void removeLayer(size_t index);

    /**
     * Remove a layer and return ownership (for undo)
     *
     * @param index Layer index to remove
     * @return Removed layer (ownership transferred)
     */
    std::unique_ptr<TerrainLayer> removeAndReturnLayer(size_t index);

    /**
     * Move a layer to a new position
     *
     * @param fromIndex Current position
     * @param toIndex New position
     */
    void moveLayer(size_t fromIndex, size_t toIndex);

    /**
     * Duplicate a layer
     *
     * @param index Layer to duplicate
     * @return New layer (ownership transferred)
     */
    std::unique_ptr<TerrainLayer> duplicateLayer(size_t index);

    /**
     * Merge a layer down into the layer below it
     *
     * @param index Layer to merge (must not be 0)
     */
    void mergeDown(size_t index);

    /**
     * Clear all layers (for loading new projects)
     */
    void clear();

    // Layer access
    size_t getLayerCount() const { return layers_.size(); }

    TerrainLayer* getLayer(size_t index);
    const TerrainLayer* getLayer(size_t index) const;

    // Active layer (currently being edited)
    size_t getActiveLayerIndex() const { return activeLayerIndex_; }
    void setActiveLayerIndex(size_t index);
    TerrainLayer* getActiveLayer();
    const TerrainLayer* getActiveLayer() const;

    // Compositing
    /**
     * Composite all visible layers into output heightmap
     *
     * This is the main rendering function that combines all layers
     * according to their blend modes, opacity, and masks.
     *
     * @param output Destination heightmap (will be cleared and filled)
     */
    void composite(HeightMap& output);

    // Dimensions
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    std::vector<std::unique_ptr<TerrainLayer>> layers_;
    size_t activeLayerIndex_;
    int width_, height_;

    /**
     * Apply blend mode to combine layer with base
     *
     * @param result Base heightmap (modified in place)
     * @param layer Layer heightmap to blend
     * @param mode Blend mode to use
     * @param opacity Layer opacity (0-1)
     * @param mask Optional layer mask (nullptr if no mask)
     */
    void applyBlendMode(HeightMap& result, const HeightMap& layer,
                        BlendMode mode, float opacity, const HeightMap* mask);

    /**
     * Blend two height values using specified mode
     *
     * @param base Base height value
     * @param layer Layer height value
     * @param mode Blend mode
     * @param opacity Effective opacity (includes mask)
     * @return Blended result
     */
    float blendPixel(float base, float layer, BlendMode mode, float opacity);
};
