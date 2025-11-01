#ifdef YMIRGE_SDL_UI_ENABLED

#include "UIManagerImGui.h"
#include "../layers/LayerGroup.h"
#include <SDL2/SDL.h>

UIManagerImGui::UIManagerImGui()
    : paramsChanged_(false)
    , selectedPreset_(-1)
    , monochromeMode_(false)
    , generateRequested_(false)
    , heightmapImportRequested_(false)
    , heightmapExportRequested_(false)
    , splatmapExportRequested_(false)
    , loadProjectRequested_(false)
    , saveProjectRequested_(false)
    , targetResolution_(Resolution::STANDARD)
    , resolutionChanged_(false)
    , enableRealTimePreview_(true)
    , exportFormat_(ExportFormat::PNG16)
    , activeTool_(BrushType::VIEW)
    , brushSize_(10)
    , brushStrength_(0.5f)
    , brushChanged_(false)
    , stampScale_(1.0f)
    , stampRotation_(0.0f)
    , stampOpacity_(1.0f)
    , stampHeight_(1.0f)
    , stampBlendMode_(StampBlendMode::BLEND)
    , selectedStampIndex_(0)
    , stampChanged_(false)
    , exportMessage_(nullptr)
    , exportMessageTime_(0.0)
    , exportMessageIsError_(false)
    , showAboutDialog_(false)
    , showShortcutsDialog_(false)
    , undoRequested_(false)
    , redoRequested_(false)
    , clearHistoryRequested_(false)
    , resetCameraRequested_(false)
    , exitRequested_(false)
    , layerStack_(nullptr)
    , layerUndoStack_(nullptr)
    , compositeRequested_(false)
    , showLayerRenameDialog_(false)
    , renameLayerIndex_(0)
    , editMode_(EditMode::LAYER)
    , maskEditLayerIndex_(0) {
    // Generate default procedural stamps
    stampLibrary_.generateDefaultStamps();
}

void UIManagerImGui::update() {
    paramsChanged_ = false;
}

ImVec4 UIManagerImGui::render() {
    renderMenuBar();
    renderToolPanel();
    renderControlPanel();
    renderLayersPanel();

    ImGuiIO& io = ImGui::GetIO();
    const float menuBarHeight = 20.0f;

    ImVec2 viewportPos(0, menuBarHeight);
    ImVec2 viewportSize(io.DisplaySize.x, io.DisplaySize.y - menuBarHeight);

    if (showAboutDialog_) {
        renderAboutDialog();
    }
    if (showShortcutsDialog_) {
        renderShortcutsDialog();
    }

    lastViewportRect_ = ImVec4(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);
    return lastViewportRect_;
}

