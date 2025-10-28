#ifdef YMIRGE_SDL_UI_ENABLED

#include "TerrainRendererGL.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>

// Undefine Windows min/max macros that conflict with std::min/max
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// Vertex structure for terrain mesh
struct TerrainVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;
};

TerrainRendererGL::TerrainRendererGL(int width, int height)
    : width_(width)
    , height_(height)
    , terrainVAO_(0)
    , terrainVBO_(0)
    , terrainEBO_(0)
    , terrainVertexCount_(0)
    , terrainIndexCount_(0)
    , meshLoaded_(false)
    , seaVAO_(0)
    , seaVBO_(0)
    , seaEBO_(0)
    , seaIndexCount_(0)
    , seaLevel_(0.25f)
    , seaPlaneLoaded_(false) {

    // Create shader
    shader_ = std::make_unique<Shader>("shaders/terrain.vert", "shaders/terrain.frag");

    // Initialize OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    std::cout << "TerrainRendererGL initialized" << std::endl;
}

TerrainRendererGL::~TerrainRendererGL() {
    if (meshLoaded_) {
        glDeleteVertexArrays(1, &terrainVAO_);
        glDeleteBuffers(1, &terrainVBO_);
        glDeleteBuffers(1, &terrainEBO_);
    }

    if (seaPlaneLoaded_) {
        glDeleteVertexArrays(1, &seaVAO_);
        glDeleteBuffers(1, &seaVBO_);
        glDeleteBuffers(1, &seaEBO_);
    }
}

void TerrainRendererGL::updateTexture(const HeightMap& heightMap, bool monochrome) {
    createMesh(heightMap, monochrome);
    createSeaPlane();
}

void TerrainRendererGL::updateCamera(int mouseX, int mouseY, bool leftButton, bool rightButton, float scrollDelta) {
    camera_.update(mouseX, mouseY, leftButton, rightButton, scrollDelta);
}

void TerrainRendererGL::resetCamera() {
    camera_.reset();
}

void TerrainRendererGL::setSeaLevel(float level) {
    seaLevel_ = level;
}

glm::vec4 TerrainRendererGL::getTerrainColor(float height) {
    struct ColorStop {
        float height;
        glm::vec3 color;
    };

    static const ColorStop colors[] = {
        {0.0f,  glm::vec3(15, 30, 80) / 255.0f},
        {0.2f,  glm::vec3(25, 60, 120) / 255.0f},
        {0.25f, glm::vec3(194, 178, 128) / 255.0f},
        {0.3f,  glm::vec3(80, 120, 50) / 255.0f},
        {0.45f, glm::vec3(60, 100, 40) / 255.0f},
        {0.6f,  glm::vec3(90, 90, 50) / 255.0f},
        {0.75f, glm::vec3(100, 80, 60) / 255.0f},
        {0.85f, glm::vec3(130, 120, 110) / 255.0f},
        {1.0f,  glm::vec3(240, 240, 250) / 255.0f}      // Snow peaks
    };

    constexpr int numColors = sizeof(colors) / sizeof(ColorStop);

    int lowerIdx = 0;
    int upperIdx = numColors - 1;

    for (int i = 0; i < numColors - 1; ++i) {
        if (height >= colors[i].height && height <= colors[i + 1].height) {
            lowerIdx = i;
            upperIdx = i + 1;
            break;
        }
    }

    const ColorStop& lower = colors[lowerIdx];
    const ColorStop& upper = colors[upperIdx];

    float range = upper.height - lower.height;
    float t = (range < 0.001f) ? 0.0f : (height - lower.height) / range;

    glm::vec3 color = glm::mix(lower.color, upper.color, t);
    return glm::vec4(color, 1.0f);
}

