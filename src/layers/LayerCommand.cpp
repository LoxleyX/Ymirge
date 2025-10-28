#include "LayerCommand.h"
#include <sstream>

// ============================================================================
// LayerPropertyCommand
// ============================================================================

LayerPropertyCommand::LayerPropertyCommand(LayerStack* stack, size_t layerIndex, PropertyType type)
    : stack_(stack)
    , layerIndex_(layerIndex)
    , type_(type)
    , oldOpacity_(0), newOpacity_(0)
    , oldBlendMode_(BlendMode::NORMAL), newBlendMode_(BlendMode::NORMAL)
    , oldVisibility_(true), newVisibility_(true)
    , oldLocked_(false), newLocked_(false) {

    captureOldValue();
}

void LayerPropertyCommand::captureOldValue() {
    LayerBase* layer = stack_->getLayer(layerIndex_);
    if (!layer) return;

    switch (type_) {
        case PropertyType::OPACITY:
            oldOpacity_ = layer->getOpacity();
            break;
        case PropertyType::BLEND_MODE:
            oldBlendMode_ = layer->getBlendMode();
            break;
        case PropertyType::VISIBILITY:
            oldVisibility_ = layer->isVisible();
            break;
        case PropertyType::LOCKED:
            oldLocked_ = layer->isLocked();
            break;
        case PropertyType::NAME:
            oldName_ = layer->getName();
            break;
    }
}

void LayerPropertyCommand::setOpacity(float newOpacity) {
    newOpacity_ = newOpacity;
}

void LayerPropertyCommand::setBlendMode(BlendMode newMode) {
    newBlendMode_ = newMode;
}

void LayerPropertyCommand::setVisibility(bool newVisible) {
    newVisibility_ = newVisible;
}

void LayerPropertyCommand::setLocked(bool newLocked) {
    newLocked_ = newLocked;
}

void LayerPropertyCommand::setName(const std::string& newName) {
    newName_ = newName;
}

void LayerPropertyCommand::execute() {
    LayerBase* layer = stack_->getLayer(layerIndex_);
    if (!layer) return;

    switch (type_) {
        case PropertyType::OPACITY:
            layer->setOpacity(newOpacity_);
            break;
        case PropertyType::BLEND_MODE:
            layer->setBlendMode(newBlendMode_);
            break;
        case PropertyType::VISIBILITY:
            layer->setVisible(newVisibility_);
            break;
        case PropertyType::LOCKED:
            layer->setLocked(newLocked_);
            break;
        case PropertyType::NAME:
            layer->setName(newName_);
            break;
    }
}

void LayerPropertyCommand::undo() {
    LayerBase* layer = stack_->getLayer(layerIndex_);
    if (!layer) return;

    switch (type_) {
        case PropertyType::OPACITY:
            layer->setOpacity(oldOpacity_);
            break;
        case PropertyType::BLEND_MODE:
            layer->setBlendMode(oldBlendMode_);
            break;
        case PropertyType::VISIBILITY:
            layer->setVisible(oldVisibility_);
            break;
        case PropertyType::LOCKED:
            layer->setLocked(oldLocked_);
            break;
        case PropertyType::NAME:
            layer->setName(oldName_);
            break;
    }
}

std::string LayerPropertyCommand::getDescription() const {
    std::ostringstream oss;
    oss << "Change layer property: ";

    switch (type_) {
        case PropertyType::OPACITY:
            oss << "Opacity";
            break;
        case PropertyType::BLEND_MODE:
            oss << "Blend Mode";
            break;
        case PropertyType::VISIBILITY:
            oss << "Visibility";
            break;
        case PropertyType::LOCKED:
            oss << "Lock";
            break;
        case PropertyType::NAME:
            oss << "Name";
            break;
    }

    return oss.str();
}

size_t LayerPropertyCommand::getMemoryUsage() const {
    size_t size = sizeof(*this);
    size += oldName_.capacity() + newName_.capacity();
    return size;
}

// ============================================================================
// AddLayerCommand
// ============================================================================

AddLayerCommand::AddLayerCommand(LayerStack* stack, std::unique_ptr<LayerBase> layer, size_t insertIndex)
    : stack_(stack)
    , layer_(std::move(layer))
    , insertIndex_(insertIndex)
    , executed_(false) {
}

void AddLayerCommand::execute() {
    if (!executed_) {
        stack_->insertLayer(insertIndex_, std::move(layer_));
        executed_ = true;
    }
}

void AddLayerCommand::undo() {
    if (executed_) {
        // Remove the layer and take ownership back
        layer_ = stack_->removeAndReturnLayer(insertIndex_);
        executed_ = false;
    }
}