void UIManagerImGui::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import Heightmap PNG", "Ctrl+O")) {
                heightmapImportRequested_ = true;
            if (ImGui::MenuItem("Load Project", "Ctrl+Shift+O")) {
                loadProjectRequested_ = true;
            }
            if (ImGui::MenuItem("Save Project", "Ctrl+S")) {
                saveProjectRequested_ = true;
            }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Heightmap")) {
                heightmapExportRequested_ = true;
            }
            if (ImGui::MenuItem("Save Splatmap")) {
                splatmapExportRequested_ = true;
            }
            if (ImGui::MenuItem("Export Mesh (OBJ)")) {
                exportFormat_ = ExportFormat::OBJ;
                heightmapExportRequested_ = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                exitRequested_ = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                undoRequested_ = true;
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                redoRequested_ = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Clear History")) {
                clearHistoryRequested_ = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Toggle Monochrome", "", monochromeMode_)) {
                monochromeMode_ = !monochromeMode_;
                paramsChanged_ = true;
            }
            if (ImGui::MenuItem("Reset Camera", "C")) {
                resetCameraRequested_ = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Keyboard Shortcuts")) {
                showShortcutsDialog_ = true;
            }
            if (ImGui::MenuItem("About Ymirge")) {
                showAboutDialog_ = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void UIManagerImGui::renderToolPanel() {
    const float margin = 10.0f;
    const float menuBarHeight = 20.0f;

    // Always position relative to current window size
    ImVec2 toolPanelPos(margin, menuBarHeight + margin);
    ImGui::SetNextWindowPos(toolPanelPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(220, 0), ImGuiCond_FirstUseEver);

    ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_NoCollapse);

    const char* tools[] = {"View (V)", "Raise (R)", "Lower (L)", "Smooth (S)", "Flatten (F)", "Stamp (T)"};
    BrushType toolTypes[] = {BrushType::VIEW, BrushType::RAISE, BrushType::LOWER,
                             BrushType::SMOOTH, BrushType::FLATTEN, BrushType::STAMP};

    for (int i = 0; i < 6; i++) {
        if (ImGui::Selectable(tools[i], activeTool_ == toolTypes[i])) {
            activeTool_ = toolTypes[i];
            brushChanged_ = true;
        }
    }

    ImGui::Separator();

    if (activeTool_ == BrushType::STAMP) {
        ImGui::Text("Stamp Settings");

        // Stamp selector
        const auto& stamps = stampLibrary_.getStamps();
        if (!stamps.empty()) {
            std::vector<const char*> stampNames;
            for (const auto& stamp : stamps) {
                stampNames.push_back(stamp.name.c_str());
            }

            if (ImGui::Combo("Stamp", &selectedStampIndex_, stampNames.data(), (int)stampNames.size())) {
                stampChanged_ = true;
            }

            // Show stamp description
            if (selectedStampIndex_ >= 0 && selectedStampIndex_ < (int)stamps.size()) {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", stamps[selectedStampIndex_].description.c_str());
            }
        }

        ImGui::Separator();

        if (ImGui::SliderFloat("Scale", &stampScale_, 0.1f, 5.0f)) {
            brushChanged_ = true;
        }
        if (ImGui::SliderFloat("Rotation", &stampRotation_, 0.0f, 360.0f)) {
            brushChanged_ = true;
        }
        if (ImGui::SliderFloat("Opacity", &stampOpacity_, 0.0f, 1.0f)) {
            brushChanged_ = true;
        }
        if (ImGui::SliderFloat("Height", &stampHeight_, 0.1f, 3.0f)) {
            brushChanged_ = true;
        }

        const char* blendModes[] = {"Blend", "Add", "Subtract", "Multiply", "Max", "Min", "Replace"};
        int currentBlend = static_cast<int>(stampBlendMode_);
        if (ImGui::Combo("Blend Mode", &currentBlend, blendModes, 7)) {
            stampBlendMode_ = static_cast<StampBlendMode>(currentBlend);
            brushChanged_ = true;
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Click to place stamp");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Adjust settings above");
    } else if (activeTool_ != BrushType::VIEW) {
        ImGui::Text("Brush Settings");
        if (ImGui::SliderInt("Size", &brushSize_, 1, 50)) {
            brushChanged_ = true;
        }
        if (ImGui::SliderFloat("Strength", &brushStrength_, 0.0f, 1.0f)) {
            brushChanged_ = true;
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Shortcuts:");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Ctrl+Z: Undo");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Ctrl+Y: Redo");
    } else {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "View Controls:");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Left-drag: Rotate");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Right-drag: Pan");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Wheel: Zoom");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "C: Reset camera");
    }

    ImGui::End();
}

