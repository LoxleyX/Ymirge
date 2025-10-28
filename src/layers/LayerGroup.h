#pragma once

#include "LayerBase.h"
#include "TerrainLayer.h"
#include <vector>
#include <memory>
#include <string>

/**
 * Layer Group - Container for organizing layers hierarchically
 *
 * Groups can contain:
 * - Individual TerrainLayers
 * - Other LayerGroups (nested groups)
 *
 * Groups have their own blend mode and opacity, which applies to all children.
 */
class LayerGroup : public LayerBase {
public:
    /**
     * Create a new layer group
     *
     * @param name Group name
     * @param width Heightmap width (must match child layers)
     * @param height Heightmap height (must match child layers)
     */
    LayerGroup(const std::string& name, int width, int height);

    // LayerBase interface implementation
    LayerType getType() const override { return LayerType::GROUP; }
    bool isGroup() const override { return true; }
    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }

    void composite(HeightMap& output, const HeightMap& below) override;

    // Child management
    size_t getChildCount() const { return children_.size(); }
    LayerBase* getChild(size_t index);
    const LayerBase* getChild(size_t index) const;

    /**
     * Add a child layer or group
     * Ownership is transferred to this group
     */
    void addChild(std::unique_ptr<LayerBase> child);

    /**
     * Remove and return child at index
     */
    std::unique_ptr<LayerBase> removeChild(size_t index);

    /**
     * Move child from one index to another
     */
    void moveChild(size_t fromIndex, size_t toIndex);

    /**
     * Find index of child by pointer
     * Returns -1 if not found
     */
    int findChild(const LayerBase* child) const;

    // Group state
    bool isExpanded() const { return expanded_; }
    void setExpanded(bool expanded) { expanded_ = expanded; }

private:
    std::vector<std::unique_ptr<LayerBase>> children_;
    int width_;
    int height_;
    bool expanded_ = true;  // UI state: is group expanded in tree view
};