std::string AddLayerCommand::getDescription() const {
    return "Add Layer";
}

size_t AddLayerCommand::getMemoryUsage() const {
    size_t size = sizeof(*this);
    if (layer_) {
        // Check if it's a TerrainLayer (has heightmap data)
        if (!layer_->isGroup()) {
            TerrainLayer* terrainLayer = dynamic_cast<TerrainLayer*>(layer_.get());
            if (terrainLayer) {
                size += terrainLayer->getHeightMap().getSize() * sizeof(float);
                if (terrainLayer->hasMask()) {
                    size += terrainLayer->getMask().getSize() * sizeof(float);
                }
            }
        }
    }
    return size;
}

// ============================================================================
// RemoveLayerCommand
// ============================================================================

RemoveLayerCommand::RemoveLayerCommand(LayerStack* stack, size_t layerIndex)
    : stack_(stack)
    , layerIndex_(layerIndex)
    , executed_(false) {
}

void RemoveLayerCommand::execute() {
    if (!executed_) {
        removedLayer_ = stack_->removeAndReturnLayer(layerIndex_);
        executed_ = true;
    }
}

void RemoveLayerCommand::undo() {
    if (executed_) {
        stack_->insertLayer(layerIndex_, std::move(removedLayer_));
        executed_ = false;
    }
}

std::string RemoveLayerCommand::getDescription() const {
    return "Remove Layer";
}

size_t RemoveLayerCommand::getMemoryUsage() const {
    size_t size = sizeof(*this);
    if (removedLayer_) {
        // Check if it's a TerrainLayer (has heightmap data)
        if (!removedLayer_->isGroup()) {
            TerrainLayer* terrainLayer = dynamic_cast<TerrainLayer*>(removedLayer_.get());
            if (terrainLayer) {
                size += terrainLayer->getHeightMap().getSize() * sizeof(float);
                if (terrainLayer->hasMask()) {
                    size += terrainLayer->getMask().getSize() * sizeof(float);
                }
            }
        }
    }
    return size;
}

// ============================================================================
// MoveLayerCommand
// ============================================================================

MoveLayerCommand::MoveLayerCommand(LayerStack* stack, size_t fromIndex, size_t toIndex)
    : stack_(stack)
    , fromIndex_(fromIndex)
    , toIndex_(toIndex) {
}

void MoveLayerCommand::execute() {
    stack_->moveLayer(fromIndex_, toIndex_);
}

void MoveLayerCommand::undo() {
    stack_->moveLayer(toIndex_, fromIndex_);
}

std::string MoveLayerCommand::getDescription() const {
    return "Move Layer";
}

size_t MoveLayerCommand::getMemoryUsage() const {
    return sizeof(*this);
}

// ============================================================================
// MergeLayerCommand
// ============================================================================

MergeLayerCommand::MergeLayerCommand(LayerStack* stack, size_t layerIndex)
    : stack_(stack)
    , layerIndex_(layerIndex)
    , bottomLayerBackup_(0, 0)
    , executed_(false) {
}

void MergeLayerCommand::execute() {
    if (!executed_) {
        // Backup top layer and bottom layer heightmap
        LayerBase* topLayer = stack_->getLayer(layerIndex_);
        LayerBase* bottomLayer = stack_->getLayer(layerIndex_ - 1);

        if (topLayer && bottomLayer && !topLayer->isGroup() && !bottomLayer->isGroup()) {
            TerrainLayer* topTerrain = dynamic_cast<TerrainLayer*>(topLayer);
            TerrainLayer* bottomTerrain = dynamic_cast<TerrainLayer*>(bottomLayer);

            if (topTerrain && bottomTerrain) {
                // Create backup of top layer (will be removed)
                topLayerBackup_ = std::make_unique<TerrainLayer>(
                    topTerrain->getName(),
                    topTerrain->getType(),
                    topTerrain->getHeightMap().getWidth(),
                    topTerrain->getHeightMap().getHeight()
                );

                TerrainLayer* backup = dynamic_cast<TerrainLayer*>(topLayerBackup_.get());
                if (backup) {
                    backup->getHeightMap() = topTerrain->getHeightMap();
                    backup->setOpacity(topTerrain->getOpacity());
                    backup->setBlendMode(topTerrain->getBlendMode());
                    backup->setVisible(topTerrain->isVisible());
                    backup->setLocked(topTerrain->isLocked());

                    if (topTerrain->hasMask()) {
                        backup->createMask();
                        backup->getMask() = topTerrain->getMask();
                    }
                }

                // Backup bottom layer heightmap
                bottomLayerBackup_ = bottomTerrain->getHeightMap();

                // Perform merge
                stack_->mergeDown(layerIndex_);

                executed_ = true;
            }
        }
    }
}

