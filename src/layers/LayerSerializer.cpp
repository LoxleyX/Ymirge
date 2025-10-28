#include "LayerSerializer.h"
#include <nlohmann/json.hpp>
#include "LayerGroup.h"
#include <fstream>
#include <filesystem>
#include <sstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

std::string LayerSerializer::lastError_ = "";

static void serializeLayer(const LayerBase* layer, json& layerJson, const std::string& dir, int& fileCounter);
static std::unique_ptr<LayerBase> deserializeLayer(const json& layerJson, const std::string& dir, int width, int height, int& fileCounter, std::string& errorMsg);

bool LayerSerializer::save(const LayerStack& stack, const std::string& filepath) {
    try {
        std::string dir = getDirectory(filepath);
        std::string baseName = getBaseName(filepath);
        
        if (!dir.empty() && !fs::exists(dir)) {
            fs::create_directories(dir);
        }
        
        json j;
        j["version"] = "1.0";
        j["width"] = stack.getWidth();
        j["height"] = stack.getHeight();
        j["layers"] = json::array();
        
        int fileCounter = 0;
        for (size_t i = 0; i < stack.getLayerCount(); ++i) {
            const LayerBase* layer = stack.getLayer(i);
            if (!layer) continue;
            
            json layerJson;
            serializeLayer(layer, layerJson, dir, fileCounter);
            j["layers"].push_back(layerJson);
        }
        
        std::ofstream outFile(filepath);
        if (!outFile.is_open()) {
            lastError_ = "Failed to open file for writing: " + filepath;
            return false;
        }
        
        outFile << j.dump(4);
        outFile.close();
        
        lastError_ = "";
        return true;
        
    } catch (const std::exception& e) {
        lastError_ = std::string("Exception during save: ") + e.what();
        return false;
    }
}

bool LayerSerializer::load(LayerStack& stack, const std::string& filepath) {
    try {
        std::ifstream inFile(filepath);
        if (!inFile.is_open()) {
            lastError_ = "Failed to open file for reading: " + filepath;
            return false;
        }
        
        json j;
        inFile >> j;
        inFile.close();
        
        if (!j.contains("version") || j["version"] != "1.0") {
            lastError_ = "Unsupported file version";
            return false;
        }
        
        int width = j["width"];
        int height = j["height"];
        
        stack.clear();
        
        std::string dir = getDirectory(filepath);
        
        int fileCounter = 0;
        const json& layers = j["layers"];
        for (const auto& layerJson : layers) {
            std::string errorMsg;
            auto layer = deserializeLayer(layerJson, dir, width, height, fileCounter, errorMsg);
            if (!layer) {
                lastError_ = errorMsg;
                return false;
            }
            stack.addLayer(std::move(layer));
        }
        
        lastError_ = "";
        return true;
        
    } catch (const std::exception& e) {
        lastError_ = std::string("Exception during load: ") + e.what();
        return false;
    }
}

std::string LayerSerializer::getLastError() {
    return lastError_;
}

bool LayerSerializer::saveHeightMapRaw(const HeightMap& heightMap, const std::string& filepath) {
    std::ofstream outFile(filepath, std::ios::binary);
    if (!outFile.is_open()) {
        return false;
    }
    
    int width = heightMap.getWidth();
    int height = heightMap.getHeight();
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float value = heightMap.at(x, y);
            outFile.write(reinterpret_cast<const char*>(&value), sizeof(float));
        }
    }
    
    outFile.close();
    return true;
}

bool LayerSerializer::loadHeightMapRaw(HeightMap& heightMap, const std::string& filepath) {
    std::ifstream inFile(filepath, std::ios::binary);
    if (!inFile.is_open()) {
        return false;
    }
    
    int width = heightMap.getWidth();
    int height = heightMap.getHeight();
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float value;
            inFile.read(reinterpret_cast<char*>(&value), sizeof(float));
            if (inFile.fail()) {
                return false;
            }
            heightMap.at(x, y) = value;
        }
    }
    
    inFile.close();
    return true;
}

std::string LayerSerializer::layerTypeToString(LayerType type) {
    switch (type) {
        case LayerType::PROCEDURAL: return "PROCEDURAL";
        case LayerType::SCULPT: return "SCULPT";
        case LayerType::STAMP: return "STAMP";
        default: return "PROCEDURAL";
    }
}

LayerType LayerSerializer::stringToLayerType(const std::string& str) {
    if (str == "SCULPT") return LayerType::SCULPT;
    if (str == "STAMP") return LayerType::STAMP;
    return LayerType::PROCEDURAL;
}

std::string LayerSerializer::blendModeToString(BlendMode mode) {
    switch (mode) {
        case BlendMode::NORMAL: return "NORMAL";
        case BlendMode::ADD: return "ADD";
        case BlendMode::SUBTRACT: return "SUBTRACT";
        case BlendMode::MULTIPLY: return "MULTIPLY";
        case BlendMode::SCREEN: return "SCREEN";
        case BlendMode::MAX: return "MAX";
        case BlendMode::MIN: return "MIN";
        case BlendMode::OVERLAY: return "OVERLAY";
        default: return "NORMAL";
    }
}

