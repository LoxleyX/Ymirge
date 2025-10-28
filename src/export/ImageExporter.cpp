#include "ImageExporter.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>

// stb_image_write
#ifdef YMIRGE_SDL_UI_ENABLED
    // For SDL2 build: Define implementation here
    #define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
// For raylib build: Use raylib's implementation (don't define STB_IMAGE_WRITE_IMPLEMENTATION)
#include "stb_image_write.h"

// tinyexr for EXR export - only for raylib build (SDL2 build uses PNG only for now)
#ifndef YMIRGE_SDL_UI_ENABLED
    #define TINYEXR_USE_MINIZ 0
    #define TINYEXR_USE_STB_ZLIB 1
    #define TINYEXR_IMPLEMENTATION
    #include "tinyexr.h"
#endif

bool ImageExporter::exportHeightmap(const HeightMap& heightMap, const std::string& filename) {
    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    // Convert to 16-bit grayscale
    std::vector<uint16_t> pixels(width * height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            float value = heightMap.at(x, y);

            // Clamp and convert to 16-bit (0-65535)
            value = std::clamp(value, 0.0f, 1.0f);
            pixels[idx] = static_cast<uint16_t>(value * 65535.0f);
        }
    }

    // stb_image_write doesn't natively support 16-bit PNG writing
    // We'll write as 8-bit for now (common practice for heightmaps)
    std::vector<unsigned char> pixels8(width * height);
    for (int i = 0; i < width * height; ++i) {
        pixels8[i] = static_cast<unsigned char>((pixels[i] >> 8) & 0xFF);
    }

    int result = stbi_write_png(filename.c_str(), width, height, 1,
                                 pixels8.data(), width);

    if (result) {
        std::cout << "Heightmap exported to: " << filename << std::endl;
        return true;
    } else {
        std::cerr << "Failed to export heightmap to: " << filename << std::endl;
        return false;
    }
}

bool ImageExporter::exportHeightmap8bit(const HeightMap& heightMap, const std::string& filename) {
    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    // Convert to 8-bit grayscale
    std::vector<unsigned char> pixels(width * height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            float value = heightMap.at(x, y);

            // Clamp and convert to 8-bit (0-255)
            value = std::clamp(value, 0.0f, 1.0f);
            pixels[idx] = static_cast<unsigned char>(value * 255.0f);
        }
    }

    int result = stbi_write_png(filename.c_str(), width, height, 1,
                                 pixels.data(), width);

    if (result) {
        std::cout << "Heightmap (8-bit) exported to: " << filename << std::endl;
        return true;
    } else {
        std::cerr << "Failed to export heightmap to: " << filename << std::endl;
        return false;
    }
}

bool ImageExporter::exportSplatmap(const HeightMap& heightMap, const std::string& filename) {
    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    // RGBA splatmap
    std::vector<unsigned char> pixels(width * height * 4);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 4;

            float h = heightMap.at(x, y);
            float slope = calculateSlope(heightMap, x, y);

            unsigned char r, g, b, a;
            generateSplatmapPixel(h, slope, r, g, b, a);

            pixels[idx + 0] = r;  // Red: Sand
            pixels[idx + 1] = g;  // Green: Grass
            pixels[idx + 2] = b;  // Blue: Rock
            pixels[idx + 3] = a;  // Alpha: Snow
        }
    }

    int result = stbi_write_png(filename.c_str(), width, height, 4,
                                 pixels.data(), width * 4);

    if (result) {
        std::cout << "Splatmap exported to: " << filename << std::endl;
        return true;
    } else {
        std::cerr << "Failed to export splatmap to: " << filename << std::endl;
        return false;
    }
}

float ImageExporter::calculateSlope(const HeightMap& heightMap, int x, int y) {
    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    // Get neighboring heights
    float center = heightMap.at(x, y);

    float left = (x > 0) ? heightMap.at(x - 1, y) : center;
    float right = (x < width - 1) ? heightMap.at(x + 1, y) : center;
    float up = (y > 0) ? heightMap.at(x, y - 1) : center;
    float down = (y < height - 1) ? heightMap.at(x, y + 1) : center;

    // Calculate gradients
    float dx = (right - left) * 0.5f;
    float dy = (down - up) * 0.5f;

    // Slope magnitude
    float slope = std::sqrt(dx * dx + dy * dy);

    return slope;
}