void TerrainRendererGL::createMesh(const HeightMap& heightMap, bool monochrome) {
    // Clean up old mesh
    if (meshLoaded_) {
        glDeleteVertexArrays(1, &terrainVAO_);
        glDeleteBuffers(1, &terrainVBO_);
        glDeleteBuffers(1, &terrainEBO_);
        meshLoaded_ = false;
    }

    // Downsample to max 256x256 (same as raylib version)
    int mapWidth = std::min(256, heightMap.getWidth());
    int mapHeight = std::min(256, heightMap.getHeight());

    float scaleX = static_cast<float>(heightMap.getWidth()) / mapWidth;
    float scaleZ = static_cast<float>(heightMap.getHeight()) / mapHeight;

    // Terrain dimensions
    terrainWidth_ = 256.0f;
    terrainDepth_ = 256.0f;
    terrainHeight_ = 40.0f;  // Reduced from 100 for more reasonable proportions
    meshWidth_ = mapWidth;
    meshHeight_ = mapHeight;

    // Create vertices
    std::vector<TerrainVertex> vertices;
    vertices.reserve(mapWidth * mapHeight);

    // Store positions for raycasting
    meshVertices_.clear();
    meshVertices_.reserve(mapWidth * mapHeight);

    for (int z = 0; z < mapHeight; z++) {
        for (int x = 0; x < mapWidth; x++) {
            // Sample heightmap
            int srcX = static_cast<int>(x * scaleX);
            int srcY = static_cast<int>(z * scaleZ);
            srcX = std::clamp(srcX, 0, heightMap.getWidth() - 1);
            srcY = std::clamp(srcY, 0, heightMap.getHeight() - 1);
            float height = heightMap.sample(srcX, srcY);

            TerrainVertex v;
            v.position = glm::vec3(
                (x / (float)(mapWidth - 1) - 0.5f) * terrainWidth_,
                height * terrainHeight_,
                (z / (float)(mapHeight - 1) - 0.5f) * terrainDepth_
            );

            // Color
            if (monochrome) {
                v.color = glm::vec4(height, height, height, 1.0f);
            } else {
                v.color = getTerrainColor(height);
            }

            // Normal (will be calculated later)
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);

            vertices.push_back(v);
            meshVertices_.push_back(v.position);  // Store for raycasting
        }
    }

    // Create indices
    std::vector<unsigned int> indices;
    indices.reserve((mapWidth - 1) * (mapHeight - 1) * 6);

    for (int z = 0; z < mapHeight - 1; z++) {
        for (int x = 0; x < mapWidth - 1; x++) {
            unsigned int topLeft = z * mapWidth + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * mapWidth + x;
            unsigned int bottomRight = bottomLeft + 1;

            // Triangle 1
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Triangle 2
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    // Store indices for raycasting
    meshIndices_ = indices;

    // Calculate normals
    std::vector<glm::vec3> normals(vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        glm::vec3 v0 = vertices[i0].position;
        glm::vec3 v1 = vertices[i1].position;
        glm::vec3 v2 = vertices[i2].position;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        normals[i0] += normal;
        normals[i1] += normal;
        normals[i2] += normal;
    }

    // Normalize accumulated normals
    for (size_t i = 0; i < vertices.size(); i++) {
        vertices[i].normal = glm::normalize(normals[i]);
    }

    // Create OpenGL buffers
    glGenVertexArrays(1, &terrainVAO_);
    glGenBuffers(1, &terrainVBO_);
    glGenBuffers(1, &terrainEBO_);

    glBindVertexArray(terrainVAO_);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(TerrainVertex), vertices.data(), GL_STATIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, position));
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, normal));
    glEnableVertexAttribArray(1);

    // Color attribute
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    terrainVertexCount_ = vertices.size();
    terrainIndexCount_ = indices.size();
    meshLoaded_ = true;

    std::cout << "Terrain mesh created: " << mapWidth << "x" << mapHeight
              << " (" << terrainVertexCount_ << " vertices, " << terrainIndexCount_ << " indices)" << std::endl;
}