BlendMode LayerSerializer::stringToBlendMode(const std::string& str) {
    if (str == "ADD") return BlendMode::ADD;
    if (str == "SUBTRACT") return BlendMode::SUBTRACT;
    if (str == "MULTIPLY") return BlendMode::MULTIPLY;
    if (str == "SCREEN") return BlendMode::SCREEN;
    if (str == "MAX") return BlendMode::MAX;
    if (str == "MIN") return BlendMode::MIN;
    if (str == "OVERLAY") return BlendMode::OVERLAY;
    return BlendMode::NORMAL;
}

std::string LayerSerializer::getDirectory(const std::string& filepath) {
    fs::path p(filepath);
    if (p.has_parent_path()) {
        return p.parent_path().string();
    }
    return "";
}

std::string LayerSerializer::getBaseName(const std::string& filepath) {
    fs::path p(filepath);
    return p.stem().string();
}

static void serializeLayer(const LayerBase* layer, json& layerJson, const std::string& dir, int& fileCounter) {
    if (!layer) return;
    
    layerJson["name"] = layer->getName();
    layerJson["blendMode"] = LayerSerializer::blendModeToString(layer->getBlendMode());
    layerJson["opacity"] = layer->getOpacity();
    layerJson["visible"] = layer->isVisible();
    layerJson["locked"] = layer->isLocked();
    
    if (layer->isGroup()) {
        layerJson["type"] = "GROUP";
        layerJson["children"] = json::array();
        
        const LayerGroup* group = dynamic_cast<const LayerGroup*>(layer);
        if (group) {
            for (size_t i = 0; i < group->getChildCount(); ++i) {
                json childJson;
                serializeLayer(group->getChild(i), childJson, dir, fileCounter);
                layerJson["children"].push_back(childJson);
            }
        }
    } else {
        const TerrainLayer* terrainLayer = dynamic_cast<const TerrainLayer*>(layer);
        if (terrainLayer) {
            layerJson["type"] = LayerSerializer::layerTypeToString(terrainLayer->getType());
            
            std::ostringstream heightmapFilename;
            heightmapFilename << "layer_" << fileCounter++ << "_heightmap.raw";
            std::string heightmapPath = dir.empty() ? heightmapFilename.str() : (dir + "/" + heightmapFilename.str());
            
            if (LayerSerializer::saveHeightMapRaw(terrainLayer->getHeightMap(), heightmapPath)) {
                layerJson["heightmap"] = heightmapFilename.str();
            }
            
            if (terrainLayer->hasMask()) {
                std::ostringstream maskFilename;
                maskFilename << "layer_" << fileCounter++ << "_mask.raw";
                std::string maskPath = dir.empty() ? maskFilename.str() : (dir + "/" + maskFilename.str());
                
                if (LayerSerializer::saveHeightMapRaw(terrainLayer->getMask(), maskPath)) {
                    layerJson["mask"] = maskFilename.str();
                }
            }
        }
    }
}

static std::unique_ptr<LayerBase> deserializeLayer(const json& layerJson, const std::string& dir, int width, int height, int& fileCounter, std::string& errorMsg) {
    std::string type = layerJson["type"];
    std::string name = layerJson["name"];
    
    std::unique_ptr<LayerBase> layer;
    
    if (type == "GROUP") {
        auto group = std::make_unique<LayerGroup>(name, width, height);
        
        if (layerJson.contains("children")) {
            for (const auto& childJson : layerJson["children"]) {
                auto child = deserializeLayer(childJson, dir, width, height, fileCounter, errorMsg);
                if (!child) {
                    return nullptr;
                }
                group->addChild(std::move(child));
            }
        }
        
        layer = std::move(group);
    } else {
        LayerType layerType = LayerSerializer::stringToLayerType(type);
        auto terrainLayer = std::make_unique<TerrainLayer>(name, layerType, width, height);
        
        if (layerJson.contains("heightmap")) {
            std::string heightmapFilename = layerJson["heightmap"];
            std::string heightmapPath = dir.empty() ? heightmapFilename : (dir + "/" + heightmapFilename);
            
            if (!LayerSerializer::loadHeightMapRaw(terrainLayer->getHeightMap(), heightmapPath)) {
                errorMsg = "Failed to load heightmap: " + heightmapPath;
                return nullptr;
            }
        }
        
        if (layerJson.contains("mask")) {
            terrainLayer->createMask();
            std::string maskFilename = layerJson["mask"];
            std::string maskPath = dir.empty() ? maskFilename : (dir + "/" + maskFilename);
            
            if (!LayerSerializer::loadHeightMapRaw(terrainLayer->getMask(), maskPath)) {
                errorMsg = "Failed to load mask: " + maskPath;
                return nullptr;
            }
        }
        
        layer = std::move(terrainLayer);
    }
    
    if (layer) {
        layer->setBlendMode(LayerSerializer::stringToBlendMode(layerJson["blendMode"]));
        layer->setOpacity(layerJson["opacity"]);
        layer->setVisible(layerJson["visible"]);
        layer->setLocked(layerJson["locked"]);
    }
    
    return layer;
}