void ImageExporter::generateSplatmapPixel(float height, float slope,
                                         unsigned char& outR,
                                         unsigned char& outG,
                                         unsigned char& outB,
                                         unsigned char& outA) {
    // Initialize all channels
    outR = 0;  // Sand
    outG = 0;  // Grass
    outB = 0;  // Rock
    outA = 255; // Always fully opaque (don't use alpha for data)

    // Slope thresholds
    const float GENTLE_SLOPE = 0.10f;  // Lowered for more variety
    const float STEEP_SLOPE = 0.20f;

    // Height thresholds (adjusted to work with normalized 0-1 range)
    const float SAND_LEVEL = 0.20f;    // Below this: sand
    const float GRASS_START = 0.20f;   // Grass starts here
    const float GRASS_END = 0.60f;     // Grass ends here
    const float ROCK_START = 0.60f;    // Rock starts here
    const float SNOW_LEVEL = 0.80f;    // Snow starts here

    // Priority system: Snow > Rock (slope) > Grass > Sand > Rock (elevation)

    // Snow (highest elevations) - Use BLUE channel for visibility
    if (height > SNOW_LEVEL) {
        outB = 255;  // Show as blue (easier to see than white in alpha)
        outR = 200;  // Add some white for snow effect
        outG = 200;
        return;
    }

    // Rock on steep slopes (any elevation)
    if (slope > STEEP_SLOPE) {
        outB = 255;
        return;
    }

    // Grass (medium elevation, gentle slopes)
    if (height >= GRASS_START && height <= GRASS_END && slope <= GENTLE_SLOPE) {
        outG = 255;
        return;
    }

    // Rock on medium slopes in grass zone
    if (height >= GRASS_START && height <= GRASS_END && slope > GENTLE_SLOPE) {
        outB = 200;
        return;
    }

    // Sand/Beach (low elevation)
    if (height < SAND_LEVEL) {
        outR = 255;
        return;
    }

    // Rock (high elevation but not snow)
    if (height > ROCK_START) {
        outB = 255;
        return;
    }

    // Default to grass
    outG = 255;
}

bool ImageExporter::exportHeightmapRAW16(const HeightMap& heightMap, const std::string& filename) {
    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    // Convert to 16-bit grayscale (big-endian for Unity/Unreal compatibility)
    std::vector<uint16_t> pixels(width * height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            float value = heightMap.at(x, y);

            // Clamp and convert to 16-bit (0-65535)
            value = std::clamp(value, 0.0f, 1.0f);
            uint16_t pixel = static_cast<uint16_t>(value * 65535.0f);

            // Convert to big-endian
            pixels[idx] = ((pixel & 0xFF) << 8) | ((pixel >> 8) & 0xFF);
        }
    }

    // Write raw binary file
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }

    file.write(reinterpret_cast<const char*>(pixels.data()), pixels.size() * sizeof(uint16_t));
    file.close();

    if (file.good()) {
        std::cout << "Heightmap (RAW16) exported to: " << filename << std::endl;
        std::cout << "  Format: 16-bit big-endian, " << width << "x" << height << std::endl;
        return true;
    } else {
        std::cerr << "Failed to write RAW16 heightmap to: " << filename << std::endl;
        return false;
    }
}

#ifndef YMIRGE_SDL_UI_ENABLED
// EXR export only available for raylib build (requires TinyEXR)
bool ImageExporter::exportHeightmapEXR(const HeightMap& heightMap, const std::string& filename) {
    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    // Convert to 32-bit float array
    std::vector<float> pixels(width * height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            pixels[idx] = heightMap.at(x, y);
        }
    }

    // EXR image descriptor
    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = 1;

    std::vector<float*> image_ptr(1);
    image_ptr[0] = pixels.data();

    image.images = reinterpret_cast<unsigned char**>(image_ptr.data());
    image.width = width;
    image.height = height;

    header.num_channels = 1;
    header.channels = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo) * header.num_channels);
    // Grayscale channel
#ifdef _WIN32
    strncpy_s(header.channels[0].name, 255, "Y", 1);
#else
    strncpy(header.channels[0].name, "Y", 255);
