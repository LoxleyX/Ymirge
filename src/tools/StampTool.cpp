#include "StampTool.h"
#include "ImageExporter.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <filesystem>

// Image loading support
#ifdef YMIRGE_SDL_UI_ENABLED
    // For SDL build, include stb_image directly
    #define STB_IMAGE_IMPLEMENTATION
    #define STBI_ONLY_PNG
    #include "../../vendor/stb_image.h"
#else
    #include "raylib.h"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

StampTool::StampTool()
    : stampData_(1, 1)  // Initialize with minimal size
    , blendMode_(StampBlendMode::BLEND)
    , stampLoaded_(false) {
}

StampTool::~StampTool() {
}

bool StampTool::loadStamp(const std::string& filepath) {
    std::cout << "Loading stamp: " << filepath << std::endl;

    // Check if this is a procedural stamp request (starts with "procedural:")
    if (filepath.find("procedural:") == 0) {
        std::string type = filepath.substr(11); // Remove "procedural:" prefix
        stampData_ = StampLibrary::createProceduralStamp(type);
        stampLoaded_ = true;
        std::cout << "Procedural stamp created: " << type << " ("
                  << stampData_.getWidth() << "x" << stampData_.getHeight() << ")" << std::endl;
        return true;
    }

    // Try to load PNG file
    if (std::filesystem::exists(filepath)) {
        int width, height, channels;

        // Try loading as 16-bit PNG first (preferred for heightmaps)
        unsigned short* data16 = stbi_load_16(filepath.c_str(), &width, &height, &channels, 1);

        if (data16) {
            stampData_ = HeightMap(width, height);

            // Convert 16-bit to float [0, 1]
            for (int i = 0; i < width * height; i++) {
                stampData_.getData()[i] = static_cast<float>(data16[i]) / 65535.0f;
            }

            stbi_image_free(data16);
            stampLoaded_ = true;
            std::cout << "Stamp loaded (16-bit): " << width << "x" << height << std::endl;
            return true;
        }

        // Try loading as 8-bit PNG
        unsigned char* data8 = stbi_load(filepath.c_str(), &width, &height, &channels, 1);

        if (data8) {
            stampData_ = HeightMap(width, height);

            // Convert 8-bit to float [0, 1]
            for (int i = 0; i < width * height; i++) {
                stampData_.getData()[i] = static_cast<float>(data8[i]) / 255.0f;
            }

            stbi_image_free(data8);
            stampLoaded_ = true;
            std::cout << "Stamp loaded (8-bit): " << width << "x" << height << std::endl;
            return true;
        }

        std::cerr << "Failed to load stamp image: " << filepath << std::endl;
        return false;
    }

    std::cerr << "Stamp file not found: " << filepath << std::endl;
    return false;
}

void StampTool::applyStamp(HeightMap& map,
                            int centerX, int centerY,
                            float scale,
                            float rotation,
                            float opacity,
                            float heightScale) {
    if (!stampLoaded_) {
        std::cerr << "No stamp loaded!" << std::endl;
        return;
    }

    int stampW = stampData_.getWidth();
    int stampH = stampData_.getHeight();

    // Calculate stamp bounds in heightmap space
    int radius = static_cast<int>((std::max(stampW, stampH) / 2.0f) * scale);

    int minX = std::max(0, centerX - radius);
    int maxX = std::min(map.getWidth() - 1, centerX + radius);
    int minY = std::max(0, centerY - radius);
    int maxY = std::min(map.getHeight() - 1, centerY + radius);

    // Apply stamp with edge feathering
    float featherRadius = std::max(stampW, stampH) / 2.0f;  // Half-size in stamp coordinates
    float featherStart = featherRadius * 0.75f;  // Start feathering at 75% of radius

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            // Convert to stamp-local coordinates
            float localX = (x - centerX) / scale;
            float localY = (y - centerY) / scale;

            // Calculate distance from stamp center (for edge feathering)
            float distFromCenter = std::sqrt(localX * localX + localY * localY);

            // Sample stamp at this position (with rotation)
            float stampValue = sampleStamp(localX, localY, scale, rotation);

            // Apply height scaling
            stampValue *= heightScale;

            // Calculate edge feathering (smooth falloff at edges)
            float edgeFalloff = 1.0f;
            if (distFromCenter > featherStart) {
                // Smoothstep falloff from featherStart to featherRadius
                float t = (distFromCenter - featherStart) / (featherRadius - featherStart);
                t = std::clamp(t, 0.0f, 1.0f);
                // Apply smoothstep: 3t² - 2t³
                edgeFalloff = 1.0f - (t * t * (3.0f - 2.0f * t));
            }

            // Apply edge feathering to opacity
            float finalOpacity = opacity * edgeFalloff;

            // Skip pixels with negligible opacity to avoid undo artifacts
            if (finalOpacity < 0.001f) {
                continue;
            }

            // Blend with terrain
            float terrainValue = map.at(x, y);
            float blended = blendValue(terrainValue, stampValue, finalOpacity);

            map.at(x, y) = blended;
        }
    }
}

