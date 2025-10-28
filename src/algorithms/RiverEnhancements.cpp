#include "RiverEnhancements.h"
#include <algorithm>
#include <cmath>
#include <random>

void RiverEnhancements::apply(HeightMap& heightMap, const Params& params, ThreadPool* pool) {
    (void)pool;  // Reserved for future parallel implementation

    if (params.intensity < 0.01f) return;

    int numRivers = static_cast<int>(params.intensity * 6) + 2;  // 2-8 main rivers

    // Calculate flow field for gradient-based rivers
    FlowField flowField = calculateFlowField(heightMap);

    // Find river sources and destinations
    auto sources = findRiverSources(heightMap, numRivers);
    auto destinations = findRiverDestinations(heightMap, numRivers);

    // Ensure we have at least some rivers
    size_t riverCount = std::min(sources.size(), destinations.size());

    std::vector<RiverPath> allRivers;

    // Generate main rivers
    for (size_t i = 0; i < riverCount; ++i) {
        RiverPath mainRiver = generateFlowBasedRiver(
            heightMap, flowField,
            sources[i], destinations[i],
            params);
        mainRiver.isMain = true;
        allRivers.push_back(mainRiver);

        // Generate tributaries for this main river
        if (params.enableTributaries && mainRiver.points.size() > 10) {
            // Branch points at 25%, 50%, 75% along main river
            std::vector<float> branchPositions = {0.25f, 0.5f, 0.75f};

            for (int t = 0; t < params.tributariesPerRiver && t < static_cast<int>(branchPositions.size()); ++t) {
                size_t branchIdx = static_cast<size_t>(mainRiver.points.size() * branchPositions[t]);
                if (branchIdx >= mainRiver.points.size()) branchIdx = mainRiver.points.size() - 1;

                RiverPath tributary = generateTributary(
                    heightMap, flowField,
                    mainRiver.points[branchIdx].pos,
                    params);
                tributary.isMain = false;
                allRivers.push_back(tributary);
            }
        }
    }

    // Carve all rivers
    for (const auto& river : allRivers) {
        float riverIntensity = river.isMain ? params.intensity : params.intensity * 0.5f;
        carveRiverPath(heightMap, river, riverIntensity);
    }

    // Apply wetlands
    if (params.enableWetlands) {
        for (const auto& river : allRivers) {
            if (river.isMain) {  // Only main rivers create wetlands
                applyWetlands(heightMap, river, params);
            }
        }
    }
}

RiverEnhancements::FlowField RiverEnhancements::calculateFlowField(const HeightMap& map) {
    int width = map.getWidth();
    int height = map.getHeight();

    FlowField field(width, height);

    // Calculate gradient at each point
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            // Central difference for gradient
            float gradX = (map.at(x + 1, y) - map.at(x - 1, y)) * 0.5f;
            float gradY = (map.at(x, y + 1) - map.at(x, y - 1)) * 0.5f;

            // Flow direction is negative gradient (downhill)
            glm::vec2 flowDir(-gradX, -gradY);

            // Normalize
            float len = glm::length(flowDir);
            if (len > 0.0001f) {
                flowDir /= len;
            }

            field.at(x, y) = flowDir;
        }
    }

    return field;
}