void UIManagerImGui::renderControlPanel() {
    ImGuiIO& io = ImGui::GetIO();
    const float margin = 10.0f;
    const float menuBarHeight = 20.0f;
    const float controlPanelWidth = 380.0f;

    // Always position relative to current window size (anchored to top-right)
    ImVec2 controlPanelPos(io.DisplaySize.x - controlPanelWidth - margin, menuBarHeight + margin);
    ImGui::SetNextWindowPos(controlPanelPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(controlPanelWidth, 750), ImGuiCond_FirstUseEver);

    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoCollapse);

    // Presets
    if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto names = presetManager_.getPresetNames();
        for (size_t i = 0; i < names.size(); i++) {
            if (ImGui::Selectable(names[i].c_str(), selectedPreset_ == (int)i, 0, ImVec2(360, 0))) {
                presetManager_.applyPreset(names[i], params_);
                selectedPreset_ = (int)i;
                paramsChanged_ = true;
            }
        }
    }

    // Real-Time Preview
    if (ImGui::CollapsingHeader("Real-Time Preview", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable Real-Time Preview", &enableRealTimePreview_);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Update terrain while dragging sliders (512x512).\n"
                             "Disable for faster slider adjustment without regenerating.\n"
                             "When disabled, use the Generate button to update terrain.");
        }
    }

    // Resolution
    if (ImGui::CollapsingHeader("Resolution")) {
        const char* resNames[] = {"Preview (128)", "Standard (512)", "High (1024)", "Export (2048)", "Ultra (4096)"};

        // Convert enum to combo index
        int currentResIndex = 0;
        if (targetResolution_ == Resolution::PREVIEW) currentResIndex = 0;
        else if (targetResolution_ == Resolution::STANDARD) currentResIndex = 1;
        else if (targetResolution_ == Resolution::HIGH) currentResIndex = 2;
        else if (targetResolution_ == Resolution::EXPORT) currentResIndex = 3;
        else if (targetResolution_ == Resolution::ULTRA) currentResIndex = 4;

        if (ImGui::Combo("Target", &currentResIndex, resNames, 5)) {
            // Convert combo index to enum
            const Resolution resolutions[] = {
                Resolution::PREVIEW,
                Resolution::STANDARD,
                Resolution::HIGH,
                Resolution::EXPORT,
                Resolution::ULTRA
            };
            targetResolution_ = resolutions[currentResIndex];
            resolutionChanged_ = true;
        }
    }

    // Export Format
    if (ImGui::CollapsingHeader("Export Format")) {
        const char* formatNames[] = {"PNG16 (16-bit PNG)", "RAW16 (Unity/Unreal)", "EXR (Not Available)", "OBJ (3D Mesh)"};
        int currentFormat = (int)exportFormat_;
        ImGui::Combo("Format", &currentFormat, formatNames, 4);
        exportFormat_ = (ExportFormat)currentFormat;

        if (exportFormat_ == ExportFormat::EXR) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Note: EXR is not available in SDL2 build");
        }
    }

    // Parameters
    if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("Scale", &params_.scale, 20.0f, 300.0f)) {
            paramsChanged_ = true;
            selectedPreset_ = -1;
        }
        if (ImGui::SliderFloat("Peaks", &params_.peaks, 0.0f, 1.0f)) {
            paramsChanged_ = true;
            selectedPreset_ = -1;
        }
        if (ImGui::SliderFloat("Island", &params_.island, 0.0f, 1.0f)) {
            paramsChanged_ = true;
            selectedPreset_ = -1;
        }
        if (ImGui::SliderFloat("Island Shape", &params_.islandShape, 1.0f, 4.0f)) {
            paramsChanged_ = true;
            selectedPreset_ = -1;
        }
        if (ImGui::SliderFloat("River Intensity", &params_.riverIntensity, 0.0f, 1.0f)) {
            paramsChanged_ = true;
            selectedPreset_ = -1;
        }
        if (ImGui::SliderFloat("Erosion", &params_.erosion, 0.0f, 1.0f)) {
            paramsChanged_ = true;
            selectedPreset_ = -1;
        }
        if (ImGui::SliderFloat("Terrain Smoothness", &params_.terrainSmoothness, 0.0f, 1.0f)) {
            paramsChanged_ = true;
            selectedPreset_ = -1;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Reduces slopes in low/mid elevations while preserving peaks.\nCreates gentle, buildable terrain surrounded by dramatic mountains.");
        }
        if (params_.terrainSmoothness > 0.01f) {
            if (ImGui::SliderFloat("Softening Threshold", &params_.softeningThreshold, 0.3f, 0.9f)) {
                paramsChanged_ = true;
                selectedPreset_ = -1;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Elevation below which terrain gets smoothed.\n0.5 = bottom 50%, 0.7 = bottom 70%\nHigher values preserve more peaks.");
            }
        }
        if (ImGui::SliderFloat("Edge Padding", &params_.edgePadding, 0.0f, 0.5f)) {
            paramsChanged_ = true;
            selectedPreset_ = -1;
        }
        if (ImGui::SliderFloat("Sea Level", &params_.seaLevel, 0.0f, 0.5f)) {
            paramsChanged_ = true;
            selectedPreset_ = -1;
        }
    }

    // Generate button
    ImGui::Separator();
    if (ImGui::Button("Generate Terrain (G)", ImVec2(-1, 40))) {
        generateRequested_ = true;
    }

    // Export buttons
    if (ImGui::Button("Export Heightmap", ImVec2(-1, 30))) {
        heightmapExportRequested_ = true;
    }
    if (ImGui::Button("Export Splatmap", ImVec2(-1, 30))) {
        splatmapExportRequested_ = true;
    }

    // Monochrome toggle
    ImGui::Checkbox("Monochrome", &monochromeMode_);

    ImGui::End();

    // Export message (if active)
    if (exportMessage_ && (SDL_GetTicks() / 1000.0 - exportMessageTime_) < 3.0) {
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2 - 150, io.DisplaySize.y - 80), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, 60), ImGuiCond_Always);
        ImGui::Begin("Export Message", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse);

        ImVec4 color = exportMessageIsError_ ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f) : ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
        ImGui::TextColored(color, "%s", exportMessage_);
        ImGui::End();
    }
}

