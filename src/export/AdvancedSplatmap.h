#pragma once

#include "HeightMap.h"
#include <string>
#include <vector>

/**
 * Advanced Splatmap Generation (Phase 2)
 *
 * Features:
 * - Multi-material blending (8+ materials)
 * - Smooth transitions between biomes
 * - Slope-based material selection
 * - Height-based biome zones
 * - Configurable material parameters
 */
class AdvancedSplatmap {
public:
    // Material definition
    struct Material {
        std::string name;
        float heightMin;      // Minimum elevation (0-1)
        float heightMax;      // Maximum elevation (0-1)
        float slopeMin;       // Minimum slope (radians, ~0-1.5)
        float slopeMax;       // Maximum slope
        float blendRange;     // Transition smoothness
        int priority;         // Higher priority wins conflicts

        Material(const std::string& n, float hMin, float hMax,
                float sMin, float sMax, float blend = 0.05f, int prio = 0)
            : name(n), heightMin(hMin), heightMax(hMax),
              slopeMin(sMin), slopeMax(sMax), blendRange(blend), priority(prio) {}
    };

    // Splatmap export parameters
    struct ExportParams {
        std::vector<Material> materials;
        bool enableSmoothing = true;    // Smooth transitions between materials
        float transitionWidth = 0.05f;   // Width of transition zones
        int outputChannels = 4;          // 4 or 8 channel output
    };

    /**
     * Export advanced splatmap with multiple materials
     *
     * For 8+ materials, exports multiple RGBA textures:
     * - splatmap_0.png (materials 0-3)
     * - splatmap_1.png (materials 4-7)
     * - etc.
     */
    static bool exportMultiMaterial(
        const HeightMap& heightMap,
        const ExportParams& params,
        const std::string& baseFilename);

    /**
     * Export single RGBA splatmap with custom materials
     * (limited to 4 materials)
     */
    static bool exportRGBA(
        const HeightMap& heightMap,
        const Material& mat0,
        const Material& mat1,
        const Material& mat2,
        const Material& mat3,
        const std::string& filename);

    /**
     * Create default terrain material set
     * Returns 8 materials for realistic terrain
     */
    static std::vector<Material> createDefaultMaterials();

    /**
     * Create desert biome materials
     */
    static std::vector<Material> createDesertMaterials();

    /**
     * Create alpine biome materials
     */
    static std::vector<Material> createAlpineMaterials();

private:
    // Calculate material weight at given height/slope
    static float calculateMaterialWeight(
        float height,
        float slope,
        const Material& material);

    // Calculate slope at given position (radians)
    static float calculateSlope(
        const HeightMap& heightMap,
        int x, int y);

    // Smooth step interpolation
    static float smoothstep(float edge0, float edge1, float x);
};
