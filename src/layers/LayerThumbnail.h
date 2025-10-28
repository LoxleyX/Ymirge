#pragma once

#include "HeightMap.h"
#include <glad/glad.h>

/**
 * LayerThumbnail
 *
 * Generates and caches 64x64 thumbnails of layer heightmaps for UI display.
 * Thumbnails are rendered as OpenGL textures for use with ImGui::Image().
 */
class LayerThumbnail {
public:
    LayerThumbnail();
    ~LayerThumbnail();

    /**
     * Update thumbnail from heightmap
     *
     * Downsamples heightmap to 64x64 and uploads to GPU as OpenGL texture.
     *
     * @param heightMap Source heightmap (any size)
     */
    void update(const HeightMap& heightMap);

    /**
     * Get OpenGL texture ID for ImGui::Image()
     *
     * @return Texture ID (0 if not initialized)
     */
    GLuint getTextureID() const { return textureID_; }

    /**
     * Get thumbnail size
     */
    int getSize() const { return THUMBNAIL_SIZE; }

    /**
     * Check if thumbnail is valid
     */
    bool isValid() const { return textureID_ != 0; }

    /**
     * Clear thumbnail (free GPU memory)
     */
    void clear();

private:
    static constexpr int THUMBNAIL_SIZE = 64;  // 64x64 pixels

    GLuint textureID_;  // OpenGL texture ID

    /**
     * Downsample heightmap to thumbnail size using bilinear filtering
     *
     * @param source Source heightmap
     * @param dest Destination buffer (must be THUMBNAIL_SIZE * THUMBNAIL_SIZE * 4 bytes)
     */
    void downsample(const HeightMap& source, unsigned char* dest);

    /**
     * Convert float [0, 1] heightmap to grayscale RGBA bytes
     *
     * @param heightValue Height value [0, 1]
     * @param r, g, b, a Output color components
     */
    void heightToColor(float heightValue, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a);
};
