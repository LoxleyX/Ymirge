#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>

class HeightMap {
public:
    HeightMap(int width, int height);
    HeightMap(const HeightMap& other);
    HeightMap(HeightMap&& other) noexcept;
    HeightMap& operator=(const HeightMap& other);
    HeightMap& operator=(HeightMap&& other) noexcept;

    float& at(int x, int y);
    float at(int x, int y) const;
    float sample(int x, int y) const;

    void normalize();
    void normalizeToRange(float min, float max);

    void clear();
    void fill(float value);
    void copyTo(HeightMap& dest) const;

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    float* getData() { return data_.data(); }
    const float* getData() const { return data_.data(); }
    size_t getSize() const { return data_.size(); }

    float getMin() const;
    float getMax() const;
    void getMinMax(float& outMin, float& outMax) const;

private:
    int width_;
    int height_;
    std::vector<float> data_;
};
