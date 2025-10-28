#pragma once

#include "LayerStack.h"
#include <string>
#include <memory>
class LayerSerializer {
public:
    /**
     * Save layer stack to .ymlayers project file
     *
     * Creates:
     * - project_name.ymlayers (JSON metadata)
     * - layer_0_heightmap.raw, layer_1_heightmap.raw, etc.
     * - layer_0_mask.raw (if layer has mask), etc.
     *
     * @param stack Layer stack to save
     * @param filepath Path to .ymlayers file (e.g., "terrain_project.ymlayers")
     * @return true on success, false on error
     */
    static bool save(const LayerStack& stack, const std::string& filepath);

    /**
     * Load layer stack from .ymlayers project file
     *
     * @param stack Layer stack to load into (will be cleared first)
     * @param filepath Path to .ymlayers file
     * @return true on success, false on error
     */
    static bool load(LayerStack& stack, const std::string& filepath);

    /**
     * Get last error message (if save/load failed)
     */
    static std::string getLastError();

public:
    /**
     * Save a single heightmap to RAW file (32-bit float format)
     */
    static bool saveHeightMapRaw(const HeightMap& heightMap, const std::string& filepath);

    /**
     * Load a single heightmap from RAW file (32-bit float format)
     */
    static bool loadHeightMapRaw(HeightMap& heightMap, const std::string& filepath);

    /**
     * Convert LayerType enum to string
     */
    static std::string layerTypeToString(LayerType type);

    /**
     * Convert string to LayerType enum
     */
    static LayerType stringToLayerType(const std::string& str);

    /**
     * Convert BlendMode enum to string
     */
    static std::string blendModeToString(BlendMode mode);

    /**
     * Convert string to BlendMode enum
     */
    static BlendMode stringToBlendMode(const std::string& str);

    /**
     * Get directory from filepath
     */
    static std::string getDirectory(const std::string& filepath);

    /**
     * Get filename without extension
     */
    static std::string getBaseName(const std::string& filepath);
    /**
     * Recursively serialize a layer or group to JSON
     */

    static std::string lastError_;
};