void TerrainRendererGL::createSeaPlane() {
    // Clean up old sea plane
    if (seaPlaneLoaded_) {
        glDeleteVertexArrays(1, &seaVAO_);
        glDeleteBuffers(1, &seaVBO_);
        glDeleteBuffers(1, &seaEBO_);
    }

    // Create a simple plane for the sea
    float planeSize = 256.0f;
    float seaHeight = seaLevel_ * 40.0f;  // Match terrain height scaling

    // Water color: semi-transparent blue-green
    TerrainVertex vertices[] = {
        {{-planeSize / 2, seaHeight, -planeSize / 2}, {0.0f, 1.0f, 0.0f}, {0.1f, 0.5f, 0.8f, 0.5f}},
        {{ planeSize / 2, seaHeight, -planeSize / 2}, {0.0f, 1.0f, 0.0f}, {0.1f, 0.5f, 0.8f, 0.5f}},
        {{ planeSize / 2, seaHeight,  planeSize / 2}, {0.0f, 1.0f, 0.0f}, {0.1f, 0.5f, 0.8f, 0.5f}},
        {{-planeSize / 2, seaHeight,  planeSize / 2}, {0.0f, 1.0f, 0.0f}, {0.1f, 0.5f, 0.8f, 0.5f}}
    };

    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    glGenVertexArrays(1, &seaVAO_);
    glGenBuffers(1, &seaVBO_);
    glGenBuffers(1, &seaEBO_);

    glBindVertexArray(seaVAO_);

    glBindBuffer(GL_ARRAY_BUFFER, seaVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, seaEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    seaIndexCount_ = 6;
    seaPlaneLoaded_ = true;
}

void TerrainRendererGL::render(int viewportX, int viewportY, int viewportWidth, int viewportHeight) {
    if (!meshLoaded_) {
        return;
    }

    // Set viewport
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);

    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use shader
    shader_->use();

    // Set matrices
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera_.getViewMatrix();
    glm::mat4 projection = camera_.getProjectionMatrix((float)viewportWidth / viewportHeight);

    shader_->setMat4("model", model);
    shader_->setMat4("view", view);
    shader_->setMat4("projection", projection);

    // Set lighting uniforms
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
    shader_->setVec3("lightDir", lightDir);
    shader_->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    shader_->setVec3("viewPos", camera_.getPosition());
    shader_->setFloat("ambientStrength", 0.6f);  // Increased for brighter terrain

    // Render terrain
    glBindVertexArray(terrainVAO_);
    glDrawElements(GL_TRIANGLES, terrainIndexCount_, GL_UNSIGNED_INT, 0);

    // Render sea plane with blending
    if (seaPlaneLoaded_ && seaLevel_ > 0.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);  // Don't write to depth buffer

        glBindVertexArray(seaVAO_);
        glDrawElements(GL_TRIANGLES, seaIndexCount_, GL_UNSIGNED_INT, 0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    glBindVertexArray(0);
}

// Helper function for ray-triangle intersection
static bool rayIntersectsTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                   const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                                   float& t) {
    const float EPSILON = 0.0000001f;

    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(rayDir, edge2);
    float a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON) {
        return false;  // Ray is parallel to triangle
    }

    float f = 1.0f / a;
    glm::vec3 s = rayOrigin - v0;
    float u = f * glm::dot(s, h);

    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(rayDir, q);

    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    t = f * glm::dot(edge2, q);
    return t > EPSILON;  // Ray intersection
}

