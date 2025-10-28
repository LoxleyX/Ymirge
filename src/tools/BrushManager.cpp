#include "BrushManager.h"
#include <iostream>

BrushManager::BrushManager(UndoStack* undoStack)
    : undoStack_(undoStack)
    , strokeActive_(false)
    , lastX_(-1)
    , lastY_(-1) {

    // Create brush instances
    raiseBrush_ = std::make_unique<RaiseBrush>();
    lowerBrush_ = std::make_unique<LowerBrush>();
    smoothBrush_ = std::make_unique<SmoothBrush>();
    flattenBrush_ = std::make_unique<FlattenBrush>();

    // Set default tool (View mode)
    activeBrush_ = nullptr;
    activeType_ = BrushType::VIEW;
}

void BrushManager::setActiveBrush(BrushType type) {
    activeType_ = type;

    switch (type) {
        case BrushType::VIEW:
            // View mode - no brush active
            activeBrush_ = nullptr;
            std::cout << "Tool: Switched to View (camera controls)" << std::endl;
            break;
        case BrushType::RAISE:
            activeBrush_ = raiseBrush_.get();
            std::cout << "Brush: Switched to " << activeBrush_->getName() << std::endl;
            break;
        case BrushType::LOWER:
            activeBrush_ = lowerBrush_.get();
            std::cout << "Brush: Switched to " << activeBrush_->getName() << std::endl;
            break;
        case BrushType::SMOOTH:
            activeBrush_ = smoothBrush_.get();
            std::cout << "Brush: Switched to " << activeBrush_->getName() << std::endl;
            break;
        case BrushType::FLATTEN:
            activeBrush_ = flattenBrush_.get();
            std::cout << "Brush: Switched to " << activeBrush_->getName() << std::endl;
            break;
    }
}

void BrushManager::setBrushSize(int radius) {
    raiseBrush_->setRadius(radius);
    lowerBrush_->setRadius(radius);
    smoothBrush_->setRadius(radius);
    flattenBrush_->setRadius(radius);
}

void BrushManager::setBrushStrength(float strength) {
    raiseBrush_->setStrength(strength);
    lowerBrush_->setStrength(strength);
    smoothBrush_->setStrength(strength);
    flattenBrush_->setStrength(strength);
}

void BrushManager::beginStroke(HeightMap& map, int x, int y) {
    if (strokeActive_) {
        endStroke();  // End previous stroke if still active
    }

    // Create new undo command
    currentCommand_ = std::make_unique<HeightMapEditCommand>(
        &map,
        std::string("Brush: ") + activeBrush_->getName()
    );

    // For flatten brush, sample target height from click position
    if (activeType_ == BrushType::FLATTEN) {
        float targetHeight = map.at(x, y);
        flattenBrush_->setTargetHeight(targetHeight);
        std::cout << "Flatten: Target height = " << targetHeight << std::endl;
    }

    strokeActive_ = true;
    lastX_ = x;
    lastY_ = y;

    std::cout << "Brush: Started stroke at (" << x << ", " << y << ")" << std::endl;
}

bool BrushManager::applyStroke(HeightMap& map, int x, int y, float deltaTime) {
    if (!strokeActive_ || !currentCommand_) {
        return false;
    }

    // Capture region before applying brush
    currentCommand_->captureRegion(x, y, activeBrush_->getRadius());

    // Apply brush
    activeBrush_->apply(map, x, y, deltaTime);

    // Finalize region (record deltas)
    currentCommand_->finalizeRegion();

    lastX_ = x;
    lastY_ = y;

    return true;
}

void BrushManager::endStroke() {
    if (!strokeActive_ || !currentCommand_) {
        return;
    }

    // Push command to undo stack
    undoStack_->push(std::move(currentCommand_));

    strokeActive_ = false;
    lastX_ = -1;
    lastY_ = -1;

    std::cout << "Brush: Ended stroke" << std::endl;
}

#ifdef YMIRGE_UI_ENABLED
bool BrushManager::screenToHeightMapCoords(int& outX, int& outY,
                                            int screenX, int screenY,
                                            const Camera3D& camera,
                                            const Mesh& mesh,
                                            const Matrix& transform,
                                            const HeightMap& map) const {
    // Create ray from camera through screen position
    Ray ray = GetMouseRay(Vector2{static_cast<float>(screenX), static_cast<float>(screenY)}, camera);

    // Test intersection with terrain mesh
    RayCollision collision = GetRayCollisionMesh(ray, mesh, transform);

    if (!collision.hit) {
        static int missCount = 0;
        if (++missCount % 60 == 0) {  // Log every 60 misses
            std::cout << "Brush: Ray missed terrain (mesh vertices: " << mesh.vertexCount << ")" << std::endl;
        }
        return false;
    }

    static int hitCount = 0;
    if (++hitCount % 60 == 0) {  // Log every 60 hits
        std::cout << "Brush: Ray hit terrain at (" << collision.point.x << ", "
                  << collision.point.y << ", " << collision.point.z << ")" << std::endl;
    }

    // Convert 3D world position to heightmap coordinates
    // Assuming terrain is centered at origin and scaled to match heightmap dimensions

    // Get mesh dimensions (assuming square terrain)
    int meshWidth = mesh.vertexCount > 0 ? static_cast<int>(std::sqrt(mesh.vertexCount)) : 256;

    // World position is relative to terrain center
    // Convert to heightmap coordinates [0, width) × [0, height)
    float worldX = collision.point.x;
    float worldZ = collision.point.z;

    // Calculate heightmap coordinates
    // Assuming terrain spans from -meshWidth/2 to +meshWidth/2 in world space
    float normalizedX = (worldX / static_cast<float>(meshWidth)) + 0.5f;  // 0 to 1
    float normalizedZ = (worldZ / static_cast<float>(meshWidth)) + 0.5f;  // 0 to 1

    outX = static_cast<int>(normalizedX * map.getWidth());
    outY = static_cast<int>(normalizedZ * map.getHeight());

    // Clamp to valid range
    outX = std::max(0, std::min(outX, map.getWidth() - 1));
    outY = std::max(0, std::min(outY, map.getHeight() - 1));

    return true;
}

bool BrushManager::getCursorPosition(int& outX, int& outY,
                                      const Vector2& mousePos,
                                      const Camera3D& camera,
                                      const Mesh& mesh,
                                      const Matrix& transform,
                                      const HeightMap& map) const {
    return screenToHeightMapCoords(
        outX, outY,
        static_cast<int>(mousePos.x),
        static_cast<int>(mousePos.y),
        camera, mesh, transform, map
    );
}
#endif // YMIRGE_UI_ENABLED
