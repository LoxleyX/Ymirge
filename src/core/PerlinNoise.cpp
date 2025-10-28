#include "PerlinNoise.h"
#include <cmath>
#include <algorithm>
#include <random>

PerlinNoise::PerlinNoise(uint32_t seed) : seed_(seed) {
    generatePermutation(seed);
}

void PerlinNoise::generatePermutation(uint32_t seed) {
    permutation_.resize(256);

    // Initialize with sequential values
    for (int i = 0; i < 256; ++i) {
        permutation_[i] = i;
    }

    // Shuffle using seed
    std::mt19937 rng(seed);
    std::shuffle(permutation_.begin(), permutation_.end(), rng);

    // Duplicate for easy wrapping
    p_.resize(512);
    for (int i = 0; i < 256; ++i) {
        p_[i] = permutation_[i];
        p_[i + 256] = permutation_[i];
    }
}

void PerlinNoise::setSeed(uint32_t seed) {
    seed_ = seed;
    generatePermutation(seed);
}

float PerlinNoise::fade(float t) {
    // 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float PerlinNoise::lerp(float t, float a, float b) {
    return a + t * (b - a);
}

float PerlinNoise::grad(int hash, float x, float y) {
    // Convert low 4 bits of hash code into 12 gradient directions
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0.0f);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float PerlinNoise::noise(float x, float y) const {
    // Find unit grid cell containing point
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;

    // Get relative xy coordinates of point within cell
    x -= std::floor(x);
    y -= std::floor(y);

    // Compute fade curves for x and y
    float u = fade(x);
    float v = fade(y);

    // Hash coordinates of the 4 square corners
    int A = p_[X] + Y;
    int AA = p_[A];
    int AB = p_[A + 1];
    int B = p_[X + 1] + Y;
    int BA = p_[B];
    int BB = p_[B + 1];

    // Blend results from 4 corners
    float res = lerp(v,
        lerp(u, grad(p_[AA], x, y), grad(p_[BA], x - 1.0f, y)),
        lerp(u, grad(p_[AB], x, y - 1.0f), grad(p_[BB], x - 1.0f, y - 1.0f))
    );

    return res;
}

float PerlinNoise::octaveNoise(float x, float y, int octaves,
                               float persistence, float lacunarity) const {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;  // Used for normalizing result to 0.0 - 1.0

    for (int i = 0; i < octaves; ++i) {
        total += noise(x * frequency, y * frequency) * amplitude;

        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return total / maxValue;
}