void MergeLayerCommand::undo() {
    if (executed_) {
        // Restore bottom layer heightmap
        LayerBase* bottomLayer = stack_->getLayer(layerIndex_ - 1);
        if (bottomLayer && !bottomLayer->isGroup()) {
            TerrainLayer* bottomTerrain = dynamic_cast<TerrainLayer*>(bottomLayer);
            if (bottomTerrain) {
                bottomTerrain->getHeightMap() = bottomLayerBackup_;
            }
        }

        // Re-insert top layer
        stack_->insertLayer(layerIndex_, std::move(topLayerBackup_));

        executed_ = false;
    }
}

std::string MergeLayerCommand::getDescription() const {
    return "Merge Layer Down";
}

size_t MergeLayerCommand::getMemoryUsage() const {
    size_t size = sizeof(*this);
    if (topLayerBackup_) {
        // Check if it's a TerrainLayer (has heightmap data)
        if (!topLayerBackup_->isGroup()) {
            TerrainLayer* terrainLayer = dynamic_cast<TerrainLayer*>(topLayerBackup_.get());
            if (terrainLayer) {
                size += terrainLayer->getHeightMap().getSize() * sizeof(float);
                if (terrainLayer->hasMask()) {
                    size += terrainLayer->getMask().getSize() * sizeof(float);
                }
            }
        }
    }
    size += bottomLayerBackup_.getSize() * sizeof(float);
    return size;
}

// ============================================================================
// LayerUndoStack
// ============================================================================

LayerUndoStack::LayerUndoStack(size_t maxCommands)
    : currentIndex_(0)
    , maxCommands_(maxCommands) {
}

void LayerUndoStack::push(std::unique_ptr<LayerCommand> command) {
    // Execute the command first
    command->execute();

    // Remove any commands after current index (redo history)
    if (currentIndex_ < commands_.size()) {
        commands_.erase(commands_.begin() + currentIndex_, commands_.end());
    }

    // Add new command
    commands_.push_back(std::move(command));
    currentIndex_++;

    // Trim if exceeds max
    if (commands_.size() > maxCommands_) {
        commands_.erase(commands_.begin());
        currentIndex_--;
    }
}

bool LayerUndoStack::undo() {
    if (!canUndo()) {
        return false;
    }

    currentIndex_--;
    commands_[currentIndex_]->undo();
    return true;
}

bool LayerUndoStack::redo() {
    if (!canRedo()) {
        return false;
    }

    commands_[currentIndex_]->execute();
    currentIndex_++;
    return true;
}

void LayerUndoStack::clear() {
    commands_.clear();
    currentIndex_ = 0;
}

std::string LayerUndoStack::getUndoDescription() const {
    if (canUndo()) {
        return commands_[currentIndex_ - 1]->getDescription();
    }
    return "";
}

std::string LayerUndoStack::getRedoDescription() const {
    if (canRedo()) {
        return commands_[currentIndex_]->getDescription();
    }
    return "";
}

size_t LayerUndoStack::getMemoryUsage() const {
    size_t total = sizeof(*this);
    for (const auto& cmd : commands_) {
        total += cmd->getMemoryUsage();
    }
    return total;
}

// ============================================================================
// CreateGroupCommand
// ============================================================================

CreateGroupCommand::CreateGroupCommand(LayerStack* stack, const std::string& groupName, size_t insertIndex)
    : stack_(stack)
    , groupName_(groupName)
    , insertIndex_(insertIndex)
    , executed_(false) {
}

void CreateGroupCommand::execute() {
    if (!executed_) {
        group_ = std::make_unique<LayerGroup>(groupName_, stack_->getWidth(), stack_->getHeight());
        stack_->insertLayer(insertIndex_, std::move(group_));
        executed_ = true;
    }
}

void CreateGroupCommand::undo() {
    if (executed_) {
        group_ = stack_->removeAndReturnLayer(insertIndex_);
        executed_ = false;
    }
}

std::string CreateGroupCommand::getDescription() const {
    return "Create Group";
}

size_t CreateGroupCommand::getMemoryUsage() const {
    return sizeof(*this);
}

// ============================================================================
// GroupLayersCommand
// ============================================================================

GroupLayersCommand::GroupLayersCommand(LayerStack* stack, const std::string& groupName,
                                     const std::vector<size_t>& layerIndices)
    : stack_(stack)
    , groupName_(groupName)
    , layerIndices_(layerIndices)
    , groupInsertIndex_(0)
    , executed_(false) {
}

