#include "PresetManager.h"

PresetManager::PresetManager() {
    definePresets();
}

bool PresetManager::applyPreset(const std::string& name, TerrainParams& params) {
    auto it = presets_.find(name);
    if (it != presets_.end()) {
        params = it->second;
        return true;
    }
    return false;
}

std::vector<std::string> PresetManager::getPresetNames() const {
    return presetOrder_;
}

void PresetManager::definePresets() {
    // Game World - Best for open-world games
    {
        TerrainParams params;
        params.scale = 200.0f;
        params.octaves = 4;
        params.persistence = 0.38f;
        params.lacunarity = 1.8f;
        params.valleyStrength = 0.4f;
        params.valleySharpness = 1.5f;
        params.valleyWidth = 0.75f;
        params.flattenValleys = 0.95f;
        params.valleyConnectivity = 0.9f;
        params.riverIntensity = 0.3f;
        params.riverWidth = 0.02f;
        params.erosion = 0.4f;
        params.terracing = 0;
        params.peaks = 0.2f;
        params.island = 0.0f;
        params.edgePadding = 0.2f;
        params.islandShape = 1.5f;

        presets_["Game World"] = params;
        presetOrder_.push_back("Game World");
    }

    // Mountain Fortress - Dramatic sharp peaks with flat valleys
    {
        TerrainParams params;
        params.scale = 180.0f;
        params.octaves = 6;
        params.persistence = 0.5f;
        params.lacunarity = 2.1f;
        params.valleyStrength = 0.65f;
        params.valleySharpness = 2.2f;
        params.valleyWidth = 0.7f;
        params.flattenValleys = 0.92f;
        params.valleyConnectivity = 0.85f;
        params.riverIntensity = 0.2f;
        params.riverWidth = 0.018f;
        params.erosion = 0.15f;
        params.terracing = 0;
        params.peaks = 0.55f;
        params.island = 0.0f;
        params.edgePadding = 0.15f;
        params.islandShape = 1.5f;

        presets_["Mountain Fortress"] = params;
        presetOrder_.push_back("Mountain Fortress");
    }

    // Plains - Flattest option
    {
        TerrainParams params;
        params.scale = 220.0f;
        params.octaves = 3;
        params.persistence = 0.3f;
        params.lacunarity = 1.6f;
        params.valleyStrength = 0.2f;
        params.valleySharpness = 0.6f;
        params.valleyWidth = 0.85f;
        params.flattenValleys = 0.97f;
        params.valleyConnectivity = 0.95f;
        params.riverIntensity = 0.4f;
        params.riverWidth = 0.025f;
        params.erosion = 0.5f;
        params.terracing = 0;
        params.peaks = 0.0f;
        params.island = 0.0f;
        params.edgePadding = 0.25f;
        params.islandShape = 1.5f;

        presets_["Plains"] = params;
        presetOrder_.push_back("Plains");
    }

    // Tropical Island
    {
        TerrainParams params;
        params.scale = 120.0f;
        params.octaves = 5;
        params.persistence = 0.45f;
        params.lacunarity = 2.0f;
        params.valleyStrength = 0.45f;
        params.valleySharpness = 1.6f;
        params.valleyWidth = 0.6f;
        params.flattenValleys = 0.85f;
        params.valleyConnectivity = 0.75f;
        params.riverIntensity = 0.25f;
        params.riverWidth = 0.02f;
        params.erosion = 0.35f;
        params.terracing = 0;
        params.peaks = 0.35f;
        params.island = 0.85f;
        params.edgePadding = 0.12f;
        params.islandShape = 2.5f;

        presets_["Tropical Island"] = params;
        presetOrder_.push_back("Tropical Island");
    }

    // Alpine Peaks
    {
        TerrainParams params;
        params.scale = 95.0f;
        params.octaves = 7;
        params.persistence = 0.55f;
        params.lacunarity = 2.3f;
        params.valleyStrength = 0.5f;
        params.valleySharpness = 3.0f;
        params.valleyWidth = 0.5f;
        params.flattenValleys = 0.8f;
        params.valleyConnectivity = 0.6f;
        params.riverIntensity = 0.15f;
        params.riverWidth = 0.015f;
        params.erosion = 0.1f;
        params.terracing = 0;
        params.peaks = 0.75f;
        params.island = 0.0f;
        params.edgePadding = 0.1f;
        params.islandShape = 1.5f;

        presets_["Alpine Peaks"] = params;
        presetOrder_.push_back("Alpine Peaks");
    }

    // Canyon Lands
    {
        TerrainParams params;
        params.scale = 140.0f;
        params.octaves = 5;
        params.persistence = 0.48f;
        params.lacunarity = 2.0f;
        params.valleyStrength = 0.7f;
        params.valleySharpness = 4.5f;
        params.valleyWidth = 0.65f;
        params.flattenValleys = 0.9f;
        params.valleyConnectivity = 0.7f;
        params.riverIntensity = 0.35f;
        params.riverWidth = 0.022f;
        params.erosion = 0.05f;
        params.terracing = 7;
        params.peaks = 0.3f;
        params.island = 0.0f;
        params.edgePadding = 0.15f;
        params.islandShape = 1.5f;

        presets_["Canyon Lands"] = params;
        presetOrder_.push_back("Canyon Lands");
    }

    // Archipelago
    {
        TerrainParams params;
        params.scale = 110.0f;
        params.octaves = 5;
        params.persistence = 0.42f;
        params.lacunarity = 2.0f;
        params.valleyStrength = 0.35f;
        params.valleySharpness = 1.4f;
        params.valleyWidth = 0.55f;
        params.flattenValleys = 0.75f;
        params.valleyConnectivity = 0.5f;
        params.riverIntensity = 0.15f;
        params.riverWidth = 0.015f;
        params.erosion = 0.3f;
        params.terracing = 0;
        params.peaks = 0.25f;
        params.island = 0.75f;
        params.edgePadding = 0.08f;
        params.islandShape = 2.0f;

        presets_["Archipelago"] = params;
        presetOrder_.push_back("Archipelago");
    }

    // Extreme Contrast
    {
        TerrainParams params;
        params.scale = 150.0f;
        params.octaves = 7;
        params.persistence = 0.6f;
        params.lacunarity = 2.5f;
        params.valleyStrength = 0.75f;
        params.valleySharpness = 4.0f;
        params.valleyWidth = 0.8f;
        params.flattenValleys = 0.98f;
        params.valleyConnectivity = 0.9f;
        params.riverIntensity = 0.2f;
        params.riverWidth = 0.018f;
        params.erosion = 0.08f;
        params.terracing = 0;
        params.peaks = 0.8f;
        params.island = 0.0f;
        params.edgePadding = 0.12f;
        params.islandShape = 1.5f;

        presets_["Extreme Contrast"] = params;
        presetOrder_.push_back("Extreme Contrast");
    }
}
