#include "AdvancedSplatmap.h"
#include <algorithm>
#include <cmath>
#include <iostream>

#include "stb_image_write.h"

bool AdvancedSplatmap::exportMultiMaterial(
    const HeightMap& heightMap,
    const ExportParams& params,
    const std::string& baseFilename) {

    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    size_t numMaterials = params.materials.size();
    if (numMaterials == 0) {
        std::cerr << "Error: No materials defined for splatmap export" << std::endl;
        return false;
    }

    // Calculate number of texture sets needed (4 materials per RGBA texture)
    size_t numTextures = (numMaterials + 3) / 4;

    // Generate each texture set
    for (size_t texIdx = 0; texIdx < numTextures; ++texIdx) {
        std::vector<unsigned char> pixels(width * height * 4, 0);

        // Process each pixel
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int pixelIdx = (y * width + x) * 4;

                float h = heightMap.at(x, y);
                float slope = calculateSlope(heightMap, x, y);

                // Calculate weights for the 4 materials in this texture
                float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                float totalWeight = 0.0f;

                for (int localIdx = 0; localIdx < 4; ++localIdx) {
                    size_t matIdx = texIdx * 4 + localIdx;
                    if (matIdx >= numMaterials) break;

                    const Material& mat = params.materials[matIdx];
                    float weight = calculateMaterialWeight(h, slope, mat);

                    weights[localIdx] = weight;
                    totalWeight += weight;
                }

                // Normalize weights
                if (totalWeight > 0.0001f) {
                    for (int i = 0; i < 4; ++i) {
                        weights[i] /= totalWeight;
                    }
                }

                // Write to RGBA channels
                pixels[pixelIdx + 0] = static_cast<unsigned char>(weights[0] * 255.0f);
                pixels[pixelIdx + 1] = static_cast<unsigned char>(weights[1] * 255.0f);
                pixels[pixelIdx + 2] = static_cast<unsigned char>(weights[2] * 255.0f);
                pixels[pixelIdx + 3] = static_cast<unsigned char>(weights[3] * 255.0f);
            }
        }

        // Generate filename for this texture
        std::string filename;
        if (numTextures == 1) {
            filename = baseFilename;
        } else {
            // Extract base name and extension
            size_t dotPos = baseFilename.find_last_of('.');
            std::string baseName = (dotPos != std::string::npos) ?
                baseFilename.substr(0, dotPos) : baseFilename;
            std::string ext = (dotPos != std::string::npos) ?
                baseFilename.substr(dotPos) : ".png";

            filename = baseName + "_" + std::to_string(texIdx) + ext;
        }

        // Export this texture
        int result = stbi_write_png(filename.c_str(), width, height, 4,
                                     pixels.data(), width * 4);

        if (!result) {
            std::cerr << "Failed to export splatmap texture: " << filename << std::endl;
            return false;
        }

        std::cout << "Splatmap texture exported: " << filename << std::endl;
    }

    return true;
}

bool AdvancedSplatmap::exportRGBA(
    const HeightMap& heightMap,
    const Material& mat0,
    const Material& mat1,
    const Material& mat2,
    const Material& mat3,
    const std::string& filename) {

    ExportParams params;
    params.materials = {mat0, mat1, mat2, mat3};
    params.outputChannels = 4;

    return exportMultiMaterial(heightMap, params, filename);
}

std::vector<AdvancedSplatmap::Material> AdvancedSplatmap::createDefaultMaterials() {
    std::vector<Material> materials;

    // 0: Deep Water (lowest elevations, any slope)
    materials.emplace_back("DeepWater", 0.0f, 0.20f, 0.0f, 3.0f, 0.05f, 0);

    // 1: Sand/Beach (low to medium-low, gentle slopes)
    materials.emplace_back("Sand", 0.18f, 0.30f, 0.0f, 0.4f, 0.06f, 1);

    // 2: Grass (medium elevation, gentle to moderate slopes)
    materials.emplace_back("Grass", 0.25f, 0.65f, 0.0f, 0.6f, 0.08f, 2);

    // 3: Forest (medium elevation, gentle slopes)
    materials.emplace_back("Forest", 0.35f, 0.70f, 0.0f, 0.5f, 0.07f, 3);

    // 4: Rock (steep slopes at any elevation, or high elevation)
    materials.emplace_back("Rock", 0.0f, 1.0f, 0.5f, 3.0f, 0.05f, 5);

    // 5: Cliff (very steep slopes)
    materials.emplace_back("Cliff", 0.0f, 1.0f, 0.8f, 3.0f, 0.03f, 6);

    // 6: Snow (high elevation, gentle to moderate slopes)
    materials.emplace_back("Snow", 0.80f, 1.0f, 0.0f, 0.7f, 0.10f, 4);

    // 7: Ice (high elevation, very steep slopes)
    materials.emplace_back("Ice", 0.85f, 1.0f, 0.6f, 3.0f, 0.05f, 7);

    return materials;
}

