#include "LayerGroup.h"
#include <algorithm>
#include <stdexcept>

LayerGroup::LayerGroup(const std::string& name, int width, int height)
    : width_(width), height_(height), expanded_(true) {
    name_ = name;
    blendMode_ = BlendMode::NORMAL;
    opacity_ = 1.0f;
    visible_ = true;
    locked_ = false;
}

void LayerGroup::composite(HeightMap& output, const HeightMap& below) {
    if (!visible_ || opacity_ < 0.01f) {
        // Group not visible, just copy below
        output = below;
        return;
    }

    // Composite all children recursively
    // Start with the "below" heightmap
    HeightMap groupResult = below;

    for (auto& child : children_) {
        if (!child->isVisible()) continue;

        HeightMap childOutput(width_, height_);
        child->composite(childOutput, groupResult);

        // Child result becomes the new "below" for next child
        groupResult = childOutput;
    }

    // Apply group opacity and blend mode
    if (opacity_ >= 0.99f) {
        output = groupResult;
    } else {
        // Blend group result with below using group opacity
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                float belowValue = below.at(x, y);
                float groupValue = groupResult.at(x, y);
                output.at(x, y) = belowValue + (groupValue - belowValue) * opacity_;
            }
        }
    }
}

LayerBase* LayerGroup::getChild(size_t index) {
    if (index >= children_.size()) {
        throw std::out_of_range("Layer group child index out of range");
    }
    return children_[index].get();
}

const LayerBase* LayerGroup::getChild(size_t index) const {
    if (index >= children_.size()) {
        throw std::out_of_range("Layer group child index out of range");
    }
    return children_[index].get();
}

void LayerGroup::addChild(std::unique_ptr<LayerBase> child) {
    if (!child) {
        throw std::invalid_argument("Cannot add null child to layer group");
    }

    // Verify dimensions match
    if (child->getWidth() != width_ || child->getHeight() != height_) {
        throw std::invalid_argument("Child layer dimensions must match group dimensions");
    }

    children_.push_back(std::move(child));
}

std::unique_ptr<LayerBase> LayerGroup::removeChild(size_t index) {
    if (index >= children_.size()) {
        throw std::out_of_range("Layer group child index out of range");
    }

    auto child = std::move(children_[index]);
    children_.erase(children_.begin() + index);
    return child;
}

void LayerGroup::moveChild(size_t fromIndex, size_t toIndex) {
    if (fromIndex >= children_.size() || toIndex >= children_.size()) {
        throw std::out_of_range("Layer group child index out of range");
    }

    if (fromIndex == toIndex) return;

    auto child = std::move(children_[fromIndex]);
    children_.erase(children_.begin() + fromIndex);

    // Adjust toIndex if it was after fromIndex
    if (toIndex > fromIndex) {
        toIndex--;
    }

    children_.insert(children_.begin() + toIndex, std::move(child));
}

int LayerGroup::findChild(const LayerBase* child) const {
    for (size_t i = 0; i < children_.size(); ++i) {
        if (children_[i].get() == child) {
            return static_cast<int>(i);
        }
    }
    return -1;
}