void UIManagerImGui::renderAboutDialog() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2 - 250, ImGui::GetIO().DisplaySize.y / 2 - 150), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Always);

    ImGui::Begin("About Ymirge", &showAboutDialog_,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Ymirge - Terrain Generator");
    ImGui::Separator();
    ImGui::Text("Version 1.0.0 (SDL2 Build)");
    ImGui::Spacing();
    ImGui::TextWrapped("A procedural terrain generation tool for game development and world building.");
    ImGui::Spacing();
    ImGui::Text("Features:");
    ImGui::BulletText("Multi-resolution generation (128x128 to 4096x4096)");
    ImGui::BulletText("Interactive terrain sculpting with 5 brush tools");
    ImGui::BulletText("Export: PNG16, RAW16, OBJ");
    ImGui::BulletText("Undo/Redo system with full history");
    ImGui::BulletText("8 beautiful terrain presets");

    ImGui::Spacing();
    if (ImGui::Button("Close", ImVec2(120, 0))) {
        showAboutDialog_ = false;
    }

    ImGui::End();
}

void UIManagerImGui::renderShortcutsDialog() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2 - 300, ImGui::GetIO().DisplaySize.y / 2 - 250), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Always);

    ImGui::Begin("Keyboard Shortcuts", &showShortcutsDialog_,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Tools:");
    ImGui::BulletText("V - View (camera controls)");
    ImGui::BulletText("R - Raise terrain");
    ImGui::BulletText("L - Lower terrain");
    ImGui::BulletText("S - Smooth terrain");
    ImGui::BulletText("F - Flatten terrain");
    ImGui::BulletText("T - Stamp tool");

    ImGui::Spacing();
    ImGui::Text("View:");
    ImGui::BulletText("C - Reset camera");
    ImGui::BulletText("Left-drag - Rotate camera");
    ImGui::BulletText("Right-drag - Pan camera");
    ImGui::BulletText("Wheel - Zoom in/out");

    ImGui::Spacing();
    ImGui::Text("Edit:");
    ImGui::BulletText("Ctrl+Z - Undo");
    ImGui::BulletText("Ctrl+Y - Redo");

    ImGui::Spacing();
    ImGui::Text("Generation:");
    ImGui::BulletText("G - Generate at high quality");

    ImGui::Spacing();
    if (ImGui::Button("Close", ImVec2(120, 0))) {
        showShortcutsDialog_ = false;
    }

    ImGui::End();
}