float StampTool::sampleStamp(float x, float y, float scale, float rotation) {
    int stampW = stampData_.getWidth();
    int stampH = stampData_.getHeight();

    // Apply rotation
    if (rotation != 0.0f) {
        float rad = rotation * static_cast<float>(M_PI) / 180.0f;
        float cosR = std::cos(rad);
        float sinR = std::sin(rad);
        float rotX = x * cosR - y * sinR;
        float rotY = x * sinR + y * cosR;
        x = rotX;
        y = rotY;
    }

    // Convert to stamp pixel coordinates (centered)
    float stampX = x + stampW / 2.0f;
    float stampY = y + stampH / 2.0f;

    // Check bounds
    if (stampX < 0 || stampX >= stampW - 1 ||
        stampY < 0 || stampY >= stampH - 1) {
        return 0.0f;
    }

    // Bilinear interpolation
    int x0 = static_cast<int>(std::floor(stampX));
    int y0 = static_cast<int>(std::floor(stampY));
    int x1 = std::min(x0 + 1, stampW - 1);
    int y1 = std::min(y0 + 1, stampH - 1);

    float fx = stampX - x0;
    float fy = stampY - y0;

    float v00 = stampData_.at(x0, y0);
    float v10 = stampData_.at(x1, y0);
    float v01 = stampData_.at(x0, y1);
    float v11 = stampData_.at(x1, y1);

    float v0 = v00 * (1 - fx) + v10 * fx;
    float v1 = v01 * (1 - fx) + v11 * fx;

    return v0 * (1 - fy) + v1 * fy;
}

float StampTool::blendValue(float terrain, float stamp, float opacity) {
    switch (blendMode_) {
        case StampBlendMode::BLEND:
            return terrain * (1.0f - opacity) + stamp * opacity;

        case StampBlendMode::ADD:
            return terrain + stamp * opacity;

        case StampBlendMode::SUBTRACT:
            return terrain - stamp * opacity;

        case StampBlendMode::MULTIPLY:
            return terrain * (1.0f + stamp * opacity);

        case StampBlendMode::MAX:
            return std::max(terrain, stamp * opacity);

        case StampBlendMode::MIN:
            return std::min(terrain, stamp * opacity);

        case StampBlendMode::REPLACE:
            return stamp;

        default:
            return terrain * (1.0f - opacity) + stamp * opacity;
    }
}

int StampTool::getStampWidth() const {
    return stampLoaded_ ? stampData_.getWidth() : 0;
}

int StampTool::getStampHeight() const {
    return stampLoaded_ ? stampData_.getHeight() : 0;
}

void StampTool::clear() {
    stampData_ = HeightMap(1, 1);
    stampLoaded_ = false;
}

// StampLibrary implementation

StampLibrary::StampLibrary() {
}

void StampLibrary::scanDirectory(const std::string& directory) {
    stamps_.clear();

    // Check if directory exists
    if (!std::filesystem::exists(directory)) {
        std::cerr << "Stamp directory not found: " << directory << std::endl;
        return;
    }

    // Scan for PNG files
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".png") {
                StampInfo info;
                info.filepath = entry.path().string();
                info.name = entry.path().stem().string();

                // Determine category from parent directory
                std::string parentDir = entry.path().parent_path().filename().string();
                info.category = parentDir;

                // TODO: Load image to get dimensions
                info.width = 128;
                info.height = 128;
                info.description = "";

                stamps_.push_back(info);
            }
        }
    }

    std::cout << "Found " << stamps_.size() << " stamps in " << directory << std::endl;
}

std::vector<StampInfo> StampLibrary::getStampsByCategory(const std::string& category) const {
    std::vector<StampInfo> result;
    for (const auto& stamp : stamps_) {
        if (stamp.category == category) {
            result.push_back(stamp);
        }
    }
    return result;
}

std::vector<std::string> StampLibrary::getCategories() const {
    std::vector<std::string> categories;
    for (const auto& stamp : stamps_) {
        if (std::find(categories.begin(), categories.end(), stamp.category) == categories.end()) {
            categories.push_back(stamp.category);
        }
    }
    return categories;
}

const StampInfo* StampLibrary::findStamp(const std::string& name) const {
    for (const auto& stamp : stamps_) {
        if (stamp.name == name) {
            return &stamp;
        }
    }
    return nullptr;
}