bool TerrainRendererGL::screenToHeightMapCoords(int& outX, int& outY,
                                                  int screenX, int screenY,
                                                  int viewportX, int viewportY,
                                                  int viewportWidth, int viewportHeight,
                                                  const HeightMap& heightMap) {
    if (!meshLoaded_ || meshVertices_.empty()) {
        return false;
    }

    // Convert screen coords to normalized device coordinates
    float x = (2.0f * (screenX - viewportX)) / viewportWidth - 1.0f;
    float y = 1.0f - (2.0f * (screenY - viewportY)) / viewportHeight;

    // Get view and projection matrices
    glm::mat4 view = camera_.getViewMatrix();
    glm::mat4 projection = camera_.getProjectionMatrix((float)viewportWidth / viewportHeight);
    glm::mat4 invVP = glm::inverse(projection * view);

    // Unproject near and far points
    glm::vec4 rayClipNear = glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 rayClipFar = glm::vec4(x, y, 1.0f, 1.0f);

    glm::vec4 rayWorldNear = invVP * rayClipNear;
    glm::vec4 rayWorldFar = invVP * rayClipFar;

    // Perspective divide
    rayWorldNear /= rayWorldNear.w;
    rayWorldFar /= rayWorldFar.w;

    // Create ray from near to far
    glm::vec3 rayOrigin = glm::vec3(rayWorldNear);
    glm::vec3 rayDir = glm::normalize(glm::vec3(rayWorldFar) - glm::vec3(rayWorldNear));

    // Test ray against all triangles
    float closestT = FLT_MAX;
    bool hit = false;

    for (size_t i = 0; i < meshIndices_.size(); i += 3) {
        const glm::vec3& v0 = meshVertices_[meshIndices_[i]];
        const glm::vec3& v1 = meshVertices_[meshIndices_[i + 1]];
        const glm::vec3& v2 = meshVertices_[meshIndices_[i + 2]];

        float t;
        if (rayIntersectsTriangle(rayOrigin, rayDir, v0, v1, v2, t)) {
            if (t < closestT) {
                closestT = t;
                hit = true;
            }
        }
    }

    if (hit) {
        // Get intersection point
        glm::vec3 hitPoint = rayOrigin + rayDir * closestT;

        // Convert world position to heightmap coordinates
        float normalizedX = (hitPoint.x / terrainWidth_) + 0.5f;
        float normalizedZ = (hitPoint.z / terrainDepth_) + 0.5f;

        outX = static_cast<int>(normalizedX * heightMap.getWidth());
        outY = static_cast<int>(normalizedZ * heightMap.getHeight());

        // Clamp to valid range
        outX = std::clamp(outX, 0, heightMap.getWidth() - 1);
        outY = std::clamp(outY, 0, heightMap.getHeight() - 1);

        return true;
    }

    return false;
}

void TerrainRendererGL::renderBrushCursor(int heightMapX, int heightMapY, int radius,
                                           const HeightMap& heightMap,
                                           int viewportWidth, int viewportHeight) {
    if (!meshLoaded_) {
        return;
    }

    // Get world position from heightmap coordinates
    float normalizedX = (heightMapX / (float)heightMap.getWidth()) - 0.5f;
    float normalizedZ = (heightMapY / (float)heightMap.getHeight()) - 0.5f;
    float worldX = normalizedX * terrainWidth_;
    float worldZ = normalizedZ * terrainDepth_;
    float worldY = heightMap.at(heightMapX, heightMapY) * terrainHeight_;

    // Draw circle at cursor position using line loop
    shader_->use();

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera_.getViewMatrix();
    glm::mat4 projection = camera_.getProjectionMatrix((float)viewportWidth / viewportHeight);

    shader_->setMat4("model", model);
    shader_->setMat4("view", view);
    shader_->setMat4("projection", projection);

    // Create circle vertices
    const int segments = 32;
    float radiusWorld = (radius / (float)heightMap.getWidth()) * terrainWidth_;
    std::vector<glm::vec3> circleVerts;

    for (int i = 0; i <= segments; i++) {
        float angle = (i / (float)segments) * 2.0f * 3.14159f;
        float x = worldX + cos(angle) * radiusWorld;
        float z = worldZ + sin(angle) * radiusWorld;
        circleVerts.push_back(glm::vec3(x, worldY + 0.5f, z));  // Slightly above terrain
    }

    // Create temporary VAO for cursor
    unsigned int cursorVAO, cursorVBO;
    glGenVertexArrays(1, &cursorVAO);
    glGenBuffers(1, &cursorVBO);

    glBindVertexArray(cursorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cursorVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVerts.size() * sizeof(glm::vec3), circleVerts.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Render as green line loop
    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.0f);

    // Override shader color to green for cursor
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));
    shader_->setVec3("lightDir", lightDir);
    shader_->setVec3("lightColor", glm::vec3(0.0f, 1.0f, 0.0f));
    shader_->setVec3("viewPos", camera_.getPosition());
    shader_->setFloat("ambientStrength", 1.0f);  // Full bright for cursor

    glDrawArrays(GL_LINE_LOOP, 0, segments + 1);

    glEnable(GL_DEPTH_TEST);
    glLineWidth(1.0f);

    // Cleanup
    glDeleteBuffers(1, &cursorVBO);
    glDeleteVertexArrays(1, &cursorVAO);
}

#endif // YMIRGE_SDL_UI_ENABLED
