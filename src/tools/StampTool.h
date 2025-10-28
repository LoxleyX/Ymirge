#pragma once

#include "HeightMap.h"
#include <string>
#include <vector>

/**
 * Blend modes for stamp application
 * ORDER MUST MATCH UI COMBO BOX: {"Blend", "Add", "Subtract", "Multiply", "Max", "Min", "Replace"}
 */
enum class StampBlendMode {
    BLEND,      // Alpha blend (using opacity) - DEFAULT
    ADD,        // Add stamp height to terrain
    SUBTRACT,   // Subtract stamp from terrain
    MULTIPLY,   // Multiply terrain by stamp
    MAX,        // Take maximum of terrain and stamp
    MIN,        // Take minimum of terrain and stamp
    REPLACE     // Replace terrain with stamp
};

/**
 * Stamp metadata
 */
struct StampInfo {
    std::string name;
    std::string category;  // mountains, craters, plateaus, valleys, man-made
    std::string filepath;
    int width;
    int height;
    std::string description;
};

/**
 * Stamp Tool - Apply pre-made heightmap stamps to terrain
 *
 * Based on World Creator and Unity Terrain stamp systems.
 * Allows quick placement of features like mountains, craters, plateaus.
 */
class StampTool {
public:
    StampTool();
    ~StampTool();

    /**
     * Load a stamp from file (PNG heightmap)
     */
    bool loadStamp(const std::string& filepath);

    /**
     * Apply stamp to heightmap at specified position
     *
     * @param map Target heightmap
     * @param centerX X coordinate of stamp center
     * @param centerY Y coordinate of stamp center
     * @param scale Scale factor (1.0 = original size)
     * @param rotation Rotation in degrees (0-360)
     * @param opacity Blend strength (0.0-1.0)
     * @param heightScale Vertical scale of stamp (1.0 = original height)
     */
    void applyStamp(HeightMap& map,
                    int centerX, int centerY,
                    float scale = 1.0f,
                    float rotation = 0.0f,
                    float opacity = 1.0f,
                    float heightScale = 1.0f);

    /**
     * Set blend mode for stamp application
     */
    void setBlendMode(StampBlendMode mode) { blendMode_ = mode; }
    StampBlendMode getBlendMode() const { return blendMode_; }

    /**
     * Get stamp dimensions
     */
    int getStampWidth() const;
    int getStampHeight() const;

    /**
     * Check if stamp is loaded
     */
    bool isLoaded() const { return stampLoaded_; }

    /**
     * Get stamp data for preview
     */
    const HeightMap& getStampData() const { return stampData_; }

    /**
     * Clear loaded stamp
     */
    void clear();

private:
    // Sample stamp at rotated/scaled position
    float sampleStamp(float x, float y, float scale, float rotation);

    // Blend stamp value with terrain value
    float blendValue(float terrain, float stamp, float opacity);

    HeightMap stampData_;
    StampBlendMode blendMode_;
    bool stampLoaded_;
};

/**
 * Stamp Library Manager
 *
 * Manages collection of available stamps
 */
class StampLibrary {
public:
    StampLibrary();

    /**
     * Scan directory for stamp files
     */
    void scanDirectory(const std::string& directory);

    /**
     * Generate default procedural stamps
     * Creates mountain, crater, plateau, and valley stamps
     */
    void generateDefaultStamps();

    /**
     * Get list of available stamps
     */
    const std::vector<StampInfo>& getStamps() const { return stamps_; }

    /**
     * Get stamps by category
     */
    std::vector<StampInfo> getStampsByCategory(const std::string& category) const;

    /**
     * Get all categories
     */
    std::vector<std::string> getCategories() const;

    /**
     * Find stamp by name
     */
    const StampInfo* findStamp(const std::string& name) const;

    /**
     * Get stamp index by name
     */
    int findStampIndex(const std::string& name) const;

    /**
     * Create procedural stamp heightmap
     */
    static HeightMap createProceduralStamp(const std::string& type, int size = 128);

private:
    std::vector<StampInfo> stamps_;
};