RiverEnhancements::RiverPath RiverEnhancements::generateFlowBasedRiver(
    const HeightMap& map,
    const FlowField& flowField,
    const glm::vec2& start,
    const glm::vec2& target,
    const Params& params) {

    RiverPath path;
    path.isMain = true;

    glm::vec2 currentPos = start;
    glm::vec2 currentDir(0.0f);

    const int maxSteps = 2000;
    const float stepSize = 2.0f;
    const float targetRadius = 15.0f;

    for (int step = 0; step < maxSteps; ++step) {
        // Add current point to path
        RiverPoint point;
        point.pos = currentPos;
        point.width = params.width;
        point.depth = 0.5f + (static_cast<float>(step) / maxSteps) * 0.5f;  // Depth increases
        path.points.push_back(point);

        // Check if reached target
        if (glm::distance(currentPos, target) < targetRadius) {
            break;
        }

        // Get flow direction at current position
        int x = static_cast<int>(currentPos.x);
        int y = static_cast<int>(currentPos.y);

        if (x < 1 || x >= map.getWidth() - 1 || y < 1 || y >= map.getHeight() - 1) {
            break;  // Out of bounds
        }

        glm::vec2 flowDir = flowField.at(x, y);

        // Direction toward target
        glm::vec2 targetDir = glm::normalize(target - currentPos);

        // Blend flow direction with target direction
        glm::vec2 desiredDir = glm::normalize(flowDir * 0.3f + targetDir * 0.7f);

        // Apply inertia (smooth direction changes)
        currentDir = glm::normalize(currentDir * params.flowSmoothing + desiredDir * (1.0f - params.flowSmoothing));

        // Move to next position
        currentPos += currentDir * stepSize;

        // Clamp to map bounds
        currentPos.x = std::clamp(currentPos.x, 1.0f, static_cast<float>(map.getWidth() - 2));
        currentPos.y = std::clamp(currentPos.y, 1.0f, static_cast<float>(map.getHeight() - 2));
    }

    // Smooth the path
    smoothPath(path, 0.5f);

    return path;
}

RiverEnhancements::RiverPath RiverEnhancements::generateTributary(
    const HeightMap& map,
    const FlowField& flowField,
    const glm::vec2& branchPoint,
    const Params& params) {

    RiverPath tributary;
    tributary.isMain = false;

    // Find a spawn point perpendicular to main river
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(-1.57f, 1.57f);  // -90° to +90°

    float angle = angleDist(gen);
    float spawnDist = 100.0f;

    glm::vec2 tributaryStart = branchPoint + glm::vec2(
        std::cos(angle) * spawnDist,
        std::sin(angle) * spawnDist);

    // Clamp to map
    tributaryStart.x = std::clamp(tributaryStart.x, 1.0f, static_cast<float>(map.getWidth() - 2));
    tributaryStart.y = std::clamp(tributaryStart.y, 1.0f, static_cast<float>(map.getHeight() - 2));

    // Flow from tributary start to branch point
    Params tributaryParams = params;
    tributaryParams.width *= params.tributaryWidth;

    tributary = generateFlowBasedRiver(map, flowField, tributaryStart, branchPoint, tributaryParams);
    tributary.isMain = false;

    return tributary;
}

void RiverEnhancements::carveRiverPath(
    HeightMap& map,
    const RiverPath& path,
    float intensity) {

    int mapWidth = map.getWidth();
    int mapHeight = map.getHeight();

    for (const auto& point : path.points) {
        int cx = static_cast<int>(point.pos.x);
        int cy = static_cast<int>(point.pos.y);

        float radius = point.width * 800.0f;  // Convert to pixels
        int iRadius = static_cast<int>(std::max(2.0f, radius));

        for (int dy = -iRadius; dy <= iRadius; ++dy) {
            for (int dx = -iRadius; dx <= iRadius; ++dx) {
                int x = cx + dx;
                int y = cy + dy;

                if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) continue;

                float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                if (dist > radius) continue;

                // Smooth falloff from center
                float falloff = std::pow(1.0f - (dist / radius), 1.8f);

                // Carve based on intensity and depth
                float carvingAmount = intensity * 0.12f * falloff * point.depth;

                // Lower the terrain (create river channel)
                map.at(x, y) = std::max(0.0f, map.at(x, y) - carvingAmount);
            }
        }
    }
}

void RiverEnhancements::applyWetlands(
    HeightMap& map,
    const RiverPath& path,
    const Params& params) {

    int mapWidth = map.getWidth();
    int mapHeight = map.getHeight();

    const float wetlandRadius = params.wetlandRadius;
    const float wetlandStrength = params.wetlandStrength;

    // For each point on the river, create moisture zone
    for (const auto& point : path.points) {
        int cx = static_cast<int>(point.pos.x);
        int cy = static_cast<int>(point.pos.y);

        int iRadius = static_cast<int>(wetlandRadius);

        for (int dy = -iRadius; dy <= iRadius; ++dy) {
            for (int dx = -iRadius; dx <= iRadius; ++dx) {
                int x = cx + dx;
                int y = cy + dy;

                if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) continue;

                float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                if (dist > wetlandRadius) continue;

                // Smooth gradient from river to dry land
                float moistureFactor = 1.0f - (dist / wetlandRadius);
                moistureFactor = moistureFactor * moistureFactor;  // Squared falloff

                // Slightly lower wetland areas (marshy depression)
                float lowering = wetlandStrength * 0.02f * moistureFactor;

                map.at(x, y) = std::max(0.0f, map.at(x, y) - lowering);
            }
        }
    }
}

