#include "ResolutionManager.h"
#include <iostream>

ResolutionManager::ResolutionManager(ThreadPool* threadPool)
    : threadPool_(threadPool)
    , currentRes_(Resolution::STANDARD)
    , targetRes_(Resolution::STANDARD)
    , isGenerating_(false)
    , paramsChanged_(false) {

    // Create initial generator at standard resolution
    int size = getResolutionValue(currentRes_);
    generator_ = std::make_unique<TerrainGenerator>(size, size, threadPool_);

    // Initialize timer
    lastInteraction_ = std::chrono::steady_clock::now();
}

ResolutionManager::~ResolutionManager() {
    cancelGeneration();
}

void ResolutionManager::generateAt(Resolution res, const TerrainParams& params) {
    // Cancel any pending generation
    cancelGeneration();

    // Store parameters
    currentParams_ = params;
    paramsChanged_ = false;

    // Start generation
    startGeneration(res, params);

    // Reset interaction timer
    onUserInteraction();
}

void ResolutionManager::startGeneration(Resolution res, const TerrainParams& params) {
    int size = getResolutionValue(res);

    // Recreate generator if resolution changed
    if (res != currentRes_ || !generator_) {
        std::cout << "Creating generator at " << size << "x" << size << std::endl;
        generator_ = std::make_unique<TerrainGenerator>(size, size, threadPool_);
    }

    // Start async generation
    std::cout << "Generating terrain at " << size << "x" << size << "..." << std::endl;
    isGenerating_ = true;

    generationFuture_ = generator_->generateAsync(params);
}

void ResolutionManager::update() {
    // Check if current generation completed
    if (isGenerating_) {
        checkGenerationComplete();
    }

    // Check if we should auto-upgrade
    if (!isGenerating_ && shouldAutoUpgrade()) {
        std::cout << "Auto-upgrading to " << getResolutionName(targetRes_) << std::endl;
        startGeneration(targetRes_, currentParams_);
    }
}

void ResolutionManager::checkGenerationComplete() {
    if (!generationFuture_.valid()) {
        isGenerating_ = false;
        return;
    }

    // Check if generation finished (non-blocking)
    auto status = generationFuture_.wait_for(std::chrono::milliseconds(0));

    if (status == std::future_status::ready) {
        // Generation complete
        generationFuture_.get(); // Collect result

        isGenerating_ = false;
        currentRes_ = targetRes_;

        int size = getResolutionValue(currentRes_);
        std::cout << "Generation complete at " << size << "x" << size << std::endl;
    }
}

bool ResolutionManager::shouldAutoUpgrade() const {
    // Don't upgrade if:
    // - Already at target resolution
    // - Params changed (need to regenerate)
    // - Not enough time passed since last interaction

    if (currentRes_ >= targetRes_)
        return false;

    if (paramsChanged_)
        return false;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastInteraction_);

    return elapsed >= upgradeDelay_;
}

void ResolutionManager::onUserInteraction() {
    lastInteraction_ = std::chrono::steady_clock::now();
    paramsChanged_ = true;
}

void ResolutionManager::cancelGeneration() {
    if (isGenerating_ && generationFuture_.valid()) {
        // Wait for current generation to finish
        // (We can't actually cancel mid-generation safely)
        generationFuture_.wait();
        isGenerating_ = false;
    }
}

const HeightMap& ResolutionManager::getHeightMap() const {
    return generator_->getHeightMap();
}

HeightMap& ResolutionManager::getHeightMapMutable() {
    return generator_->getHeightMapMutable();
}

void ResolutionManager::setHeightMap(const HeightMap& newMap) {
    // Cancel any pending generation
    cancelGeneration();

    // Replace the heightmap
    generator_->setHeightMap(newMap);

    // Update current resolution based on imported size
    int size = newMap.getWidth();
    if (size <= 128) {
        currentRes_ = Resolution::PREVIEW;
    } else if (size <= 512) {
        currentRes_ = Resolution::STANDARD;
    } else if (size <= 1024) {
        currentRes_ = Resolution::HIGH;
    } else if (size <= 2048) {
        currentRes_ = Resolution::EXPORT;
    } else {
        currentRes_ = Resolution::ULTRA;
    }
}

const char* ResolutionManager::getResolutionName(Resolution res) {
    switch (res) {
        case Resolution::PREVIEW: return "Preview (128x128)";
        case Resolution::STANDARD: return "Standard (512x512)";
        case Resolution::HIGH: return "High (1024x1024)";
        case Resolution::EXPORT: return "Export (2048x2048)";
        case Resolution::ULTRA: return "Ultra (4096x4096)";
        default: return "Unknown";
    }
}
