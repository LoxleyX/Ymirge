#pragma once

#include "TerrainParams.h"
#include <string>
#include <vector>
#include <unordered_map>

/**
 * Preset Manager
 *
 * Manages terrain generation presets.
 * Presets are hardcoded for simplicity (can be moved to JSON later).
 */
class PresetManager {
public:
    PresetManager();

    /**
     * Apply a preset to parameters
     *
     * @param name Preset name
     * @param params Parameters to update
     * @return true if preset was found and applied
     */
    bool applyPreset(const std::string& name, TerrainParams& params);

    /**
     * Get list of all preset names
     */
    std::vector<std::string> getPresetNames() const;

    /**
     * Get number of presets
     */
    size_t getPresetCount() const { return presets_.size(); }

private:
    void definePresets();

    std::unordered_map<std::string, TerrainParams> presets_;
    std::vector<std::string> presetOrder_;  // For consistent ordering
};