void UIManagerImGui::renderLayersPanel() {
    if (!layerStack_) {
        return;  // No layer stack set yet
    }

    // Position underneath the Tools panel on the left side
    const float margin = 10.0f;
    const float menuBarHeight = 20.0f;
    const float panelWidth = 280.0f;
    const float panelHeight = 350.0f;
    const float spacing = 10.0f;

    // Get the Tools window to calculate position below it
    // Assume Tools panel is at standard height, or we can use a fixed offset
    const float toolPanelEstimatedHeight = 300.0f;  // Approximate height of tools panel

    // Position below Tools panel with spacing
    ImVec2 layerPanelPos(margin, menuBarHeight + margin + toolPanelEstimatedHeight + spacing);
    ImGui::SetNextWindowPos(layerPanelPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Layers", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    // Add/Remove/Duplicate buttons
    if (ImGui::Button("+", ImVec2(40, 0))) {
        // Add new empty layer
        auto newLayer = std::make_unique<TerrainLayer>("New Layer", LayerType::SCULPT,
                                                       layerStack_->getWidth(), layerStack_->getHeight());
        layerStack_->addLayer(std::move(newLayer));
        compositeRequested_ = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Add new layer");
    }

    ImGui::SameLine();
    if (ImGui::Button("-", ImVec2(40, 0))) {
        // Remove active layer (keep at least one)
        size_t activeIdx = layerStack_->getActiveLayerIndex();
        if (layerStack_->getLayerCount() > 1) {
            layerStack_->removeLayer(activeIdx);
            compositeRequested_ = true;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Remove active layer");
    }

    ImGui::SameLine();
    if (ImGui::Button("Dup", ImVec2(50, 0))) {
        // Duplicate active layer
        size_t activeIdx = layerStack_->getActiveLayerIndex();
        auto duplicate = layerStack_->duplicateLayer(activeIdx);
        layerStack_->addLayer(std::move(duplicate));
        compositeRequested_ = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Duplicate active layer");
    }

    ImGui::SameLine();
    if (ImGui::Button("Merge ↓", ImVec2(60, 0))) {
        // Merge down
        size_t activeIdx = layerStack_->getActiveLayerIndex();
        if (activeIdx > 0) {
            layerStack_->mergeDown(activeIdx);
            compositeRequested_ = true;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Merge layer down");
    }

    ImGui::SameLine();
    if (ImGui::Button("New", ImVec2(50, 0))) {
        // Create new empty group
        if (layerUndoStack_) {
            auto cmd = std::make_unique<CreateGroupCommand>(layerStack_, "Group", layerStack_->getLayerCount());
            layerUndoStack_->push(std::move(cmd));
            compositeRequested_ = true;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Create new group");
    }

    ImGui::Separator();

    // Layer list (in reverse order - topmost layer at top)
    ImGui::BeginChild("LayerList", ImVec2(0, 0), true);

    size_t layerCount = layerStack_->getLayerCount();
    for (int i = static_cast<int>(layerCount) - 1; i >= 0; i--) {
        LayerBase* layer = layerStack_->getLayer(i);
        if (!layer) continue;
        renderLayerTreeNode(layer, i, true);
    }

    // Layer rename dialog
    if (showLayerRenameDialog_) {
        ImGui::OpenPopup("Rename Layer");
        showLayerRenameDialog_ = false;  // Only open once
    }

    if (ImGui::BeginPopupModal("Rename Layer", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter new layer name:");
        ImGui::SetKeyboardFocusHere();
        bool enterPressed = ImGui::InputText("##rename", renameLayerBuffer_, sizeof(renameLayerBuffer_), ImGuiInputTextFlags_EnterReturnsTrue);

        if (ImGui::Button("OK", ImVec2(120, 0)) || enterPressed) {
            // Apply rename with undo support
            if (layerUndoStack_) {
                auto cmd = std::make_unique<LayerPropertyCommand>(layerStack_, renameLayerIndex_, LayerPropertyCommand::PropertyType::NAME);
                cmd->setName(renameLayerBuffer_);
                layerUndoStack_->push(std::move(cmd));
                compositeRequested_ = true;
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::EndChild();

    ImGui::End();
}

void UIManagerImGui::renderLayerTreeNode(LayerBase* layer, size_t layerIndex, bool isRootLevel) {
    if (!layer) return;

    ImGui::PushID(static_cast<int>(layerIndex));

    bool isGroup = layer->isGroup();
    LayerGroup* group = isGroup ? dynamic_cast<LayerGroup*>(layer) : nullptr;
    TerrainLayer* terrainLayer = !isGroup ? dynamic_cast<TerrainLayer*>(layer) : nullptr;

    if (isGroup && group) {
        // GROUP RENDERING
        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (group->isExpanded()) {
            nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        bool visible = group->isVisible();
        if (ImGui::Checkbox("##vis", &visible)) {
            group->setVisible(visible);
            compositeRequested_ = true;
        }
        ImGui::SameLine();

        std::string groupLabel = "[Group] " + group->getName();
        bool nodeOpen = ImGui::TreeNodeEx(groupLabel.c_str(), nodeFlags);
        group->setExpanded(nodeOpen);

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Ungroup")) {
                if (layerUndoStack_) {
                    auto cmd = std::make_unique<UngroupCommand>(layerStack_, layerIndex);
                    layerUndoStack_->push(std::move(cmd));
                    compositeRequested_ = true;
                }
            }
            if (ImGui::MenuItem("Delete")) {
                layerStack_->removeLayer(layerIndex);
                compositeRequested_ = true;
            }
            ImGui::EndPopup();
        }

        if (nodeOpen) {
            for (int i = static_cast<int>(group->getChildCount()) - 1; i >= 0; i--) {
                renderLayerTreeNode(group->getChild(i), i, false);
            }
            ImGui::TreePop();
        }

    } else if (terrainLayer) {
        // LAYER RENDERING
        bool visible = terrainLayer->isVisible();
        if (ImGui::Checkbox("##vis", &visible)) {
            terrainLayer->setVisible(visible);
            compositeRequested_ = true;
        }
        ImGui::SameLine();

        if (isRootLevel && layerThumbnails_.find(layerIndex) == layerThumbnails_.end()) {
            layerThumbnails_[layerIndex] = std::make_unique<LayerThumbnail>();
            layerThumbnails_[layerIndex]->update(terrainLayer->getHeightMap());
        }

        if (isRootLevel) {
            LayerThumbnail* thumbnail = layerThumbnails_[layerIndex].get();
            if (thumbnail && thumbnail->isValid()) {
                ImGui::Image((void*)(intptr_t)thumbnail->getTextureID(),
                             ImVec2(48, 48), ImVec2(0, 1), ImVec2(1, 0));
                ImGui::SameLine();
            }
        }

        bool isActive = isRootLevel && (layerIndex == layerStack_->getActiveLayerIndex());
        ImGuiSelectableFlags selectableFlags = ImGuiSelectableFlags_AllowDoubleClick;
        if (terrainLayer->isLocked()) {
            selectableFlags |= ImGuiSelectableFlags_Disabled;
        }

        if (ImGui::Selectable(terrainLayer->getName().c_str(), isActive, selectableFlags)) {
            if (isRootLevel) {
                layerStack_->setActiveLayerIndex(layerIndex);
            }
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            size_t dragIndex = layerIndex;
            ImGui::SetDragDropPayload("LAYER_INDEX", &dragIndex, sizeof(size_t));
            ImGui::Text("Moving: %s", terrainLayer->getName().c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LAYER_INDEX")) {
                size_t fromIndex = *(const size_t*)payload->Data;
                size_t toIndex = layerIndex;
                if (fromIndex != toIndex && layerUndoStack_ && isRootLevel) {
                    auto cmd = std::make_unique<MoveLayerCommand>(layerStack_, fromIndex, toIndex);
                    layerUndoStack_->push(std::move(cmd));
                    compositeRequested_ = true;
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Rename...")) {
                showLayerRenameDialog_ = true;
                renameLayerIndex_ = layerIndex;
                strncpy_s(renameLayerBuffer_, sizeof(renameLayerBuffer_), terrainLayer->getName().c_str(), _TRUNCATE);
            }
            if (isRootLevel && ImGui::MenuItem("Duplicate")) {
                auto duplicate = layerStack_->duplicateLayer(layerIndex);
                layerStack_->addLayer(std::move(duplicate));
                compositeRequested_ = true;
            }
            if (isRootLevel && layerStack_->getLayerCount() > 1 && ImGui::MenuItem("Delete")) {
                layerStack_->removeLayer(layerIndex);
                compositeRequested_ = true;
            }
            ImGui::Separator();
            bool locked = terrainLayer->isLocked();
            if (ImGui::MenuItem("Lock", nullptr, &locked)) {
                terrainLayer->setLocked(locked);
            }
            ImGui::EndPopup();
        }

        if (isActive && isRootLevel) {
            const char* blendModes[] = {"Normal", "Add", "Subtract", "Multiply", "Screen", "Max", "Min", "Overlay"};
            int currentMode = static_cast<int>(terrainLayer->getBlendMode());
            ImGui::SetNextItemWidth(-1);
            if (ImGui::Combo("##blend", &currentMode, blendModes, IM_ARRAYSIZE(blendModes))) {
                terrainLayer->setBlendMode(static_cast<BlendMode>(currentMode));
                compositeRequested_ = true;
            }

            float opacity = terrainLayer->getOpacity() * 100.0f;
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat("##opacity", &opacity, 0.0f, 100.0f, "%.0f%%")) {
                terrainLayer->setOpacity(opacity / 100.0f);
                compositeRequested_ = true;
            }
            ImGui::Separator();
        }
    }

    ImGui::PopID();
}

void UIManagerImGui::showExportSuccess(const char* message) {
    exportMessage_ = message;
    exportMessageTime_ = SDL_GetTicks() / 1000.0;
    exportMessageIsError_ = false;
}

void UIManagerImGui::showExportError(const char* message) {
    exportMessage_ = message;
    exportMessageTime_ = SDL_GetTicks() / 1000.0;
    exportMessageIsError_ = true;
}

#endif // YMIRGE_SDL_UI_ENABLED
