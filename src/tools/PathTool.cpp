#include "PathTool.h"
#include <cmath>
#include <algorithm>

PathTool::PathTool() {
}

void PathTool::addPoint(float x, float y) {
    controlPoints_.push_back(glm::vec2(x, y));
}

void PathTool::removeLastPoint() {
    if (!controlPoints_.empty()) {
        controlPoints_.pop_back();
    }
}

void PathTool::clear() {
    controlPoints_.clear();
}

std::vector<glm::vec2> PathTool::generateSpline(float smoothness) const {
    if (controlPoints_.size() < 2) {
        return controlPoints_;  // Not enough points for spline
    }

    if (controlPoints_.size() == 2) {
        // Linear interpolation for 2 points
        std::vector<glm::vec2> result;
        glm::vec2 p0 = controlPoints_[0];
        glm::vec2 p1 = controlPoints_[1];
        int steps = static_cast<int>(glm::length(p1 - p0) * smoothness / 10.0f);
        steps = std::max(10, steps);

        for (int i = 0; i <= steps; ++i) {
            float t = static_cast<float>(i) / steps;
            result.push_back(p0 + t * (p1 - p0));
        }

        return result;
    }

    // Catmull-Rom spline for 3+ points
    std::vector<glm::vec2> splinePoints;

    for (size_t i = 0; i < controlPoints_.size() - 1; ++i) {
        // Get control points for this segment
        glm::vec2 p0 = (i == 0) ? controlPoints_[i] : controlPoints_[i - 1];
        glm::vec2 p1 = controlPoints_[i];
        glm::vec2 p2 = controlPoints_[i + 1];
        glm::vec2 p3 = (i + 2 < controlPoints_.size()) ? controlPoints_[i + 2] : controlPoints_[i + 1];

        // Interpolate segment
        int steps = static_cast<int>(smoothness);
        for (int j = 0; j <= steps; ++j) {
            float t = static_cast<float>(j) / steps;
            glm::vec2 point = catmullRom(p0, p1, p2, p3, t);
            splinePoints.push_back(point);
        }
    }

    return splinePoints;
}

void PathTool::applyToHeightMap(HeightMap& heightMap, const Params& params) const {
    if (controlPoints_.size() < 2) {
        return;  // Need at least 2 points
    }

    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    // Generate smooth spline
    std::vector<glm::vec2> splinePoints = generateSpline(params.smoothness);

    if (splinePoints.empty()) {
        return;
    }

    // Calculate average height along path for flattening
    float pathHeight = params.autoFlatten ? getPathHeight(heightMap, splinePoints) : 0.0f;

    // Apply path effect to heightmap
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            glm::vec2 point(static_cast<float>(x), static_cast<float>(y));

            // Find closest distance to path
            float minDist = std::numeric_limits<float>::max();

            for (size_t i = 0; i < splinePoints.size() - 1; ++i) {
                float dist = distanceToSegment(point, splinePoints[i], splinePoints[i + 1]);
                minDist = std::min(minDist, dist);
            }

            // Calculate influence based on distance
            float influence = 0.0f;
            float totalWidth = params.width + params.falloff;

            if (minDist <= params.width) {
                // Inside path
                influence = 1.0f;
            } else if (minDist < totalWidth) {
                // Falloff zone
                float t = (minDist - params.width) / params.falloff;
                // Smoothstep for smooth transition
                influence = 1.0f - (t * t * (3.0f - 2.0f * t));
            }

            if (influence > 0.0f) {
                float currentHeight = heightMap.at(x, y);
                float targetHeight;

                switch (params.mode) {
                    case PathMode::CARVE:
                        targetHeight = currentHeight - params.depth;
                        break;

                    case PathMode::RAISE:
                        targetHeight = currentHeight + params.depth;
                        break;

                    case PathMode::FLATTEN:
                        targetHeight = pathHeight;
                        break;
                }

                // Blend based on influence
                float newHeight = currentHeight + (targetHeight - currentHeight) * influence;
                heightMap.at(x, y) = newHeight;
            }
        }
    }
}

glm::vec2 PathTool::catmullRom(const glm::vec2& p0, const glm::vec2& p1,
                                 const glm::vec2& p2, const glm::vec2& p3, float t) {
    // Catmull-Rom spline matrix
    float t2 = t * t;
    float t3 = t2 * t;

    float q0 = -t3 + 2.0f * t2 - t;
    float q1 = 3.0f * t3 - 5.0f * t2 + 2.0f;
    float q2 = -3.0f * t3 + 4.0f * t2 + t;
    float q3 = t3 - t2;

    return 0.5f * (p0 * q0 + p1 * q1 + p2 * q2 + p3 * q3);
}

float PathTool::distanceToSegment(const glm::vec2& point, const glm::vec2& a, const glm::vec2& b) {
    glm::vec2 ab = b - a;
    glm::vec2 ap = point - a;

    float abLengthSq = glm::dot(ab, ab);

    if (abLengthSq == 0.0f) {
        // Degenerate segment (point)
        return glm::length(ap);
    }

    // Project point onto line
    float t = glm::clamp(glm::dot(ap, ab) / abLengthSq, 0.0f, 1.0f);

    // Find closest point on segment
    glm::vec2 closest = a + t * ab;

    return glm::length(point - closest);
}

float PathTool::getPathHeight(const HeightMap& heightMap, const std::vector<glm::vec2>& splinePoints) const {
    if (splinePoints.empty()) {
        return 0.0f;
    }

    // Sample height along path and average
    float sumHeight = 0.0f;
    int samples = 0;
    int step = std::max(1, static_cast<int>(splinePoints.size() / 20));  // Sample ~20 points

    for (size_t i = 0; i < splinePoints.size(); i += step) {
        int x = static_cast<int>(splinePoints[i].x);
        int y = static_cast<int>(splinePoints[i].y);

        if (x >= 0 && x < heightMap.getWidth() && y >= 0 && y < heightMap.getHeight()) {
            sumHeight += heightMap.sample(x, y);
            samples++;
        }
    }

    return (samples > 0) ? (sumHeight / samples) : 0.0f;
}
