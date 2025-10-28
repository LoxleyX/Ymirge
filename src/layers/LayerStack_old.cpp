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

void LayerStack::addLayer(std::unique_ptr<TerrainLayer> layer) {
    if (layer->getWidth() != width_ || layer->getHeight() != height_) {
        throw std::runtime_error("Layer dimensions must match stack dimensions");
    }

    layers_.push_back(std::move(layer));
    activeLayerIndex_ = layers_.size() - 1;  // Make new layer active
}

void LayerStack::insertLayer(size_t index, std::unique_ptr<TerrainLayer> layer) {
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

std::unique_ptr<TerrainLayer> LayerStack::duplicateLayer(size_t index) {
    if (index >= layers_.size()) {
        throw std::out_of_range("Layer index out of range");
    }

    const TerrainLayer* original = layers_[index].get();

    auto duplicate = std::make_unique<TerrainLayer>(
        original->getName() + " Copy",
        original->getType(),
        width_,
        height_
    );

    // Copy heightmap data
    duplicate->getHeightMap() = original->getHeightMap();

    // Copy properties
    duplicate->setBlendMode(original->getBlendMode());
    duplicate->setOpacity(original->getOpacity());
    duplicate->setVisible(original->isVisible());
    duplicate->setLocked(original->isLocked());

    // Copy mask if present
    if (original->hasMask()) {
        duplicate->createMask();
        duplicate->getMask() = original->getMask();
    }

    return duplicate;
}

void LayerStack::mergeDown(size_t index) {
    if (index == 0 || index >= layers_.size()) {
        throw std::out_of_range("Cannot merge bottom layer or invalid index");
    }

    TerrainLayer* upper = layers_[index].get();
    TerrainLayer* lower = layers_[index - 1].get();

    if (!upper->isVisible()) {
        // Just remove the invisible layer
        removeLayer(index);
        return;
    }

    // Composite upper layer onto lower layer
    HeightMap& lowerHeightMap = lower->getHeightMap();
    const HeightMap& upperHeightMap = upper->getHeightMap();

    const HeightMap* mask = upper->hasMask() ? &upper->getMask() : nullptr;

    applyBlendMode(lowerHeightMap, upperHeightMap, upper->getBlendMode(),
                   upper->getOpacity(), mask);

    // Remove upper layer
    removeLayer(index);
}

void LayerStack::clear() {
    layers_.clear();
    activeLayerIndex_ = 0;
}

TerrainLayer* LayerStack::getLayer(size_t index) {
    if (index >= layers_.size()) {
        return nullptr;
    }
    return layers_[index].get();
}

const TerrainLayer* LayerStack::getLayer(size_t index) const {
    if (index >= layers_.size()) {
        return nullptr;
    }
    return layers_[index].get();
}

void LayerStack::setActiveLayerIndex(size_t index) {
    if (index >= layers_.size()) {
        throw std::out_of_range("Layer index out of range");
    }
    activeLayerIndex_ = index;
}

TerrainLayer* LayerStack::getActiveLayer() {
    return getLayer(activeLayerIndex_);
}

const TerrainLayer* LayerStack::getActiveLayer() const {
    return getLayer(activeLayerIndex_);
}

void LayerStack::composite(HeightMap& output) {
    // Ensure output is correct size
    if (output.getWidth() != width_ || output.getHeight() != height_) {
        throw std::runtime_error("Output heightmap dimensions must match stack dimensions");
    }

    // Start with base layer (layer 0)
    if (layers_.empty()) {
        output.clear();
        return;
    }

    // Copy base layer to output
    output = layers_[0]->getHeightMap();

    // Composite each layer on top
    for (size_t i = 1; i < layers_.size(); i++) {
        const TerrainLayer* layer = layers_[i].get();

        if (!layer->isVisible()) {
            continue;  // Skip invisible layers
        }

        const HeightMap& layerHeightMap = layer->getHeightMap();
        const HeightMap* mask = layer->hasMask() ? &layer->getMask() : nullptr;

        applyBlendMode(output, layerHeightMap, layer->getBlendMode(),
                       layer->getOpacity(), mask);
    }
}

void LayerStack::applyBlendMode(HeightMap& result, const HeightMap& layer,
                                 BlendMode mode, float opacity, const HeightMap* mask) {
    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            float base = result.at(x, y);
            float layerValue = layer.at(x, y);

            // Apply mask if present (white = full effect, black = no effect)
            float effectiveOpacity = opacity;
            if (mask) {
                effectiveOpacity *= mask->at(x, y);
            }

            // Skip if no effect
            if (effectiveOpacity <= 0.0f) {
                continue;
            }

            // Apply blend mode
            float blended = blendPixel(base, layerValue, mode, effectiveOpacity);

            result.at(x, y) = std::clamp(blended, 0.0f, 1.0f);
        }
    }
}

float LayerStack::blendPixel(float base, float layer, BlendMode mode, float opacity) {
    float result;

    switch (mode) {
        case BlendMode::NORMAL:
            // Alpha blend
            result = base * (1.0f - opacity) + layer * opacity;
            break;

        case BlendMode::ADD:
            // Additive blending
            result = base + layer * opacity;
            break;

        case BlendMode::SUBTRACT:
            // Subtractive blending
            result = base - layer * opacity;
            break;

        case BlendMode::MULTIPLY:
            // Multiply (darkening)
            result = base * (1.0f - opacity) + (base * layer) * opacity;
            break;

        case BlendMode::SCREEN:
            // Screen (brightening) - inverse multiply
            {
                float inverted = 1.0f - ((1.0f - base) * (1.0f - layer));
                result = base * (1.0f - opacity) + inverted * opacity;
            }
            break;

        case BlendMode::MAX:
            // Take maximum
            result = std::max(base, layer * opacity);
            break;

        case BlendMode::MIN:
            // Take minimum
            result = std::min(base, layer * opacity);
            break;

        case BlendMode::OVERLAY:
            // Overlay - multiply for dark values, screen for bright values
            {
                float overlaid;
                if (base < 0.5f) {
                    overlaid = 2.0f * base * layer;
                } else {
                    overlaid = 1.0f - 2.0f * (1.0f - base) * (1.0f - layer);
                }
                result = base * (1.0f - opacity) + overlaid * opacity;
            }
            break;

        default:
            result = base;
            break;
    }

    return result;
}

std::unique_ptr<TerrainLayer> LayerStack::removeAndReturnLayer(size_t index) {
    if (index >= layers_.size()) {
        return nullptr;
    }

    // Cannot remove base layer (layer 0)
    if (index == 0 && layers_.size() == 1) {
        return nullptr;
    }

    // Extract layer
    std::unique_ptr<TerrainLayer> removed = std::move(layers_[index]);
    layers_.erase(layers_.begin() + index);

    // Adjust active layer index if needed
    if (activeLayerIndex_ >= layers_.size()) {
        activeLayerIndex_ = layers_.size() - 1;
    }

    return removed;
}
