#pragma once

#ifdef YMIRGE_SDL_UI_ENABLED

#include "TerrainParams.h"
#include "PresetManager.h"
#include "ResolutionManager.h"
#include "ImageExporter.h"
#include "BrushManager.h"
#include "StampTool.h"
#include "LayerStack.h"
#include "LayerCommand.h"
#include "LayerThumbnail.h"
#include <imgui.h>
#include <map>

enum class EditMode {
    LAYER,
    MASK
};

class UIManagerImGui {
public:
    UIManagerImGui();
    void update();
    ImVec4 render();

    const TerrainParams& getParams() const { return params_; }
    bool hasParamsChanged() const { return paramsChanged_; }
    void clearParamsChanged() { paramsChanged_ = false; }

    bool isMonochromeMode() const { return monochromeMode_; }
    bool isGenerateRequested() const { return generateRequested_; }
    void clearGenerateRequested() { generateRequested_ = false; }

    bool isHeightmapImportRequested() const { return heightmapImportRequested_; }
    void clearHeightmapImportRequested() { heightmapImportRequested_ = false; }

    bool isHeightmapExportRequested() const { return heightmapExportRequested_; }
    bool isSplatmapExportRequested() const { return splatmapExportRequested_; }
    void clearExportRequests() {
        heightmapExportRequested_ = false;
        splatmapExportRequested_ = false;
    }

    bool isLoadProjectRequested() const { return loadProjectRequested_; }
    void clearLoadProjectRequested() { loadProjectRequested_ = false; }

    bool isSaveProjectRequested() const { return saveProjectRequested_; }
    void clearSaveProjectRequested() { saveProjectRequested_ = false; }

    void showExportSuccess(const char* message);
    void showExportError(const char* message);

    Resolution getTargetResolution() const { return targetResolution_; }
    bool hasResolutionChanged() const { return resolutionChanged_; }
    void clearResolutionChanged() { resolutionChanged_ = false; }

    bool isRealTimePreviewEnabled() const { return enableRealTimePreview_; }

    ExportFormat getExportFormat() const { return exportFormat_; }

    BrushType getActiveTool() const { return activeTool_; }
    void setActiveTool(BrushType tool) { activeTool_ = tool; brushChanged_ = true; }
    int getBrushSize() const { return brushSize_; }
    float getBrushStrength() const { return brushStrength_; }

    bool hasBrushChanged() const { return brushChanged_; }
    void clearBrushChanged() { brushChanged_ = false; }

    float getStampScale() const { return stampScale_; }
    float getStampRotation() const { return stampRotation_; }
    float getStampOpacity() const { return stampOpacity_; }
    float getStampHeight() const { return stampHeight_; }
    StampBlendMode getStampBlendMode() const { return stampBlendMode_; }

    const StampLibrary& getStampLibrary() const { return stampLibrary_; }
    int getSelectedStampIndex() const { return selectedStampIndex_; }
    bool hasStampChanged() const { return stampChanged_; }
    void clearStampChanged() { stampChanged_ = false; }

    void setLayerStack(LayerStack* layerStack) { layerStack_ = layerStack; }
    void setLayerUndoStack(LayerUndoStack* undoStack) { layerUndoStack_ = undoStack; }
    void setDpiScale(float scale) { dpiScale_ = scale; }
    float getDpiScale() const { return dpiScale_; }
    LayerUndoStack* getLayerUndoStack() const { return layerUndoStack_; }

    EditMode getEditMode() const { return editMode_; }
    void setEditMode(EditMode mode) { editMode_ = mode; }
    size_t getMaskEditLayerIndex() const { return maskEditLayerIndex_; }
    void setMaskEditLayerIndex(size_t index) { maskEditLayerIndex_ = index; }
    LayerStack* getLayerStack() const { return layerStack_; }

    bool isCompositeRequested() const { return compositeRequested_; }
    void clearCompositeRequested() { compositeRequested_ = false; }
    void requestComposite() { compositeRequested_ = true; }

    bool isUndoRequested() const { return undoRequested_; }
    bool isRedoRequested() const { return redoRequested_; }
    bool isClearHistoryRequested() const { return clearHistoryRequested_; }
    bool isResetCameraRequested() const { return resetCameraRequested_; }
    bool isExitRequested() const { return exitRequested_; }

    ImVec4 getLastViewportRect() const { return lastViewportRect_; }

    void clearMenuRequests() {
        undoRequested_ = false;
        redoRequested_ = false;
        clearHistoryRequested_ = false;
        resetCameraRequested_ = false;
        exitRequested_ = false;
    }

private:
    void renderMenuBar();
    void renderToolPanel();
    void renderControlPanel();
    void renderLayersPanel();
    void renderLayerTreeNode(LayerBase* layer, size_t layerIndex, bool isRootLevel);
    void renderAboutDialog();
    void renderShortcutsDialog();
    TerrainParams params_;
    bool paramsChanged_;

    PresetManager presetManager_;
    int selectedPreset_;

    bool monochromeMode_;
    bool generateRequested_;
    bool heightmapImportRequested_;
    bool heightmapExportRequested_;
    bool splatmapExportRequested_;

    bool loadProjectRequested_;
    bool saveProjectRequested_;

    Resolution targetResolution_;
    bool resolutionChanged_;
    bool enableRealTimePreview_;

    ExportFormat exportFormat_;

    float dpiScale_ = 1.0f;

    BrushType activeTool_;
    int brushSize_;
    float brushStrength_;
    bool brushChanged_;

    float stampScale_;
    float stampRotation_;
    float stampOpacity_;
    float stampHeight_;
    StampBlendMode stampBlendMode_;

    StampLibrary stampLibrary_;
    int selectedStampIndex_;
    bool stampChanged_;

    const char* exportMessage_;
    double exportMessageTime_;
    bool exportMessageIsError_;

    bool showAboutDialog_;
    bool showShortcutsDialog_;

    bool undoRequested_;
    bool redoRequested_;
    bool clearHistoryRequested_;
    bool resetCameraRequested_;
    bool exitRequested_;

    LayerStack* layerStack_;
    LayerUndoStack* layerUndoStack_;

    bool showLayerRenameDialog_;
    size_t renameLayerIndex_;
    char renameLayerBuffer_[256];

    EditMode editMode_;
    size_t maskEditLayerIndex_;
    bool compositeRequested_;

    std::map<size_t, std::unique_ptr<LayerThumbnail>> layerThumbnails_;

    ImVec4 lastViewportRect_;
};

#endif // YMIRGE_SDL_UI_ENABLED
