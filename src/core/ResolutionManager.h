#pragma once

#include "HeightMap.h"
#include "TerrainGenerator.h"
#include "TerrainParams.h"
#include "ThreadPool.h"
#include <memory>
#include <future>
#include <chrono>

/**
 * Resolution levels for terrain generation
 *
 * PREVIEW (128x128)   - Real-time slider updates (<16ms target)
 * STANDARD (512x512)  - Default quality (~2s)
 * HIGH (1024x1024)    - High quality (~8s)
 * EXPORT (2048x2048)  - Export quality (~30s)
 * ULTRA (4096x4096)   - Maximum quality (~2min)
 */
enum class Resolution {
    PREVIEW = 128,
    STANDARD = 512,
    HIGH = 1024,
    EXPORT = 2048,
    ULTRA = 4096
};

/**
 * ResolutionManager
 *
 * Manages terrain generation at multiple resolutions with automatic
 * upgrading after user stops interacting.
 *
 * Workflow:
 * 1. User drags slider -> Generate at PREVIEW (instant feedback)
 * 2. User stops -> Wait 500ms -> Auto-upgrade to STANDARD
 * 3. User clicks "Generate" -> Generate at HIGH
 * 4. User exports -> Generate at EXPORT or ULTRA
 */
class ResolutionManager {
public:
    ResolutionManager(ThreadPool* threadPool);
    ~ResolutionManager();

    /**
     * Generate terrain at specified resolution
     *
     * @param res Resolution level
     * @param params Terrain parameters
     */
    void generateAt(Resolution res, const TerrainParams& params);

    /**
     * Update state - call every frame
     * Handles auto-upgrade timer
     */
    void update();

    /**
     * Check if generation is in progress
     */
    bool isGenerating() const { return isGenerating_; }

    /**
     * Get current heightmap (thread-safe)
     */
    const HeightMap& getHeightMap() const;

    /**
     * Get mutable heightmap for in-place editing (brush tools)
     *
     * WARNING: Only use this for direct terrain editing. For generation,
     * use generateAt() which properly manages undo/async state.
     */
    HeightMap& getHeightMapMutable();

    /**
     * Replace current heightmap (e.g., for importing)
     *
     * @param newMap New heightmap to use
     */
    void setHeightMap(const HeightMap& newMap);

    /**
     * Get current resolution
     */
    Resolution getCurrentResolution() const { return currentRes_; }

    /**
     * Get target resolution (may be higher if upgrading)
     */
    Resolution getTargetResolution() const { return targetRes_; }

    /**
     * Set target resolution for auto-upgrade
     *
     * @param res Target resolution to upgrade to
     */
    void setTargetResolution(Resolution res) { targetRes_ = res; }

    /**
     * Mark that user is interacting (reset upgrade timer)
     */
    void onUserInteraction();

    /**
     * Cancel any pending generation
     */
    void cancelGeneration();

    /**
     * Get resolution as integer
     */
    static int getResolutionValue(Resolution res) {
        return static_cast<int>(res);
    }

    /**
     * Get resolution name as string
     */
    static const char* getResolutionName(Resolution res);

private:
    // Check if auto-upgrade should happen
    bool shouldAutoUpgrade() const;

    // Start generation task
    void startGeneration(Resolution res, const TerrainParams& params);

    // Check if async generation completed
    void checkGenerationComplete();

    ThreadPool* threadPool_;

    Resolution currentRes_;       // Currently displayed resolution
    Resolution targetRes_;        // Target for auto-upgrade
    TerrainParams currentParams_; // Last parameters used

    std::unique_ptr<TerrainGenerator> generator_;
    std::future<void> generationFuture_;
    bool isGenerating_;

    // Auto-upgrade timer
    std::chrono::steady_clock::time_point lastInteraction_;
    static constexpr std::chrono::milliseconds upgradeDelay_{500};

    // Flag to prevent duplicate generations
    bool paramsChanged_;
};
