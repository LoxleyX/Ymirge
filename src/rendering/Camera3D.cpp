#ifdef YMIRGE_SDL_UI_ENABLED

#include "Camera3D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

Camera3D::Camera3D()
    : distance_(250.0f)
    , angleX_(45.0f)
    , angleY_(45.0f)
    , target_(0.0f, 0.0f, 0.0f)
    , lastMouseX_(0)
    , lastMouseY_(0)
    , isDragging_(false)
    , isPanning_(false)
    , fov_(45.0f)
    , nearPlane_(0.1f)
    , farPlane_(1000.0f) {
}

void Camera3D::update(int mouseX, int mouseY, bool leftButton, bool rightButton, float scrollDelta) {
    // Handle rotation (left mouse button)
    if (leftButton) {
        if (!isDragging_) {
            isDragging_ = true;
            lastMouseX_ = mouseX;
            lastMouseY_ = mouseY;
        } else {
            int deltaX = mouseX - lastMouseX_;
            int deltaY = mouseY - lastMouseY_;

            // Update angles
            angleY_ += deltaX * 0.3f;  // Horizontal rotation
            angleX_ -= deltaY * 0.3f;  // Vertical rotation (inverted)

            // Clamp vertical angle
            angleX_ = std::clamp(angleX_, -89.0f, 89.0f);

            lastMouseX_ = mouseX;
            lastMouseY_ = mouseY;
        }
    } else {
        isDragging_ = false;
    }

    // Handle panning (right mouse button)
    if (rightButton) {
        if (!isPanning_) {
            isPanning_ = true;
            lastMouseX_ = mouseX;
            lastMouseY_ = mouseY;
        } else {
            int deltaX = mouseX - lastMouseX_;
            int deltaY = mouseY - lastMouseY_;

            // Calculate right and up vectors
            float radX = glm::radians(angleX_);
            float radY = glm::radians(angleY_);

            glm::vec3 right(cosf(radY), 0.0f, -sinf(radY));
            glm::vec3 up(-sinf(radY) * sinf(radX), cosf(radX), -cosf(radY) * sinf(radX));

            // Pan speed scales with distance
            float panSpeed = distance_ * 0.002f;

            target_ -= right * (float)deltaX * panSpeed;
            target_ += up * (float)deltaY * panSpeed;

            lastMouseX_ = mouseX;
            lastMouseY_ = mouseY;
        }
    } else {
        isPanning_ = false;
    }

    // Handle zoom (mouse wheel)
    if (scrollDelta != 0.0f) {
        distance_ -= scrollDelta * 15.0f;
        distance_ = std::clamp(distance_, 50.0f, 500.0f);
    }
}

void Camera3D::reset() {
    distance_ = 250.0f;
    angleX_ = 45.0f;
    angleY_ = 45.0f;
    target_ = glm::vec3(0.0f, 0.0f, 0.0f);
    isDragging_ = false;
    isPanning_ = false;
}

glm::mat4 Camera3D::getViewMatrix() const {
    glm::vec3 position = getPosition();
    return glm::lookAt(position, target_, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera3D::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fov_), aspectRatio, nearPlane_, farPlane_);
}

glm::vec3 Camera3D::getPosition() const {
    float radX = glm::radians(angleX_);
    float radY = glm::radians(angleY_);

    float x = target_.x + distance_ * cosf(radX) * sinf(radY);
    float y = target_.y + distance_ * sinf(radX);
    float z = target_.z + distance_ * cosf(radX) * cosf(radY);

    return glm::vec3(x, y, z);
}

#endif // YMIRGE_SDL_UI_ENABLED
