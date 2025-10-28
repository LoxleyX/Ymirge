#include "HeightMapEditCommand.h"
#include <algorithm>
#include <cmath>

HeightMapEditCommand::HeightMapEditCommand(HeightMap* heightMap, const std::string& description)
    : heightMap_(heightMap)
    , description_(description) {
}

void HeightMapEditCommand::recordChange(int x, int y, float oldValue, float newValue) {
    // Only record if value actually changed
    if (std::abs(oldValue - newValue) < 0.0001f) {
        return;
    }

    deltas_.push_back({x, y, oldValue, newValue});
}

void HeightMapEditCommand::captureRegion(int centerX, int centerY, int radius, bool useSquare) {
    capturedPixels_.clear();

    int width = heightMap_->getWidth();
    int height = heightMap_->getHeight();

    // Capture all pixels in radius
    int radiusSq = radius * radius;

    for (int y = centerY - radius; y <= centerY + radius; ++y) {
        for (int x = centerX - radius; x <= centerX + radius; ++x) {
            // Check bounds
            if (x < 0 || x >= width || y < 0 || y >= height) {
                continue;
            }

            // Check if within shape (circular for brushes, square for stamps)
            if (!useSquare) {
                int dx = x - centerX;
                int dy = y - centerY;
                if (dx * dx + dy * dy > radiusSq) {
                    continue;
                }
            }

            // Capture old value
            capturedPixels_.push_back({x, y, heightMap_->at(x, y)});
        }
    }
}

void HeightMapEditCommand::finalizeRegion() {
    // Record deltas for all captured pixels
    for (const auto& pixel : capturedPixels_) {
        float newValue = heightMap_->at(pixel.x, pixel.y);
        recordChange(pixel.x, pixel.y, pixel.oldValue, newValue);
    }

    capturedPixels_.clear();
}

void HeightMapEditCommand::execute() {
    // Apply new values
    for (const auto& delta : deltas_) {
        heightMap_->at(delta.x, delta.y) = delta.newValue;
    }
}

void HeightMapEditCommand::undo() {
    // Restore old values
    for (const auto& delta : deltas_) {
        heightMap_->at(delta.x, delta.y) = delta.oldValue;
    }
}

const char* HeightMapEditCommand::getDescription() const {
    return description_.c_str();
}

size_t HeightMapEditCommand::getMemoryUsage() const {
    // PixelDelta = 12 bytes (2 ints + 2 floats)
    // + string overhead + vector overhead
    return deltas_.size() * sizeof(PixelDelta) + description_.size() + 100;
}
