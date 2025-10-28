#pragma once

#include "UndoCommand.h"
#include "HeightMap.h"
#include <vector>
#include <string>

/**
 * PixelDelta - Stores a single pixel change
 *
 * Memory efficient: Only 12 bytes per changed pixel
 */
struct PixelDelta {
    int x;
    int y;
    float oldValue;
    float newValue;
};

/**
 * HeightMapEditCommand - Undoable heightmap modification
 *
 * Uses delta storage: only stores changed pixels, not entire heightmap copies.
 * Memory usage: ~12 bytes per changed pixel + overhead
 *
 * Example: Brush stroke affecting 1000 pixels = ~12KB
 *          Full 1024x1024 copy = ~4MB
 *          Savings: 99.7%
 */
class HeightMapEditCommand : public UndoCommand {
public:
    /**
     * Constructor
     *
     * @param heightMap Target heightmap to modify
     * @param description User-friendly description (e.g., "Raise Brush")
     */
    HeightMapEditCommand(HeightMap* heightMap, const std::string& description);

    /**
     * Record a pixel change
     *
     * Call this for each pixel before modifying it.
     *
     * @param x X coordinate
     * @param y Y coordinate
     * @param oldValue Current value (before change)
     * @param newValue New value (after change)
     */
    void recordChange(int x, int y, float oldValue, float newValue);

    /**
     * Record changes from a region
     *
     * Convenience method for brush operations.
     * Captures old values, then you apply changes, then call finalize().
     *
     * @param centerX Center X coordinate
     * @param centerY Center Y coordinate
     * @param radius Brush radius in pixels
     * @param useSquare If true, captures square region; if false, captures circular (default: false)
     */
    void captureRegion(int centerX, int centerY, int radius, bool useSquare = false);

    /**
     * Finalize region capture after changes applied
     *
     * Call after captureRegion() and after applying your changes.
     * Records the new values for all captured pixels.
     */
    void finalizeRegion();

    // UndoCommand interface
    void execute() override;
    void undo() override;
    const char* getDescription() const override;
    size_t getMemoryUsage() const override;

private:
    HeightMap* heightMap_;
    std::vector<PixelDelta> deltas_;
    std::string description_;

    // Temporary storage for captureRegion/finalizeRegion workflow
    struct CapturedPixel {
        int x;
        int y;
        float oldValue;
    };
    std::vector<CapturedPixel> capturedPixels_;
};