#endif

    header.pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
    header.pixel_types[0] = TINYEXR_PIXELTYPE_FLOAT;
    header.requested_pixel_types[0] = TINYEXR_PIXELTYPE_FLOAT;

    const char* err = nullptr;
    int ret = SaveEXRImageToFile(&image, &header, filename.c_str(), &err);

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);

    if (ret != TINYEXR_SUCCESS) {
        std::cerr << "Failed to export EXR heightmap: ";
        if (err) {
            std::cerr << err << std::endl;
            FreeEXRErrorMessage(err);
        } else {
            std::cerr << "Unknown error" << std::endl;
        }
        return false;
    }

    std::cout << "Heightmap (EXR) exported to: " << filename << std::endl;
    std::cout << "  Format: 32-bit float, " << width << "x" << height << std::endl;
    return true;
}
#endif // YMIRGE_SDL_UI_ENABLED

bool ImageExporter::exportMeshOBJ(const HeightMap& heightMap, const std::string& filename,
                                  int maxSize, float scaleXZ, float scaleY) {
    int width = heightMap.getWidth();
    int height = heightMap.getHeight();

    // Downsample if needed
    int step = 1;
    if (maxSize > 0 && (width > maxSize || height > maxSize)) {
        step = std::max(width / maxSize, height / maxSize);
    }

    int meshWidth = (width + step - 1) / step;
    int meshHeight = (height + step - 1) / step;

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }

    // Write header
    file << "# Ymirge Terrain Export\n";
    file << "# Vertices: " << (meshWidth * meshHeight) << "\n";
    file << "# Faces: " << ((meshWidth - 1) * (meshHeight - 1) * 2) << "\n\n";

    // Write vertices
    for (int y = 0; y < meshHeight; ++y) {
        for (int x = 0; x < meshWidth; ++x) {
            int srcX = std::min(x * step, width - 1);
            int srcY = std::min(y * step, height - 1);

            float h = heightMap.at(srcX, srcY);
            float posX = (x - meshWidth / 2.0f) * scaleXZ;
            float posY = h * scaleY;
            float posZ = (y - meshHeight / 2.0f) * scaleXZ;

            file << "v " << posX << " " << posY << " " << posZ << "\n";
        }
    }

    file << "\n";

    // Calculate and write normals
    for (int y = 0; y < meshHeight; ++y) {
        for (int x = 0; x < meshWidth; ++x) {
            int srcX = std::min(x * step, width - 1);
            int srcY = std::min(y * step, height - 1);

            // Get neighboring heights for normal calculation
            float left = heightMap.sample(srcX - step, srcY);
            float right = heightMap.sample(srcX + step, srcY);
            float up = heightMap.sample(srcX, srcY - step);
            float down = heightMap.sample(srcX, srcY + step);

            // Calculate normal (cross product of tangent vectors)
            float dx = (right - left) * scaleY * 0.5f;
            float dz = (down - up) * scaleY * 0.5f;

            // Tangent vectors
            float tx = scaleXZ;
            float ty = dx;
            float tz = 0;

            float ux = 0;
            float uy = dz;
            float uz = scaleXZ;

            // Cross product: n = t × u
            float nx = ty * uz - tz * uy;
            float ny = tz * ux - tx * uz;
            float nz = tx * uy - ty * ux;

            // Normalize
            float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (len > 0.0001f) {
                nx /= len;
                ny /= len;
                nz /= len;
            }

            file << "vn " << nx << " " << ny << " " << nz << "\n";
        }
    }

    file << "\n";

    // Write faces (triangles)
    for (int y = 0; y < meshHeight - 1; ++y) {
        for (int x = 0; x < meshWidth - 1; ++x) {
            // OBJ indices are 1-based
            int v1 = y * meshWidth + x + 1;
            int v2 = y * meshWidth + (x + 1) + 1;
            int v3 = (y + 1) * meshWidth + x + 1;
            int v4 = (y + 1) * meshWidth + (x + 1) + 1;

            // First triangle
            file << "f " << v1 << "//" << v1 << " "
                 << v2 << "//" << v2 << " "
                 << v3 << "//" << v3 << "\n";

            // Second triangle
            file << "f " << v2 << "//" << v2 << " "
                 << v4 << "//" << v4 << " "
                 << v3 << "//" << v3 << "\n";
        }
    }

    file.close();

    if (file.good()) {
        std::cout << "Mesh (OBJ) exported to: " << filename << std::endl;
        std::cout << "  Vertices: " << (meshWidth * meshHeight)
                  << ", Faces: " << ((meshWidth - 1) * (meshHeight - 1) * 2) << std::endl;
        return true;
    } else {
        std::cerr << "Failed to write OBJ mesh to: " << filename << std::endl;
        return false;
    }
}