int StampLibrary::findStampIndex(const std::string& name) const {
    for (size_t i = 0; i < stamps_.size(); i++) {
        if (stamps_[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void StampLibrary::generateDefaultStamps() {
    stamps_.clear();

    // Create default procedural stamps
    std::vector<std::string> stampTypes = {
        "mountain",
        "crater",
        "plateau",
        "valley",
        "ridge",
        "hill",
        "canyon",
        "mesa"
    };

    for (const auto& type : stampTypes) {
        StampInfo info;
        info.name = type;
        info.filepath = "procedural:" + type;
        info.category = "Procedural";
        info.width = 128;
        info.height = 128;

        if (type == "mountain") {
            info.description = "Gaussian mountain peak";
        } else if (type == "crater") {
            info.description = "Impact crater depression";
        } else if (type == "plateau") {
            info.description = "Flat-topped elevation";
        } else if (type == "valley") {
            info.description = "U-shaped valley depression";
        } else if (type == "ridge") {
            info.description = "Linear mountain ridge";
        } else if (type == "hill") {
            info.description = "Rounded hill";
        } else if (type == "canyon") {
            info.description = "Deep narrow canyon";
        } else if (type == "mesa") {
            info.description = "Flat plateau with steep sides";
        }

        stamps_.push_back(info);
    }

    std::cout << "Generated " << stamps_.size() << " default procedural stamps" << std::endl;
}

HeightMap StampLibrary::createProceduralStamp(const std::string& type, int size) {
    HeightMap stamp(size, size);

    if (type == "mountain") {
        // Gaussian mountain peak
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float dx = (x - size / 2.0f) / (size / 2.0f);
                float dy = (y - size / 2.0f) / (size / 2.0f);
                float dist = std::sqrt(dx * dx + dy * dy);

                // Gaussian with sharp falloff
                float height = std::exp(-dist * dist * 3.0f);
                stamp.at(x, y) = height;
            }
        }
    }
    else if (type == "crater") {
        // Impact crater with raised rim
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float dx = (x - size / 2.0f) / (size / 2.0f);
                float dy = (y - size / 2.0f) / (size / 2.0f);
                float dist = std::sqrt(dx * dx + dy * dy);

                // Inverted Gaussian with rim
                float crater = -std::exp(-dist * dist * 4.0f) * 0.8f;
                float rim = std::exp(-std::pow(dist - 0.4f, 2.0f) * 20.0f) * 0.3f;
                stamp.at(x, y) = std::max(0.0f, crater + rim + 0.5f);
            }
        }
    }
    else if (type == "plateau") {
        // Flat-topped plateau with smooth edges
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float dx = (x - size / 2.0f) / (size / 2.0f);
                float dy = (y - size / 2.0f) / (size / 2.0f);
                float dist = std::sqrt(dx * dx + dy * dy);

                // Smoothstep falloff from flat top
                float t = std::clamp((0.6f - dist) / 0.3f, 0.0f, 1.0f);
                float height = t * t * (3.0f - 2.0f * t);
                stamp.at(x, y) = height;
            }
        }
    }
    else if (type == "valley") {
        // U-shaped valley depression
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float dx = (x - size / 2.0f) / (size / 2.0f);
                float dy = (y - size / 2.0f) / (size / 2.0f);

                // Valley along Y axis
                float depth = 1.0f - std::exp(-dx * dx * 4.0f);
                float falloff = std::exp(-dy * dy * 2.0f);
                stamp.at(x, y) = 1.0f - depth * falloff * 0.6f;
            }
        }
    }
    else if (type == "ridge") {
        // Linear mountain ridge
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float dx = (x - size / 2.0f) / (size / 2.0f);
                float dy = (y - size / 2.0f) / (size / 2.0f);

                // Ridge along Y axis
                float height = std::exp(-dx * dx * 8.0f);
                float falloff = std::exp(-dy * dy * 1.5f);
                stamp.at(x, y) = height * falloff;
            }
        }
    }
    else if (type == "hill") {
        // Gentle rounded hill
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float dx = (x - size / 2.0f) / (size / 2.0f);
                float dy = (y - size / 2.0f) / (size / 2.0f);
                float dist = std::sqrt(dx * dx + dy * dy);

                // Wider, gentler Gaussian
                float height = std::exp(-dist * dist * 1.5f);
                stamp.at(x, y) = height * 0.7f;
            }
        }
    }
    else if (type == "canyon") {
        // Deep narrow canyon
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float dx = (x - size / 2.0f) / (size / 2.0f);
                float dy = (y - size / 2.0f) / (size / 2.0f);

                // V-shaped canyon along Y axis
                float depth = 1.0f - std::abs(dx);
                float falloff = std::exp(-dy * dy * 2.0f);
                stamp.at(x, y) = std::max(0.0f, 1.0f - depth * falloff * 0.8f);
            }
        }
    }
    else if (type == "mesa") {
        // Flat plateau with very steep sides
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float dx = (x - size / 2.0f) / (size / 2.0f);
                float dy = (y - size / 2.0f) / (size / 2.0f);
                float dist = std::sqrt(dx * dx + dy * dy);

                // Very steep transition
                float t = std::clamp((0.5f - dist) / 0.1f, 0.0f, 1.0f);
                stamp.at(x, y) = t;
            }
        }
    }
    else {
        // Default to simple mountain if type unknown
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float dx = (x - size / 2.0f) / (size / 2.0f);
                float dy = (y - size / 2.0f) / (size / 2.0f);
                float dist = std::sqrt(dx * dx + dy * dy);
                stamp.at(x, y) = std::max(0.0f, 1.0f - dist);
            }
        }
    }

    return stamp;
}
