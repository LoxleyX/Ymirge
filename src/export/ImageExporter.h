#pragma once

#include "HeightMap.h"
#include <string>
#include <vector>

enum class ExportFormat {
    PNG16,
    RAW16,
    EXR,
    OBJ
};

class ImageExporter {
public:
    static bool exportHeightmap(const HeightMap& heightMap, const std::string& filename);
    static bool exportHeightmap8bit(const HeightMap& heightMap, const std::string& filename);

    // RGBA splatmap: R=sand/beach, G=grass, B=rock, A=snow
    static bool exportSplatmap(const HeightMap& heightMap, const std::string& filename);

    // 16-bit RAW, big-endian (Unity/Unreal compatible)
    static bool exportHeightmapRAW16(const HeightMap& heightMap, const std::string& filename);

    // 32-bit float EXR (Houdini/Maya/Blender)
    static bool exportHeightmapEXR(const HeightMap& heightMap, const std::string& filename);

    static bool exportMeshOBJ(const HeightMap& heightMap, const std::string& filename,
                              int maxSize = 512, float scaleXZ = 1.0f, float scaleY = 100.0f);

private:
    static float calculateSlope(const HeightMap& heightMap, int x, int y);

    static void generateSplatmapPixel(float height, float slope,
                                      unsigned char& outR,
                                      unsigned char& outG,
                                      unsigned char& outB,
                                      unsigned char& outA);
};