void GroupLayersCommand::execute() {
    if (!executed_) {
        // Create new group
        auto group = std::make_unique<LayerGroup>(groupName_, stack_->getWidth(), stack_->getHeight());
        LayerGroup* groupPtr = group.get();

        // Find lowest index (where group will be inserted)
        groupInsertIndex_ = *std::min_element(layerIndices_.begin(), layerIndices_.end());

        // Remove layers from stack (in reverse order to maintain indices)
        // and add them to the group
        std::vector<size_t> sortedIndices = layerIndices_;
        std::sort(sortedIndices.rbegin(), sortedIndices.rend());  // Sort descending

        for (size_t idx : sortedIndices) {
            std::unique_ptr<LayerBase> layer = stack_->removeAndReturnLayer(idx);
            removedLayers_.push_back(std::move(layer));
        }

        // Reverse removedLayers so they're in original order
        std::reverse(removedLayers_.begin(), removedLayers_.end());

        // Add layers to group
        for (auto& layer : removedLayers_) {
            groupPtr->addChild(std::move(layer));
        }
        removedLayers_.clear();

        // Insert group at lowest index
        stack_->insertLayer(groupInsertIndex_, std::move(group));

        executed_ = true;
    }
}

void GroupLayersCommand::undo() {
    if (executed_) {
        // Remove group
        std::unique_ptr<LayerBase> group = stack_->removeAndReturnLayer(groupInsertIndex_);
        LayerGroup* groupPtr = dynamic_cast<LayerGroup*>(group.get());

        if (groupPtr) {
            // Extract children from group
            while (groupPtr->getChildCount() > 0) {
                removedLayers_.push_back(groupPtr->removeChild(0));
            }

            // Re-insert layers at their original positions
            std::vector<size_t> sortedIndices = layerIndices_;
            std::sort(sortedIndices.begin(), sortedIndices.end());  // Sort ascending

            for (size_t i = 0; i < sortedIndices.size() && i < removedLayers_.size(); ++i) {
                stack_->insertLayer(sortedIndices[i], std::move(removedLayers_[i]));
            }
            removedLayers_.clear();
        }

        executed_ = false;
    }
}

std::string GroupLayersCommand::getDescription() const {
    return "Group Layers";
}

size_t GroupLayersCommand::getMemoryUsage() const {
    size_t size = sizeof(*this);
    for (const auto& layer : removedLayers_) {
        if (layer && !layer->isGroup()) {
            TerrainLayer* terrainLayer = dynamic_cast<TerrainLayer*>(layer.get());
            if (terrainLayer) {
                size += terrainLayer->getHeightMap().getSize() * sizeof(float);
            }
        }
    }
    return size;
}

// ============================================================================
// UngroupCommand
// ============================================================================

UngroupCommand::UngroupCommand(LayerStack* stack, size_t groupIndex)
    : stack_(stack)
    , groupIndex_(groupIndex)
    , executed_(false) {
}

void UngroupCommand::execute() {
    if (!executed_) {
        // Remove group from stack
        removedGroup_ = stack_->removeAndReturnLayer(groupIndex_);
        LayerGroup* group = dynamic_cast<LayerGroup*>(removedGroup_.get());

        if (group) {
            // Insert children at group's position
            size_t insertPos = groupIndex_;
            while (group->getChildCount() > 0) {
                std::unique_ptr<LayerBase> child = group->removeChild(0);
                stack_->insertLayer(insertPos, std::move(child));
                insertPos++;
            }
        }

        executed_ = true;
    }
}

void UngroupCommand::undo() {
    if (executed_) {
        LayerGroup* group = dynamic_cast<LayerGroup*>(removedGroup_.get());
        if (group) {
            // Determine how many children the group originally had
            size_t childCount = group->getChildCount();

            // Remove children from stack and add back to group
            // (in reverse order since we're removing from the same position)
            for (size_t i = 0; i < childCount; ++i) {
                std::unique_ptr<LayerBase> child = stack_->removeAndReturnLayer(groupIndex_);
                group->addChild(std::move(child));
            }

            // Reverse children in group to restore original order
            // (This is a limitation - LayerGroup would need a method to reorder children)
        }

        // Re-insert group
        stack_->insertLayer(groupIndex_, std::move(removedGroup_));

        executed_ = false;
    }
}

std::string UngroupCommand::getDescription() const {
    return "Ungroup";
}

size_t UngroupCommand::getMemoryUsage() const {
    return sizeof(*this);
}
