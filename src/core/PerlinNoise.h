#pragma once

#include <vector>
#include <cstdint>

class PerlinNoise {
public:
    explicit PerlinNoise(uint32_t seed = 12345);

    float noise(float x, float y) const;
    float octaveNoise(float x, float y, int octaves,
                     float persistence, float lacunarity) const;

    void setSeed(uint32_t seed);

private:
    void generatePermutation(uint32_t seed);

    static float fade(float t);
    static float lerp(float t, float a, float b);
    static float grad(int hash, float x, float y);

    uint32_t seed_;
    std::vector<int> permutation_;
    std::vector<int> p_;  // Doubled permutation for wrapping
};