std::vector<glm::vec2> RiverEnhancements::findRiverSources(
    const HeightMap& map,
    int numRivers) {

    std::vector<glm::vec2> sources;

    int width = map.getWidth();
    int height = map.getHeight();

    // Find high-elevation edge points (mountain springs)
    std::vector<glm::vec2> candidates;

    // Sample edges
    const int step = 20;

    // Top and bottom edges
    for (int x = 0; x < width; x += step) {
        candidates.push_back(glm::vec2(x, 0));
        candidates.push_back(glm::vec2(x, height - 1));
    }

    // Left and right edges
    for (int y = 0; y < height; y += step) {
        candidates.push_back(glm::vec2(0, y));
        candidates.push_back(glm::vec2(width - 1, y));
    }

    // Sort by elevation (prefer higher elevations as sources)
    std::sort(candidates.begin(), candidates.end(),
        [&map](const glm::vec2& a, const glm::vec2& b) {
            int ax = static_cast<int>(a.x);
            int ay = static_cast<int>(a.y);
            int bx = static_cast<int>(b.x);
            int by = static_cast<int>(b.y);
            return map.at(ax, ay) > map.at(bx, by);
        });

    // Select well-spaced sources
    float minSpacing = width * 0.25f;

    for (const auto& candidate : candidates) {
        if (sources.size() >= static_cast<size_t>(numRivers)) break;

        // Check spacing
        bool tooClose = false;
        for (const auto& existing : sources) {
            if (glm::distance(candidate, existing) < minSpacing) {
                tooClose = true;
                break;
            }
        }

        if (!tooClose) {
            sources.push_back(candidate);
        }
    }

    return sources;
}

std::vector<glm::vec2> RiverEnhancements::findRiverDestinations(
    const HeightMap& map,
    int numRivers) {

    std::vector<glm::vec2> destinations;

    int width = map.getWidth();
    int height = map.getHeight();

    // Find lowest valleys
    std::vector<glm::vec2> valleys;

    const int margin = static_cast<int>(width * 0.15f);
    const int step = 8;

    for (int y = margin; y < height - margin; y += step) {
        for (int x = margin; x < width - margin; x += step) {
            float h = map.at(x, y);

            if (h < 0.35f) {  // Low elevation
                valleys.push_back(glm::vec2(x, y));
            }
        }
    }

    // Sort by elevation (lowest first)
    std::sort(valleys.begin(), valleys.end(),
        [&map](const glm::vec2& a, const glm::vec2& b) {
            int ax = static_cast<int>(a.x);
            int ay = static_cast<int>(a.y);
            int bx = static_cast<int>(b.x);
            int by = static_cast<int>(b.y);
            return map.at(ax, ay) < map.at(bx, by);
        });

    // Select well-spaced destinations
    float minSpacing = width * 0.2f;

    for (const auto& valley : valleys) {
        if (destinations.size() >= static_cast<size_t>(numRivers)) break;

        bool tooClose = false;
        for (const auto& existing : destinations) {
            if (glm::distance(valley, existing) < minSpacing) {
                tooClose = true;
                break;
            }
        }

        if (!tooClose) {
            destinations.push_back(valley);
        }
    }

    return destinations;
}

void RiverEnhancements::smoothPath(RiverPath& path, float amount) {
    if (path.points.size() < 3) return;

    std::vector<RiverPoint> smoothed = path.points;

    // Multiple passes for smoother results
    for (int pass = 0; pass < 3; ++pass) {
        for (size_t i = 1; i < path.points.size() - 1; ++i) {
            glm::vec2 avgPos = (path.points[i - 1].pos + path.points[i].pos + path.points[i + 1].pos) / 3.0f;
            smoothed[i].pos = glm::mix(path.points[i].pos, avgPos, amount);
        }
        path.points = smoothed;
    }
}
