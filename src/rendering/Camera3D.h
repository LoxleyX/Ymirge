#pragma once

#ifdef YMIRGE_SDL_UI_ENABLED

#include <glm/glm.hpp>

/**
 * 3D Camera
 *
 * Orbital camera system with rotation, panning, and zooming.
 * Similar interface to raylib's Camera3D but using GLM.
 */
class Camera3D {
public:
    Camera3D();

    /**
     * Update camera based on mouse input
     *
     * @param mouseX Current mouse X position
     * @param mouseY Current mouse Y position
     * @param leftButton Is left mouse button down
     * @param rightButton Is right mouse button down
     * @param scrollDelta Mouse wheel scroll delta
     */
    void update(int mouseX, int mouseY, bool leftButton, bool rightButton, float scrollDelta);

    /**
     * Reset camera to default position
     */
    void reset();

    /**
     * Get view matrix
     */
    glm::mat4 getViewMatrix() const;

    /**
     * Get projection matrix
     *
     * @param aspectRatio Width / height of viewport
     */
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    /**
     * Get camera position in world space
     */
    glm::vec3 getPosition() const;

    /**
     * Get camera target (look-at point)
     */
    glm::vec3 getTarget() const { return target_; }

private:
    // Camera state
    float distance_;        // Distance from target
    float angleX_;          // Vertical rotation angle (degrees)
    float angleY_;          // Horizontal rotation angle (degrees)
    glm::vec3 target_;      // Look-at point

    // Input state
    int lastMouseX_;
    int lastMouseY_;
    bool isDragging_;
    bool isPanning_;

    // Camera parameters
    float fov_;             // Field of view (degrees)
    float nearPlane_;
    float farPlane_;

    // Update internal position from angles
    void updatePosition();
};

#endif // YMIRGE_SDL_UI_ENABLED
