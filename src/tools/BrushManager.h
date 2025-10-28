#pragma once

#include "BrushTool.h"
#include "RaiseBrush.h"
#include "LowerBrush.h"
#include "SmoothBrush.h"
#include "FlattenBrush.h"
#include "UndoStack.h"
#include "HeightMapEditCommand.h"

#ifdef YMIRGE_UI_ENABLED
#include "raylib.h"
#endif

#include <memory>

/**
 * Tool type enumeration
 * VIEW: Camera controls (default)
 * RAISE/LOWER/SMOOTH/FLATTEN: Terrain editing brushes
 * STAMP: Stamp tool for placing pre-made features
 */
enum class BrushType {
    VIEW,      // Camera control mode (V)
    RAISE,     // Raise terrain (R)
    LOWER,     // Lower terrain (L)
    SMOOTH,    // Smooth terrain (S)
    FLATTEN,   // Flatten terrain (F)
    STAMP      // Stamp tool (T)
};

/**
 * BrushManager - Manages interactive terrain sculpting
 *
 * Handles:
 * - Mouse input → 3D terrain position
 * - Brush application with undo support
 * - Stroke grouping (single undo command per stroke)
 * - Raycast intersection with terrain mesh
 */
class BrushManager {
public:
    /**
     * Constructor
     *
     * @param undoStack Undo stack for command storage
     */
    BrushManager(UndoStack* undoStack);

    /**
     * Set active brush type
     */
    void setActiveBrush(BrushType type);

    /**
     * Get active brush type
     */
    BrushType getActiveBrush() const { return activeType_; }

    /**
     * Set brush size (radius in pixels)
     */
    void setBrushSize(int radius);

    /**
     * Set brush strength (0.0 - 1.0)
     */
    void setBrushStrength(float strength);

    /**
     * Begin a new brush stroke
     *
     * Call this when mouse button is pressed.
     * Creates a new undo command for the stroke.
     *
     * @param map Heightmap to edit
     * @param x Heightmap X coordinate
     * @param y Heightmap Y coordinate
     */
    void beginStroke(HeightMap& map, int x, int y);

    /**
     * Apply brush during stroke
     *
     * Call this continuously while mouse is dragged.
     * Accumulates changes in the current undo command.
     *
     * @param map Heightmap to edit
     * @param x Heightmap X coordinate
     * @param y Heightmap Y coordinate
     * @param deltaTime Time since last application (for smooth strokes)
     * @return true if brush was applied
     */
    bool applyStroke(HeightMap& map, int x, int y, float deltaTime);

    /**
     * End the current brush stroke
     *
     * Call this when mouse button is released.
     * Pushes the accumulated undo command to the stack.
     */
    void endStroke();

    /**
     * Check if a stroke is in progress
     */
    bool isStrokeActive() const { return strokeActive_; }

#ifdef YMIRGE_UI_ENABLED
    /**
     * Convert screen coordinates to heightmap coordinates (raylib version)
     *
     * Uses raycasting to find intersection with terrain mesh.
     *
     * @param outX Output heightmap X coordinate
     * @param outY Output heightmap Y coordinate
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     * @param camera 3D camera
     * @param mesh Terrain mesh
     * @param transform Mesh transform matrix
     * @param map Heightmap (for coordinate scaling)
     * @return true if intersection found, false if mouse not over terrain
     */
    bool screenToHeightMapCoords(int& outX, int& outY,
                                  int screenX, int screenY,
                                  const Camera3D& camera,
                                  const Mesh& mesh,
                                  const Matrix& transform,
                                  const HeightMap& map) const;

    /**
     * Get cursor position in heightmap coordinates (raylib version)
     *
     * Convenience method that wraps screenToHeightMapCoords.
     *
     * @return true if cursor is over terrain
     */
    bool getCursorPosition(int& outX, int& outY,
                           const Vector2& mousePos,
                           const Camera3D& camera,
                           const Mesh& mesh,
                           const Matrix& transform,
                           const HeightMap& map) const;
#endif // YMIRGE_UI_ENABLED

private:
    // Brush instances
    std::unique_ptr<RaiseBrush> raiseBrush_;
    std::unique_ptr<LowerBrush> lowerBrush_;
    std::unique_ptr<SmoothBrush> smoothBrush_;
    std::unique_ptr<FlattenBrush> flattenBrush_;

    BrushTool* activeBrush_;
    BrushType activeType_;

    // Undo management
    UndoStack* undoStack_;
    std::unique_ptr<HeightMapEditCommand> currentCommand_;
    bool strokeActive_;

    // Last application position (for continuous strokes)
    int lastX_;
    int lastY_;
};
