#include "HeightMap.h"
#include <limits>
#include <stdexcept>

HeightMap::HeightMap(int width, int height)
    : width_(width), height_(height), data_(width * height, 0.0f) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("HeightMap dimensions must be positive");
    }
}

HeightMap::HeightMap(const HeightMap& other)
    : width_(other.width_), height_(other.height_), data_(other.data_) {
}

HeightMap::HeightMap(HeightMap&& other) noexcept
    : width_(other.width_), height_(other.height_), data_(std::move(other.data_)) {
    other.width_ = 0;
    other.height_ = 0;
}

HeightMap& HeightMap::operator=(const HeightMap& other) {
    if (this != &other) {
        width_ = other.width_;
        height_ = other.height_;
        data_ = other.data_;
    }
    return *this;
}

HeightMap& HeightMap::operator=(HeightMap&& other) noexcept {
    if (this != &other) {
        width_ = other.width_;
        height_ = other.height_;
        data_ = std::move(other.data_);
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

float& HeightMap::at(int x, int y) {
    return data_[y * width_ + x];
}

float HeightMap::at(int x, int y) const {
    return data_[y * width_ + x];
}

float HeightMap::sample(int x, int y) const {
    x = std::clamp(x, 0, width_ - 1);
    y = std::clamp(y, 0, height_ - 1);
    return data_[y * width_ + x];
}

void HeightMap::normalize() {
    float min, max;
    getMinMax(min, max);

    if (max - min < 1e-6f) {
        // Avoid division by zero - just fill with 0.5
        fill(0.5f);
        return;
    }

    float range = max - min;
    for (float& h : data_) {
        h = (h - min) / range;
    }
}

void HeightMap::normalizeToRange(float minVal, float maxVal) {
    normalize();
    float range = maxVal - minVal;
    for (float& h : data_) {
        h = minVal + h * range;
    }
}

void HeightMap::clear() {
    std::fill(data_.begin(), data_.end(), 0.0f);
}

void HeightMap::fill(float value) {
    std::fill(data_.begin(), data_.end(), value);
}

void HeightMap::copyTo(HeightMap& dest) const {
    if (dest.width_ != width_ || dest.height_ != height_) {
        dest = HeightMap(width_, height_);
    }
    dest.data_ = data_;
}

float HeightMap::getMin() const {
    return *std::min_element(data_.begin(), data_.end());
}

float HeightMap::getMax() const {
    return *std::max_element(data_.begin(), data_.end());
}

void HeightMap::getMinMax(float& outMin, float& outMax) const {
    auto [min_it, max_it] = std::minmax_element(data_.begin(), data_.end());
    outMin = *min_it;
    outMax = *max_it;
}
