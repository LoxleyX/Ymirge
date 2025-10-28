#include "PolygonSelection.h"
#include <algorithm>
#include <cmath>
#include <limits>

PolygonSelection::PolygonSelection()
    : closed_(false) {
}

void PolygonSelection::addVertex(float x, float y) {
    if (!closed_) {
        vertices_.push_back(glm::vec2(x, y));
    }
}

void PolygonSelection::removeLastVertex() {
    if (!vertices_.empty() && !closed_) {
        vertices_.pop_back();
    }
}

void PolygonSelection::clear() {
    vertices_.clear();
    closed_ = false;
}

void PolygonSelection::close() {
    if (vertices_.size() >= 3) {
        closed_ = true;
    }
}

bool PolygonSelection::isPointInside(float x, float y) const {
    if (vertices_.size() < 3 || !closed_) {
        return false;
    }

    // Ray casting algorithm: count intersections with edges
    // Odd = inside, Even = outside
    int intersections = 0;
    glm::vec2 point(x, y);

    for (size_t i = 0; i < vertices_.size(); ++i) {
        glm::vec2 v1 = vertices_[i];
        glm::vec2 v2 = vertices_[(i + 1) % vertices_.size()];

        // Check if ray from point to +X intersects edge
        if ((v1.y > y) != (v2.y > y)) {
            // Calculate X intersection point
            float xIntersect = (v2.x - v1.x) * (y - v1.y) / (v2.y - v1.y) + v1.x;

            if (x < xIntersect) {
                intersections++;
            }
        }
    }

    return (intersections % 2) == 1;
}

HeightMap PolygonSelection::generateMask(int width, int height) const {
    HeightMap mask(width, height);

    // Fill with 0 (not selected)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float value = isPointInside(static_cast<float>(x), static_cast<float>(y)) ? 1.0f : 0.0f;
            mask.at(x, y) = value;
        }
    }

    return mask;
}

HeightMap PolygonSelection::generateFeatheredMask(int width, int height, float featherRadius) const {
    HeightMap mask(width, height);

    if (vertices_.size() < 3 || !closed_) {
        return mask;  // Empty mask
    }

    // Get polygon bounds for optimization
    float minX, minY, maxX, maxY;
    getBounds(minX, minY, maxX, maxY);

    // Expand bounds by feather radius
    int startX = std::max(0, static_cast<int>(minX - featherRadius));
    int endX = std::min(width - 1, static_cast<int>(maxX + featherRadius));
    int startY = std::max(0, static_cast<int>(minY - featherRadius));
    int endY = std::min(height - 1, static_cast<int>(maxY + featherRadius));

    // Calculate distance field and feather
    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            float dist = signedDistanceToPolygon(static_cast<float>(x), static_cast<float>(y));

            float value;
            if (dist <= 0.0f) {
                // Inside polygon
                value = 1.0f;
            } else if (dist < featherRadius) {
                // Feather zone (smooth falloff)
                value = 1.0f - (dist / featherRadius);
                // Smoothstep for smoother transition
                value = value * value * (3.0f - 2.0f * value);
            } else {
                // Outside feather zone
                value = 0.0f;
            }

            mask.at(x, y) = value;
        }
    }

    return mask;
}

void PolygonSelection::getBounds(float& minX, float& minY, float& maxX, float& maxY) const {
    if (vertices_.empty()) {
        minX = minY = maxX = maxY = 0.0f;
        return;
    }

    minX = maxX = vertices_[0].x;
    minY = maxY = vertices_[0].y;

    for (const auto& v : vertices_) {
        minX = std::min(minX, v.x);
        maxX = std::max(maxX, v.x);
        minY = std::min(minY, v.y);
        maxY = std::max(maxY, v.y);
    }
}

float PolygonSelection::signedDistanceToPolygon(float x, float y) const {
    if (vertices_.size() < 3) {
        return std::numeric_limits<float>::max();
    }

    glm::vec2 point(x, y);
    float minDist = std::numeric_limits<float>::max();

    // Find distance to closest edge
    for (size_t i = 0; i < vertices_.size(); ++i) {
        glm::vec2 v1 = vertices_[i];
        glm::vec2 v2 = vertices_[(i + 1) % vertices_.size()];

        // Calculate distance from point to line segment
        glm::vec2 edge = v2 - v1;
        glm::vec2 toPoint = point - v1;

        float edgeLengthSq = glm::dot(edge, edge);
        float t = 0.0f;

        if (edgeLengthSq > 0.0f) {
            t = glm::clamp(glm::dot(toPoint, edge) / edgeLengthSq, 0.0f, 1.0f);
        }

        glm::vec2 closest = v1 + t * edge;
        float dist = glm::length(point - closest);

        minDist = std::min(minDist, dist);
    }

    // Return signed distance (negative if inside, positive if outside)
    return isPointInside(x, y) ? -minDist : minDist;
}
