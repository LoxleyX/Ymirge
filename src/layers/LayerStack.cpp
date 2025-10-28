#include "LayerStack.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

LayerStack::LayerStack(int width, int height)
    : activeLayerIndex_(0)
    , width_(width)
    , height_(height) {

    // Create default base layer
    auto baseLayer = std::make_unique<TerrainLayer>("Base Terrain", LayerType::PROCEDURAL, width, height);
    layers_.push_back(std::move(baseLayer));
}

void LayerStack::addLayer(std::unique_ptr<LayerBase> layer) {
    if (layer->getWidth() != width_ || layer->getHeight() != height_) {
        throw std::runtime_error("Layer dimensions must match stack dimensions");
    }

    layers_.push_back(std::move(layer));
    activeLayerIndex_ = layers_.size() - 1;  // Make new layer active
}

void LayerStack::insertLayer(size_t index, std::unique_ptr<LayerBase> layer) {
    if (layer->getWidth() != width_ || layer->getHeight() != height_) {
        throw std::runtime_error("Layer dimensions must match stack dimensions");
    }

    if (index > layers_.size()) {
        index = layers_.size();
    }

    layers_.insert(layers_.begin() + index, std::move(layer));
    activeLayerIndex_ = index;  // Make inserted layer active
}

void LayerStack::removeLayer(size_t index) {
    if (index >= layers_.size()) {
        throw std::out_of_range("Layer index out of range");
    }

    if (layers_.size() == 1) {
        throw std::runtime_error("Cannot remove last layer");
    }

    layers_.erase(layers_.begin() + index);

    // Adjust active layer index
    if (activeLayerIndex_ >= layers_.size()) {
        activeLayerIndex_ = layers_.size() - 1;
    }
}

std::unique_ptr<LayerBase> LayerStack::removeAndReturnLayer(size_t index) {
    if (index >= layers_.size()) {
        throw std::out_of_range("Layer index out of range");
    }

    if (layers_.size() == 1) {
        throw std::runtime_error("Cannot remove last layer");
    }

    auto layer = std::move(layers_[index]);
    layers_.erase(layers_.begin() + index);

    // Adjust active layer index
    if (activeLayerIndex_ >= layers_.size()) {
        activeLayerIndex_ = layers_.size() - 1;
    }

    return layer;
}

void LayerStack::moveLayer(size_t fromIndex, size_t toIndex) {
    if (fromIndex >= layers_.size() || toIndex >= layers_.size()) {
        throw std::out_of_range("Layer index out of range");
    }

    if (fromIndex == toIndex) {
        return;
    }

    auto layer = std::move(layers_[fromIndex]);
    layers_.erase(layers_.begin() + fromIndex);
    layers_.insert(layers_.begin() + toIndex, std::move(layer));

    activeLayerIndex_ = toIndex;
}

std::unique_ptr<LayerBase> LayerStack::duplicateLayer(size_t index) {
    if (index >= layers_.size()) {
        throw std::out_of_range("Layer index out of range");
    }

    const LayerBase* original = layers_[index].get();

    // Only support duplicating TerrainLayer (not groups yet)
    if (original->isGroup()) {
        throw std::runtime_error("Cannot duplicate groups (not yet implemented)");
    }

    const TerrainLayer* terrainLayer = dynamic_cast<const TerrainLayer*>(original);
    if (!terrainLayer) {
        throw std::runtime_error("Invalid layer type");
    }

    auto duplicate = std::make_unique<TerrainLayer>(
        terrainLayer->getName() + " Copy",
        terrainLayer->getType(),
        width_,
        height_
    );

    // Copy heightmap data
    duplicate->getHeightMap() = terrainLayer->getHeightMap();

    // Copy properties
    duplicate->setBlendMode(terrainLayer->getBlendMode());
    duplicate->setOpacity(terrainLayer->getOpacity());
    duplicate->setVisible(terrainLayer->isVisible());
    duplicate->setLocked(terrainLayer->isLocked());

    // Copy mask if present
    if (terrainLayer->hasMask()) {
        duplicate->createMask();
        duplicate->getMask() = terrainLayer->getMask();
    }

    return duplicate;
}

