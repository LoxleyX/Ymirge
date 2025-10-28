#include "LayerThumbnail.h"
#include <algorithm>
#include <cmath>

LayerThumbnail::LayerThumbnail()
    : textureID_(0) {
}

LayerThumbnail::~LayerThumbnail() {
    clear();
}

void LayerThumbnail::update(const HeightMap& heightMap) {
    // Allocate thumbnail buffer (RGBA, 8-bit per channel)
    const int bufferSize = THUMBNAIL_SIZE * THUMBNAIL_SIZE * 4;
    unsigned char* buffer = new unsigned char[bufferSize];

    // Downsample heightmap to thumbnail
    downsample(heightMap, buffer);

    // Create OpenGL texture if not yet created
    if (textureID_ == 0) {
        glGenTextures(1, &textureID_);
    }

    // Upload to GPU
    glBindTexture(GL_TEXTURE_2D, textureID_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, THUMBNAIL_SIZE, THUMBNAIL_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

    // Set texture parameters (linear filtering for smooth appearance)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Free buffer
    delete[] buffer;
}

void LayerThumbnail::clear() {
    if (textureID_ != 0) {
        glDeleteTextures(1, &textureID_);
        textureID_ = 0;
    }
}

void LayerThumbnail::downsample(const HeightMap& source, unsigned char* dest) {
    int srcWidth = source.getWidth();
    int srcHeight = source.getHeight();

    float scaleX = static_cast<float>(srcWidth) / THUMBNAIL_SIZE;
    float scaleY = static_cast<float>(srcHeight) / THUMBNAIL_SIZE;

    for (int y = 0; y < THUMBNAIL_SIZE; ++y) {
        for (int x = 0; x < THUMBNAIL_SIZE; ++x) {
            // Map thumbnail pixel to source coordinates
            float srcX = x * scaleX;
            float srcY = y * scaleY;

            // Bilinear filtering
            int x0 = static_cast<int>(srcX);
            int y0 = static_cast<int>(srcY);
            int x1 = std::min<int>(x0 + 1, srcWidth - 1);
            int y1 = std::min<int>(y0 + 1, srcHeight - 1);

            float fx = srcX - x0;
            float fy = srcY - y0;

            // Sample 4 nearest pixels
            float h00 = source.sample(x0, y0);
            float h10 = source.sample(x1, y0);
            float h01 = source.sample(x0, y1);
            float h11 = source.sample(x1, y1);

            // Bilinear interpolation
            float h0 = h00 * (1.0f - fx) + h10 * fx;
            float h1 = h01 * (1.0f - fx) + h11 * fx;
            float height = h0 * (1.0f - fy) + h1 * fy;

            // Clamp to [0, 1]
            height = (std::max)(0.0f, (std::min)(1.0f, height));

            // Convert to color
            int idx = (y * THUMBNAIL_SIZE + x) * 4;
            heightToColor(height, dest[idx], dest[idx + 1], dest[idx + 2], dest[idx + 3]);
        }
    }
}

void LayerThumbnail::heightToColor(float heightValue, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) {
    // Simple grayscale gradient with height-based coloring
    // Low = dark blue-gray, Mid = green-brown, High = white

    if (heightValue < 0.3f) {
        // Deep water / valleys (dark blue-gray)
        float t = heightValue / 0.3f;
        r = static_cast<unsigned char>(30 + t * 50);
        g = static_cast<unsigned char>(40 + t * 80);
        b = static_cast<unsigned char>(60 + t * 100);
    } else if (heightValue < 0.6f) {
        // Mid elevations (green-brown)
        float t = (heightValue - 0.3f) / 0.3f;
        r = static_cast<unsigned char>(80 + t * 80);
        g = static_cast<unsigned char>(120 + t * 60);
        b = static_cast<unsigned char>(60 + t * 40);
    } else {
        // High elevations (white peaks)
        float t = (heightValue - 0.6f) / 0.4f;
        r = static_cast<unsigned char>(160 + t * 95);
        g = static_cast<unsigned char>(180 + t * 75);
        b = static_cast<unsigned char>(100 + t * 155);
    }

    a = 255;  // Fully opaque
}
