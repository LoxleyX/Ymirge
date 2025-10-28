#pragma once

#ifdef YMIRGE_SDL_UI_ENABLED

#include "HeightMap.h"
#include "Camera3D.h"
#include "Shader.h"
#include <glad/glad.h>
#include <vector>
#include <memory>

// OpenGL 3.3+ terrain renderer with shader-based lighting and elevation colors
class TerrainRendererGL {
public:
    TerrainRendererGL(int width, int height);
    ~TerrainRendererGL();

    void updateTexture(const HeightMap& heightMap, bool monochrome);
    void updateCamera(int mouseX, int mouseY, bool leftButton, bool rightButton, float scrollDelta);
    void resetCamera();
    void render(int viewportX, int viewportY, int viewportWidth, int viewportHeight);
    void setSeaLevel(float level);

    // Convert screen coordinates to heightmap coordinates via raycasting
    bool screenToHeightMapCoords(int& outX, int& outY,
                                  int screenX, int screenY,
                                  int viewportX, int viewportY,
                                  int viewportWidth, int viewportHeight,
                                  const HeightMap& heightMap);

    void renderBrushCursor(int heightMapX, int heightMapY, int radius,
                           const HeightMap& heightMap,
                           int viewportWidth, int viewportHeight);

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    Camera3D& getCamera() { return camera_; }

private:
    void createMesh(const HeightMap& heightMap, bool monochrome);
    void createSeaPlane();
    glm::vec4 getTerrainColor(float height);

    int width_, height_;
    Camera3D camera_;
    std::unique_ptr<Shader> shader_;

    unsigned int terrainVAO_;
    unsigned int terrainVBO_;
    unsigned int terrainEBO_;
    unsigned int terrainVertexCount_;
    unsigned int terrainIndexCount_;
    bool meshLoaded_;

    std::vector<glm::vec3> meshVertices_;
    std::vector<unsigned int> meshIndices_;
    int meshWidth_;
    int meshHeight_;
    float terrainWidth_;
    float terrainDepth_;
    float terrainHeight_;

    unsigned int seaVAO_;
    unsigned int seaVBO_;
    unsigned int seaEBO_;
    unsigned int seaIndexCount_;
    float seaLevel_;
    bool seaPlaneLoaded_;
};

#endif // YMIRGE_SDL_UI_ENABLED