void LayerStack::mergeDown(size_t index) {
    if (index == 0 || index >= layers_.size()) {
        throw std::out_of_range("Cannot merge bottom layer or invalid index");
    }

    LayerBase* upper = layers_[index].get();

    // Cannot merge groups (not implemented yet)
    if (upper->isGroup()) {
        throw std::runtime_error("Cannot merge groups (not yet implemented)");
    }

    TerrainLayer* upperLayer = dynamic_cast<TerrainLayer*>(upper);
    if (!upperLayer) {
        throw std::runtime_error("Invalid layer type");
    }

    if (!upperLayer->isVisible()) {
        // Just remove the invisible layer
        removeLayer(index);
        return;
    }

    // Lower layer must also be a TerrainLayer (not a group)
    LayerBase* lower = layers_[index - 1].get();
    if (lower->isGroup()) {
        throw std::runtime_error("Cannot merge into a group");
    }

    TerrainLayer* lowerLayer = dynamic_cast<TerrainLayer*>(lower);
    if (!lowerLayer) {
        throw std::runtime_error("Invalid layer type");
    }

    // Composite upper onto lower
    HeightMap tempBelow = lowerLayer->getHeightMap();
    HeightMap tempOutput(width_, height_);

    upperLayer->composite(tempOutput, tempBelow);

    // Copy result to lower layer
    lowerLayer->getHeightMap() = tempOutput;

    // Remove upper layer
    removeLayer(index);
}

void LayerStack::clear() {
    layers_.clear();
    activeLayerIndex_ = 0;
}

LayerBase* LayerStack::getLayer(size_t index) {
    if (index >= layers_.size()) {
        return nullptr;
    }
    return layers_[index].get();
}

const LayerBase* LayerStack::getLayer(size_t index) const {
    if (index >= layers_.size()) {
        return nullptr;
    }
    return layers_[index].get();
}

TerrainLayer* LayerStack::getLayerAsTerrainLayer(size_t index) {
    LayerBase* layer = getLayer(index);
    if (!layer || layer->isGroup()) {
        return nullptr;
    }
    return dynamic_cast<TerrainLayer*>(layer);
}

const TerrainLayer* LayerStack::getLayerAsTerrainLayer(size_t index) const {
    const LayerBase* layer = getLayer(index);
    if (!layer || layer->isGroup()) {
        return nullptr;
    }
    return dynamic_cast<const TerrainLayer*>(layer);
}

void LayerStack::setActiveLayerIndex(size_t index) {
    if (index >= layers_.size()) {
        throw std::out_of_range("Layer index out of range");
    }
    activeLayerIndex_ = index;
}

LayerBase* LayerStack::getActiveLayer() {
    return getLayer(activeLayerIndex_);
}

const LayerBase* LayerStack::getActiveLayer() const {
    return getLayer(activeLayerIndex_);
}

TerrainLayer* LayerStack::getActiveTerrainLayer() {
    return getLayerAsTerrainLayer(activeLayerIndex_);
}

const TerrainLayer* LayerStack::getActiveTerrainLayer() const {
    return getLayerAsTerrainLayer(activeLayerIndex_);
}

void LayerStack::composite(HeightMap& output) {
    // Ensure output is correct size
    if (output.getWidth() != width_ || output.getHeight() != height_) {
        throw std::runtime_error("Output heightmap dimensions must match stack dimensions");
    }

    // Start with empty heightmap
    if (layers_.empty()) {
        output.clear();
        return;
    }

    // Use the new composite() method from LayerBase
    // Start with base layer (clear output first)
    output.clear();

    HeightMap below(width_, height_);
    below.clear();

    for (size_t i = 0; i < layers_.size(); i++) {
        LayerBase* layer = layers_[i].get();

        if (!layer->isVisible()) {
            continue;  // Skip invisible layers
        }

        HeightMap temp(width_, height_);
        layer->composite(temp, below);

        // Result becomes the new "below" for next layer
        below = temp;
    }

    // Final result
    output = below;
}