std::vector<AdvancedSplatmap::Material> AdvancedSplatmap::createDesertMaterials() {
    std::vector<Material> materials;

    // 0: Sand Dunes (low to medium, gentle)
    materials.emplace_back("SandDunes", 0.0f, 0.50f, 0.0f, 0.3f, 0.08f, 0);

    // 1: Rocky Desert (low to medium, moderate)
    materials.emplace_back("RockyDesert", 0.0f, 0.55f, 0.25f, 0.7f, 0.07f, 1);

    // 2: Desert Grass (medium elevation)
    materials.emplace_back("DesertGrass", 0.30f, 0.70f, 0.0f, 0.4f, 0.10f, 2);

    // 3: Red Rock (steep slopes, medium elevation)
    materials.emplace_back("RedRock", 0.20f, 0.80f, 0.5f, 3.0f, 0.05f, 4);

    // 4: Desert Plateau (high, flat)
    materials.emplace_back("DesertPlateau", 0.60f, 1.0f, 0.0f, 0.3f, 0.06f, 3);

    // 5: Canyon Walls (high, very steep)
    materials.emplace_back("CanyonWalls", 0.40f, 1.0f, 0.8f, 3.0f, 0.04f, 5);

    return materials;
}

std::vector<AdvancedSplatmap::Material> AdvancedSplatmap::createAlpineMaterials() {
    std::vector<Material> materials;

    // 0: Alpine Grass (low to medium, gentle)
    materials.emplace_back("AlpineGrass", 0.0f, 0.45f, 0.0f, 0.4f, 0.10f, 0);

    // 1: Pine Forest (medium, gentle to moderate)
    materials.emplace_back("PineForest", 0.25f, 0.65f, 0.0f, 0.5f, 0.08f, 1);

    // 2: Rock (medium to high, moderate slopes)
    materials.emplace_back("Rock", 0.40f, 0.85f, 0.4f, 0.8f, 0.07f, 3);

    // 3: Cliff (any elevation, steep)
    materials.emplace_back("Cliff", 0.0f, 1.0f, 0.7f, 3.0f, 0.04f, 5);

    // 4: Snow (high elevation, gentle)
    materials.emplace_back("Snow", 0.70f, 1.0f, 0.0f, 0.5f, 0.12f, 2);

    // 5: Glacier Ice (highest, moderate to steep)
    materials.emplace_back("GlacierIce", 0.85f, 1.0f, 0.3f, 3.0f, 0.08f, 4);

    return materials;
}

float AdvancedSplatmap::calculateMaterialWeight(
    float height,
    float slope,
    const Material& material) {

    // Height weight
    float heightWeight = 1.0f;
    if (height < material.heightMin) {
        float dist = (material.heightMin - height) / material.blendRange;
        heightWeight = 1.0f - smoothstep(0.0f, 1.0f, dist);
    } else if (height > material.heightMax) {
        float dist = (height - material.heightMax) / material.blendRange;
        heightWeight = 1.0f - smoothstep(0.0f, 1.0f, dist);
    }

    // Slope weight
    float slopeWeight = 1.0f;
    if (slope < material.slopeMin) {
        float dist = (material.slopeMin - slope) / (material.blendRange * 2.0f);
        slopeWeight = 1.0f - smoothstep(0.0f, 1.0f, dist);
    } else if (slope > material.slopeMax) {
        float dist = (slope - material.slopeMax) / (material.blendRange * 2.0f);
        slopeWeight = 1.0f - smoothstep(0.0f, 1.0f, dist);
    }

    // Combined weight
    float weight = heightWeight * slopeWeight;

    // Apply priority boost (higher priority materials win conflicts)
    weight *= (1.0f + material.priority * 0.1f);

    return std::max(0.0f, weight);
}

float AdvancedSplatmap::calculateSlope(
    const HeightMap& heightMap,
    int x, int y) {

    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    float center = heightMap.at(x, y);

    float left = (x > 0) ? heightMap.at(x - 1, y) : center;
    float right = (x < width - 1) ? heightMap.at(x + 1, y) : center;
    float up = (y > 0) ? heightMap.at(x, y - 1) : center;
    float down = (y < height - 1) ? heightMap.at(x, y + 1) : center;

    // Calculate gradients
    float dx = (right - left) * 0.5f;
    float dy = (down - up) * 0.5f;

    // Slope magnitude (radians)
    float slope = std::sqrt(dx * dx + dy * dy);

    return slope;
}

float AdvancedSplatmap::smoothstep(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
